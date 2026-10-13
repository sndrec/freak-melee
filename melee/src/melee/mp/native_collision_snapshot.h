#ifndef MELEE_MP_NATIVE_COLLISION_SNAPSHOT_H
#define MELEE_MP_NATIVE_COLLISION_SNAPSHOT_H

#include "native_collision_state.h"
#include <melee/gm/native_match_arena.h>
#include "native_collision_query.h"

// Private scene-local record. Runtime/source arrays follow in this order:
// vertices, lines, joints, source vertices, source lines, source joints.
typedef struct NativeCollisionSnapshotHeader {
    uint64_t identity, object_epoch;
    MapCollData* source;
    MapCollData description;
    CollJoint* list_start;
    CollJoint* list_end;
    mpCollisionBox bounds[2];
    struct mpIsland_80458E88_t islands;
    grDynamicAttr_UnkStruct attributes[NATIVE_COLLISION_ATTRIBUTES];
    grDynamicAttr_UnkStruct* attribute_active;
    grDynamicAttr_UnkStruct* attribute_free;
    s32 update_counter;
    int collision_counter;
    NativeCollisionQueryState query;
    bool checked_bounds;
} NativeCollisionSnapshotHeader;

// Admit the fixed scene geometry and controls once, before history is sealed.
bool nativeCollisionState_AdmitHistory(NativeMatchArena*);

#endif
