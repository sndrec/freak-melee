#include "efassets.h"

#include <melee/lb/lbfile.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/memory.h>

#include <stdint.h>

enum {
    EFFECT_IMPORT_EXTRA_BYTES = 2 * 1024 * 1024,
};

EF_ImportedData* efAssets_Load(const char* filename, const char* root_name)
{
    EF_ImportedData* output;
    HSD_DatResult status;
    void* arena;
    void* bytes;
    size_t arena_size;
    size_t size;
    u32 error_offset;

    size = lbFileGetSize(filename);
    bytes = HSD_MemAlloc((size + 31) & ~(size_t) 31);
    if (bytes == NULL) {
        return NULL;
    }
    lbFile_8001668C(filename, bytes, &size);
    if (size > (SIZE_MAX - EFFECT_IMPORT_EXTRA_BYTES) / 4) {
        return NULL;
    }
    arena_size = size * 4 + EFFECT_IMPORT_EXTRA_BYTES;
    arena = HSD_MemAlloc(arena_size);
    if (arena == NULL) {
        return NULL;
    }
    output = efAssets_Import(bytes, size, root_name, arena, arena_size, &status,
                             &error_offset);
    if (output == NULL) {
        OSReport("%s conversion failed at %lx (%d)\n", filename, error_offset,
                 status);
        return NULL;
    }
    OSReport("%s: imported %lu effects, %ld particle commands, %lu texture groups\n",
             filename, output->effect_count, output->particle_bank.command_count,
             output->particle_bank.texture_group_count);
    return output;
}
