#include <sysdolphin/baselib/sfxstream.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void check(int ok) { if (!ok) { fputs("stream metadata mismatch\n", stderr); exit(1); } }
static u32 be32(const u8* p) { return ((u32)p[0]<<24)|((u32)p[1]<<16)|((u32)p[2]<<8)|p[3]; }
int main(int argc, char** argv)
{
    check(argc == 2);
    FILE* f = fopen(argv[1], "rb"); check(f != NULL);
    u8 bytes[128]; unsigned files = 0, chunks = 0;
    while (fread(bytes, 1, 128, f) == 128) {
        HSD_StreamInfo info;
        check(HSD_StreamDecodeInfo(&info, bytes));
        check(info.voice_count == be32(bytes + 12) && info.sample_rate == be32(bytes + 8));
        for (u32 i = 0; i < info.voice_count; ++i) {
            u16 words[28]; memcpy(words, &info.address[i], 16); memcpy(words + 8, &info.adpcm[i], 40);
            for (u32 j = 0; j < 28; ++j) {
                const u8* p = bytes + 16 + i * 56 + j * 2;
                check(words[j] == ((u16)p[0] << 8 | p[1]));
            }
        }
        bytes[0] = 0; check(!HSD_StreamDecodeInfo(&info, bytes));
        check(fread(bytes, 1, 4, f) == 4);
        u32 count = be32(bytes);
        for (u32 i = 0; i < count; ++i) {
            HSD_StreamChunk chunk;
            check(fread(bytes, 1, 32, f) == 32);
            check(HSD_StreamDecodeChunk(&chunk, bytes));
            check(chunk.byte_size == be32(bytes) && chunk.end_address == be32(bytes + 4) && chunk.next_offset == be32(bytes + 8));
            for (u32 j = 0; j < 2; ++j) {
                u16 words[3]; memcpy(words, &chunk.loop[j], 6);
                for (u32 k = 0; k < 3; ++k) {
                    const u8* p = bytes + 12 + j * 8 + k * 2;
                    check(words[k] == ((u16)p[0] << 8 | p[1]));
                }
            }
            memset(bytes, 0, 4); check(!HSD_StreamDecodeChunk(&chunk, bytes));
            ++chunks;
        }
        ++files;
    }
    fclose(f); check(files == 98 && chunks > 98);
    printf("Verified %u music headers and %u chunk headers\n", files, chunks);
    return 0;
}
