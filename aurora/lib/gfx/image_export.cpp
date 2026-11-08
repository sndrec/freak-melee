#include <aurora/gfx.hpp>
#include <aurora/gfx.h>
#include "png_writer.hpp"
#include <atomic>
#include <cstring>

namespace aurora::gfx {
namespace {
struct ImageExport {
  std::atomic<AuroraImageExportState> state{AuroraImageExport_Idle};
  wgpu::Texture texture;
  wgpu::Buffer buffer;
  uint32_t width, height, pitch;
  uint64_t bytes;
  bool bgra, alpha;
  char path[1024];
} job;
EncoderTaskId task = InvalidEncoderTask;

void encode_image(const EncoderTaskContext& context, const wgpu::CommandEncoder& encoder,
                  const void*, size_t, void*) {
  const wgpu::BufferDescriptor descriptor{.label = "Image export", .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead,
                                           .size = job.bytes};
  job.buffer = context.device.CreateBuffer(&descriptor);
  const wgpu::TexelCopyTextureInfo source{.texture = job.texture};
  const wgpu::TexelCopyBufferInfo destination{.layout = {.bytesPerRow = job.pitch, .rowsPerImage = job.height},
                                             .buffer = job.buffer};
  const wgpu::Extent3D extent{job.width, job.height, 1};
  encoder.CopyTextureToBuffer(&source, &destination, &extent);
}

void submit_image(const EncoderTaskCompletionContext&, const void*, size_t, void*) {
  const auto buffer = job.buffer;
  buffer.MapAsync(wgpu::MapMode::Read, 0, job.bytes, wgpu::CallbackMode::AllowSpontaneous,
    [buffer](wgpu::MapAsyncStatus status, wgpu::StringView) {
      bool ok = false;
      if (status == wgpu::MapAsyncStatus::Success) {
        const void* pixels = buffer.GetConstMappedRange(0, job.bytes);
        ok = pixels && write_png(job.path, pixels, job.width, job.height, job.pitch, job.bgra, job.alpha);
        buffer.Unmap();
      }
      job.texture = {}; job.buffer = {};
      job.state.store(ok ? AuroraImageExport_Complete : AuroraImageExport_Failed, std::memory_order_release);
    });
}
}



static bool queue_export(const ResolvedTargets& image, const char* path, bool alpha) {
  if (job.state.load(std::memory_order_acquire) == AuroraImageExport_Pending || !image.colorTexture || !path ||
      !image.width || !image.height || std::strlen(path) >= sizeof(job.path)) return false;
  bool bgra;
  switch (image.colorFormat) {
  case wgpu::TextureFormat::BGRA8Unorm: case wgpu::TextureFormat::BGRA8UnormSrgb: bgra = true; break;
  case wgpu::TextureFormat::RGBA8Unorm: case wgpu::TextureFormat::RGBA8UnormSrgb: bgra = false; break;
  default: return false;
  }
  if (!task) task = register_encoder_task_type({.label = "PNG export", .callback = encode_image, .afterSubmit = submit_image});
  if (!task) return false;
  job.texture = image.colorTexture; job.width = image.width; job.height = image.height;
  job.pitch = (image.width * 4 + 255) & ~255u; job.bytes = uint64_t(job.pitch) * image.height;
  job.bgra = bgra; job.alpha = alpha; std::strcpy(job.path, path);
  job.state.store(AuroraImageExport_Pending, std::memory_order_release);
  if (push_encoder_task(task, nullptr, 0)) return true;
  job.texture = {}; job.state.store(AuroraImageExport_Failed, std::memory_order_release);
  return false;
}
}

extern "C" AuroraImageExportState aurora_image_export_state() {
  return aurora::gfx::job.state.load(std::memory_order_acquire);
}
extern "C" bool aurora_export_render_png(const char* path, uint32_t width, uint32_t height,
                                          bool alpha, AuroraImageDraw draw, void* userdata) {
  using namespace aurora::gfx;
  if (!draw || !path || !*path || !width || !height || width > 4096 || height > 4096 ||
      std::strlen(path) >= sizeof(job.path) || aurora_image_export_state() == AuroraImageExport_Pending ||
      !create_pass(width, height, true)) return false;
  draw(userdata);
  ResolvedTargets image;
  return resolve_pass({.color = true}, image) && queue_export(image, path, alpha);
}
