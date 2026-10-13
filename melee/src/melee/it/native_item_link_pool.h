#ifndef MELEE_IT_NATIVE_ITEM_LINK_POOL_H
#define MELEE_IT_NATIVE_ITEM_LINK_POOL_H
#ifdef MELEE_NATIVE
#include "itCharItems.h"
#include <sysdolphin/baselib/native_gobj_pool_storage.h>

// Stock tethers and authored resource nodes each publish one ItemLink per GObj.
// A shared physical owner cannot impose a smaller limit than the GObj owner.
enum { NATIVE_ITEM_LINK_CAPACITY = NATIVE_GOBJ_CAPACITY };
extern NativeMatchPool nativeItemLinkPool;
bool nativeItemLinkPool_CanBeginScene(uint64_t epoch);
bool nativeItemLinkPool_BeginScene(uint64_t epoch);
bool nativeItemLinkPool_RetireScene(void);
bool nativeItemLinkPool_Ready(void);
bool nativeItemLinkPool_Save(void* snapshot, size_t capacity, size_t* written);
bool nativeItemLinkPool_ValidateSnapshot(const void* snapshot, size_t bytes);
bool nativeItemLinkPool_Load(const void* snapshot, size_t bytes);
#endif
#endif
