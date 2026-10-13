#include "native_fighter_demo_copy.h"

#include <melee/gm/native_content_store.h>
#include <sysdolphin/baselib/dat.h>
#include <stdlib.h>
#include <string.h>

enum { DEMO_CAPACITY = 128, SOURCE_CAPACITY = 16 };

typedef struct DemoPatch {
    const uint8_t* clip;
    uint32_t size;
    uint32_t offset;
} DemoPatch;

static void write32(uint8_t* p, uint32_t value)
{
    p[0] = (uint8_t)(value >> 24);
    p[1] = (uint8_t)(value >> 16);
    p[2] = (uint8_t)(value >> 8);
    p[3] = (uint8_t)value;
}

static bool unique_root(const HSD_Dat* dat, const char* expected, u32* root)
{
    if (expected == NULL || *expected == 0) return false;
    unsigned found = 0;
    for (u32 i = 0; i < dat->root_count; ++i) {
        const char* name;
        u32 offset;
        if (!HSD_DatSymbol(dat, false, i, &offset, &name)) return false;
        if (strcmp(name, expected) == 0) { *root = offset; ++found; }
    }
    return found == 1;
}

bool nativeFighterDemoCopy_Materialize(
    const NativeFighterDemoCopyInput* input,
    const NativeFighterDemoCopySource* sources, unsigned source_count,
    NativeFighterDemoCopyOutput* output)
{
    if (input == NULL || output == NULL || output->fighter_dat != NULL ||
        output->animation_dat != NULL || output->fighter_size != 0 ||
        output->animation_size != 0 || output->copied_clips != 0 ||
        input->demo_count == 0 || input->demo_count > DEMO_CAPACITY ||
        sources == NULL || source_count == 0 || source_count > SOURCE_CAPACITY ||
        input->animation_dat == NULL || input->animation_size < 32 ||
        input->fighter_size > NATIVE_CONTENT_MAX_PACKAGE_BYTES ||
        input->animation_size > NATIVE_CONTENT_MAX_PACKAGE_BYTES)
        return false;

    HSD_Dat fighter, archives[SOURCE_CAPACITY];
    u32 root, table, bases[SOURCE_CAPACITY];
    const char* external;
    if (HSD_DatOpen(&fighter, input->fighter_dat, input->fighter_size) != HSD_DAT_OK ||
        !unique_root(&fighter, input->fighter_root, &root) ||
        !HSD_DatBytes(&fighter, root, 24) ||
        HSD_DatResolve(&fighter, root + 20, &table, &external) != HSD_DAT_REF_LOCAL ||
        !HSD_DatBytes(&fighter, table, input->demo_count * 24)) return false;
    int source_for[DEMO_CAPACITY];
    for (unsigned i = 0; i < DEMO_CAPACITY; ++i) source_for[i] = -1;
    for (unsigned s = 0; s < source_count; ++s) {
        const NativeFighterDemoCopySource* source = sources + s;
        if (source->count == 0 || source->first >= input->demo_count ||
            source->count > input->demo_count - source->first ||
            source->archive_size > NATIVE_CONTENT_MAX_PACKAGE_BYTES ||
            HSD_DatOpen(archives + s, source->archive, source->archive_size) != HSD_DAT_OK ||
            !unique_root(archives + s, source->root_name, bases + s)) return false;
        for (unsigned i = source->first; i < source->first + source->count; ++i) {
            if (source_for[i] != -1) return false;
            source_for[i] = (int)s;
        }
    }
    DemoPatch patches[DEMO_CAPACITY] = {0};
    size_t size = input->animation_size;
    uint32_t copied = 0;
    for (uint32_t i = 0; i < input->demo_count; ++i) {
        u32 clip_size, original, name_at;
        uint32_t record = table + i * 24;
        if (!HSD_DatReadU32(&fighter, record + 8, &clip_size)) return false;
        if (clip_size == 0) continue;
        if (source_for[i] < 0 || clip_size < 32 ||
            !HSD_DatReadU32(&fighter, record + 4, &original) ||
            HSD_DatResolve(&fighter, record, &name_at, &external) != HSD_DAT_REF_LOCAL)
            return false;
        const uint8_t* name = HSD_DatBytes(&fighter, name_at, 1);
        if (name == NULL || memchr(name, 0, fighter.data_size - name_at) == NULL)
            return false;
        unsigned s = (unsigned)source_for[i];
        const HSD_Dat* archive = archives + s;
        if (original > archive->data_size - bases[s]) return false;
        const uint8_t* clip = HSD_DatBytes(archive, bases[s] + original, clip_size);
        HSD_Dat nested;
        u32 nested_root;
        if (clip == NULL || HSD_DatOpen(&nested, clip, clip_size) != HSD_DAT_OK ||
            !unique_root(&nested, (const char*)name, &nested_root)) return false;
        size_t aligned = (size + 31) & ~(size_t)31;
        if (aligned > NATIVE_CONTENT_MAX_PACKAGE_BYTES ||
            clip_size > NATIVE_CONTENT_MAX_PACKAGE_BYTES - aligned) return false;
        patches[i] = (DemoPatch){clip, (uint32_t)clip_size, (uint32_t)aligned};
        size = aligned + clip_size;
        ++copied;
    }
    if (input->fighter_size > NATIVE_CONTENT_MAX_PACKAGE_BYTES - size) return false;
    NativeFighterDemoCopyOutput result = {0};
    result.fighter_dat = malloc(input->fighter_size);
    result.animation_dat = malloc(size);
    if (result.fighter_dat == NULL || result.animation_dat == NULL) {
        nativeFighterDemoCopy_Free(&result);
        return false;
    }
    result.fighter_size = input->fighter_size;
    result.animation_size = size;
    result.copied_clips = copied;
    memcpy(result.fighter_dat, input->fighter_dat, input->fighter_size);
    memcpy(result.animation_dat, input->animation_dat, input->animation_size);
    memset((uint8_t*)result.animation_dat + input->animation_size, 255,
           size - input->animation_size);
    for (unsigned i = 0; i < input->demo_count; ++i) if (patches[i].clip != NULL) {
        const DemoPatch* patch = patches + i;
        memcpy((uint8_t*)result.animation_dat + patch->offset, patch->clip, patch->size);
        uint8_t* record = (uint8_t*)result.fighter_dat + 32 + table + i * 24;
        write32(record + 4, patch->offset);
        write32(record + 8, patch->size);
    }
    *output = result;
    return true;
}

void nativeFighterDemoCopy_Free(NativeFighterDemoCopyOutput* output)
{
    if (output == NULL) return;
    free(output->fighter_dat);
    free(output->animation_dat);
    *output = (NativeFighterDemoCopyOutput){0};
}
