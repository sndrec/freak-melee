#ifndef MELEE_MP_NATIVE_COLLISION_STATE_INTERNAL_H
#define MELEE_MP_NATIVE_COLLISION_STATE_INTERNAL_H

#include "native_collision_state.h"
#include "mplib_internal.h"

typedef struct NativeCollisionScene {
    MapCollData* source;
    MapCollData geometry;
    uint64_t identity, object_epoch;
} NativeCollisionScene;

extern NativeCollisionScene nativeCollisionScene;
extern CollVtx nativeCollisionVertices[NATIVE_COLLISION_VERTICES];
extern CollLine nativeCollisionLines[NATIVE_COLLISION_LINES];
extern CollJoint nativeCollisionJoints[NATIVE_COLLISION_JOINTS];

// Descriptor ranges are checked at transition and on candidate snapshots.
bool nativeCollisionState_ValidDescription(const MapCollData*);
bool nativeCollisionState_ValidJoint(const MapJoint*, unsigned vertices, unsigned lines);
bool nativeCollisionState_ValidLine(const MapLine*, unsigned vertices, unsigned lines);

#endif
