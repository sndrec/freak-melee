#include "native_fighter_pool.h"
#ifdef MELEE_NATIVE
#include <melee/gm/native_match_snapshot_pool.h>
#include <melee/gm/native_match_arena.h>
#include "native_script_runtime.h"
#include <sysdolphin/baselib/gobj.h>
#include <stddef.h>
#include <string.h>

// Payload lives in shared reserved actor storage. Scene history copies its
// initialized prefix plus full allocator metadata and free order. The broad
// oracle also tracks the backing while other match owners migrate.
NativeMatchPool nativeFighterPool;
static NativeFighterPoolSlot* actors;
static NativeMatchPoolSlot metadata[NATIVE_FIGHTER_POOL_CAPACITY];
static uint32_t free_order[NATIVE_FIGHTER_POOL_CAPACITY];
_Static_assert(offsetof(NativeFighterPoolSlot,actor)==0,"Actor must be slot base");

static bool typed_shape(void)
{
    return nativeFighterPool.payload==(uint8_t*)actors && nativeFighterPool.slots==metadata &&
        nativeFighterPool.free_order==free_order && nativeFighterPool.capacity==NATIVE_FIGHTER_POOL_CAPACITY &&
        nativeFighterPool.object_size==sizeof(NativeFighterPoolSlot) &&
        nativeFighterPool.stride==sizeof(NativeFighterPoolSlot) && nativeFighterPool.lifetime!=0;
}

static bool boundary(void)
{
    return HSD_GObj_CurrentInvokedProcGObj==NULL && HSD_GObj_CurrentInvokedProc==NULL &&
        HSD_GObj_804D7818==NULL && HSD_GObj_DelayedProcInfo.flags==0 && nativeScriptRuntime_Idle();
}
bool nativeFighterPool_CanBeginScene(uint64_t epoch)
{
    return boundary() && epoch!=0 && nativeFighterPool.lifetime==0 && nativeFighterPool.used==0;
}
bool nativeFighterPool_BeginScene(uint64_t epoch)
{
    if(!nativeFighterPool_CanBeginScene(epoch))return false;
    actors=nativeMatchActorStorage.fighters;
    if(!actors || !nativeMatchArena_AddRoot(&nativeMatchSceneArena,&nativeFighterPool,sizeof(nativeFighterPool)) ||
       !nativeMatchArena_AddRoot(&nativeMatchSceneArena,metadata,sizeof(metadata)) ||
       !nativeMatchArena_AddRoot(&nativeMatchSceneArena,free_order,sizeof(free_order)))return false;
    return nativeMatchPool_Bind(&nativeFighterPool,actors,(NATIVE_FIGHTER_POOL_CAPACITY*sizeof(*actors)),metadata,free_order,
        NATIVE_FIGHTER_POOL_CAPACITY,sizeof(NativeFighterPoolSlot),_Alignof(NativeFighterPoolSlot),epoch) &&
        nativeMatchArena_AddCountedRoot(&nativeMatchSceneArena,actors,sizeof(*actors),
            NATIVE_FIGHTER_POOL_CAPACITY,&nativeFighterPool.initialized_count);
}
bool nativeFighterPool_RetireScene(void)
{
    if(!boundary())return false;
    nativeFighterPool.lifetime=0;nativeFighterPool.used=0;nativeFighterPool.free_count=0;
    return true;
}
NativeFighterPoolSlot* nativeFighterPool_Slot(Fighter* actor)
{
    NativeMatchPoolReference reference;
    if(!typed_shape() || !nativeMatchPool_Reference(&nativeFighterPool,actor,&reference) ||
        nativeMatchPool_Resolve(reference)!=actor)return NULL;
    return (NativeFighterPoolSlot*)actor;
}

typedef struct ActorAliases {
    void* backup;void* attributes;void* parts;void* primary;void* secondary;void* multijump;
    void* scratch0;void* scratch1;void* object;
    FighterKind kind;unsigned primary_count,secondary_count;
} ActorAliases;
static ActorAliases read_aliases(const uint8_t* payload)
{
    ActorAliases a;
#define READ(destination,member) memcpy(&a.destination,payload+offsetof(Fighter,member),sizeof(((Fighter*)0)->member))
    READ(backup,dat_attrs_backup);READ(attributes,dat_attrs);READ(parts,parts);
    READ(primary,dobj_list.data);READ(secondary,x203C.data);READ(multijump,x2D0);
    READ(scratch0,x59C);READ(scratch1,x5A0);READ(object,gobj);READ(kind,kind);
    // DObjList count is not assumed to have the host unsigned width.
    a.primary_count=0;a.secondary_count=0;
    READ(primary_count,dobj_list.count);READ(secondary_count,x203C.count);
#undef READ
    return a;
}
static bool valid_actor(const uint8_t* payload,unsigned slot)
{
    ActorAliases a=read_aliases(payload);const NativeFighterPoolSlot* own=&actors[slot];
    return a.backup==own->attributes && (a.attributes==NULL || a.attributes==own->attributes) &&
        a.parts==own->parts && a.primary==own->primary_dobjs && a.secondary==own->secondary_dobjs &&
        (a.multijump==NULL || a.multijump==own->attributes) && a.scratch0==NULL && a.scratch1==NULL &&
        a.object!=NULL && (unsigned)a.kind<Ft_Kind_RuntimeCapacity && a.kind!=Ft_Kind_None &&
        a.primary_count<=NATIVE_FIGHTER_PRIMARY_DOBJS && a.secondary_count<=NATIVE_FIGHTER_SECONDARY_DOBJS;
}
bool nativeFighterPool_Save(void* snapshot,size_t capacity,size_t* written)
{
    if(!boundary() || !typed_shape() || !nativeMatchPool_Validate(&nativeFighterPool))return false;
    for(unsigned i=0;i<nativeFighterPool.capacity;++i)
        if(nativeFighterPool.slots[i].occupied && !valid_actor((const uint8_t*)&actors[i].actor,i))return false;
    return nativeMatchSnapshotPool_Save(&nativeFighterPool,snapshot,capacity,written);
}
bool nativeFighterPool_ValidateSnapshot(const void* snapshot,size_t bytes)
{
    if(!boundary() || !typed_shape() || !nativeMatchSnapshotPool_View(&nativeFighterPool,snapshot,bytes).records)return false;
    NativeMatchPoolSnapshotHeader header;memcpy(&header,snapshot,sizeof(header));
    const uint8_t* cursor=(const uint8_t*)snapshot+sizeof(header)+
        (header.capacity+header.free_count)*sizeof(uint32_t);
    for(unsigned i=0;i<header.used;++i){
        uint32_t slot;memcpy(&slot,cursor,sizeof(slot));cursor+=sizeof(slot);
        if(!valid_actor(cursor,slot))return false;
        cursor+=sizeof(NativeFighterPoolSlot);
    }
    return true;
}
bool nativeFighterPool_Load(const void* snapshot,size_t bytes)
{
    if(!nativeFighterPool_ValidateSnapshot(snapshot,bytes))return false;
    return nativeMatchSnapshotPool_Load(&nativeFighterPool,snapshot,bytes);
}
#endif
