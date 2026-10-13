#ifndef SYSDOLPHIN_SFXBANK_H
#define SYSDOLPHIN_SFXBANK_H

#include <dolphin/ax.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct HSD_SfxVoice {
    AXPBADDR address;
    AXPBADPCM adpcm;
    AXPBADPCMLOOP loop;
} HSD_SfxVoice;

typedef struct HSD_SfxEntry {
    struct HSD_SfxEntry* next;
    int id;
    u32 voice_count;
    u32 sample_rate;
    HSD_SfxVoice voices[2];
} HSD_SfxEntry;

typedef struct HSD_SfxBank {
    struct HSD_SfxBank* next;
    int entrynum;
    u32 first_id;
    u32 count;
    u32 aram_base;
    u32 aram_size;
    HSD_SfxEntry entries[];
} HSD_SfxBank;

// Decode the header payload beginning at file offset 0x10 into caller storage.
// Bank count/first_id are initialized by the caller from the file header.
bool HSD_SfxBankDecode(HSD_SfxBank* bank, const u8* data, size_t size,
                       u32 aram_base, u32 silence_address);
void HSD_SfxBankRelocate(HSD_SfxBank* bank, u32 aram_base,
                         u32 silence_address);
#endif
