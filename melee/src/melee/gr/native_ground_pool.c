#include "native_ground_pool.h"
#include "native_green_grid.h"
#ifdef MELEE_NATIVE
#include <melee/gm/native_match_snapshot_pool.h>
#include <melee/gm/native_match_arena.h>
#include <melee/ft/native_script_runtime.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <string.h>

// Payload lives in shared reserved actor storage. Scene history copies its
// initialized prefix plus full allocator metadata and free order. The broad
// oracle also tracks the backing while other match owners migrate.
NativeMatchPool nativeGroundPool;
static Ground* actors;
static NativeMatchPoolSlot metadata[NATIVE_GROUND_CAPACITY];
static uint32_t free_order[NATIVE_GROUND_CAPACITY];

static bool boundary(void)
{
    return HSD_GObj_CurrentInvokedProcGObj==NULL && HSD_GObj_CurrentInvokedProc==NULL &&
        HSD_GObj_804D7818==NULL && HSD_GObj_DelayedProcInfo.flags==0 && nativeScriptRuntime_Idle();
}
static bool typed_shape(void)
{
    return nativeGroundPool.payload==(uint8_t*)actors && nativeGroundPool.slots==metadata &&
        nativeGroundPool.free_order==free_order && nativeGroundPool.capacity==NATIVE_GROUND_CAPACITY &&
        nativeGroundPool.object_size==sizeof(Ground) && nativeGroundPool.stride==sizeof(Ground) &&
        nativeGroundPool.lifetime!=0 && nativeGroundPool.lifetime==nativeGObjPool.lifetime;
}
bool nativeGroundPool_CanBeginScene(uint64_t epoch)
{
    return boundary() && epoch!=0 && nativeGroundPool.lifetime==0 && nativeGroundPool.used==0 && nativeGreenGrid_CanBeginScene(epoch);
}
bool nativeGroundPool_BeginScene(uint64_t epoch)
{
    if(!nativeGroundPool_CanBeginScene(epoch))return false;
    actors=nativeMatchActorStorage.ground;
    if(!actors || !nativeMatchArena_AddRoot(&nativeMatchSceneArena,&nativeGroundPool,sizeof(nativeGroundPool)) ||
       !nativeMatchArena_AddRoot(&nativeMatchSceneArena,metadata,sizeof(metadata)) ||
       !nativeMatchArena_AddRoot(&nativeMatchSceneArena,free_order,sizeof(free_order)))return false;
    return nativeMatchPool_Bind(&nativeGroundPool,actors,(NATIVE_GROUND_CAPACITY*sizeof(*actors)),metadata,free_order,
        NATIVE_GROUND_CAPACITY,sizeof(Ground),_Alignof(Ground),epoch) &&
        nativeMatchArena_AddCountedRoot(&nativeMatchSceneArena,actors,sizeof(*actors),
            NATIVE_GROUND_CAPACITY,&nativeGroundPool.initialized_count) && nativeGreenGrid_BeginScene(epoch);
}
bool nativeGroundPool_RetireScene(void)
{
    if(!boundary() || !nativeGreenGrid_RetireScene())return false;
    nativeGroundPool.lifetime=0;nativeGroundPool.used=0;nativeGroundPool.free_count=0;
    return true;
}
void nativeGroundPool_Release(void* actor)
{
    NativeMatchPoolReference reference;
    HSD_ASSERTREPORT(0,typed_shape() && nativeMatchPool_Reference(&nativeGroundPool,actor,&reference) &&
        nativeGreenGrid_DropOwner(reference) && nativeMatchPool_Free(&nativeGroundPool,reference),"Invalid native Ground userdata retirement.\n");
}

// Do not require the saved root to be occupied in the current GObj pool: an
// aggregate restore must be able to resurrect it. Check stable scene geometry
// without dereferencing it. Root occupancy/userdata topology is another owner.
static HSD_GObj* saved_root(const uint8_t* payload)
{
    HSD_GObj* root;
    memcpy(&root,payload+offsetof(Ground,gobj),sizeof(root));
    if(!root || !nativeGObjPool.payload || !nativeGObjPool.stride ||
       (uintptr_t)root<(uintptr_t)nativeGObjPool.payload)return NULL;
    uintptr_t offset=(uintptr_t)root-(uintptr_t)nativeGObjPool.payload;
    return offset%nativeGObjPool.stride==0 && offset/nativeGObjPool.stride<nativeGObjPool.capacity?root:NULL;
}
bool nativeGroundPool_Save(void* snapshot,size_t capacity,size_t* written)
{
    if(!boundary() || !typed_shape() || !nativeMatchPool_Validate(&nativeGroundPool))return false;
    for(unsigned i=0;i<nativeGroundPool.capacity;++i)if(metadata[i].occupied) {
        HSD_GObj* root=saved_root((const uint8_t*)&actors[i]);NativeMatchPoolReference reference;
        if(!root || !nativeMatchPool_Reference(&nativeGObjPool,root,&reference) ||
           nativeMatchPool_Resolve(reference)!=root || root->classifier!=HSD_GOBJ_CLASS_STAGE ||
           root->user_data_kind!=3 || root->user_data!=&actors[i] ||
           root->user_data_remove_func!=nativeGroundPool_Release)return false;
    }
    return nativeMatchSnapshotPool_Save(&nativeGroundPool,snapshot,capacity,written);
}
bool nativeGroundPool_ValidateSnapshot(const void* snapshot,size_t bytes)
{
    if(!boundary() || !typed_shape() || !nativeMatchSnapshotPool_View(&nativeGroundPool,snapshot,bytes).records)return false;
    NativeMatchPoolSnapshotHeader header;memcpy(&header,snapshot,sizeof(header));
    const uint8_t* cursor=(const uint8_t*)snapshot+sizeof(header)+
        (header.capacity+header.free_count)*sizeof(uint32_t);
    for(unsigned i=0;i<header.used;++i) {
        cursor+=sizeof(uint32_t);
        if(!saved_root(cursor))return false;
        cursor+=sizeof(Ground);
    }
    return true;
}
bool nativeGroundPool_Load(const void* snapshot,size_t bytes)
{
    if(!nativeGroundPool_ValidateSnapshot(snapshot,bytes))return false;
    return nativeMatchSnapshotPool_Load(&nativeGroundPool,snapshot,bytes);
}
#endif
