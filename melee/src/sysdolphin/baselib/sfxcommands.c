#include "sfxcommands.h"

static u32 read_word(const u8* data, size_t word)
{
    const u8* p = data + word * 4;
    return (u32) p[0] << 24 | (u32) p[1] << 16 | (u32) p[2] << 8 | p[3];
}

bool HSD_SfxCommandsDecode(HSD_SfxCommands* commands, void* data, size_t size)
{
    if (!commands || !data || (uintptr_t) data % 4 || size % 4) return false;
    const u8* bytes = data;
    size_t total = size / 4, cursor = 0;
    size_t table[5];
    u32 count[5];
    for (int i = 0; i < 5; ++i) {
        if (cursor >= total) return false;
        count[i] = read_word(bytes, cursor++);
        table[i] = cursor;
        if (count[i] > total - cursor) return false;
        cursor += count[i];
    }
    // Tables 1, 3 and 4 contain file-relative stream offsets. Tables 0 and 2
    // are bank indices. Only the SFX tables 2/3 are consumed by this driver.
    for (int i = 1; i < 5; ++i) {
        if (i == 2) continue;
        for (u32 j = 0; j < count[i]; ++j) {
            u32 offset = read_word(bytes, table[i] + j);
            if (offset % 4 || offset / 4 < cursor || offset / 4 >= total)
                return false;
        }
    }
    u32 previous = 0;
    for (u32 j = 0; j < count[2]; ++j) {
        u32 index = read_word(bytes, table[2] + j);
        if (index < previous || index > count[3]) return false;
        previous = index;
    }
    u32* words = data;
    for (size_t i = 0; i < total; ++i) words[i] = read_word(bytes, i);
    *commands = (HSD_SfxCommands) { words, words + table[2], words + table[3],
                                  count[2], count[3] };
    return true;
}
