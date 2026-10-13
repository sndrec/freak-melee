#include "screenshot.hpp"
#include "../internal.hpp"

#include <png.h>

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace aurora::gfx::screenshot {
namespace {
constexpr Module Log{"screenshot"};
std::atomic_bool busy{false};
Path pending{};

bool save_png(const char* path, const void* pixels, uint32_t width,
              uint32_t height, uint32_t pitch, bool bgra) {
  FILE* file = std::fopen(path, "wb");
  if (!file) {
    return false;
  }
  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  png_infop info = png ? png_create_info_struct(png) : nullptr;
  png_bytep row = static_cast<png_bytep>(std::malloc(size_t(width) * 3));
  if (!png || !info || !row) {
    std::free(row);
    if (png) {
      png_destroy_write_struct(&png, &info);
    }
    std::fclose(file);
    std::remove(path);
    return false;
  }

  bool written = false;
  if (setjmp(png_jmpbuf(png)) == 0) {
    png_init_io(png, file);
    png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    png_set_compression_level(png, 6);
    png_write_info(png, info);
    const auto* image = static_cast<const uint8_t*>(pixels);
    for (uint32_t y = 0; y < height; ++y) {
      const uint8_t* source = image + size_t(y) * pitch;
      for (uint32_t x = 0; x < width; ++x) {
        const uint8_t* pixel = source + size_t(x) * 4;
        uint8_t* destination = row + size_t(x) * 3;
        destination[0] = pixel[bgra ? 2 : 0];
        destination[1] = pixel[1];
        destination[2] = pixel[bgra ? 0 : 2];
      }
      png_write_row(png, row);
    }
    png_write_end(png, info);
    written = true;
  }
  png_destroy_write_struct(&png, &info);
  std::free(row);
  written = std::fclose(file) == 0 && written;
  if (!written) {
    std::remove(path);
  }
  return written;
}
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
          if (data && save_png(path.data(), data, extent.width, extent.height, pitch, bgra)) {
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
