#include "sfxbank.h"
#include <stdlib.h>
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

typedef struct SampleRange {
    u32 first_block, last_block;
} SampleRange;

static int compare_sample_range(const void* left, const void* right)
{
    const SampleRange* a = left;
    const SampleRange* b = right;
    return (a->first_block > b->first_block) - (a->first_block < b->first_block);
}

HSD_SfxBank* HSD_SfxBankReadPackage(const void* bytes, size_t size, u32 owner)
{
    if (!bytes || size < 16 || size > 4 * 1024 * 1024 ||
        owner >= HSD_PACKAGE_SOUND_OWNERS)
    {
        return NULL;
    }
    const u8* data = bytes;
    u32 metadata = read32(data);
    u32 samples = read32(data + 4);
    u32 count = read32(data + 8);
    if (!count || count > HSD_PACKAGE_SOUND_CLIPS || read32(data + 12) ||
        metadata > size - 16 || !samples || (samples & 31))
    {
        return NULL;
    }
    size_t sample_offset = (16 + metadata + 31) & ~(size_t)31;
    if (sample_offset > size || samples != size - sample_offset) {
        return NULL;
    }
    HSD_SfxBank* bank = calloc(1, sizeof(*bank) + count * sizeof(HSD_SfxEntry));
    if (!bank) {
        return NULL;
    }
    bank->first_id = HSD_PACKAGE_SOUND_BASE + owner * HSD_PACKAGE_SOUND_CLIPS;
    bank->count = count;
    if (!HSD_SfxBankDecode(bank, data + 16, metadata, 0, 0)) {
        goto invalid;
    }
    bank->sample_data = data + sample_offset;
    bank->sample_bytes = samples;
    SampleRange ranges[2 * HSD_PACKAGE_SOUND_CLIPS];
    size_t range_count = 0;
    for (u32 i = 0; i < count; ++i) {
        HSD_SfxEntry* entry = &bank->entries[i];
        if (entry->sample_rate < 8000 || entry->sample_rate > 48000) {
            goto invalid;
        }
        for (u32 j = 0; j < entry->voice_count; ++j) {
            const HSD_SfxVoice* voice = &entry->voices[j];
            const AXPBADDR* address = &voice->address;
            u32 start = (u32)address->currentAddressHi << 16 | address->currentAddressLo;
            u32 end = (u32)address->endAddressHi << 16 | address->endAddressLo;
            u32 loop = (u32)address->loopAddressHi << 16 | address->loopAddressLo;
            if (address->format || address->loopFlag > 1 || (start & 15) != 2 ||
                (end & 15) < 2 || start > end || end >= samples * 2 ||
                voice->adpcm.pred_scale > 127 ||
                (address->loopFlag && ((loop & 15) < 2 || loop < start ||
                    loop > end || voice->loop.loop_pred_scale > 127)))
            {
                goto invalid;
            }
            ranges[range_count++] = (SampleRange){start / 16, end / 16};
        }
    }
    // A DSP block holds 14 samples and a predictor/scale byte. Banks can
    // share samples across clips. Inspect each referenced block only once.
    qsort(ranges, range_count, sizeof(ranges[0]), compare_sample_range);
    u32 next = 0;
    for (size_t i = 0; i < range_count; ++i) {
        if (next < ranges[i].first_block) next = ranges[i].first_block;
        while (next <= ranges[i].last_block) {
            if (bank->sample_data[next * 8] > 127) goto invalid;
            ++next;
        }
    }
    return bank;

invalid:
    free(bank);
    return NULL;
}
