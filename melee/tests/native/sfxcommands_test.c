#include <sysdolphin/baselib/sfxcommands.h>
#include <string.h>
#include <stdio.h>

static void put(u32* data, unsigned i, u32 value)
{
    u8* p = (u8*) &data[i];
    p[0] = value >> 24; p[1] = value >> 16;
    p[2] = value >> 8; p[3] = value;
}

int main(void)
{
    const u32 fixture[] = { 0, 0, 2, 0, 1, 2, 36, 40, 0,
                            0x01001234, 0x0F000000 };
    u32 original[11], data[11];
    for (unsigned i = 0; i < 11; ++i) put(original, i, fixture[i]);
    HSD_SfxCommands commands = { 0 };
    for (size_t size = 0; size < sizeof(data); ++size) {
        memcpy(data, original, sizeof(data));
        if (HSD_SfxCommandsDecode(&commands, data, size)) return 1;
        if (memcmp(data, original, sizeof(data))) return 1;
    }
    const unsigned fields[] = { 2, 4, 6, 6, 7 };
    const u32 invalid[] = { 0xFFFFFFFF, 3, 35, 32, 44 };
    for (unsigned i = 0; i < 5; ++i) {
        memcpy(data, original, sizeof(data));
        put(data, fields[i], invalid[i]);
        if (HSD_SfxCommandsDecode(&commands, data, sizeof(data))) return 1;
    }
    memcpy(data, original, sizeof(data));
    if (!HSD_SfxCommandsDecode(&commands, data, sizeof(data))) return 1;
    if (commands.bank_count != 2 || commands.stream_count != 2 ||
        commands.banks[1] != 1 || commands.words != data ||
        commands.words[commands.streams[0] / 4] != 0x01001234 ||
        commands.words[commands.streams[1] / 4] != 0x0F000000) return 1;
    puts("SEM word conversion, bank lookup and invalid-file checks passed");
    return 0;
}
