#include "presented_capture.hpp"
#include "render_worker.hpp"
#include "png_writer.hpp"
#include "../internal.hpp"
#include <SDL3/SDL_timer.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <mutex>

namespace aurora::gfx::presented_capture {
namespace {
constexpr Module Log{"screenshot"};
struct Slot {
  webgpu::TextureWithSampler image;
  wgpu::BindGroup bind_group;
  uint64_t frame = 0, presented_at = 0;
};
// One displayed image, one pending present, and one image reserved by F2.
// Textures are allocated only at startup/resize or the first retained capture.
std::array<Slot, 3> slots;
std::mutex mutex;
int displayed = -1, reserved = -1;
uint64_t next_frame = 0, next_capture = 0;
bool active = false, pending = false, retain_image = false, show_backdrop = false;
AuroraPresentedCapture capture{};
using Path = std::array<char, 1024>;

void finish(uint64_t id, bool ok, const char* message) {
  std::lock_guard lock{mutex};
  if (capture.capture_id != id) return;
  capture.status = ok ? AURORA_CAPTURE_READY : AURORA_CAPTURE_FAILED;
  std::snprintf(capture.error, sizeof(capture.error), "%s", message ? message : "");
  pending = false;
  if (!retain_image) { active = false; reserved = -1; }
}

void readback(int slot_index, uint64_t id, const Path& path) {
  const auto& source = slots[slot_index].image;
  const bool bgra = source.format == wgpu::TextureFormat::BGRA8Unorm ||
                    source.format == wgpu::TextureFormat::BGRA8UnormSrgb;
  if (!bgra && source.format != wgpu::TextureFormat::RGBA8Unorm &&
      source.format != wgpu::TextureFormat::RGBA8UnormSrgb) {
    finish(id, false, "The screen uses an unsupported image format.");
    return;
  }
  const auto extent = source.size;
  const uint32_t pitch = (extent.width * 4 + 255) & ~255u;
  const uint64_t bytes = uint64_t(pitch) * extent.height;
  const wgpu::BufferDescriptor descriptor{
      .label = "Presented image readback", .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead,
      .size = bytes};
  auto buffer = webgpu::g_device.CreateBuffer(&descriptor);
  const auto encoder = webgpu::g_device.CreateCommandEncoder();
  const wgpu::TexelCopyTextureInfo src{.texture = source.texture};
  const wgpu::TexelCopyBufferInfo dst{.layout = {.offset = 0, .bytesPerRow = pitch, .rowsPerImage = extent.height},
                                     .buffer = buffer};
  encoder.CopyTextureToBuffer(&src, &dst, &extent);
  const auto commands = encoder.Finish();
  webgpu::g_queue.Submit(1, &commands);
  buffer.MapAsync(wgpu::MapMode::Read, 0, bytes, wgpu::CallbackMode::AllowSpontaneous,
    [buffer, bytes, extent, pitch, bgra, path, id](wgpu::MapAsyncStatus status, wgpu::StringView) {
      bool wanted;
      { std::lock_guard lock{mutex}; wanted = active && capture.capture_id == id; }
      if (status != wgpu::MapAsyncStatus::Success) {
        if (wanted) Log.error("Presented screenshot GPU readback failed");
        finish(id, false, "Could not read the captured screen from the GPU.");
        return;
      }
      const void* pixels = buffer.GetConstMappedRange(0, bytes);
      const bool ok = wanted && pixels && write_png(path.data(), pixels, extent.width, extent.height, pitch, bgra, false);
      buffer.Unmap();
      if (ok) Log.info("Saved screenshot {}", path.data());
      else if (wanted) Log.error("Screenshot PNG save failed: {}", path.data());
      finish(id, ok, ok ? "" : "Could not save the captured screen.");
    });
}
}

int retain(const wgpu::CommandEncoder& encoder, const wgpu::Texture& texture,
           wgpu::TextureFormat format, const wgpu::Extent3D& extent) {
  if (!texture || !extent.width || !extent.height) return -1;
  int index = -1;
  {
    std::lock_guard lock{mutex};
    for (int i = 0; i < int(slots.size()); ++i)
      if (i != displayed && i != reserved) { index = i; break; }
  }
  if (index < 0) return -1;
  auto& slot = slots[index];
  auto& image = slot.image;
  if (!image.texture || image.size.width != extent.width || image.size.height != extent.height || image.format != format) {
    const wgpu::TextureDescriptor descriptor{
        .label = "Retained presented image", .usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc |
            wgpu::TextureUsage::TextureBinding,
        .dimension = wgpu::TextureDimension::e2D, .size = extent, .format = format, .mipLevelCount = 1, .sampleCount = 1};
    image = {};
    image.texture = webgpu::g_device.CreateTexture(&descriptor);
    image.view = image.texture.CreateView(); image.size = extent; image.format = format;
    const wgpu::SamplerDescriptor sampler{
        .addressModeU = wgpu::AddressMode::ClampToEdge, .addressModeV = wgpu::AddressMode::ClampToEdge,
        .magFilter = wgpu::FilterMode::Linear, .minFilter = wgpu::FilterMode::Linear};
    image.sampler = webgpu::g_device.CreateSampler(&sampler);
    slot.bind_group = webgpu::create_copy_bind_group(image);
  }
  const wgpu::TexelCopyTextureInfo src{.texture = texture}, dst{.texture = image.texture};
  encoder.CopyTextureToTexture(&src, &dst, &extent);
  return index;
}

void publish(int index) {
  if (index < 0) return;
  std::lock_guard lock{mutex};
  auto& slot = slots[index];
  slot.frame = ++next_frame; slot.presented_at = SDL_GetTicksNS();
  displayed = index;
}

Backdrop backdrop() {
  std::lock_guard lock{mutex};
  if (!active || !show_backdrop || reserved < 0) return {};
  const auto& slot = slots[reserved];
  return {slot.bind_group, slot.image.size};
}

void shutdown() {
  // The render queue is already drained by gfx::shutdown. A readback callback
  // may finish later, but owns its own GPU buffer and checks the capture ID.
  std::lock_guard lock{mutex};
  active = pending = retain_image = show_backdrop = false;
  displayed = reserved = -1;
  capture = {}; ++next_capture;
  for (auto& slot : slots) slot = {};
}
}

bool aurora_capture_presented(const char* path, bool keep, AuroraPresentedCapture* info) {
  using namespace aurora::gfx::presented_capture;
  if (!path) return false;
  const size_t length = std::strlen(path);
  if (length < 5 || length >= Path{}.size() || std::strcmp(path + length - 4, ".png")) return false;
  Path owned_path{}; std::memcpy(owned_path.data(), path, length + 1);
  int index; uint64_t id;
  {
    std::lock_guard lock{mutex};
    if (active || pending || reserved >= 0 || displayed < 0) return false;
    index = reserved = displayed;
    const auto& slot = slots[index];
    id = ++next_capture;
    capture = {.capture_id = id, .displayed_frame = slot.frame, .presented_at_ns = slot.presented_at,
        .width = slot.image.size.width, .height = slot.image.size.height, .status = AURORA_CAPTURE_PENDING};
    active = pending = true; retain_image = keep; show_backdrop = false;
    if (info) *info = capture;
  }
  // The reservation above precedes this enqueue. Already queued frames cannot
  // recycle the reserved slot before the readback reaches the render worker.
  aurora::gfx::render_worker::enqueue_work([index, id, owned_path] { readback(index, id, owned_path); });
  return true;
}
void aurora_presented_capture_status(AuroraPresentedCapture* info) {
  using namespace aurora::gfx::presented_capture;
  if (!info) return;
  std::lock_guard lock{mutex}; *info = capture;
}
bool aurora_write_presented_capture(const char* path) {
  using namespace aurora::gfx::presented_capture;
  if (!path) return false;
  const size_t length = std::strlen(path);
  if (length < 5 || length >= Path{}.size() || std::strcmp(path + length - 4, ".png")) return false;
  Path owned_path{}; std::memcpy(owned_path.data(), path, length + 1);
  int index; uint64_t id;
  {
    std::lock_guard lock{mutex};
    if (!active || !retain_image || reserved < 0 || pending) return false;
    index = reserved; id = capture.capture_id;
    pending = true; capture.status = AURORA_CAPTURE_PENDING; *capture.error = 0;
  }
  aurora::gfx::render_worker::enqueue_work([index, id, owned_path] { readback(index, id, owned_path); });
  return true;
}
void aurora_set_capture_backdrop(bool enabled) {
  using namespace aurora::gfx::presented_capture;
  std::lock_guard lock{mutex}; show_backdrop = enabled && active && retain_image;
}
void aurora_release_presented_capture() {
  using namespace aurora::gfx::presented_capture;
  uint64_t id;
  { std::lock_guard lock{mutex}; active = show_backdrop = false; id = capture.capture_id; }
  // Frames already recorded with this backdrop finish before the slot can be
  // recycled. This ordering also protects cancellation during GPU readback.
  aurora::gfx::render_worker::enqueue_work([id] {
    std::lock_guard lock{mutex}; if (capture.capture_id == id) reserved = -1;
  });
}
