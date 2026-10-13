#include "native_item_pool.h"
#ifdef MELEE_NATIVE
#include "item.h"
#include <melee/gm/native_match_snapshot_pool.h>
#include <melee/gm/native_match_arena.h>
#include <melee/ft/native_script_runtime.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <string.h>

// NON-HOST .mstate until the complete explicit aggregate replaces the oracle.
// Payload lives in shared reserved actor storage. Scene history copies its
// initialized prefix plus full allocator metadata and free order. The broad
// oracle also tracks the backing while other match owners migrate.
NativeMatchPool nativeItemPool;
static NativeItemPoolSlot* actors;
static NativeMatchPoolSlot metadata[NATIVE_ITEM_CAPACITY];
static uint32_t free_order[NATIVE_ITEM_CAPACITY];
_Static_assert(offsetof(NativeItemPoolSlot,actor)==0,"Item must be slot base");

static bool boundary(void)
{
    return HSD_GObj_CurrentInvokedProcGObj==NULL && HSD_GObj_CurrentInvokedProc==NULL &&
        HSD_GObj_804D7818==NULL && HSD_GObj_DelayedProcInfo.flags==0 && nativeScriptRuntime_Idle();
}
static bool typed_shape(void)
{
    return nativeItemPool.payload==(uint8_t*)actors && nativeItemPool.slots==metadata &&
        nativeItemPool.free_order==free_order && nativeItemPool.capacity==NATIVE_ITEM_CAPACITY &&
        nativeItemPool.object_size==sizeof(NativeItemPoolSlot) &&
        nativeItemPool.stride==sizeof(NativeItemPoolSlot) && nativeItemPool.lifetime!=0 &&
        nativeItemPool.lifetime==nativeGObjPool.lifetime;
}
bool nativeItemPool_CanBeginScene(uint64_t epoch)
{
    return boundary() && epoch!=0 && nativeItemPool.lifetime==0 && nativeItemPool.used==0;
}
bool nativeItemPool_BeginScene(uint64_t epoch)
{
    if(!nativeItemPool_CanBeginScene(epoch))return false;
    actors=nativeMatchActorStorage.items;
    if(!actors || !nativeMatchArena_AddRoot(&nativeMatchSceneArena,&nativeItemPool,sizeof(nativeItemPool)) ||
       !nativeMatchArena_AddRoot(&nativeMatchSceneArena,metadata,sizeof(metadata)) ||
       !nativeMatchArena_AddRoot(&nativeMatchSceneArena,free_order,sizeof(free_order)))return false;
    return nativeMatchPool_Bind(&nativeItemPool,actors,(NATIVE_ITEM_CAPACITY*sizeof(*actors)),metadata,free_order,
        NATIVE_ITEM_CAPACITY,sizeof(NativeItemPoolSlot),_Alignof(NativeItemPoolSlot),epoch) &&
        nativeMatchArena_AddCountedRoot(&nativeMatchSceneArena,actors,sizeof(*actors),
            NATIVE_ITEM_CAPACITY,&nativeItemPool.initialized_count);
}
bool nativeItemPool_RetireScene(void)
{
    if(!boundary())return false;
    nativeItemPool.lifetime=0;nativeItemPool.used=0;nativeItemPool.free_count=0;
    return true;
}
NativeItemPoolSlot* nativeItemPool_Slot(Item* actor)
{
    NativeMatchPoolReference reference;
    if(!typed_shape() || !nativeMatchPool_Reference(&nativeItemPool,actor,&reference) ||
        nativeMatchPool_Resolve(reference)!=actor)return NULL;
    return (NativeItemPoolSlot*)actor;
}

// Candidate root occupancy may differ from the current timeline. Check stable
// geometry without dereferencing roots; the aggregate must restore GObj state
// and model graphs too. This family does not assert their ownership closure.
static HSD_GObj* saved_root(const uint8_t* payload,unsigned slot)
{
    HSD_GObj* root;DynamicBoneTable* bones;
    memcpy(&root,payload+offsetof(Item,entity),sizeof(root));
    memcpy(&bones,payload+offsetof(Item,xBBC_dynamicBoneTable),sizeof(bones));
    if(bones!=NULL && bones!=&actors[slot].bones)return NULL;
    if(!root || !nativeGObjPool.payload || !nativeGObjPool.stride ||
       (uintptr_t)root<(uintptr_t)nativeGObjPool.payload)return NULL;
    uintptr_t offset=(uintptr_t)root-(uintptr_t)nativeGObjPool.payload;
    return offset%nativeGObjPool.stride==0 && offset/nativeGObjPool.stride<nativeGObjPool.capacity?root:NULL;
}
bool nativeItemPool_Save(void* snapshot,size_t capacity,size_t* written)
{
    if(!boundary() || !typed_shape() || !nativeMatchPool_Validate(&nativeItemPool))return false;
    for(unsigned i=0;i<nativeItemPool.capacity;++i)if(metadata[i].occupied) {
        HSD_GObj* root=saved_root((const uint8_t*)&actors[i],i);NativeMatchPoolReference reference;
        if(!root || !nativeMatchPool_Reference(&nativeGObjPool,root,&reference) ||
           nativeMatchPool_Resolve(reference)!=root || root->classifier!=HSD_GOBJ_CLASS_ITEM ||
           root->user_data_kind!=6 || root->user_data!=&actors[i].actor ||
           root->user_data_remove_func!=Item_OnUserDataRemove)return false;
    }
    return nativeMatchSnapshotPool_Save(&nativeItemPool,snapshot,capacity,written);
}
bool nativeItemPool_ValidateSnapshot(const void* snapshot,size_t bytes)
{
    if(!boundary() || !typed_shape() || !nativeMatchSnapshotPool_View(&nativeItemPool,snapshot,bytes).records)return false;
    NativeMatchPoolSnapshotHeader header;memcpy(&header,snapshot,sizeof(header));
    const uint8_t* cursor=(const uint8_t*)snapshot+sizeof(header)+
        (header.capacity+header.free_count)*sizeof(uint32_t);
    for(unsigned i=0;i<header.used;++i) {
        uint32_t slot;memcpy(&slot,cursor,sizeof(slot));cursor+=sizeof(slot);
        if(!saved_root(cursor,slot))return false;
        cursor+=sizeof(NativeItemPoolSlot);
    }
    return true;
}
bool nativeItemPool_Load(const void* snapshot,size_t bytes)
{
    if(!nativeItemPool_ValidateSnapshot(snapshot,bytes))return false;
    return nativeMatchSnapshotPool_Load(&nativeItemPool,snapshot,bytes);
}
#endif
