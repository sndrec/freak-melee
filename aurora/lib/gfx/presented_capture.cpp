#include "presented_capture.hpp"
#include "frame.hpp"
#include "render_worker.hpp"
#include "png_writer.hpp"
#include "../internal.hpp"
#include <SDL3/SDL_timer.h>
#include <array>
#include <algorithm>
#include <cmath>
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
std::string encoded;
Request next_request;

void finish(uint64_t id, std::string bytes, const char* message) {
  std::lock_guard lock{mutex};
  if (!active || capture.capture_id != id) return;
  encoded = std::move(bytes);
  capture.status = encoded.empty() ? AURORA_CAPTURE_FAILED : AURORA_CAPTURE_READY;
  std::snprintf(capture.error, sizeof(capture.error), "%s", message ? message : "");
  pending = false;
  if (!retain_image) reserved = -1;
}

void readback(int slot_index, uint64_t id, wgpu::Origin3D origin = {},
              wgpu::Extent3D extent = {0, 0, 1}) {
  const auto& source = slots[slot_index].image;
  const bool bgra = source.format == wgpu::TextureFormat::BGRA8Unorm ||
                    source.format == wgpu::TextureFormat::BGRA8UnormSrgb;
  if (!bgra && source.format != wgpu::TextureFormat::RGBA8Unorm &&
      source.format != wgpu::TextureFormat::RGBA8UnormSrgb) {
    finish(id, {}, "The screen uses an unsupported image format.");
    return;
  }
  if (!extent.width) extent = source.size;
  if (!extent.height || uint64_t(extent.width) * extent.height > 16 * 1024 * 1024 ||
      extent.width > (UINT32_MAX - 255) / 4) {
    finish(id, {}, "The screen exceeds the capture size limit.");
    return;
  }
  const uint32_t pitch = (extent.width * 4 + 255) & ~255u;
  const uint64_t bytes = uint64_t(pitch) * extent.height;
  if (bytes > 68 * 1024 * 1024) {
    finish(id, {}, "The screen exceeds the readback size limit.");
    return;
  }
  const wgpu::BufferDescriptor descriptor{
      .label = "Presented image readback", .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead,
      .size = bytes};
  auto buffer = webgpu::g_device.CreateBuffer(&descriptor);
  const auto encoder = webgpu::g_device.CreateCommandEncoder();
  const wgpu::TexelCopyTextureInfo src{.texture = source.texture, .origin = origin};
  const wgpu::TexelCopyBufferInfo dst{.layout = {.offset = 0, .bytesPerRow = pitch, .rowsPerImage = extent.height},
                                     .buffer = buffer};
  encoder.CopyTextureToBuffer(&src, &dst, &extent);
  const auto commands = encoder.Finish();
  webgpu::g_queue.Submit(1, &commands);
  buffer.MapAsync(wgpu::MapMode::Read, 0, bytes, wgpu::CallbackMode::AllowSpontaneous,
    [buffer, bytes, extent, pitch, bgra, id](wgpu::MapAsyncStatus status, wgpu::StringView) {
      bool wanted;
      { std::lock_guard lock{mutex}; wanted = active && capture.capture_id == id; }
      if (status != wgpu::MapAsyncStatus::Success) {
        if (wanted) Log.error("Presented screenshot GPU readback failed");
        finish(id, {}, "Could not read the captured screen from the GPU.");
        return;
      }
      const void* pixels = buffer.GetConstMappedRange(0, bytes);
      std::string png;
      const bool ok = wanted && pixels && encode_png(png, pixels, extent.width, extent.height, pitch, bgra);
      buffer.Unmap();
      if (ok) finish(id, std::move(png), "");
      else finish(id, {}, "Could not encode the captured screen.");
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

Request take_request() {
  std::lock_guard lock{mutex};
  Request request = next_request;
  next_request = {};
  return request;
}

void publish(int index, const Request& request, const webgpu::Viewport& viewport) {
  wgpu::Origin3D origin{};
  wgpu::Extent3D extent{};
  {
    std::lock_guard lock{mutex};
    if (index >= 0) {
      auto& slot = slots[index];
      slot.frame = ++next_frame;
      slot.presented_at = SDL_GetTicksNS();
      displayed = index;
    }
    if (!request.id || !active || capture.capture_id != request.id) return;
    if (index >= 0) {
      const auto& slot = slots[index];
      extent = slot.image.size;
      if (request.viewport_only) {
        // Use the compositor's pixel rectangle on the captured surface.
        const auto pixel = [](float value, uint32_t limit) {
          return static_cast<uint32_t>(std::clamp(std::round(value), 0.f, static_cast<float>(limit)));
        };
        origin.x = pixel(viewport.left, extent.width);
        origin.y = pixel(viewport.top, extent.height);
        extent.width = pixel(viewport.left + viewport.width, extent.width) - origin.x;
        extent.height = pixel(viewport.top + viewport.height, extent.height) - origin.y;
      }
      if (extent.width && extent.height && uint64_t(extent.width) * extent.height <= 16 * 1024 * 1024) {
        reserved = index;
        capture.displayed_frame = slot.frame;
        capture.presented_at_ns = slot.presented_at;
        capture.width = extent.width;
        capture.height = extent.height;
      } else {
        index = -1;
      }
    }
  }
  if (index < 0) {
    finish(request.id, {}, "The requested frame could not be presented within the capture limits.");
    return;
  }
  readback(index, request.id, origin, extent);
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
  std::string{}.swap(encoded);
  next_request = {};
  for (auto& slot : slots) slot = {};
}
}

bool aurora_capture_next_frame(bool viewport_only, AuroraPresentedCapture* info) {
  using namespace aurora::gfx::presented_capture;
  if (!info) return false;
  std::lock_guard lock{mutex};
  if (active || pending || reserved >= 0 || next_request.id) return false;
  next_request.id = ++next_capture;
  next_request.viewport_only = viewport_only;
  capture = {.capture_id = next_request.id, .status = AURORA_CAPTURE_PENDING};
  active = pending = true;
  retain_image = false;
  show_backdrop = false;
  if (info) *info = capture;
  return true;
}

bool aurora_capture_presented(bool keep, AuroraPresentedCapture* info) {
  using namespace aurora::gfx::presented_capture;
  if (!info) return false;
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
  aurora::gfx::render_worker::enqueue_work([index, id] { readback(index, id); });
  return true;
}
void aurora_presented_capture_status(AuroraPresentedCapture* info) {
  using namespace aurora::gfx::presented_capture;
  if (!info) return;
  {
    std::lock_guard lock{mutex};
    *info = capture;
  }
  // An offline test or feedback UI can hold the final rendered frame. Pump
  // its asynchronous readback even when there is no subsequent frame submit.
  if (info->status == AURORA_CAPTURE_PENDING) aurora::gfx::enqueue_process_events();
}
const void* aurora_presented_png(uint64_t id, size_t* size) {
  using namespace aurora::gfx::presented_capture;
  if (!size) return nullptr;
  std::lock_guard lock{mutex};
  *size = 0;
  if (!active || capture.capture_id != id || capture.status != AURORA_CAPTURE_READY) return nullptr;
  *size = encoded.size();
  return encoded.data();
}
bool aurora_retry_presented_capture(uint64_t id) {
  using namespace aurora::gfx::presented_capture;
  int index;
  {
    std::lock_guard lock{mutex};
    if (!active || capture.capture_id != id || !retain_image || reserved < 0 || pending) return false;
    index = reserved;
    pending = true; capture.status = AURORA_CAPTURE_PENDING; *capture.error = 0;
  }
  aurora::gfx::render_worker::enqueue_work([index, id] { readback(index, id); });
  return true;
}
void aurora_set_capture_backdrop(bool enabled) {
  using namespace aurora::gfx::presented_capture;
  std::lock_guard lock{mutex}; show_backdrop = enabled && active && retain_image;
}
void aurora_release_presented_capture(uint64_t id) {
  using namespace aurora::gfx::presented_capture;
  {
    std::lock_guard lock{mutex};
    if (!active || capture.capture_id != id) return;
    active = pending = show_backdrop = false;
    if (capture.status == AURORA_CAPTURE_PENDING) {
      capture.status = AURORA_CAPTURE_FAILED;
      std::snprintf(capture.error, sizeof(capture.error), "Capture cancelled.");
    }
    next_request = {};
    std::string{}.swap(encoded);
  }
  // Frames already recorded with this backdrop finish before the slot can be
  // recycled. This ordering also protects cancellation during GPU readback.
  aurora::gfx::render_worker::enqueue_work([id] {
    std::lock_guard lock{mutex}; if (capture.capture_id == id) reserved = -1;
  });
}
