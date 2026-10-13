#include <melee/gm/native_match_control_history.h>
#include "native_stage_generator_snapshot.h"
#include "native_stage_spawn_snapshot.h"
#include <melee/gm/native_gm_loop_snapshot.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/it/native_item_pool.h>
#include <melee/it/item.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <stdlib.h>
#include <string.h>

// One physical scene lease across the pending/constructed driver transition.
static struct { uint64_t epoch;NativeMatchPool objects,items;grZakoGenerator_SpawnDesc* definitions;bool active; } scene;
void nativeStageGenerator_BeginScene(void)
{
    if(!scene.active || scene.objects.lifetime!=nativeGObjPool.lifetime) {
        if(scene.epoch==UINT64_MAX)abort();uint64_t epoch=scene.epoch+1;
        memset(&scene,0,sizeof(scene));scene.epoch=epoch;
    }
    scene.active=false;
    if(!nativeMatchPool_Validate(&nativeGObjPool) || !nativeMatchPool_Validate(&nativeItemPool) ||
        nativeGObjPool.lifetime!=nativeItemPool.lifetime || !nativeGObjPool.lifetime ||
        (stage_generator_state.driver && stage_generator_state.control.x4!=&stage_generator_state.table))return;
    scene.objects=nativeGObjPool;scene.items=nativeItemPool;
    if(stage_generator_state.driver)scene.definitions=stage_generator_state.control.x0;
    scene.active=true;
    nativeStageSpawn_BeginScene();
}
void nativeStageGenerator_EndScene(void) {scene.active=false;nativeStageSpawn_EndScene();}
static bool geometry(const NativeMatchPool* a,const NativeMatchPool* b)
{
    return a->payload==b->payload && a->slots==b->slots && a->free_order==b->free_order &&
        a->capacity==b->capacity && a->stride==b->stride && a->object_size==b->object_size && a->lifetime==b->lifetime;
}
static bool boundary(void)
{
    return scene.active && nativeGmLoop_SnapshotBoundary() && nativeScriptRuntime_Idle() &&
        geometry(&nativeGObjPool,&scene.objects) && geometry(&nativeItemPool,&scene.items) &&
        (!stage_generator_state.driver || (stage_generator_state.control.x0==scene.definitions &&
        stage_generator_state.control.x4==&stage_generator_state.table));
}
static bool overlap(const void* first,size_t an,const void* second,size_t bn)
{
    uintptr_t a=(uintptr_t)first,b=(uintptr_t)second;
    return an>UINTPTR_MAX-a || bn>UINTPTR_MAX-b || (a<b+bn && b<a+an);
}
static bool independent(const void* data,size_t bytes)
{
    if(!data || !bytes || bytes>UINTPTR_MAX-(uintptr_t)data ||
        overlap(data,bytes,&scene,sizeof(scene)) || overlap(data,bytes,&stage_generator_state,sizeof(stage_generator_state)) ||
        overlap(data,bytes,&nativeGObjPool,sizeof(nativeGObjPool)) || overlap(data,bytes,&nativeItemPool,sizeof(nativeItemPool)))return false;
    const NativeMatchPool* pools[]={&scene.objects,&scene.items};
    for(unsigned i=0;i<2;++i) {
        const NativeMatchPool* p=pools[i];
        if(overlap(data,bytes,p->payload,p->capacity*p->stride) ||
            overlap(data,bytes,p->slots,p->capacity*sizeof(*p->slots)) ||
            overlap(data,bytes,p->free_order,p->capacity*sizeof(*p->free_order)))return false;
    }
    return !scene.definitions || !overlap(data,bytes,scene.definitions,sizeof(*scene.definitions));
}
static bool zero(NativeMatchPoolReference r)
{return !r.pool && !r.lifetime && !r.slot && !r.generation;}
static bool claim(NativeMatchPoolReference r,const NativeMatchPool* p,const NativeMatchPool* fixed)
{return r.pool==p && r.lifetime==fixed->lifetime && r.slot<fixed->capacity && r.generation;}
static bool object_shape(HSD_GObj* pointer,NativeMatchPoolReference r)
{
    return pointer && claim(r,&nativeGObjPool,&scene.objects) &&
        pointer==(HSD_GObj*)(scene.objects.payload+(size_t)r.slot*scene.objects.stride);
}
static bool live(const NativeStageGeneratorSnapshot* snapshot)
{
    if(!snapshot->state.driver)return zero(snapshot->driver);
    HSD_GObj* driver=nativeMatchPool_Resolve(snapshot->driver);
    if(!driver || driver!=snapshot->state.driver || driver->classifier!=2 || driver->p_link!=4 ||
        driver->user_data_kind!=HSD_GOBJ_USER_DATA_NONE || driver->user_data || driver->user_data_remove_func)return false;
    for(unsigned i=0;i<STAGE_GENERATOR_ENTRIES;++i)if(snapshot->state.table.entries[i].x4) {
        HSD_GObj* object=nativeMatchPool_Resolve(snapshot->objects[i]);
        NativeItemPoolSlot* slot=nativeMatchPool_Resolve(snapshot->items[i]);
        if(!object || object!=snapshot->state.table.entries[i].x4 || !slot || nativeItemPool_Slot((Item*)slot)!=slot ||
            object->classifier!=HSD_GOBJ_CLASS_ITEM || object->user_data_kind!=6 || object->user_data!=&slot->actor ||
            object->user_data_remove_func!=Item_OnUserDataRemove || slot->actor.entity!=object)return false;
    }
    return true;
}
bool nativeStageGenerator_Validate(const void* data,size_t bytes)
{
    if(!boundary() || bytes!=sizeof(NativeStageGeneratorSnapshot) || !independent(data,bytes))return false;
    NativeStageGeneratorSnapshot snapshot;memcpy(&snapshot,data,sizeof(snapshot));
    if(snapshot.epoch!=scene.epoch)return false;
    if(!snapshot.state.driver) {
        if(!zero(snapshot.driver))return false;
        for(unsigned i=0;i<STAGE_GENERATOR_ENTRIES;++i)
            if(!zero(snapshot.objects[i]) || !zero(snapshot.items[i]))return false;
        return true;
    }
    if(snapshot.state.control.x0!=scene.definitions ||
        snapshot.state.control.x4!=&stage_generator_state.table || snapshot.state.control.x8<0 ||
        snapshot.state.control.x8>=STAGE_GENERATOR_ENTRIES || !object_shape(snapshot.state.driver,snapshot.driver))return false;
    for(unsigned i=0;i<STAGE_GENERATOR_ENTRIES;++i) {
        HSD_GObj* object=snapshot.state.table.entries[i].x4;
        if(!object) {
            if(!zero(snapshot.objects[i]) || !zero(snapshot.items[i]))return false;
        } else if(!object_shape(object,snapshot.objects[i]) || !claim(snapshot.items[i],&nativeItemPool,&scene.items))return false;
    }
    return true;
}
bool nativeStageGenerator_Save(void* data,size_t capacity,size_t* written)
{
    size_t bytes=sizeof(NativeStageGeneratorSnapshot);
    if(!boundary() || capacity<bytes || !independent(data,bytes) || !independent(written,sizeof(*written)) ||
        overlap(data,bytes,written,sizeof(*written)))return false;
    NativeStageGeneratorSnapshot snapshot;memset(&snapshot,0,sizeof(snapshot));
    snapshot.epoch=scene.epoch;snapshot.state=stage_generator_state;
    if(snapshot.state.driver && !nativeMatchPool_Reference(&nativeGObjPool,snapshot.state.driver,&snapshot.driver))return false;
    for(unsigned i=0;i<STAGE_GENERATOR_ENTRIES;++i)if(snapshot.state.driver && snapshot.state.table.entries[i].x4) {
        HSD_GObj* object=snapshot.state.table.entries[i].x4;
        if(!nativeMatchPool_Reference(&nativeGObjPool,object,&snapshot.objects[i]) ||
            !nativeMatchPool_Reference(&nativeItemPool,object->user_data,&snapshot.items[i]))return false;
    }
    if(!nativeStageGenerator_Validate(&snapshot,bytes) || !live(&snapshot))return false;
    memcpy(data,&snapshot,bytes);*written=bytes;return true;
}
bool nativeStageGenerator_Load(const void* data,size_t bytes)
{
    if(!nativeStageGenerator_Validate(data,bytes))return false;
    NativeStageGeneratorSnapshot snapshot;memcpy(&snapshot,data,sizeof(snapshot));
    // GObj/Item companions restore first. Aggregate candidate-wide callback,
    // graph and definition retention agreement is still required before writes.
    if(!live(&snapshot))return false;
    memcpy(&stage_generator_state,&snapshot.state,sizeof(stage_generator_state));return true;
}

#ifdef MELEE_NATIVE
bool nativeStageGenerator_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || !arena->lifetime)return false;
#define ROOT(v) do { if(!nativeMatchArena_AddRoot(arena,&(v),sizeof(v)))return false; } while(0)
    ROOT(stage_generator_state);
#undef ROOT
    return true;
}
#endif
