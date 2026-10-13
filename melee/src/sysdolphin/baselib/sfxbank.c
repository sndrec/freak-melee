#include "sfxbank.h"
#include <string.h>

static u32 read32(const u8* p)
{
    return ((u32) p[0] << 24) | ((u32) p[1] << 16) |
           ((u32) p[2] << 8) | p[3];
}

bool HSD_SfxBankDecode(HSD_SfxBank* bank, const u8* data, size_t size,
                       u32 aram_base, u32 silence_address)
{
    size_t cursor = 0;
    for (u32 i = 0; i < bank->count; ++i) {
        if (size - cursor < 8) {
            return false;
        }
        HSD_SfxEntry* entry = &bank->entries[i];
        memset(entry, 0, sizeof(*entry));
        entry->id = bank->first_id + i;
        entry->voice_count = read32(data + cursor);
        entry->sample_rate = read32(data + cursor + 4);
        cursor += 8;
        if (entry->voice_count < 1 || entry->voice_count > 2 ||
            size - cursor < entry->voice_count * 64) {
            return false;
        }
        for (u32 j = 0; j < entry->voice_count; ++j) {
            // Each disk voice contains 16 address, 40 ADPCM, 6 loop bytes,
            // then two padding bytes. None of these fields contains a pointer.
            u16 words[31];
            for (u32 k = 0; k < 31; ++k) {
                words[k] = ((u16) data[cursor + k * 2] << 8) |
                           data[cursor + k * 2 + 1];
            }
            _Static_assert(sizeof(AXPBADDR) == 16, "address fields");
            _Static_assert(sizeof(AXPBADPCM) == 40, "ADPCM fields");
            _Static_assert(sizeof(AXPBADPCMLOOP) == 6, "loop fields");
            memcpy(&entry->voices[j].address, words, 16);
            memcpy(&entry->voices[j].adpcm, words + 8, 40);
            memcpy(&entry->voices[j].loop, words + 28, 6);
            cursor += 64;
        }
    }
    if (cursor != size) {
        return false;
    }
    bank->aram_base = 0;
    HSD_SfxBankRelocate(bank, aram_base, silence_address);
    return true;
}

void HSD_SfxBankRelocate(HSD_SfxBank* bank, u32 aram_base,
                         u32 silence_address)
{
    u32 delta = (aram_base - bank->aram_base) * 2;
    for (u32 i = 0; i < bank->count; ++i) {
        for (u32 j = 0; j < bank->entries[i].voice_count; ++j) {
            AXPBADDR* a = &bank->entries[i].voices[j].address;
            u32 loop = silence_address;
            if (a->loopFlag) {
                loop = (((u32) a->loopAddressHi << 16) |
                        a->loopAddressLo) + delta;
            }
            u32 end = (((u32) a->endAddressHi << 16) |
                       a->endAddressLo) + delta;
            u32 current = (((u32) a->currentAddressHi << 16) |
                           a->currentAddressLo) + delta;
            a->loopAddressHi = loop >> 16;
            a->loopAddressLo = (u16) loop;
            a->endAddressHi = end >> 16;
            a->endAddressLo = (u16) end;
            a->currentAddressHi = current >> 16;
            a->currentAddressLo = (u16) current;
        }
    }
    bank->aram_base = aram_base;
}
