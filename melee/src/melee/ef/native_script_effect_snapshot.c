#include "native_script_effect_snapshot.h"
#include "native_script_effect_internal.h"
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/ftparts.h>
#include <string.h>
#include <melee/gm/native_match_arena.h>

static bool overlap(const void* first,size_t an,const void* second,size_t bn)
{uintptr_t a=(uintptr_t)first,b=(uintptr_t)second;return an>UINTPTR_MAX-a||bn>UINTPTR_MAX-b||(a<b+bn&&b<a+an);}
static bool independent(const void* data,size_t bytes)
{
    return data&&bytes<=UINTPTR_MAX-(uintptr_t)data&&
        !overlap(data,bytes,nativeScriptEffectActors,sizeof(nativeScriptEffectActors))&&
        !overlap(data,bytes,nativeScriptEffectByObject,sizeof(nativeScriptEffectByObject))&&
        !overlap(data,bytes,&nativeScriptEffectEpoch,sizeof(nativeScriptEffectEpoch));
}
static const NativeScriptEffectProgram* program(const NativeScriptEffectSnapshotRecord* r)
{return nativeScriptEffect_Program((FighterKind)r->author,r->owner);}
static bool reference(NativeMatchPoolReference r)
{return r.pool==&nativeGObjPool&&r.lifetime==nativeScriptEffectEpoch&&r.slot<NATIVE_GOBJ_CAPACITY&&r.slot<nativeGObjPool.capacity&&r.generation!=0;}
static bool valid(const NativeScriptEffectSnapshotRecord* r)
{
    const NativeScriptEffectProgram* p=program(r);
    if(r->slot>=NATIVE_SCRIPT_EFFECT_ACTORS||r->faulted>1||r->completed>r->tick||!reference(r->creator)||!p||p->identity!=r->program_identity||
       !r->member_count||r->member_count>EF_MODEL_GROUP_MAX||r->member_count!=p->member_count||r->state_bytes>NATIVE_SCRIPT_EFFECT_STATE_BYTES||!r->origin||!r->origin_identity.creator_root||r->origin_identity.on_part>1||
       (r->origin_identity.on_part&&(!r->origin_identity.package||!r->origin_identity.parts||!r->origin_identity.info||r->origin_identity.part>=MAX_FT_PARTS)))return false;
    const NativeScriptStorageOwner* row=nativeScriptStorage_Owner(&p->bank->storage_layout,p->owner);
    if(!row||row->state_bytes!=r->state_bytes||p->bank->storage_layout.identity!=r->layout_identity||
       p->bank->storage_parameters.identity!=r->parameter_identity)return false;
    for(unsigned i=0;i<r->member_count;++i)if(!reference(r->members[i])||!r->generations[i])return false;
    return true;
}
static bool capture(const NativeScriptEffectActor* a,uint32_t slot,NativeScriptEffectSnapshotRecord* r)
{
    if(!a->active||a->running||!a->storage.live||!nativeScriptEffect_Peers(a))return false;
    const NativeScriptEffectProgram* p=a->program;const NativeScriptStorageOwner* row=nativeScriptStorage_Owner(&p->bank->storage_layout,p->owner);
    if(!row||a->storage.layout!=&p->bank->storage_layout||a->storage.parameters!=&p->bank->storage_parameters||
       a->storage.owner!=p->owner||a->storage.identity!=a->generations[0]||a->storage.generation!=a->generations[0]||
       a->execution.storage!=&a->storage||a->execution.declarations!=&p->bank->declarations)return false;
    for(unsigned i=0;i<a->member_count;++i)if(nativeScriptEffectByObject[a->members[i].slot]!=slot)return false;
    *r=(NativeScriptEffectSnapshotRecord){.creator=a->creator,.origin=a->origin,.origin_identity=a->origin_identity,.program_identity=a->program_identity,
        .layout_identity=p->bank->storage_layout.identity,.parameter_identity=p->bank->storage_parameters.identity,
        .creator_layout=a->creator_layout,.creator_parameters=a->creator_parameters,.slot=slot,.author=(uint32_t)p->author_kind,
        .owner=p->owner,.member_count=a->member_count,.creator_generation=a->creator_generation,.tick=a->tick,.completed=a->completed,
        .faulted=a->faulted,.state_bytes=row->state_bytes};
    memcpy(r->members,a->members,sizeof(r->members));memcpy(r->generations,a->generations,sizeof(r->generations));return valid(r);
}
bool nativeScriptEffect_Save(void* snapshot,size_t capacity,size_t* written)
{
    if(!written||!nativeScriptEffect_Boundary()||!nativeScriptEffectEpoch)return false;
    size_t bytes=sizeof(NativeScriptEffectSnapshotHeader);unsigned count=0;NativeScriptEffectSnapshotRecord r;
    for(unsigned slot=0;slot<nativeScriptEffectInitializedCount;++slot)if(nativeScriptEffectActors[slot].active){if(!capture(&nativeScriptEffectActors[slot],slot,&r))return false;bytes+=sizeof(r)+r.state_bytes;++count;}
    if(bytes>UINT32_MAX||capacity<bytes||!independent(snapshot,bytes)||overlap(written,sizeof(*written),snapshot,bytes)||!independent(written,sizeof(*written)))return false;
    NativeScriptEffectSnapshotHeader h={.epoch=nativeScriptEffectEpoch,.count=count,.bytes=(uint32_t)bytes,.initialized_count=nativeScriptEffectInitializedCount};memcpy(snapshot,&h,sizeof(h));uint8_t* cursor=(uint8_t*)snapshot+sizeof(h);
    for(unsigned slot=0;slot<nativeScriptEffectInitializedCount;++slot)if(nativeScriptEffectActors[slot].active){capture(&nativeScriptEffectActors[slot],slot,&r);memcpy(cursor,&r,sizeof(r));cursor+=sizeof(r);memcpy(cursor,nativeScriptEffectActors[slot].bytes,r.state_bytes);cursor+=r.state_bytes;}
    *written=bytes;return true;
}
bool nativeScriptEffect_ValidateSnapshot(const void* snapshot,size_t bytes)
{
    if(bytes<sizeof(NativeScriptEffectSnapshotHeader)||!independent(snapshot,bytes)||!nativeScriptEffect_Boundary())return false;
    NativeScriptEffectSnapshotHeader h;memcpy(&h,snapshot,sizeof(h));if(!h.epoch||h.epoch!=nativeScriptEffectEpoch||h.count>h.initialized_count||h.initialized_count>NATIVE_SCRIPT_EFFECT_ACTORS||h.bytes!=bytes)return false;
    const uint8_t* cursor=(const uint8_t*)snapshot+sizeof(h);size_t remaining=bytes-sizeof(h);uint32_t previous=0;bool objects[NATIVE_GOBJ_CAPACITY]={0};
    for(unsigned i=0;i<h.count;++i){NativeScriptEffectSnapshotRecord r;if(remaining<sizeof(r))return false;memcpy(&r,cursor,sizeof(r));
        if(r.slot>=h.initialized_count||!valid(&r)||(i&&r.slot<=previous)||r.state_bytes>remaining-sizeof(r))return false;previous=r.slot;
        for(unsigned member=0;member<r.member_count;++member){if(objects[r.members[member].slot])return false;objects[r.members[member].slot]=true;}
        const NativeScriptEffectProgram* p=program(&r);const uint8_t* state=cursor+sizeof(r);
        for(unsigned f=0;f<p->bank->storage_layout.field_count;++f){const NativeScriptStorageField* field=&p->bank->storage_layout.fields[f];if(field->owner!=p->owner||field->parameter||field->type!=NativeScriptType_Bool)continue;
            if(field->offset>r.state_bytes||field->count>r.state_bytes-field->offset)return false;for(unsigned n=0;n<field->count;++n)if(state[field->offset+n]>1)return false;}
        cursor+=sizeof(r)+r.state_bytes;remaining-=sizeof(r)+r.state_bytes;
    }
    return remaining==0;
}
static bool restored(const NativeScriptEffectSnapshotRecord* r,NativeScriptEffectActor* a)
{
    memset(a,0,sizeof(*a));const NativeScriptEffectProgram* p=program(r);a->program=p;a->program_identity=r->program_identity;
    a->creator=r->creator;a->creator_generation=r->creator_generation;a->origin=r->origin;a->origin_identity=r->origin_identity;a->creator_layout=r->creator_layout;a->creator_parameters=r->creator_parameters;
    a->member_count=a->remaining=r->member_count;a->tick=r->tick;a->completed=r->completed;a->faulted=r->faulted!=0;a->active=true;
    memcpy(a->members,r->members,sizeof(a->members));memcpy(a->generations,r->generations,sizeof(a->generations));for(unsigned i=0;i<a->member_count;++i)a->member_live[i]=true;
    Fighter_GObj* creator=nativeMatchPool_Resolve(r->creator);
    if(!creator||!nativeScriptFighter_View(creator,r->creator_generation,&a->creator_view)||
       a->creator_view.program!=&p->bank->declarations||!nativeScriptEffect_Peers(a))return false;
    NativeScriptDiagnostic d={0};return nativeScriptEffect_Bind(a,p,r->generations[0],&d);
}
bool nativeScriptEffect_Load(const void* snapshot,size_t bytes)
{
    if(!nativeScriptEffect_ValidateSnapshot(snapshot,bytes))return false;
    NativeScriptEffectSnapshotHeader h;memcpy(&h,snapshot,sizeof(h));const uint8_t* cursor=(const uint8_t*)snapshot+sizeof(h);
    NativeScriptEffectActor scratch;NativeScriptEffectSnapshotRecord r;
    // Every immutable/schema/native peer and execution bind passes before any
    // owner state changes. The second pass copies those exact callback-free views.
    for(unsigned i=0;i<h.count;++i){memcpy(&r,cursor,sizeof(r));if(!restored(&r,&scratch))return false;cursor+=sizeof(r)+r.state_bytes;}
    unsigned clear_count=nativeScriptEffectInitializedCount>h.initialized_count?nativeScriptEffectInitializedCount:h.initialized_count;
    for(unsigned slot=0;slot<clear_count;++slot){nativeScriptStorage_Destroy(&nativeScriptEffectActors[slot].storage);nativeScriptEffectActors[slot].active=false;}
    nativeScriptEffectInitializedCount=h.initialized_count;
    memset(nativeScriptEffectByObject,0xFF,sizeof(nativeScriptEffectByObject));cursor=(const uint8_t*)snapshot+sizeof(h);
    for(unsigned i=0;i<h.count;++i){memcpy(&r,cursor,sizeof(r));restored(&r,&scratch);cursor+=sizeof(r);memcpy(scratch.bytes,cursor,r.state_bytes);cursor+=r.state_bytes;
        NativeScriptEffectActor* a=&nativeScriptEffectActors[r.slot];*a=scratch;a->storage.state=a->bytes;a->execution.storage=&a->storage;
        for(unsigned member=0;member<a->member_count;++member)nativeScriptEffectByObject[a->members[member].slot]=(uint16_t)r.slot;
    }
    return true;
}

bool nativeScriptEffect_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || arena->lifetime!=nativeScriptEffectEpoch || !nativeScriptEffectEpoch ||
       nativeScriptEffectEpoch!=nativeGObjPool.lifetime || nativeScriptEffectInitializedCount>NATIVE_SCRIPT_EFFECT_ACTORS ||
       !nativeScriptEffect_Boundary())return false;
    for(unsigned i=0;i<nativeScriptEffectInitializedCount;++i)if(nativeScriptEffectActors[i].active){
        NativeScriptEffectSnapshotRecord saved;
        if(!capture(&nativeScriptEffectActors[i],i,&saved) || nativeScriptEffectActors[i].storage.state!=nativeScriptEffectActors[i].bytes ||
           nativeScriptEffectActors[i].execution.storage!=&nativeScriptEffectActors[i].storage)return false;
    }
    return nativeMatchArena_AddRoot(arena,&nativeScriptEffectEpoch,sizeof(nativeScriptEffectEpoch)) &&
        nativeMatchArena_AddRoot(arena,&nativeScriptEffectInitializedCount,sizeof(nativeScriptEffectInitializedCount)) &&
        nativeMatchArena_AddRoot(arena,nativeScriptEffectByObject,sizeof(nativeScriptEffectByObject)) &&
        nativeMatchArena_AddCountedRoot(arena,nativeScriptEffectActors,sizeof(*nativeScriptEffectActors),NATIVE_SCRIPT_EFFECT_ACTORS,&nativeScriptEffectInitializedCount);
}
