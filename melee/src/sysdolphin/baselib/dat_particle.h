#ifndef SYSDOLPHIN_BASELIB_DAT_PARTICLE_H
#define SYSDOLPHIN_BASELIB_DAT_PARTICLE_H

#include "dat_import.h"
#include "psstructs.h"

typedef struct HSD_PSDataBank {
    s32 command_count;
    HSD_PSCmdList** commands;
    u32 texture_group_count;
    HSD_PSTexGroup** texture_groups;
} HSD_PSDataBank;

bool HSD_DatImportParticleBank(HSD_DatImport* import, u32 command_offset,
                               u32 texture_offset, HSD_PSDataBank* bank);

#endif
