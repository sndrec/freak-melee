#include "screenshot.hpp"
#include "../internal.hpp"

#include "png_writer.hpp"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace aurora::gfx::screenshot {
namespace {
constexpr Module Log{"screenshot"};
std::atomic_bool busy{false};
Path pending{};


}

bool request(const char* path) {
  if (!path) {
    return false;
  }
  const size_t length = std::strlen(path);
  if (length < 5 || length >= pending.size() ||
      std::strcmp(path + length - 4, ".png") != 0) {
    return false;
  }
  bool expected = false;
  if (!busy.compare_exchange_strong(expected, true)) {
    return false;
  }
  std::memcpy(pending.data(), path, length + 1);
  return true;
}

Path take_request() {
  Path result = pending;
  pending[0] = 0;
  return result;
}

void encode(const wgpu::CommandEncoder& encoder, const Path& path,
            const wgpu::Texture& presentedTexture, wgpu::TextureFormat format,
            const wgpu::Extent3D& extent,
            std::vector<std::function<void()>>& afterSubmit) {
  if (!path[0]) {
    return;
  }
  bool bgra;
  switch (format) {
  case wgpu::TextureFormat::BGRA8Unorm:
  case wgpu::TextureFormat::BGRA8UnormSrgb:
    bgra = true;
    break;
  case wgpu::TextureFormat::RGBA8Unorm:
  case wgpu::TextureFormat::RGBA8UnormSrgb:
    bgra = false;
    break;
  default:
    Log.error("Unsupported screenshot texture format");
    busy.store(false);
    return;
  }
  if (!presentedTexture || extent.width == 0 || extent.height == 0) {
    Log.error("No presented window frame available for screenshot");
    busy.store(false);
    return;
  }
  const uint32_t pitch = (extent.width * 4 + 255) & ~255u;
  const uint64_t size = uint64_t(pitch) * extent.height;
  const wgpu::BufferDescriptor descriptor{
      .label = "Screenshot readback", .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead,
      .size = size};
  auto buffer = webgpu::g_device.CreateBuffer(&descriptor);
  const wgpu::TexelCopyTextureInfo src{.texture = presentedTexture};
  const wgpu::TexelCopyBufferInfo dst{.layout = {.offset = 0, .bytesPerRow = pitch,
                                                .rowsPerImage = extent.height},
                                      .buffer = buffer};
  encoder.CopyTextureToBuffer(&src, &dst, &extent);
  afterSubmit.push_back([buffer, size, extent, pitch, bgra, path] {
    buffer.MapAsync(wgpu::MapMode::Read, 0, size, wgpu::CallbackMode::AllowSpontaneous,
      [buffer, size, extent, pitch, bgra, path](wgpu::MapAsyncStatus status, wgpu::StringView) {
        if (status == wgpu::MapAsyncStatus::Success) {
          const void* data = buffer.GetConstMappedRange(0, size);
          if (data && write_png(path.data(), data, extent.width, extent.height, pitch, bgra, false)) {
            Log.info("Saved screenshot {}", path.data());
          } else {
            Log.error("Screenshot PNG save failed: {}", path.data());
          }
          buffer.Unmap();
        } else {
          Log.error("Screenshot GPU readback failed");
        }
        busy.store(false);
      });
  });
}
}
