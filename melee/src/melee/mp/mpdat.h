#ifndef MELEE_MP_MPDAT_H
#define MELEE_MP_MPDAT_H

#include "types.h"
#include <sysdolphin/baselib/dat.h>

// Produces the original collision structures in caller-owned writable storage.
// No source pointers survive decoding. On failure, storage may be partially
// written, but the output descriptor is unchanged.
HSD_DatResult mpDecodeCollision(const HSD_Dat* dat, u32 offset,
    MapCollData* output, Vec2* vertices, size_t vertex_capacity,
    MapLine* lines, size_t line_capacity, MapJoint* joints, size_t joint_capacity);

#endif
