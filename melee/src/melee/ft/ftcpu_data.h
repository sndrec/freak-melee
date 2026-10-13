#ifndef MELEE_FT_FTCPU_DATA_H
#define MELEE_FT_FTCPU_DATA_H

#include "fighter.h"

#include <sysdolphin/baselib/dat_import.h>



typedef struct FtCpuData {
    struct Fighter_804D64FC_t descriptor;
    u8* scripts[FT_CPU_SCRIPT_COUNT];
    ftCo_AttackEntry* attacks[FT_CPU_ATTACK_FAMILIES]
                              [FT_CPU_RUNTIME_FIGHTER_COUNT];
    ftCo_AttackEntry empty_attack;
    f32 distance_thresholds[FT_CPU_RUNTIME_FIGHTER_COUNT];
    f32 weapon_reach[6];
    u32 script_count;
    u32 script_byte_count;
    u32 command_count;
    u32 attack_array_count;
    u32 attack_entry_count;
} FtCpuData;

bool ftCpuData_Import(HSD_DatImport* import, u32 source, FtCpuData* output);

#endif
