#include <gtest/gtest.h>
#include <aurora/aurora.h>
#include "gfx/presented_capture.hpp"
#include "gfx/render_worker.hpp"
#include "webgpu/gpu.hpp"
#include <SDL3/SDL_timer.h>
#include <png.h>
#include <array>
#include <filesystem>
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
  gfx::presented_capture::publish(slot);
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

void expect_pixels(const std::filesystem::path& path, unsigned width, unsigned height,
                   std::array<uint8_t, 3> color) {
  png_image image{}; image.version = PNG_IMAGE_VERSION;
  ASSERT_TRUE(png_image_begin_read_from_file(&image, path.string().c_str()));
  ASSERT_EQ(image.width, width); ASSERT_EQ(image.height, height);
  image.format = PNG_FORMAT_RGB;
  std::vector<uint8_t> pixels(PNG_IMAGE_SIZE(image));
  ASSERT_TRUE(png_image_finish_read(&image, nullptr, pixels.data(), 0, nullptr));
  png_image_free(&image);
  for (size_t i = 0; i < pixels.size(); i += 3)
    ASSERT_TRUE(std::equal(color.begin(), color.end(), pixels.begin() + i)) << "Pixel " << i / 3;
}

TEST(PresentedCapture, KeepsPressedFrameAcrossQueuedRendersResizeReleaseAndFailure) {
  const auto directory = std::filesystem::temp_directory_path() / "freak-melee-capture-test";
  std::filesystem::create_directories(directory);
  const auto red = (directory / "reserved-red.png").string();
  const auto green = (directory / "later-green.png").string();
  const AuroraConfig config{.appName = "Presented capture test", .hideWindow = true,
      .windowWidth = 640, .windowHeight = 480, .logLevel = LOG_WARNING};
  char executable[] = "presented_capture_tests"; char* args[]{executable, nullptr};
  aurora_initialize(1, args, &config);
  ASSERT_FALSE(aurora_capture_presented(red.c_str(), true, nullptr));
  gfx::render_worker::enqueue_work([] { present_color(64, 32, {255, 0, 0, 255}); });
  gfx::render_worker::synchronize();

  // Future frames are already queued when the keypress reserves the red frame.
  // A delayed readback must not silently bind to these newer green frames.
  std::latch gate{1};
  gfx::render_worker::enqueue_work([&] { gate.wait(); });
  for (unsigned i = 0; i < 8; ++i)
    gfx::render_worker::enqueue_work([] { present_color(128, 64, {0, 255, 0, 255}); });
  AuroraPresentedCapture pressed{};
  const bool accepted = aurora_capture_presented(red.c_str(), true, &pressed);
  gate.count_down();
  ASSERT_TRUE(accepted);
  EXPECT_EQ(pressed.width, 64u); EXPECT_EQ(pressed.height, 32u);
  const auto saved = wait_capture();
  ASSERT_EQ(saved.status, AURORA_CAPTURE_READY) << saved.error;
  EXPECT_EQ(saved.displayed_frame, pressed.displayed_frame);
  EXPECT_EQ(saved.capture_id, pressed.capture_id);
  expect_pixels(red, 64, 32, {255, 0, 0});
  const auto exported = (directory / "reserved-export.png").string();
  EXPECT_TRUE(aurora_write_presented_capture(exported.c_str()));
  EXPECT_EQ(wait_capture().capture_id, pressed.capture_id);
  expect_pixels(exported, 64, 32, {255, 0, 0});
  EXPECT_FALSE(aurora_capture_presented(green.c_str(), false, nullptr));
  aurora_set_capture_backdrop(true);
  const auto frozen = gfx::presented_capture::backdrop();
  EXPECT_TRUE(frozen.bind_group); EXPECT_EQ(frozen.extent.width, 64u);
  aurora_release_presented_capture(); gfx::render_worker::synchronize();
  EXPECT_FALSE(gfx::presented_capture::backdrop().bind_group);
  AuroraPresentedCapture later{};
  ASSERT_TRUE(aurora_capture_presented(green.c_str(), false, &later));
  ASSERT_EQ(wait_capture().status, AURORA_CAPTURE_READY);
  EXPECT_GT(later.displayed_frame, pressed.displayed_frame);
  expect_pixels(green, 128, 64, {0, 255, 0});
  EXPECT_FALSE(aurora_capture_presented("invalid.bmp", false, nullptr));
  const auto invalid = (directory / "missing-directory" / "failed.png").string();
  ASSERT_TRUE(aurora_capture_presented(invalid.c_str(), false, nullptr));
  EXPECT_EQ(wait_capture().status, AURORA_CAPTURE_FAILED);
  // A cancelled request cannot keep the fixed image slots occupied forever.
  // Cancelling before writing also preserves a previously saved destination.
  std::latch cancel_gate{1};
  gfx::render_worker::enqueue_work([&] { cancel_gate.wait(); });
  const bool cancelled_accepted = aurora_capture_presented(green.c_str(), true, nullptr);
  aurora_release_presented_capture(); cancel_gate.count_down(); gfx::render_worker::synchronize();
  ASSERT_TRUE(cancelled_accepted);
  wait_capture();
  expect_pixels(green, 128, 64, {0, 255, 0});
  ASSERT_TRUE(aurora_capture_presented(green.c_str(), false, nullptr));
  EXPECT_EQ(wait_capture().status, AURORA_CAPTURE_READY);
  aurora_shutdown();
  std::filesystem::remove(red); std::filesystem::remove(green); std::filesystem::remove(exported);
}
}
