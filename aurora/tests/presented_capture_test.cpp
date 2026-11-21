#include <gtest/gtest.h>
#include <aurora/aurora.h>
#include "gfx/presented_capture.hpp"
#include "gfx/render_worker.hpp"
#include "webgpu/gpu.hpp"
#include <SDL3/SDL_timer.h>
#include <png.h>
#include <array>
#include <string>
#include <latch>
#include <vector>

namespace {
using namespace aurora;

void present_color(uint32_t width, uint32_t height, std::array<uint8_t, 4> color) {
  const wgpu::Extent3D extent{width, height, 1};
  const wgpu::TextureDescriptor description{.usage = wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst,
      .dimension = wgpu::TextureDimension::e2D, .size = extent, .format = wgpu::TextureFormat::RGBA8Unorm,
      .mipLevelCount = 1, .sampleCount = 1};
  const auto source = webgpu::g_device.CreateTexture(&description);
  std::vector<uint8_t> pixels(size_t(width) * height * 4);
  for (size_t i = 0; i < pixels.size(); i += 4) std::copy(color.begin(), color.end(), pixels.begin() + i);
  const wgpu::TexelCopyTextureInfo destination{.texture = source};
  const wgpu::TexelCopyBufferLayout layout{.bytesPerRow = width * 4, .rowsPerImage = height};
  webgpu::g_queue.WriteTexture(&destination, pixels.data(), pixels.size(), &layout, &extent);
  const auto encoder = webgpu::g_device.CreateCommandEncoder();
  const int slot = gfx::presented_capture::retain(encoder, source, description.format, extent);
  EXPECT_GE(slot, 0);
  const auto commands = encoder.Finish(); webgpu::g_queue.Submit(1, &commands);
  gfx::presented_capture::publish(slot, gfx::presented_capture::take_request(), {0, 0, float(width), float(height)});
}

AuroraPresentedCapture wait_capture() {
  AuroraPresentedCapture result{};
  const auto deadline = SDL_GetTicks() + 10000;
  do {
    // Dawn's event pump can submit GPU work. Keep it on the renderer's owner
    // thread, just as the production frame loop does.
    gfx::render_worker::enqueue_work([] { webgpu::g_instance.ProcessEvents(); });
    gfx::render_worker::synchronize();
    aurora_presented_capture_status(&result);
    if (result.status != AURORA_CAPTURE_PENDING) return result;
    SDL_Delay(1);
  } while (SDL_GetTicks() < deadline);
  return result;
}

void expect_pixels(uint64_t id, unsigned width, unsigned height,
                   std::array<uint8_t, 3> color) {
  png_image image{}; image.version = PNG_IMAGE_VERSION;
  size_t size = 0;
  const void* bytes = aurora_presented_png(id, &size);
  ASSERT_NE(bytes, nullptr);
  ASSERT_TRUE(png_image_begin_read_from_memory(&image, bytes, size));
  ASSERT_EQ(image.width, width); ASSERT_EQ(image.height, height);
  image.format = PNG_FORMAT_RGB;
  std::vector<uint8_t> pixels(PNG_IMAGE_SIZE(image));
  ASSERT_TRUE(png_image_finish_read(&image, nullptr, pixels.data(), 0, nullptr));
  png_image_free(&image);
  for (size_t i = 0; i < pixels.size(); i += 3)
    ASSERT_TRUE(std::equal(color.begin(), color.end(), pixels.begin() + i)) << "Pixel " << i / 3;
}

TEST(PresentedCapture, KeepsPressedFrameAcrossQueuedRendersResizeReleaseAndFailure) {
  const AuroraConfig config{.appName = "Presented capture test", .hideWindow = true,
      .windowWidth = 640, .windowHeight = 480, .logLevel = LOG_WARNING};
  char executable[] = "presented_capture_tests"; char* args[]{executable, nullptr};
  aurora_initialize(1, args, &config);
  AuroraPresentedCapture pressed{}, later{};
  ASSERT_FALSE(aurora_capture_presented(true, &pressed));
  gfx::render_worker::enqueue_work([] { present_color(64, 32, {255, 0, 0, 255}); });
  gfx::render_worker::synchronize();

  // Future frames are already queued when the keypress reserves the red frame.
  // A delayed readback must not silently bind to these newer green frames.
  std::latch gate{1};
  gfx::render_worker::enqueue_work([&] { gate.wait(); });
  for (unsigned i = 0; i < 8; ++i)
    gfx::render_worker::enqueue_work([] { present_color(128, 64, {0, 255, 0, 255}); });
  const bool accepted = aurora_capture_presented(true, &pressed);
  gate.count_down();
  ASSERT_TRUE(accepted);
  EXPECT_EQ(pressed.width, 64u); EXPECT_EQ(pressed.height, 32u);
  const auto saved = wait_capture();
  ASSERT_EQ(saved.status, AURORA_CAPTURE_READY) << saved.error;
  EXPECT_EQ(saved.displayed_frame, pressed.displayed_frame);
  EXPECT_EQ(saved.capture_id, pressed.capture_id);
  expect_pixels(pressed.capture_id, 64, 32, {255, 0, 0});
  EXPECT_TRUE(aurora_retry_presented_capture(pressed.capture_id));
  EXPECT_EQ(wait_capture().capture_id, pressed.capture_id);
  expect_pixels(pressed.capture_id, 64, 32, {255, 0, 0});
  EXPECT_FALSE(aurora_capture_presented(false, &later));
  aurora_set_capture_backdrop(true);
  const auto frozen = gfx::presented_capture::backdrop();
  EXPECT_TRUE(frozen.bind_group); EXPECT_EQ(frozen.extent.width, 64u);
  aurora_release_presented_capture(pressed.capture_id); gfx::render_worker::synchronize();
  EXPECT_FALSE(gfx::presented_capture::backdrop().bind_group);
  ASSERT_TRUE(aurora_capture_presented(false, &later));
  ASSERT_EQ(wait_capture().status, AURORA_CAPTURE_READY);
  EXPECT_GT(later.displayed_frame, pressed.displayed_frame);
  aurora_release_presented_capture(pressed.capture_id);
  expect_pixels(later.capture_id, 128, 64, {0, 255, 0});
  aurora_release_presented_capture(later.capture_id);
  gfx::render_worker::synchronize();

  AuroraPresentedCapture next{};
  ASSERT_TRUE(aurora_capture_next_frame(true, &next));
  gfx::render_worker::enqueue_work([] {
    gfx::presented_capture::publish(-1, gfx::presented_capture::take_request(), {});
  });
  EXPECT_EQ(wait_capture().status, AURORA_CAPTURE_FAILED);
  aurora_release_presented_capture(next.capture_id);
  gfx::render_worker::synchronize();
  ASSERT_TRUE(aurora_capture_next_frame(true, &next));
  aurora_release_presented_capture(next.capture_id);
  gfx::render_worker::synchronize();
  std::latch cancel_gate{1};
  gfx::render_worker::enqueue_work([&] { cancel_gate.wait(); });
  const bool accepted_for_cancel = aurora_capture_presented(true, &next);
  aurora_release_presented_capture(next.capture_id);
  cancel_gate.count_down();
  gfx::render_worker::synchronize();
  ASSERT_TRUE(accepted_for_cancel);
  size_t size = 1;
  EXPECT_EQ(aurora_presented_png(next.capture_id, &size), nullptr);
  EXPECT_EQ(size, 0u);
  ASSERT_TRUE(aurora_capture_presented(false, &later));
  ASSERT_EQ(wait_capture().status, AURORA_CAPTURE_READY);
  expect_pixels(later.capture_id, 128, 64, {0, 255, 0});
  aurora_release_presented_capture(later.capture_id);
  gfx::render_worker::synchronize();
  aurora_shutdown();
}
}
