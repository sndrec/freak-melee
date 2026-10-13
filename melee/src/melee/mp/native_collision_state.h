#ifndef MELEE_MP_NATIVE_COLLISION_STATE_H
#define MELEE_MP_NATIVE_COLLISION_STATE_H

#include "types.h"
#include <melee/gr/types.h>
#include <stddef.h>
#include <stdint.h>

enum {
    NATIVE_COLLISION_VERTICES = 2048,
    NATIVE_COLLISION_LINES = 1536,
    NATIVE_COLLISION_JOINTS = 256,
    NATIVE_COLLISION_ATTRIBUTES = 4
};

// Actual engine storage, not a second collision representation. Original query
// and mutation call sites read these same arrays/list controls.
extern CollJoint* groundCollJoint;
extern CollJoint* jointListStart;
extern CollJoint* jointListEnd;
extern bool didCheckBounding;
extern s32 mpLib_804D64CC;
extern grDynamicAttr_UnkStruct nativeCollisionAttributes[NATIVE_COLLISION_ATTRIBUTES];
extern grDynamicAttr_UnkStruct* nativeCollisionAttributeActive;
extern grDynamicAttr_UnkStruct* nativeCollisionAttributeFree;

// Rebind only at a scene transition, before history. Source descriptor buffers
// and all borrowed graph/callback targets must remain resident during history.
bool nativeCollisionState_BeginScene(MapCollData*);
size_t nativeCollisionState_SnapshotBytes(void);
bool nativeCollisionState_Save(void*, size_t capacity, size_t* written);
bool nativeCollisionState_ValidateSnapshot(const void*, size_t bytes);
bool nativeCollisionState_Load(const void*, size_t bytes);

#endif
