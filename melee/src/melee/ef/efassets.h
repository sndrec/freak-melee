#ifndef MELEE_EF_EFASSETS_H
#define MELEE_EF_EFASSETS_H

#include "types.h"

#include <sysdolphin/baselib/dat_particle.h>

typedef struct EF_ImportedData {
    HSD_PSDataBank particle_bank;
    EF_EffectDesc* effects;
    u32 effect_count;
} EF_ImportedData;

EF_ImportedData* efAssets_Import(const void* bytes, size_t size,
                                 const char* root_name, void* arena,
                                 size_t arena_size, HSD_DatResult* status,
                                 u32* error_offset);
EF_ImportedData* efAssets_Load(const char* filename, const char* root_name);

#endif
