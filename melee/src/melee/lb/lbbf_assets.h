#ifndef MELEE_LB_LBBF_ASSETS_H
#define MELEE_LB_LBBF_ASSETS_H

#include "types.h"
#include <sysdolphin/baselib/dat.h>

enum {
    LB_BF_ENTRY_COUNT = 16,
    LB_BF_COMMAND_CAPACITY = 256,
};

typedef struct LbBfAssets {
    void* owned_bytes;
    HSD_Dat dat;
    struct Fighter_804D653C_t entries[LB_BF_ENTRY_COUNT];
    union ColorOverlay_x8_t commands[LB_BF_COMMAND_CAPACITY];
    u32 command_count;
    HSD_DatResult status;
    u32 error_offset;
} LbBfAssets;

bool lbBfAssets_Import(const void* bytes, size_t size, LbBfAssets* output);

#endif
