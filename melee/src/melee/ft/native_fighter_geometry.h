#ifndef MELEE_FT_NATIVE_FIGHTER_GEOMETRY_H
#define MELEE_FT_NATIVE_FIGHTER_GEOMETRY_H

#include <melee/ft/forward.h>
#include <melee/it/forward.h>
#include <dolphin/mtx.h>
#include <stdbool.h>

// Probe a copied collision box with caller-selected ledge reach. Only the
// selected live ledge ID/flag is written; velocity decisions stay in scripts.
bool nativeFighterGeometry_ProbeLedge(Fighter_GObj*, float snap_y,
                                      float snap_height);

// Signed XY direction from the second bone-local point toward the first.
float nativeFighterGeometry_BoneSegmentAngle(Fighter_GObj*, Fighter_Part,
                                            Vec3* endpoint, Vec3* start);

void nativeFighterGeometry_ItemEffect(Item_GObj*, s32 effect, Fighter_Part,
                                     Vec3* offset, float angle);

#endif
