#include "gx_test_common.hpp"

#include <atomic>
#include <thread>

namespace aurora::gfx {
extern uint32_t g_testDrawCount;
extern size_t g_testVertexBytes;
extern size_t g_testIndexBytes;
namespace testing {
extern std::atomic<uint32_t> beginOffscreenCount;
extern std::atomic<uint32_t> endOffscreenCount;
extern std::atomic<uint32_t> resolvePassCount;
}
}

namespace {
unsigned callbacks;
std::thread::id callbackThread;
void draw_done() { ++callbacks; callbackThread = std::this_thread::get_id(); }
void vertices() {
  GXPosition3u8(0, 1, 2);
  GXPosition3u8(3, 4, 5);
  GXPosition3u8(6, 7, 8);
  GXEnd();
}
}

TEST_F(GXFifoTest, CpuOnlyFrameConsumesAllDrawEncodingsAndResumesOutput) {
  using namespace aurora::gfx;
  aurora::gx::fifo::init();
  aurora::gx::fifo::begin_frame(false);
  EXPECT_EQ(aurora::gx::fifo::processing_mode(), aurora::gx::fifo::ProcessingMode::Inline);
  GXClearVtxDesc();
  GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
  GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_U8, 0);
  g_testDrawCount = 0;
  g_testVertexBytes = 0;
  g_testIndexBytes = 0;
  callbacks = 0;
  GXSetDrawDoneCallback(draw_done);

  GXBegin(GX_TRIANGLES, GX_VTXFMT0, 3);
  vertices();
  GXBegin(GX_TRIANGLES, GX_VTXFMT0, GX_AUTO);
  vertices();
  const u16 indices[] = {0, 1, 2};
  GXBeginIndexed(GX_VTXFMT0, 3, indices, 3);
  vertices();
  GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
  GXDrawDone();
  aurora::gx::fifo::end_frame();
  EXPECT_EQ(callbacks, 1u);
  EXPECT_EQ(callbackThread, std::this_thread::get_id());
  EXPECT_EQ(g_testDrawCount, 0u);
  EXPECT_EQ(g_testVertexBytes, 0u);
  EXPECT_EQ(g_testIndexBytes, 0u);
  EXPECT_EQ(gxState().blendMode, GX_BM_BLEND);
  EXPECT_EQ(gxState().vtxDesc[GX_VA_POS], GX_DIRECT);
  EXPECT_EQ(aurora::gx::fifo::get_buffer_size(), 0u);

  // No repeated GX format or blend setters: the CPU-only decoder retained them.
  aurora::gx::fifo::begin_frame(true);
  EXPECT_EQ(aurora::gx::fifo::processing_mode(), aurora::gx::fifo::ProcessingMode::Thread);
  GXBegin(GX_TRIANGLES, GX_VTXFMT0, 3);
  vertices();
  aurora::gx::fifo::end_frame();
  EXPECT_EQ(g_testDrawCount, 1u);
  EXPECT_EQ(g_testVertexBytes, 9u);
  EXPECT_EQ(gxState().blendMode, GX_BM_BLEND);
  GXSetDrawDoneCallback(nullptr);
}

TEST_F(GXFifoTest, CpuOnlyDecoderOwnershipAlternatesWithoutDuplicateCommands) {
  aurora::gx::fifo::init();
  callbacks = 0;
  GXSetDrawDoneCallback(draw_done);
  // Exercise the handoff repeatedly with unpublished tails as well as an
  // explicit callback publication. State must survive without duplicate work.
  for (unsigned frame = 0; frame < 512; ++frame) {
    const bool output = (frame & 1) == 0;
    aurora::gx::fifo::begin_frame(output);
    GXSetColorUpdate((frame & 2) != 0);
    GXSetDrawDone();
    GXSetAlphaUpdate((frame & 4) != 0);
    aurora::gx::fifo::end_frame();
    EXPECT_EQ(callbacks, frame + 1);
    EXPECT_EQ(gxState().colorUpdate, (frame & 2) != 0);
    EXPECT_EQ(gxState().alphaUpdate, (frame & 4) != 0);
    if (output) EXPECT_NE(callbackThread, std::this_thread::get_id());
    else EXPECT_EQ(callbackThread, std::this_thread::get_id());
    EXPECT_EQ(aurora::gx::fifo::get_buffer_size(), 0u);
  }
  GXSetDrawDoneCallback(nullptr);
}

TEST_F(GXFifoTest, CpuOnlyFrameSkipsCopyOffscreenAndDepthWork) {
  using namespace aurora::gfx::testing;
  aurora::gx::fifo::init();
  beginOffscreenCount.store(0);
  endOffscreenCount.store(0);
  resolvePassCount.store(0);
  aurora::gx::fifo::begin_frame(false);
  GXCreateFrameBuffer(320, 180);
  GXCopyTex(reinterpret_cast<void*>(0x1234), GX_FALSE);
  GXRestoreFrameBuffer();
  u32 depth;
  GXPeekZ(0, 0, &depth);
  aurora::gx::fifo::end_frame();
  EXPECT_EQ(beginOffscreenCount.load(), 0u);
  EXPECT_EQ(endOffscreenCount.load(), 0u);
  EXPECT_EQ(resolvePassCount.load(), 0u);
  EXPECT_FALSE(aurora::gfx::depth_peek::testing::snapshot_requested());
  EXPECT_TRUE(gxState().copyTextures.empty());

  aurora::gx::fifo::begin_frame(true);
  GXCreateFrameBuffer(320, 180);
  GXCopyTex(reinterpret_cast<void*>(0x1234), GX_FALSE);
  GXRestoreFrameBuffer();
  GXPeekZ(0, 0, &depth);
  aurora::gx::fifo::end_frame();
  EXPECT_EQ(beginOffscreenCount.load(), 1u);
  EXPECT_EQ(endOffscreenCount.load(), 1u);
  EXPECT_EQ(resolvePassCount.load(), 1u);
  EXPECT_TRUE(aurora::gfx::depth_peek::testing::snapshot_requested());
}
