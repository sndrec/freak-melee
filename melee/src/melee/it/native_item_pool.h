#ifndef MELEE_IT_NATIVE_ITEM_POOL_H
#define MELEE_IT_NATIVE_ITEM_POOL_H
#ifdef MELEE_NATIVE
#include "types.h"
#include <sysdolphin/baselib/native_gobj_pool_storage.h>

// Some retail hold kinds have no total cap. The native budget permits one
// item per possible GObj; gameplay category/move limits still apply separately.
enum { NATIVE_ITEM_CAPACITY=NATIVE_GOBJ_CAPACITY };
typedef struct NativeItemPoolSlot {
    _Alignas(16) Item actor;
    DynamicBoneTable bones;
} NativeItemPoolSlot;

extern NativeMatchPool nativeItemPool;
bool nativeItemPool_CanBeginScene(uint64_t epoch);
bool nativeItemPool_BeginScene(uint64_t epoch);
bool nativeItemPool_RetireScene(void);
// Return only an occupied actor's actual owning slot. The constructor binds
// the optional bone table at the original model-construction phase.
NativeItemPoolSlot* nativeItemPool_Slot(Item* actor);
bool nativeItemPool_Save(void* snapshot,size_t capacity,size_t* written);
bool nativeItemPool_ValidateSnapshot(const void* snapshot,size_t bytes);
bool nativeItemPool_Load(const void* snapshot,size_t bytes);
#endif
#endif
