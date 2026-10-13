#include "gx_test_common.hpp"
#include "__gx.h"

#include <chrono>
#include <cstdio>

#ifdef _MSC_VER
#define REFERENCE_NOINLINE __declspec(noinline)
#else
#define REFERENCE_NOINLINE __attribute__((noinline))
#endif

namespace {
// The scalar encoding before packet batching, kept only as the independent
// reference for byte ordering and frontend mutations in these tests.
REFERENCE_NOINLINE void scalar_texture(GXTexObj_& obj, GXTexMapID id) {
  const u32 bank = id < 4 ? 0x80 : 0xa0;
  const u32 slot = id & 3;
  obj.mode0 = (obj.mode0 & 0xffffff) | ((bank + slot) << 24);
  obj.mode1 = (obj.mode1 & 0xffffff) | ((bank + 4 + slot) << 24);
  obj.image0 = (obj.image0 & 0xffffff) | ((bank + 8 + slot) << 24);
  obj.image3 = (obj.image3 & 0xffffff) | ((bank + 20 + slot) << 24);
  GX_WRITE_RAS_REG(obj.mode0);
  GX_WRITE_RAS_REG(obj.mode1);
  GX_WRITE_RAS_REG(obj.image0);
  GX_WRITE_RAS_REG((bank + 12 + slot) << 24);
  GX_WRITE_RAS_REG((bank + 16 + slot) << 24);
  GX_WRITE_RAS_REG(obj.image3);
  if ((obj.flags & 2) == 0)
    GX_WRITE_RAS_REG(((bank + 24 + slot) << 24) | (static_cast<u32>(obj.tlut) & 1023));
  __gx->tImage0[id] = obj.image0;
  __gx->tMode0[id] = obj.mode0;
  __gx->dirtyState |= 1;
  __gx->bpSent = 1;
  GX_WRITE_AURORA(GX_AURORA_LOAD_TEXOBJ);
  GX_WRITE_U8(static_cast<u8>(id));
  GX_WRITE_U64(reinterpret_cast<u64>(obj.data));
  GX_WRITE_U32(obj.width());
  GX_WRITE_U32(obj.height());
  GX_WRITE_U32(obj.format());
  GX_WRITE_U32(static_cast<u32>(obj.tlut));
  GX_WRITE_U8(static_cast<u8>(obj.has_mips()));
  GX_WRITE_U32(obj.texObjId);
  GX_WRITE_U32(obj.texDataVersion);
}

REFERENCE_NOINLINE void scalar_palette(const GXTlutObj_& obj, u32 idx) {
  __GXFlushTextureState();
  GX_WRITE_RAS_REG(obj.loadTlut0);
  GX_WRITE_RAS_REG(0x65000000 | ((obj.numEntries - 1) & 1023) << 10 | (idx & 1023));
  __GXFlushTextureState();
  GX_WRITE_AURORA(GX_AURORA_LOAD_TLUT);
  GX_WRITE_U8(static_cast<u8>(idx));
  GX_WRITE_U64(reinterpret_cast<u64>(obj.data));
  GX_WRITE_U32(static_cast<u32>(obj.format));
  GX_WRITE_U16(obj.numEntries);
  GX_WRITE_U32(obj.tlutObjId);
  GX_WRITE_U32(obj.tlutDataVersion);
}

void expect_texture(const GXTexObj_& decoded, const GXTexObj_& source) {
  EXPECT_EQ(decoded.data, source.data);
  EXPECT_EQ(decoded.width(), source.width());
  EXPECT_EQ(decoded.height(), source.height());
  EXPECT_EQ(decoded.format(), source.format());
  EXPECT_EQ(decoded.tlut, source.tlut);
  EXPECT_EQ(decoded.has_mips(), source.has_mips());
  EXPECT_EQ(decoded.texObjId, source.texObjId);
  EXPECT_EQ(decoded.texDataVersion, source.texDataVersion);
}
} // namespace

TEST_F(GXFifoTest, TexturePacketsMatchScalarEncodingAcrossEveryMapAndFormat) {
  std::array<u8, 4096> data{};
  for (u32 map = 0; map < 8; ++map) {
    for (u32 indexed = 0; indexed < 2; ++indexed) {
      for (u32 mip = 0; mip < 2; ++mip) {
        SCOPED_TRACE(map * 4 + indexed * 2 + mip);
        GXTexObj texture{};
        if (indexed)
          GXInitTexObjCI(&texture, data.data(), 123, 69, GX_TF_C8, GX_REPEAT, GX_MIRROR,
                        static_cast<GXBool>(mip), GX_TLUT5);
        else
          GXInitTexObj(&texture, data.data(), 123, 69, GX_TF_RGBA8, GX_REPEAT, GX_MIRROR,
                       static_cast<GXBool>(mip));
        GXInitTexObjLOD(&texture, GX_LIN_MIP_LIN, GX_LINEAR, 1, 4, -0.5f,
                        GX_TRUE, GX_FALSE, GX_ANISO_2);
        GXInitTexObjData(&texture, data.data() + 32);
        auto reference = *reinterpret_cast<GXTexObj_*>(&texture);
        const auto frontend = *__gx;
        scalar_texture(reference, static_cast<GXTexMapID>(map));
        const auto expectedFrontend = *__gx;
        const auto expected = capture_fifo();
        *__gx = frontend;
        GXLoadTexObj(&texture, static_cast<GXTexMapID>(map));
        const auto actual = capture_fifo();
        EXPECT_EQ(actual, expected);
        EXPECT_EQ(actual.size(), indexed ? 72u : 67u);
        EXPECT_EQ(std::memcmp(__gx, &expectedFrontend, sizeof(*__gx)), 0);
        EXPECT_EQ(std::memcmp(&texture, &reference, sizeof(reference)), 0);
        reset_gx_state();
        decode_fifo(actual);
        expect_texture(gxState().loadedTextures[map], reference);
        EXPECT_EQ(gxState().loadedTextures[map].mode0, reference.mode0);
        EXPECT_EQ(gxState().loadedTextures[map].mode1, reference.mode1);
      }
    }
  }
}

TEST_F(GXFifoTest, PalettePacketsMatchScalarEncodingAndFrontendState) {
  std::array<u8, 512> data{};
  for (u32 slot = 0; slot < aurora::gx::MaxTluts; ++slot) {
    SCOPED_TRACE(slot);
    GXTlutObj palette{};
    GXInitTlutObj(&palette, data.data(), static_cast<GXTlutFmt>(slot % 3), 127);
    GXInitTlutObjData(&palette, data.data() + 32);
    const auto& reference = *reinterpret_cast<GXTlutObj_*>(&palette);
    __gx->bpMask = 0xfe123456;
    __gx->bpSent = 0;
    const auto frontend = *__gx;
    scalar_palette(reference, slot);
    const auto expectedFrontend = *__gx;
    const auto expected = capture_fifo();
    *__gx = frontend;
    GXLoadTlut(&palette, slot);
    const auto actual = capture_fifo();
    EXPECT_EQ(actual, expected);
    EXPECT_EQ(actual.size(), 46u);
    EXPECT_EQ(std::memcmp(__gx, &expectedFrontend, sizeof(*__gx)), 0);
    reset_gx_state();
    decode_fifo(actual);
    const auto& decoded = gxState().loadedTluts[slot];
    EXPECT_EQ(decoded.data, reference.data);
    EXPECT_EQ(decoded.format, reference.format);
    EXPECT_EQ(decoded.numEntries, reference.numEntries);
    EXPECT_EQ(decoded.tlutObjId, reference.tlutObjId);
    EXPECT_EQ(decoded.tlutDataVersion, reference.tlutDataVersion);
  }
}

TEST_F(GXFifoTest, TexturePacketsSurviveDisplayListCaptureAndDecoderOwnershipChanges) {
  std::array<u8, 512> data{};
  std::array<u8, 2048> list{};
  std::array<u8, 2048> referenceList{};
  GXTexObj texture{};
  GXTlutObj palette{};
  GXInitTexObjCI(&texture, data.data(), 16, 16, GX_TF_C8, GX_MIRROR, GX_REPEAT, GX_FALSE, GX_TLUT3);
  GXInitTlutObj(&palette, data.data() + 256, GX_TL_RGB5A3, 16);
  __GXSetDirtyState();
  capture_fifo();
  const auto frontend = *__gx;
  auto referenceTexture = *reinterpret_cast<GXTexObj_*>(&texture);
  GXBeginDisplayList(referenceList.data(), static_cast<u32>(referenceList.size()));
  scalar_palette(*reinterpret_cast<GXTlutObj_*>(&palette), GX_TLUT3);
  scalar_texture(referenceTexture, GX_TEXMAP7);
  const u32 referenceSize = GXEndDisplayList();
  *__gx = frontend;
  capture_fifo();
  aurora::gx::fifo::init();
  aurora::gx::fifo::begin_frame(false);
  GXBeginDisplayList(list.data(), static_cast<u32>(list.size()));
  GXLoadTlut(&palette, GX_TLUT3);
  GXLoadTexObj(&texture, GX_TEXMAP7);
  const u32 size = GXEndDisplayList();
  ASSERT_LE(size, list.size());
  ASSERT_EQ(size, referenceSize);
  EXPECT_TRUE(std::equal(list.begin(), list.begin() + size, referenceList.begin()));
  GXCallDisplayList(list.data(), size);
  aurora::gx::fifo::end_frame();
  expect_texture(gxState().loadedTextures[7], *reinterpret_cast<GXTexObj_*>(&texture));
  EXPECT_EQ(gxState().loadedTluts[3].data, data.data() + 256);
  // No repeated loads in the next visible frame: bindings remain available.
  aurora::gx::fifo::begin_frame(true);
  GXSetColorUpdate(GX_TRUE);
  aurora::gx::fifo::end_frame();
  expect_texture(gxState().loadedTextures[7], *reinterpret_cast<GXTexObj_*>(&texture));
  EXPECT_EQ(gxState().loadedTluts[3].numEntries, 16);
}

TEST_F(GXFifoTest, UndersizedDisplayListDoesNotReceivePartialTexturePacket) {
  GXTexObj texture{};
  GXInitTexObj(&texture, nullptr, 16, 16, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
  std::array<u8, 69> guarded;
  guarded.fill(0xa5);
  // An invalid undersized capture is rejected atomically, rather than writing
  // scalar fragments of a malformed texture load. Neither guard is touched.
  aurora::gx::fifo::begin_display_list(guarded.data() + 1, 66);
  GXLoadTexObj(&texture, GX_TEXMAP0);
  EXPECT_EQ(aurora::gx::fifo::end_display_list(), 0u);
  EXPECT_TRUE(std::all_of(guarded.begin(), guarded.end(), [](u8 b) { return b == 0xa5; }));
}

TEST_F(GXFifoTest, TexturePacketRemainsCompleteAcrossFifoGrowth) {
  GXTexObj texture{};
  GXInitTexObjCI(&texture, nullptr, 16, 16, GX_TF_C8, GX_CLAMP, GX_CLAMP, GX_FALSE, GX_TLUT2);
  auto reference = *reinterpret_cast<GXTexObj_*>(&texture);
  scalar_texture(reference, GX_TEXMAP2);
  const auto expected = capture_fifo();
  // Leave less than one load in the current allocation, forcing the existing
  // growth path rather than the ordinary contiguous append.
  const u32 prefixSize = aurora::gx::fifo::detail::sBufferCapacity - 8;
  const std::vector<u8> prefix(prefixSize, GX_NOP);
  aurora::gx::fifo::write_data(prefix.data(), prefixSize);
  GXLoadTexObj(&texture, GX_TEXMAP2);
  const auto actual = capture_fifo();
  ASSERT_EQ(actual.size(), prefixSize + expected.size());
  EXPECT_TRUE(std::equal(expected.begin(), expected.end(), actual.begin() + prefixSize));
  reset_gx_state();
  decode_fifo(actual);
  expect_texture(gxState().loadedTextures[2], reference);
}

// Optional diagnostic, excluded from ordinary correctness runs. The scalar
// reference and real API both cross a function boundary, reuse the same FIFO
// capacity and mutate the same frontend; decoding/GPU costs are not measured.
TEST_F(GXFifoTest, DISABLED_TexturePacketAppendBenchmark) {
  std::array<u8, 512> data{};
  GXTexObj texture{};
  GXTlutObj palette{};
  GXInitTexObjCI(&texture, data.data(), 16, 16, GX_TF_C8, GX_MIRROR, GX_REPEAT, GX_TRUE, GX_TLUT3);
  GXInitTlutObj(&palette, data.data() + 256, GX_TL_RGB5A3, 16);
  for (bool tlut : {false, true}) {
    for (u32 sample = 0; sample < 8; ++sample) {
      for (u32 order = 0; order < 2; ++order) {
        const bool batched = ((sample + order) & 1) != 0;
        const auto emit = [&] {
          for (u32 item = 0; item < 512; ++item) {
            if (tlut) {
              if (batched) GXLoadTlut(&palette, GX_TLUT3);
              else scalar_palette(*reinterpret_cast<GXTlutObj_*>(&palette), GX_TLUT3);
            } else {
              if (batched) GXLoadTexObj(&texture, GX_TEXMAP7);
              else scalar_texture(*reinterpret_cast<GXTexObj_*>(&texture), GX_TEXMAP7);
            }
          }
          EXPECT_EQ(aurora::gx::fifo::get_buffer_size(), 512u * (tlut ? 46u : 72u));
          aurora::gx::fifo::clear_buffer();
        };
        emit(); // Grow/warm capacity before measuring.
        const auto start = std::chrono::steady_clock::now();
        for (u32 batch = 0; batch < 128; ++batch) emit();
        const double us = std::chrono::duration<double, std::micro>(
            std::chrono::steady_clock::now() - start).count();
        std::printf("[texture-append] palette=%u batched=%u sample=%u loads=65536 ns_per_load=%.3f\n",
                    static_cast<unsigned>(tlut), static_cast<unsigned>(batched), sample, us * 1000 / 65536);
      }
    }
  }
}
