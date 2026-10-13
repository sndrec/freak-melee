#ifndef SYSDOLPHIN_SFXSTREAM_H
#define SYSDOLPHIN_SFXSTREAM_H
#include <dolphin/ax.h>
#include <stdbool.h>

typedef struct HSD_StreamInfo {
    u32 sample_rate, voice_count;
    AXPBADDR address[2];
    AXPBADPCM adpcm[2];
} HSD_StreamInfo;
typedef struct HSD_StreamChunk {
    u32 byte_size, end_address, next_offset;
    AXPBADPCMLOOP loop[2];
} HSD_StreamChunk;
// Input buffers are complete 0x80-byte file and 0x20-byte chunk headers.
bool HSD_StreamDecodeInfo(HSD_StreamInfo* info, const u8* bytes);
bool HSD_StreamDecodeChunk(HSD_StreamChunk* chunk, const u8* bytes);
#endif
