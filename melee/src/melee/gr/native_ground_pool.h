#ifndef MELEE_GR_NATIVE_GROUND_POOL_H
#define MELEE_GR_NATIVE_GROUND_POOL_H
#ifdef MELEE_NATIVE
#include "types.h"
#include <sysdolphin/baselib/native_gobj_pool_storage.h>

// Fixed stage-actor budget, independently bounded from the shared GObj pool.
// Measured eight-fighter Green Greens demand peaks at 26 records.
enum { NATIVE_GROUND_CAPACITY=512 };
extern NativeMatchPool nativeGroundPool;
bool nativeGroundPool_CanBeginScene(uint64_t epoch);
bool nativeGroundPool_BeginScene(uint64_t epoch);
bool nativeGroundPool_RetireScene(void);
// Actual GObj userdata destructor, not a general-purpose heap free operation.
void nativeGroundPool_Release(void* actor);
bool nativeGroundPool_Save(void* snapshot,size_t capacity,size_t* written);
bool nativeGroundPool_ValidateSnapshot(const void* snapshot,size_t bytes);
bool nativeGroundPool_Load(const void* snapshot,size_t bytes);
#endif
#endif
