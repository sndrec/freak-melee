#ifndef MELEE_LB_LBCOLANIM_DATA_H
#define MELEE_LB_LBCOLANIM_DATA_H

#include "types.h"

#include <sysdolphin/baselib/dat.h>

typedef struct LbColorScriptTable {
    u32 offset;
    u32 count;
    struct Fighter_804D653C_t* entries;
} LbColorScriptTable;

bool lbColorScript_Import(const HSD_Dat* dat, const LbColorScriptTable* tables,
                          u32 table_count, u32 script_end,
                          union ColorOverlay_x8_t* commands,
                          u32 command_capacity, u32* command_count,
                          HSD_DatResult* status, u32* error_offset);
bool lbColorScript_ImportDirect(
    const HSD_Dat* dat, u32 pointer_slot, u32 script_end,
    union ColorOverlay_x8_t* commands, u32 command_capacity,
    u32* command_count, union ColorOverlay_x8_t** script,
    HSD_DatResult* status, u32* error_offset);

#endif
