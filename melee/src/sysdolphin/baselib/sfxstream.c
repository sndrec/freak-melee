#include "sfxstream.h"
#include <string.h>
static u32 read32(const u8* p)
{
    return ((u32) p[0] << 24) | ((u32) p[1] << 16) |
           ((u32) p[2] << 8) | p[3];
}
bool HSD_StreamDecodeInfo(HSD_StreamInfo* info, const u8* bytes)
{
    if (memcmp(bytes, " HALPST", 8) != 0) return false;
    info->sample_rate = read32(bytes + 8);
    info->voice_count = read32(bytes + 12);
    if (!info->sample_rate || info->voice_count < 1 || info->voice_count > 2)
        return false;
    for (u32 i = 0; i < info->voice_count; ++i) {
        u16 words[28];
        const u8* p = bytes + 16 + i * 56;
        for (u32 j = 0; j < 28; ++j)
            words[j] = ((u16) p[j * 2] << 8) | p[j * 2 + 1];
        _Static_assert(sizeof(AXPBADDR) == 16, "address fields");
        _Static_assert(sizeof(AXPBADPCM) == 40, "ADPCM fields");
        memcpy(&info->address[i], words, 16);
        memcpy(&info->adpcm[i], words + 8, 40);
    }
    return true;
}
bool HSD_StreamDecodeChunk(HSD_StreamChunk* chunk, const u8* bytes)
{
    chunk->byte_size = read32(bytes);
    chunk->end_address = read32(bytes + 4);
    chunk->next_offset = read32(bytes + 8);
    // Each ring slot holds 64 KiB of mono or consecutive channel blocks.
    if (!chunk->byte_size || chunk->byte_size > 0x10000 ||
        (chunk->byte_size & 31)) return false;
    for (u32 i = 0; i < 2; ++i) {
        const u8* p = bytes + 12 + i * 8;
        chunk->loop[i].loop_pred_scale = ((u16) p[0] << 8) | p[1];
        chunk->loop[i].loop_yn1 = ((u16) p[2] << 8) | p[3];
        chunk->loop[i].loop_yn2 = ((u16) p[4] << 8) | p[5];
    }
    return true;
}
