#include "native_script_item_snapshot.h"
#include "native_script_item_actor_internal.h"
#include <melee/ft/native_script_actor_access.h>
#include "types.h"
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_script_runtime.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/gobj.h>
#include <string.h>
#include <melee/gm/native_match_arena.h>
#include <math.h>

static bool overlaps(const void* a,size_t an,const void* b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if(an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y)return true;
    return x<y+bn && y<x+an;
}
static bool independent(const void* data,size_t bytes)
{
    return data && bytes<=UINTPTR_MAX-(uintptr_t)data &&
        !overlaps(data,bytes,nativeScriptItemActors,sizeof(nativeScriptItemActors)) &&
        !overlaps(data,bytes,nativeScriptItemActorByObject,sizeof(nativeScriptItemActorByObject)) &&
        !overlaps(data,bytes,&nativeScriptItemSceneEpoch,sizeof(nativeScriptItemSceneEpoch));
}
static const NativeScriptItemProgram* program(const NativeScriptItemSnapshotRecord* r)
{
    return nativeScriptItem_Program((FighterKind)r->author_kind,r->owner);
}
static bool relation_shape(const NativeScriptItemRelation* relation)
{
    const NativeMatchPoolReference* ref=&relation->object;
    if(!ref->pool)return !ref->lifetime && !ref->slot && !ref->generation &&
        !relation->data && !relation->generation && !relation->kind;
    // Staleness is a legitimate saved state after a creator leaves the scene.
    // Validate structure here; getters resolve full physical identity on use.
    return ref->pool==&nativeGObjPool && ref->lifetime==nativeScriptItemSceneEpoch &&
        ref->slot<nativeGObjPool.capacity && ref->generation && relation->data &&
        relation->kind<Ft_Kind_RuntimeCapacity && relation->kind!=Ft_Kind_None;
}
static bool valid_record(const NativeScriptItemSnapshotRecord* r,uint32_t previous,bool first)
{
    if(r->slot>=NATIVE_SCRIPT_ITEM_ACTORS || (!first&&r->slot<=previous) || r->faulted>1 ||
       r->author_kind>=Ft_Kind_RuntimeCapacity || r->author_kind==Ft_Kind_None ||
       r->state_bytes>NATIVE_SCRIPT_ITEM_STATE_BYTES || r->object.pool!=&nativeGObjPool ||
       r->object.lifetime!=nativeScriptItemSceneEpoch || r->object.slot>=nativeGObjPool.capacity ||
       r->object.slot>=NATIVE_GOBJ_CAPACITY || !r->object.generation ||
       !relation_shape(&r->creator) || !relation_shape(&r->attachment_owner) || !isfinite(r->creation_value))return false;
    const NativeScriptItemProgram* p=program(r);
    if(!nativeScriptItem_ProgramValid(p) || p->identity!=r->program_identity || p->owner!=r->owner ||
       (uint32_t)p->kind!=r->kind || p->recipe_index!=r->recipe_index ||
       p->recipe!=r->recipe || p->article!=r->article || p->projectile!=r->projectile || p->attachment!=r->attachment)return false;
    const NativeScriptCompileBank* b=p->bank;
    const NativeScriptStorageOwner* row=nativeScriptStorage_Owner(&b->storage_layout,p->owner);
    if(!row || row->state_bytes!=r->state_bytes || b->storage_layout.identity!=r->layout_identity ||
       b->storage_parameters.identity!=r->parameter_identity)return false;
    for(unsigned slot=0;slot<NativeScriptItemCallback_Count;++slot)
        if(!nativeScriptItem_CallbackValid(p,slot,r->callbacks[slot]))return false;
    if(r->selected_state==UINT32_MAX){
        if(r->revision)return false;
        for(unsigned phase=0;phase<NativeScriptItemPhase_Count;++phase)if(r->phases[phase]!=UINT32_MAX)return false;
    }else{
        if(r->selected_state>=p->state_count || !r->revision)return false;
        for(unsigned phase=0;phase<NativeScriptItemPhase_Count;++phase)
            if(r->phases[phase]!=UINT32_MAX && r->phases[phase]!=p->states[r->selected_state].functions[phase])return false;
    }
    return true;
}
static bool record(const NativeScriptItemActor* a,unsigned slot,NativeScriptItemSnapshotRecord* output)
{
    const NativeScriptItemProgram* p=a->program;
    if(!p || !a->storage.live || a->running || a->program_identity!=p->identity ||
       a->object_reference.slot>=NATIVE_GOBJ_CAPACITY || nativeScriptItemActorByObject[a->object_reference.slot]!=slot ||
       nativeMatchPool_Resolve(a->object_reference)!=a->object ||
       !nativeScriptItem_NativeIdentity(a->object,p,a->generation))return false;
    const Item* item=a->object->user_data;
    if(item->entity!=a->object || item->kind!=p->kind || item->native_removal_in_progress || (uint32_t)item->msid!=a->selected_state ||
       a->storage.layout!=&p->bank->storage_layout || a->storage.parameters!=&p->bank->storage_parameters ||
       a->storage.owner!=p->owner || a->storage.identity!=a->generation || a->storage.generation!=a->generation ||
       a->execution.storage!=&a->storage || a->execution.declarations!=&p->bank->declarations)return false;
    const NativeScriptStorageOwner* row=nativeScriptStorage_Owner(a->storage.layout,p->owner);if(!row)return false;
    NativeScriptItemSnapshotRecord r={0};r.object=a->object_reference;r.program_identity=a->program_identity;
    r.creator=a->creator;r.attachment_owner=a->attachment_owner;
    r.creation_value=a->creation_value;
    r.layout_identity=a->storage.layout->identity;r.parameter_identity=a->storage.parameters->identity;
    r.revision=a->revision;r.slot=slot;r.generation=a->generation;r.author_kind=(uint32_t)p->author_kind;
    r.recipe=p->recipe;r.article=p->article;r.projectile=p->projectile;r.attachment=p->attachment;
    r.kind=(uint32_t)p->kind;r.recipe_index=p->recipe_index;r.owner=p->owner;r.selected_state=a->selected_state;
    r.tick=a->tick;r.completed_callbacks=a->completed_callbacks;r.state_bytes=row->state_bytes;r.faulted=a->faulted;
    memcpy(r.phases,a->phases,sizeof(r.phases));memcpy(r.callbacks,a->callbacks,sizeof(r.callbacks));if(!valid_record(&r,0,true))return false;
    *output=r;return true;
}
bool nativeScriptItem_Save(void* snapshot,size_t capacity,size_t* written)
{
    if(!nativeScriptItemSceneEpoch || nativeScriptItemSceneEpoch!=nativeGObjPool.lifetime ||
       !nativeScriptItem_Boundary() || !independent(snapshot,capacity) || !independent(written,sizeof(*written)) ||
       overlaps(snapshot,capacity,written,sizeof(*written)))return false;
    NativeScriptItemSnapshotHeader h={.epoch=nativeScriptItemSceneEpoch,.initialized_count=nativeScriptItemInitializedCount};
    NativeScriptItemSnapshotRecord records[NATIVE_SCRIPT_ITEM_ACTORS];size_t bytes=sizeof(h);
    for(unsigned i=0;i<nativeScriptItemInitializedCount;++i)if(nativeScriptItemActors[i].active){
        if(!record(&nativeScriptItemActors[i],i,&records[h.count]))return false;
        size_t add=sizeof(*records)+records[h.count].state_bytes;if(add>SIZE_MAX-bytes)return false;
        bytes+=add;++h.count;
    }
    if(bytes>capacity || bytes>UINT32_MAX)return false;h.total_bytes=(uint32_t)bytes;
    uint8_t* cursor=snapshot;memcpy(cursor,&h,sizeof(h));cursor+=sizeof(h);
    for(unsigned i=0;i<h.count;++i){
        memcpy(cursor,&records[i],sizeof(*records));cursor+=sizeof(*records);
        memcpy(cursor,nativeScriptItemActors[records[i].slot].bytes,records[i].state_bytes);cursor+=records[i].state_bytes;
    }
    *written=bytes;return true;
}
bool nativeScriptItem_ValidateSnapshot(const void* snapshot,size_t bytes)
{
    if(bytes<sizeof(NativeScriptItemSnapshotHeader) || !independent(snapshot,bytes) ||
       !nativeScriptItemSceneEpoch || nativeScriptItemSceneEpoch!=nativeGObjPool.lifetime || !nativeScriptItem_Boundary())return false;
    NativeScriptItemSnapshotHeader h;memcpy(&h,snapshot,sizeof(h));
    if(h.epoch!=nativeScriptItemSceneEpoch || h.total_bytes!=bytes || h.count>h.initialized_count || h.initialized_count>NATIVE_SCRIPT_ITEM_ACTORS)return false;
    const uint8_t* cursor=(const uint8_t*)snapshot+sizeof(h);size_t remaining=bytes-sizeof(h);
    uint32_t previous=0,objects[NATIVE_SCRIPT_ITEM_ACTORS];
    for(unsigned i=0;i<h.count;++i){
        if(remaining<sizeof(NativeScriptItemSnapshotRecord))return false;
        NativeScriptItemSnapshotRecord r;memcpy(&r,cursor,sizeof(r));
        if(r.slot>=h.initialized_count || !valid_record(&r,previous,i==0) || r.state_bytes>remaining-sizeof(r))return false;
        for(unsigned j=0;j<i;++j)if(objects[j]==r.object.slot)return false;
        objects[i]=r.object.slot;previous=r.slot;const NativeScriptItemProgram* p=program(&r);
        const uint8_t* state=cursor+sizeof(r);
        for(unsigned f=0;f<p->bank->storage_layout.field_count;++f){
            const NativeScriptStorageField* field=&p->bank->storage_layout.fields[f];
            if(field->owner!=p->owner || field->parameter || field->type!=NativeScriptType_Bool)continue;
            if(field->offset>r.state_bytes || field->count>r.state_bytes-field->offset)return false;
            for(unsigned n=0;n<field->count;++n)if(state[field->offset+n]>1)return false;
        }
        cursor+=sizeof(r)+r.state_bytes;remaining-=sizeof(r)+r.state_bytes;
    }
    return remaining==0;
}
bool nativeScriptItem_Load(const void* snapshot,size_t bytes)
{
    if(!nativeScriptItem_ValidateSnapshot(snapshot,bytes))return false;
    NativeScriptItemSnapshotHeader h;memcpy(&h,snapshot,sizeof(h));
    NativeScriptItemSnapshotRecord records[NATIVE_SCRIPT_ITEM_ACTORS];
    NativeScriptStorageInstance views[NATIVE_SCRIPT_ITEM_ACTORS];NativeScriptExecution executions[NATIVE_SCRIPT_ITEM_ACTORS];
    const uint8_t* cursor=(const uint8_t*)snapshot+sizeof(h);
    for(unsigned i=0;i<h.count;++i){
        memcpy(&records[i],cursor,sizeof(*records));cursor+=sizeof(*records)+records[i].state_bytes;
        const NativeScriptItemSnapshotRecord* r=&records[i];Item_GObj* object=nativeMatchPool_Resolve(r->object);
        if(!object || !nativeScriptItem_NativeIdentity(object,program(r),r->generation))return false;
        const Item* item=object->user_data;const NativeScriptItemProgram* p=program(r);
        if(item->native_removal_in_progress || (uint32_t)item->msid!=r->selected_state)return false;
        const NativeScriptCompileBank* b=p->bank;
        views[i]=(NativeScriptStorageInstance){.layout=&b->storage_layout,.parameters=&b->storage_parameters,
            .state=nativeScriptItemActors[r->slot].bytes,.capacity=NATIVE_SCRIPT_ITEM_STATE_BYTES,.owner=r->owner,
            .identity=r->generation,.generation=r->generation,.live=true};
        executions[i]=(NativeScriptExecution){.declarations=&b->declarations,.code=&b->code,.storage=&views[i],
            .vm=nativeScriptRuntime_VM(),.payloads=nativeScriptRuntime_Payloads(),
            .functions=b->execution_functions,.function_count=b->declarations.function_count,.receiver_lookup=nativeScriptActors_Lookup,.receiver_helper=nativeScriptActors_Helper,.fighter_kind=p->author_kind};
        NativeScriptDiagnostic d={0};if(!nativeScriptExecution_Bind(&executions[i],&d))return false;
    }
    // Complete immutable, typed-state and native peer preflight passed. No
    // callback, storage constructor or default initialization occurs on load.
    unsigned clear_count=nativeScriptItemInitializedCount>h.initialized_count?nativeScriptItemInitializedCount:h.initialized_count;
    for(unsigned i=0;i<clear_count;++i){nativeScriptStorage_Destroy(&nativeScriptItemActors[i].storage);nativeScriptItemActors[i].active=false;}
    nativeScriptItemInitializedCount=h.initialized_count;
    memset(nativeScriptItemActorByObject,0xFF,sizeof(nativeScriptItemActorByObject));
    cursor=(const uint8_t*)snapshot+sizeof(h);
    for(unsigned i=0;i<h.count;++i){
        const NativeScriptItemSnapshotRecord* r=&records[i];NativeScriptItemActor* a=&nativeScriptItemActors[r->slot];
        a->object=nativeMatchPool_Resolve(r->object);a->object_reference=r->object;a->program=program(r);a->program_identity=r->program_identity;
        a->creator=r->creator;a->attachment_owner=r->attachment_owner;
        a->creation_value=r->creation_value;
        a->generation=r->generation;a->revision=r->revision;a->selected_state=r->selected_state;a->tick=r->tick;
        a->completed_callbacks=r->completed_callbacks;a->faulted=r->faulted!=0;a->running=0;
        memcpy(a->phases,r->phases,sizeof(a->phases));cursor+=sizeof(*r);memcpy(a->bytes,cursor,r->state_bytes);cursor+=r->state_bytes;
        memcpy(a->callbacks,r->callbacks,sizeof(a->callbacks));
        a->storage=views[i];a->execution=executions[i];a->execution.storage=&a->storage;a->active=true;
        nativeScriptItemActorByObject[r->object.slot]=(uint16_t)r->slot;
    }
    return true;
}

bool nativeScriptItem_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || arena->lifetime!=nativeScriptItemSceneEpoch || !nativeScriptItemSceneEpoch ||
       nativeScriptItemSceneEpoch!=nativeGObjPool.lifetime || nativeScriptItemInitializedCount>NATIVE_SCRIPT_ITEM_ACTORS ||
       !nativeScriptItem_Boundary())return false;
    for(unsigned i=0;i<nativeScriptItemInitializedCount;++i)if(nativeScriptItemActors[i].active){
        NativeScriptItemSnapshotRecord saved;
        if(!record(&nativeScriptItemActors[i],i,&saved) || nativeScriptItemActors[i].storage.state!=nativeScriptItemActors[i].bytes ||
           nativeScriptItemActors[i].execution.storage!=&nativeScriptItemActors[i].storage)return false;
    }
    return nativeMatchArena_AddRoot(arena,&nativeScriptItemSceneEpoch,sizeof(nativeScriptItemSceneEpoch)) &&
        nativeMatchArena_AddRoot(arena,&nativeScriptItemInitializedCount,sizeof(nativeScriptItemInitializedCount)) &&
        nativeMatchArena_AddRoot(arena,nativeScriptItemActorByObject,sizeof(nativeScriptItemActorByObject)) &&
        nativeMatchArena_AddCountedRoot(arena,nativeScriptItemActors,sizeof(*nativeScriptItemActors),NATIVE_SCRIPT_ITEM_ACTORS,&nativeScriptItemInitializedCount);
}
