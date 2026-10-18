#ifndef MELEE_LB_LBANIM_DAT_H
#define MELEE_LB_LBANIM_DAT_H

#include "lbanim.h"
#include <sysdolphin/baselib/dat.h>

#ifdef __cplusplus
extern "C" {
#endif

// The caller supplies track storage. Node counts and compressed streams retain
// references into the immutable DAT; keep both buffers alive during playback.
HSD_DatResult lbAnim_DecodeFigaTree(const HSD_Dat* dat, u32 offset,
    FigaTree* tree, FigaTrack* tracks, size_t capacity, u32* node_count);

#ifdef __cplusplus
}
#endif

#endif
