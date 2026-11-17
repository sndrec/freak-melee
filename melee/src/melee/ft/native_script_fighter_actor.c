#include "native_script_fighter.h"
#include "native_script_runtime.h"
#include "native_script_actor_access.h"
#include "native_callback_lifetime.h"
#include "types.h"
#include "native_script_fighter_snapshot.h"
#include <melee/gm/native_match_arena.h>
#include <melee/gm/native_workshop_practice.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/gobj.h>
#include <dolphin/os.h>

#include <stdio.h>
#include <string.h>

typedef struct ScriptFighterActor {
    Fighter_GObj* object;
    uint32_t generation,kind,tick,completed_ticks;
    uint32_t running;
    NativeMatchPoolReference object_reference;
    bool active,faulted,loading;
    bool accessory_cue;
    uint32_t motion_functions[NativeScriptMotion_Count];
    uint32_t callbacks[NativeScriptFighterCallback_Count];
    uint8_t bytes[NATIVE_SCRIPT_FIGHTER_STATE_BYTES];
    NativeScriptStorageInstance storage;
    NativeScriptExecution execution;
} ScriptFighterActor;

// Actual mutable owners. This unit is not host-excluded: actor slot identity,
// storage, fault state and tick counters rewind with the broad global oracle.
static ScriptFighterActor actors[NATIVE_SCRIPT_FIGHTER_ACTORS];
static uint64_t scene_epoch;
static uint32_t initialized_count;

bool nativeScriptFighter_HasActors(FighterKind kind)
{
    for(unsigned i=0;i<initialized_count;++i)
        if((actors[i].active||actors[i].running) && actors[i].kind==(uint32_t)kind)return true;
    return false;
}
bool nativeScriptFighter_ReferencesProgram(const NativeScriptCompileBank* bank)
{
    if(!bank)return false;
    for(unsigned i=0;i<initialized_count;++i) {
        const ScriptFighterActor* actor=&actors[i];
        if((actor->active||actor->running) &&
           (actor->execution.declarations==&bank->declarations ||
            (actor->storage.layout&&actor->storage.layout->program==&bank->declarations)))return true;
    }
    return false;
}

static ScriptFighterActor* find(Fighter_GObj* object)
{
    for(unsigned i=0;i<initialized_count;++i)
        if(actors[i].active && actors[i].object==object)return &actors[i];
    return NULL;
}

bool nativeScriptFighter_IsAttached(Fighter_GObj* object){return find(object)!=NULL;}

static uint64_t state_fingerprint(const ScriptFighterActor* actor)
{
    const NativeScriptStorageOwner* owner = nativeScriptStorage_Owner(actor->storage.layout, 0);
    uint64_t hash = UINT64_C(1469598103934665603);
    if (owner)
        for (uint32_t i = 0; i < owner->state_bytes; ++i) {
            hash ^= actor->bytes[i];
            hash *= UINT64_C(1099511628211);
        }
    return hash;
}

uint64_t nativeScriptFighter_StateFingerprint(Fighter_GObj* object)
{
    const ScriptFighterActor* actor = find(object);
    return actor ? state_fingerprint(actor) : 0;
}

static NativeScriptVMStatus status(const ScriptFighterActor* actor)
{
    if(!actor || !actor->active || !actor->storage.live ||
       nativeMatchPool_Resolve(actor->object_reference)!=actor->object ||
       !nativeCallbackSource_Alive(actor->object,actor->generation,NativeCallbackSource_Fighter))return NativeScriptVM_SourceRetired;
    const NativeScriptFighterProgram* program=nativeScriptFighter_Program((FighterKind)actor->kind);
    if(!program || actor->execution.declarations!=&program->bank->declarations ||
       ((Fighter*)actor->object->user_data)->kind!=(FighterKind)actor->kind)return NativeScriptVM_SourceRetired;
    return actor->faulted?NativeScriptVM_Error:NativeScriptVM_Ok;
}
NativeScriptVMStatus nativeScriptFighter_Status(Fighter_GObj* object){return status(find(object));}
bool nativeScriptFighter_IsLoading(Fighter_GObj* object)
{
    const ScriptFighterActor* actor=find(object);
    return status(actor)==NativeScriptVM_Ok && actor->loading && actor->running!=0;
}

bool nativeScriptFighter_CanActivateForm(Fighter_GObj* object)
{
    const ScriptFighterActor* actor=find(object);
    return status(actor)==NativeScriptVM_Ok && actor->running==0;
}

void nativeScriptFighter_NewLife(Fighter_GObj* object,uint32_t generation)
{
    ScriptFighterActor* actor=find(object);if(!actor)return;
    // A common respawn changes creation identity without replacing the fighter
    // allocation or persistent authored variables. Old callback contexts retire.
    actor->callbacks[NativeScriptFighterCallback_Death1]=UINT32_MAX;
    actor->callbacks[NativeScriptFighterCallback_Death2]=UINT32_MAX;
    actor->callbacks[NativeScriptFighterCallback_Death3]=UINT32_MAX;
    actor->callbacks[NativeScriptFighterCallback_AccessoryBefore]=UINT32_MAX;
    actor->callbacks[NativeScriptFighterCallback_AccessoryHitlag]=UINT32_MAX;
    actor->generation=generation;actor->storage.identity=generation;actor->storage.generation=generation;
    NativeScriptDiagnostic diagnostic={0};
    if(!nativeScriptExecution_Bind(&actor->execution,&diagnostic))actor->faulted=true;
}

bool nativeScriptFighter_HasHook(Fighter_GObj* object,uint32_t hook)
{
    ScriptFighterActor* actor=find(object);
    if(!actor || hook>=NativeScriptFighter_HookCount)return false;
    const NativeScriptFighterProgram* selected=nativeScriptFighter_Program((FighterKind)actor->kind);
    return selected && selected->hooks[hook]!=UINT32_MAX;
}

bool nativeScriptFighter_Attach(Fighter_GObj* object)
{
    uint32_t generation;
    if(!nativeCallbackSource_Snapshot(object,NativeCallbackSource_Fighter,&generation))return false;
    Fighter* fighter=object->user_data;
    const NativeScriptFighterProgram* selected=nativeScriptFighter_Program(fighter->kind);
    if(!selected)return true;
    NativeMatchPoolReference reference;
    if(!scene_epoch || scene_epoch!=nativeGObjPool.lifetime ||
        !nativeMatchPool_Reference(&nativeGObjPool,object,&reference))return false;
    if(find(object))return false;
    ScriptFighterActor* actor=NULL;
    for(unsigned i=0;i<initialized_count;++i)if(!actors[i].active&&!actors[i].running){actor=&actors[i];break;}
    if(!actor){
        if(initialized_count==NATIVE_SCRIPT_FIGHTER_ACTORS)return false;
        actor=&actors[initialized_count++];
    }
    memset(actor,0,sizeof(*actor));
    for(unsigned i=0;i<NativeScriptFighterCallback_Count;++i)actor->callbacks[i]=UINT32_MAX;
    for(unsigned i=0;i<NativeScriptMotion_Count;++i)actor->motion_functions[i]=UINT32_MAX;
    const NativeScriptCompileBank* bank=selected->bank;
    NativeScriptDiagnostic diagnostic={0};
    if(!nativeScriptStorage_Create(&bank->storage_layout,&bank->storage_parameters,0,generation,generation,
            actor->bytes,sizeof(actor->bytes),&actor->storage,&diagnostic))return false;
    actor->execution=(NativeScriptExecution){.declarations=&bank->declarations,.code=&bank->code,
        .storage=&actor->storage,.vm=nativeScriptRuntime_VM(),.payloads=nativeScriptRuntime_Payloads(),
        .functions=bank->execution_functions,.function_count=bank->declarations.function_count,
        .receiver_lookup=nativeScriptActors_Lookup,.receiver_helper=nativeScriptActors_Helper,.fighter_kind=fighter->kind};
    if(!nativeScriptExecution_Bind(&actor->execution,&diagnostic))return false;
    actor->object=object;actor->object_reference=reference;actor->generation=generation;actor->kind=(uint32_t)fighter->kind;actor->active=true;
    actor->loading=true;
    bool loaded=nativeScriptFighter_Run(object,NativeScriptFighter_Load,NULL,0,NULL,NULL)==NativeScriptVM_Ok;
    actor->loading=false;
    return loaded;
}

static NativeScriptVMStatus runFunction(ScriptFighterActor* actor,uint32_t function,uint32_t event,uint32_t tick,
    unsigned result_type,const NativeScriptValue* args,uint32_t count,NativeScriptEventRecordIO* record,NativeScriptValue* output)
{
    // Each entry point admits the actor before reading its program/slots. Only
    // local contract checks occur between that admission and this function;
    // there is no callback that can retire it in that interval. Reentry below
    // still requires a fresh check before publishing records or return values.
    NativeScriptValue result={.type=(uint8_t)result_type};
    if(function==UINT32_MAX){if(output)*output=result;return NativeScriptVM_Ok;}
    uint8_t record_output[NATIVE_SCRIPT_EVENT_RECORD_BYTES];
    NativeScriptEventRecordIO staged_record={0};
    if(record) {
        if(!record->data || !record->bytes || record->bytes>sizeof(record_output))return NativeScriptVM_Error;
        memcpy(record_output,record->data,record->bytes);
        staged_record=(NativeScriptEventRecordIO){record_output,record->bytes};
    }
    uint32_t generation=actor->generation;
    ++actor->running;
    NativeScriptVMStatus state_after=nativeScriptExecution_Run(&actor->execution,actor->object,NativeCallbackSource_Fighter,
        generation,tick,scene_epoch,function,args,count,record?&staged_record:NULL,&result);
    --actor->running;
    if(actor->generation!=generation)return NativeScriptVM_SourceRetired;
    NativeScriptVMStatus actor_status=status(actor);
    if(actor_status==NativeScriptVM_SourceRetired)return NativeScriptVM_SourceRetired;
    if(state_after!=NativeScriptVM_Ok) {
        if(state_after==NativeScriptVM_SourceRetired)return state_after;
        actor->faulted=true;
        const NativeScriptVM* vm=actor->execution.vm;
        nativeWorkshopPractice_ScriptFault("fighter", actor->generation,
            vm->diagnostic.source.line, vm->diagnostic.source.column, vm->diagnostic.message);
        OSReport("[script] Fighter %u event %u frame %u source %u:%u: %s\n",actor->generation,event,tick,
            vm->diagnostic.source.line,vm->diagnostic.source.column,vm->diagnostic.message);
        return NativeScriptVM_Error;
    }
    if(actor_status!=NativeScriptVM_Ok)return actor_status;
    if(result.type!=result_type){actor->faulted=true;return NativeScriptVM_Error;}
    if(record)memcpy(record->data,record_output,record->bytes);
    if(output)*output=result;return NativeScriptVM_Ok;
}

NativeScriptVMStatus nativeScriptFighter_Run(Fighter_GObj* object,uint32_t hook,
    const NativeScriptValue* args,uint32_t count, NativeScriptEventRecordIO* record,NativeScriptValue* result)
{
    ScriptFighterActor* actor=find(object);
    if(hook>=NativeScriptFighter_HookCount)return NativeScriptVM_Error;
    NativeScriptVMStatus state=status(actor);if(state!=NativeScriptVM_Ok)return state;
    const NativeScriptFighterHookContract* contract=&nativeScriptFighterHookContracts[hook];
    if(!contract->name || count!=contract->argument_count || (count&&!args))return NativeScriptVM_Error;
    if(contract->record_bytes ? (!record || !record->data || record->bytes!=contract->record_bytes) : record!=NULL)
        return NativeScriptVM_Error;
    for(unsigned i=0;i<count;++i)if(args[i].type!=contract->arguments[i])return NativeScriptVM_Error;
    const NativeScriptFighterProgram* selected=nativeScriptFighter_Program((FighterKind)actor->kind);
    uint32_t tick=actor->tick;
    if(hook==NativeScriptFighter_Tick)++actor->tick;
    state=runFunction(actor,selected->hooks[hook],hook,tick,contract->result_type,args,count,record,result);
    if(state==NativeScriptVM_Ok && hook==NativeScriptFighter_Tick && selected->hooks[hook]!=UINT32_MAX)++actor->completed_ticks;
    return state;
}

// These are engine-owned slots, not authored C entry points. Keeping the actual
// pointers clear lets shared item/common-state assignments explicitly take over.
static HSD_GObjEvent* nativeSlot(Fighter* fighter,uint32_t slot)
{
    switch(slot) {
    case NativeScriptFighterCallback_Damage:return &fighter->take_dmg_cb;
    case NativeScriptFighterCallback_Death1:return &fighter->death1_cb;
    case NativeScriptFighterCallback_Death2:return &fighter->death2_cb;
    case NativeScriptFighterCallback_Death3:return &fighter->death3_cb;
    case NativeScriptFighterCallback_PreHitlag:return &fighter->pre_hitlag_cb;
    case NativeScriptFighterCallback_PostHitlag:return &fighter->post_hitlag_cb;
    case NativeScriptFighterCallback_DealDamage:return &fighter->deal_dmg_cb;
    case NativeScriptFighterCallback_StateChange:return &fighter->x21EC;
    case NativeScriptFighterCallback_ReflectHit:return &fighter->reflect_hit_cb;
    case NativeScriptFighterCallback_Reflect:return &fighter->x21F8;
    case NativeScriptFighterCallback_HurtboxDetect:return &fighter->hurtbox_detect_cb;
    case NativeScriptFighterCallback_ShieldHit:return &fighter->shield_hit_cb;
    case NativeScriptFighterCallback_AccessoryBefore:return &fighter->accessory2_cb;
    case NativeScriptFighterCallback_AccessoryHitlag:return &fighter->accessory3_cb;
    case NativeScriptFighterCallback_SecondaryDamage:return &fighter->take_dmg_2_cb;
    default:return NULL;
    }
}

uint32_t nativeScriptFighter_CallbackFunction(Fighter_GObj* object,uint32_t slot)
{
    ScriptFighterActor* actor=find(object);if(!actor || slot>=NativeScriptFighterCallback_Count)return UINT32_MAX;
    const NativeScriptFighterProgram* program=nativeScriptFighter_Program((FighterKind)actor->kind);
    return program?program->callbacks[slot]:UINT32_MAX;
}

bool nativeScriptFighter_BindCallback(Fighter_GObj* object,uint32_t slot,uint32_t function)
{
    ScriptFighterActor* actor=find(object);
    if(!actor || actor->faulted || slot>=NativeScriptFighterCallback_Count ||
        !nativeCallbackSource_Alive(object,actor->generation,NativeCallbackSource_Fighter))return false;
    HSD_GObjEvent* pointer=nativeSlot(object->user_data,slot);
    if(!pointer)return false;
    if(function!=UINT32_MAX) {
        const NativeScriptProgramIndex* index=actor->execution.declarations;
        if(function>=index->function_count)return false;
        const NativeScriptFunctionDeclaration* declaration=&index->functions[function];
        if(declaration->owner || declaration->parameter_count || declaration->result_type!=NativeScriptType_Void)return false;
        if(declaration->callback) {
            if(nativeScriptFighter_CallbackFunction(object,slot)!=function)return false;
        }
    }
    actor->callbacks[slot]=function;*pointer=NULL;return true;
}

bool nativeScriptFighter_CallbackBound(Fighter_GObj* object,uint32_t slot)
{
    ScriptFighterActor* actor=find(object);
    if(!actor || slot>=NativeScriptFighterCallback_Count ||
        !nativeCallbackSource_Alive(object,actor->generation,NativeCallbackSource_Fighter))return false;
    HSD_GObjEvent* pointer=nativeSlot(object->user_data,slot);
    if(pointer && *pointer)actor->callbacks[slot]=UINT32_MAX;
    return actor->callbacks[slot]!=UINT32_MAX;
}

bool nativeScriptFighter_CallbackMatches(Fighter_GObj* object,uint32_t slot,uint32_t function,bool* result)
{
    const ScriptFighterActor* actor=find(object);
    if(!result || slot>=NativeScriptFighterCallback_Count || status(actor)!=NativeScriptVM_Ok)return false;
    HSD_GObjEvent* pointer=nativeSlot(object->user_data,slot);if(!pointer)return false;
    if(function!=UINT32_MAX) {
        const NativeScriptProgramIndex* index=actor->execution.declarations;
        if(function>=index->function_count)return false;
        const NativeScriptFunctionDeclaration* declaration=&index->functions[function];
        if(declaration->owner || declaration->callback || declaration->parameter_count ||
           declaration->result_type!=NativeScriptType_Void)return false;
    }
    // Do not lazily discard actor metadata here: this API is a pure observation.
    // A native takeover is acknowledged by CallbackBound at dispatch time.
    *result=*pointer==NULL && actor->callbacks[slot]==function;return true;
}

bool nativeScriptFighter_RunCallback(Fighter_GObj* object,uint32_t slot)
{
    ScriptFighterActor* actor=find(object);
    if(!actor || slot>=NativeScriptFighterCallback_Count)return false;
    if(status(actor)!=NativeScriptVM_Ok)return false;
    // x21EC is one-shot. Clear after execution, matching the native call site,
    // including a re-registration performed by the callback itself.
    bool success=runFunction(actor,actor->callbacks[slot],slot,actor->tick,NativeScriptType_Void,NULL,0,NULL,NULL)==NativeScriptVM_Ok;
    if(success && slot==NativeScriptFighterCallback_StateChange) {
        actor->callbacks[slot]=UINT32_MAX;
        *nativeSlot(object->user_data,slot)=NULL;
    }
    return success;
}

void nativeScriptFighter_ResetMotionCallbacks(Fighter_GObj* object)
{
    ScriptFighterActor* actor=find(object);if(!actor)return;
    static const uint8_t slots[]={NativeScriptFighterCallback_Damage,NativeScriptFighterCallback_Death2,
        NativeScriptFighterCallback_PreHitlag,NativeScriptFighterCallback_PostHitlag,NativeScriptFighterCallback_DealDamage,
        NativeScriptFighterCallback_ReflectHit,NativeScriptFighterCallback_Reflect,
        NativeScriptFighterCallback_HurtboxDetect,NativeScriptFighterCallback_ShieldHit,
        NativeScriptFighterCallback_SecondaryDamage};
    for(unsigned i=0;i<sizeof(slots);++i)actor->callbacks[slots[i]]=UINT32_MAX;
}

void nativeScriptFighter_Detach(Fighter_GObj* object)
{
    ScriptFighterActor* actor=find(object);
    if(!actor)return;
    (void)nativeScriptFighter_Run(object,NativeScriptFighter_Remove,NULL,0, NULL,NULL);
    const uint64_t hash = state_fingerprint(actor);
    OSReport("[script] Retired fighter %u: %u completed / %u requested interpreted ticks, state=%016llx, faulted=%u.\n",
        actor->generation,actor->completed_ticks,actor->tick,(unsigned long long)hash,actor->faulted);
    nativeScriptStorage_Destroy(&actor->storage);actor->active=false;
}

static HSD_GObjEvent* motionSlot(Fighter* fighter,uint32_t phase)
{
    switch(phase) {
    case NativeScriptMotion_Animation:return &fighter->anim_cb;
    case NativeScriptMotion_Interrupt:return &fighter->input_cb;
    case NativeScriptMotion_Physics:return &fighter->phys_cb;
    case NativeScriptMotion_Collision:return &fighter->coll_cb;
    case NativeScriptMotion_Accessory:return &fighter->accessory4_cb;
    default:return NULL;
    }
}
void nativeScriptFighter_SelectMotion(Fighter_GObj* object,uint32_t motion)
{
    ScriptFighterActor* actor=find(object);if(!actor)return;
    const NativeScriptFighterProgram* program=nativeScriptFighter_Program((FighterKind)actor->kind);
    for(unsigned p=0;p<NativeScriptMotion_Count;++p)actor->motion_functions[p]=UINT32_MAX;
    actor->accessory_cue=false;
    if(!program || !nativeCallbackSource_Alive(object,actor->generation,NativeCallbackSource_Fighter))return;
    uint32_t begin=0,end=program->motion_count;
    while(begin<end){uint32_t mid=begin+(end-begin)/2;if(program->motions[mid].motion<motion)begin=mid+1;else end=mid;}
    if(begin==program->motion_count || program->motions[begin].motion!=motion)return;
    actor->accessory_cue=program->motions[begin].accessory_cue;
    for(unsigned phase=0;phase<NativeScriptMotion_Count;++phase) {
        uint32_t function=program->motions[begin].functions[phase];actor->motion_functions[phase]=function;
        if(function!=UINT32_MAX)*motionSlot(object->user_data,phase)=NULL;
    }
}
bool nativeScriptFighter_MotionBound(Fighter_GObj* object,uint32_t phase)
{
    ScriptFighterActor* actor=find(object);
    if(!actor || phase>=NativeScriptMotion_Count || !nativeCallbackSource_Alive(object,actor->generation,NativeCallbackSource_Fighter))return false;
    HSD_GObjEvent* pointer=motionSlot(object->user_data,phase);
    if(pointer && *pointer){actor->motion_functions[phase]=UINT32_MAX;if(phase==NativeScriptMotion_Accessory)actor->accessory_cue=false;}
    return actor->motion_functions[phase]!=UINT32_MAX;
}
bool nativeScriptFighter_RunMotion(Fighter_GObj* object,uint32_t phase)
{
    ScriptFighterActor* actor=find(object);if(!actor || phase>=NativeScriptMotion_Count)return false;
    if(status(actor)!=NativeScriptVM_Ok)return false;
    if(phase==NativeScriptMotion_Accessory && actor->accessory_cue) {
        Fighter* fighter=object->user_data;
        if(!fighter->throw_flags_b0)return true;
        fighter->throw_flags_b0=false;
    }
    return runFunction(actor,actor->motion_functions[phase],phase,actor->tick,NativeScriptType_Void,NULL,0,NULL,NULL)==NativeScriptVM_Ok;
}
bool nativeScriptFighter_BindMotionPhase(Fighter_GObj* object,uint32_t phase,uint32_t function,bool cue)
{
    ScriptFighterActor* actor=find(object);
    if(phase>=NativeScriptMotion_Count || status(actor)!=NativeScriptVM_Ok ||
       (cue && phase!=NativeScriptMotion_Accessory))return false;
    if(function!=UINT32_MAX) {
        const NativeScriptProgramIndex* index=actor->execution.declarations;
        if(function>=index->function_count)return false;
        const NativeScriptFunctionDeclaration* declaration=&index->functions[function];
        const NativeScriptFighterProgram* program=nativeScriptFighter_Program((FighterKind)actor->kind);
        bool jump_hook=phase==NativeScriptMotion_Physics && !cue && program &&
            program->hooks[NativeScriptFighter_AnimationDrivenJumpPhysics]==function;
        if(!declaration->signature_resolved || declaration->owner ||
           (declaration->callback && !jump_hook) || declaration->parameter_count ||
           declaration->result_type!=NativeScriptType_Void)return false;
    }
    actor->motion_functions[phase]=function;
    if(phase==NativeScriptMotion_Accessory)actor->accessory_cue=function!=UINT32_MAX && cue;
    *motionSlot(object->user_data,phase)=NULL;return true;
}

static void reportState(const ScriptFighterActor* actor)
{
    const NativeScriptProgramIndex* index=actor->execution.declarations;
    unsigned printed=0;
    for(uint32_t i=0;i<index->value_count && printed<32;++i) {
        const NativeScriptValueDeclaration* field=&index->values[i];
        if(field->owner || field->parameter || field->array)continue;
        NativeScriptValue value;NativeScriptDiagnostic diagnostic;
        if(!nativeScriptStorage_Read(&actor->storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&value,&diagnostic))continue;
        const char* name=index->tree->source+field->name.offset;
        if(value.type==NativeScriptType_F32)
            OSReport("[script] State fighter %u %.*s = %.9g\n",actor->generation,(int)field->name.length,name,(double)value.as.f32);
        else if(value.type==NativeScriptType_I32)
            OSReport("[script] State fighter %u %.*s = %lld\n",actor->generation,(int)field->name.length,name,(long long)value.as.i64);
        else if(value.type==NativeScriptType_Bool || value.type==NativeScriptType_U32)
            OSReport("[script] State fighter %u %.*s = %llu\n",actor->generation,(int)field->name.length,name,(unsigned long long)value.as.u64);
        else continue;
        ++printed;
    }
}

void nativeScriptFighter_Report(void)
{
    for(unsigned at=0;at<initialized_count;++at) {
        const ScriptFighterActor* actor=&actors[at];if(!actor->active)continue;
        const uint64_t hash = state_fingerprint(actor);
        OSReport("[script] Live fighter %u kind %u: %u completed / %u requested interpreted ticks, state=%016llx, faulted=%u.\n",
            actor->generation,actor->kind,actor->completed_ticks,actor->tick,(unsigned long long)hash,actor->faulted);
        reportState(actor);
    }
}

bool nativeScriptFighter_RetireScene(void)
{
    if(!nativeScriptRuntime_Idle())return false;
    scene_epoch=0;
    // Exit callbacks already ran. Preload may reclaim pointed assets next, so
    // retire identities now without calling authored removal/destructors again.
    for(unsigned i=0;i<initialized_count;++i)if(actors[i].active) {
        nativeScriptStorage_Destroy(&actors[i].storage);actors[i].active=false;
    }
    return true;
}

bool nativeScriptFighter_CanBeginScene(uint64_t epoch)
{
    if(!epoch || scene_epoch || !nativeScriptRuntime_Idle())return false;
    for(unsigned i=0;i<initialized_count;++i)if(actors[i].active||actors[i].running)return false;
    return true;
}
bool nativeScriptFighter_BeginScene(uint64_t epoch)
{
    if(!nativeScriptFighter_CanBeginScene(epoch))return false;
    initialized_count=0;scene_epoch=epoch;return true;
}
static bool snapshotRecord(const ScriptFighterActor* actor,unsigned slot,NativeScriptFighterSnapshotRecord* output)
{
    const NativeScriptFighterProgram* program=nativeScriptFighter_Program((FighterKind)actor->kind);
    const NativeScriptStorageOwner* owner=program?nativeScriptStorage_Owner(&program->bank->storage_layout,0):NULL;
    if(actor->running || status(actor)==NativeScriptVM_SourceRetired || !owner || owner->state_bytes>sizeof(actor->bytes) || actor->storage.layout!=&program->bank->storage_layout ||
        actor->execution.declarations!=&program->bank->declarations || !actor->storage.live ||
        actor->storage.generation!=actor->generation || actor->storage.identity!=actor->generation ||
        !nativeCallbackSource_Alive(actor->object,actor->generation,NativeCallbackSource_Fighter))return false;
    NativeScriptFighterSnapshotRecord record={0};
    if(!nativeMatchPool_Reference(&nativeGObjPool,actor->object,&record.object))return false;
    record.slot=slot;record.program_identity=program->identity;record.layout_identity=program->bank->storage_layout.identity;
    record.parameter_identity=program->bank->storage_parameters.identity;record.state_bytes=owner->state_bytes;
    record.generation=actor->generation;record.kind=actor->kind;record.tick=actor->tick;
    record.completed_ticks=actor->completed_ticks;record.faulted=actor->faulted;
    record.accessory_cue=actor->accessory_cue;
    memcpy(record.motions,actor->motion_functions,sizeof(record.motions));memcpy(record.callbacks,actor->callbacks,sizeof(record.callbacks));
    *output=record;return true;
}
bool nativeScriptFighter_Save(void* snapshot,size_t capacity,size_t* written)
{
    if(!snapshot || !written || !scene_epoch || scene_epoch!=nativeGObjPool.lifetime || !nativeScriptRuntime_Idle())return false;
    uintptr_t start=(uintptr_t)snapshot,end=(uintptr_t)actors+sizeof(actors);
    if(start>UINTPTR_MAX-capacity || (start<end && start+capacity>(uintptr_t)actors))return false;
    uintptr_t output=(uintptr_t)written;
    if(output>UINTPTR_MAX-sizeof(*written) ||
        (output<end && output+sizeof(*written)>(uintptr_t)actors) ||
        (output<start+capacity && output+sizeof(*written)>start))return false;
    NativeScriptFighterSnapshotHeader header={.epoch=scene_epoch,.initialized_count=initialized_count};size_t bytes=sizeof(header);
    NativeScriptFighterSnapshotRecord records[NATIVE_SCRIPT_FIGHTER_ACTORS];
    for(unsigned i=0;i<initialized_count;++i)if(actors[i].active) {
        if(!snapshotRecord(&actors[i],i,&records[header.count]))return false;
        bytes+=sizeof(*records)+records[header.count].state_bytes;++header.count;
    }
    if(bytes>capacity || bytes>UINT32_MAX)return false;
    header.total_bytes=(uint32_t)bytes;uint8_t* cursor=snapshot;memcpy(cursor,&header,sizeof(header));cursor+=sizeof(header);
    for(unsigned i=0;i<header.count;++i){
        memcpy(cursor,&records[i],sizeof(*records));cursor+=sizeof(*records);
        memcpy(cursor,actors[records[i].slot].bytes,records[i].state_bytes);cursor+=records[i].state_bytes;
    }
    *written=bytes;return true;
}
bool nativeScriptFighter_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || !scene_epoch || scene_epoch!=nativeGObjPool.lifetime ||
       initialized_count>NATIVE_SCRIPT_FIGHTER_ACTORS || !nativeScriptRuntime_Idle())return false;
    for(unsigned i=0;i<initialized_count;++i){
        if(actors[i].running || actors[i].loading)return false;
        if(actors[i].active){
            NativeScriptFighterSnapshotRecord record;
            if(!snapshotRecord(&actors[i],i,&record) || actors[i].storage.state!=actors[i].bytes ||
               actors[i].execution.storage!=&actors[i].storage)return false;
        }
    }
    return nativeMatchArena_AddRoot(arena,&scene_epoch,sizeof(scene_epoch)) &&
        nativeMatchArena_AddRoot(arena,&initialized_count,sizeof(initialized_count)) &&
        nativeMatchArena_AddCountedRoot(arena,actors,sizeof(*actors),NATIVE_SCRIPT_FIGHTER_ACTORS,&initialized_count);
}
static bool validRecord(const NativeScriptFighterSnapshotRecord* record,uint32_t previous,bool first)
{
    if(record->slot>=NATIVE_SCRIPT_FIGHTER_ACTORS || (!first && record->slot<=previous) || record->faulted>1 || record->accessory_cue>1 ||
        record->completed_ticks>record->tick || record->kind>=Ft_Kind_RuntimeCapacity || record->kind==Ft_Kind_None ||
        record->object.pool!=&nativeGObjPool || record->object.lifetime!=scene_epoch ||
        record->object.slot>=nativeGObjPool.capacity || !record->object.generation)return false;
    if(record->accessory_cue && record->motions[NativeScriptMotion_Accessory]==UINT32_MAX)return false;
    const NativeScriptFighterProgram* program=nativeScriptFighter_Program((FighterKind)record->kind);
    const NativeScriptStorageOwner* owner=program?nativeScriptStorage_Owner(&program->bank->storage_layout,0):NULL;
    if(!owner || program->identity!=record->program_identity || program->bank->storage_layout.identity!=record->layout_identity ||
        program->bank->storage_parameters.identity!=record->parameter_identity || owner->state_bytes!=record->state_bytes ||
        record->state_bytes>NATIVE_SCRIPT_FIGHTER_STATE_BYTES)return false;
    for(unsigned group=0;group<2;++group) {
        const uint32_t* functions=group?record->callbacks:record->motions;
        unsigned count=group?NativeScriptFighterCallback_Count:NativeScriptMotion_Count;
        for(unsigned i=0;i<count;++i)if(functions[i]!=UINT32_MAX){
            if(functions[i]>=program->bank->declarations.function_count)return false;
            const NativeScriptFunctionDeclaration* function=&program->bank->declarations.functions[functions[i]];
            if(function->owner || function->parameter_count || function->result_type!=NativeScriptType_Void)return false;
            if(group && function->callback && program->callbacks[i]!=functions[i])return false;
            if(!group && function->callback) {
                bool installed=i==NativeScriptMotion_Physics &&
                    program->hooks[NativeScriptFighter_AnimationDrivenJumpPhysics]==functions[i];
                for(uint32_t motion=0;motion<program->motion_count;++motion)
                    if(program->motions[motion].functions[i]==functions[i] &&
                       (i!=NativeScriptMotion_Accessory || program->motions[motion].accessory_cue==(record->accessory_cue!=0))){installed=true;break;}
                // Authored helpers may replace any phase. Event callbacks
                // remain restricted to their prepared phase/cue or the explicit
                // shared animation-driven jump Physics contract.
                if(!installed)return false;
            }
        }
    }
    return true;
}
bool nativeScriptFighter_ValidateSnapshot(const void* snapshot,size_t bytes)
{
    if(!snapshot || bytes<sizeof(NativeScriptFighterSnapshotHeader) || !scene_epoch ||
        scene_epoch!=nativeGObjPool.lifetime || !nativeScriptRuntime_Idle())return false;
    NativeScriptFighterSnapshotHeader header;memcpy(&header,snapshot,sizeof(header));
    if(header.epoch!=scene_epoch || header.total_bytes!=bytes || header.count>header.initialized_count || header.initialized_count>NATIVE_SCRIPT_FIGHTER_ACTORS)return false;
    const uint8_t* cursor=(const uint8_t*)snapshot+sizeof(header);size_t remaining=bytes-sizeof(header);
    uint32_t previous=0;NativeMatchPoolReference references[NATIVE_SCRIPT_FIGHTER_ACTORS];
    for(unsigned i=0;i<header.count;++i){
        if(remaining<sizeof(NativeScriptFighterSnapshotRecord))return false;
        NativeScriptFighterSnapshotRecord record;memcpy(&record,cursor,sizeof(record));
        if(record.slot>=header.initialized_count || !validRecord(&record,previous,i==0) || record.state_bytes>remaining-sizeof(record))return false;
        for(unsigned j=0;j<i;++j)if(references[j].slot==record.object.slot)return false;
        references[i]=record.object;previous=record.slot;
        // Owned Boolean state is canonical. Avoid accepting invalid bytes that
        // would otherwise change the meaning of later native Bool operations.
        const NativeScriptFighterProgram* program=nativeScriptFighter_Program((FighterKind)record.kind);
        const uint8_t* state=cursor+sizeof(record);
        for(uint32_t f=0;f<program->bank->storage_layout.field_count;++f){
            const NativeScriptStorageField* field=&program->bank->storage_layout.fields[f];
            if(field->owner || field->parameter || field->type!=NativeScriptType_Bool)continue;
            for(uint32_t n=0;n<field->count;++n)if(state[field->offset+n]>1)return false;
        }
        cursor+=sizeof(record)+record.state_bytes;remaining-=sizeof(record)+record.state_bytes;
    }
    return remaining==0;
}
bool nativeScriptFighter_Load(const void* snapshot,size_t bytes)
{
    uintptr_t start=(uintptr_t)snapshot,end=(uintptr_t)actors+sizeof(actors);
    if(start>UINTPTR_MAX-bytes || (start<end && start+bytes>(uintptr_t)actors))return false;
    if(!nativeScriptFighter_ValidateSnapshot(snapshot,bytes))return false;
    NativeScriptFighterSnapshotHeader header;memcpy(&header,snapshot,sizeof(header));
    const uint8_t* cursor=(const uint8_t*)snapshot+sizeof(header);
    NativeScriptFighterSnapshotRecord records[NATIVE_SCRIPT_FIGHTER_ACTORS];
    NativeScriptStorageInstance storage[NATIVE_SCRIPT_FIGHTER_ACTORS];NativeScriptExecution execution[NATIVE_SCRIPT_FIGHTER_ACTORS];
    for(unsigned i=0;i<header.count;++i){
        memcpy(&records[i],cursor,sizeof(*records));cursor+=sizeof(*records)+records[i].state_bytes;
        Fighter_GObj* object=nativeMatchPool_Resolve(records[i].object);
        if(!object || !nativeCallbackSource_Alive(object,records[i].generation,NativeCallbackSource_Fighter) ||
            ((Fighter*)object->user_data)->kind!=(FighterKind)records[i].kind)return false;
        const NativeScriptCompileBank* bank=nativeScriptFighter_Program((FighterKind)records[i].kind)->bank;
        storage[i]=(NativeScriptStorageInstance){.layout=&bank->storage_layout,.parameters=&bank->storage_parameters,
            .state=actors[records[i].slot].bytes,.capacity=NATIVE_SCRIPT_FIGHTER_STATE_BYTES,.owner=0,
            .identity=records[i].generation,.generation=records[i].generation,.live=true};
        execution[i]=(NativeScriptExecution){.declarations=&bank->declarations,.code=&bank->code,.storage=&storage[i],
            .vm=nativeScriptRuntime_VM(),.payloads=nativeScriptRuntime_Payloads(),.functions=bank->execution_functions,
            .function_count=bank->declarations.function_count,.receiver_lookup=nativeScriptActors_Lookup,
            .receiver_helper=nativeScriptActors_Helper,.fighter_kind=records[i].kind};
        NativeScriptDiagnostic diagnostic;if(!nativeScriptExecution_Bind(&execution[i],&diagnostic))return false;
    }
    // All immutable identities, typed records and restored peer lifetimes passed.
    // Inactive reservations are not copied; attach always initializes them.
    unsigned clear_count=initialized_count>header.initialized_count?initialized_count:header.initialized_count;
    for(unsigned i=0;i<clear_count;++i){actors[i].active=false;nativeScriptStorage_Destroy(&actors[i].storage);}
    initialized_count=header.initialized_count;
    cursor=(const uint8_t*)snapshot+sizeof(header);
    for(unsigned i=0;i<header.count;++i){
        ScriptFighterActor* actor=&actors[records[i].slot];actor->object=nativeMatchPool_Resolve(records[i].object);
        actor->object_reference=records[i].object;actor->running=0;actor->loading=false;
        actor->generation=records[i].generation;actor->kind=records[i].kind;actor->tick=records[i].tick;
        actor->completed_ticks=records[i].completed_ticks;actor->faulted=records[i].faulted!=0;
        actor->accessory_cue=records[i].accessory_cue!=0;
        memcpy(actor->motion_functions,records[i].motions,sizeof(records[i].motions));memcpy(actor->callbacks,records[i].callbacks,sizeof(records[i].callbacks));
        cursor+=sizeof(*records);memcpy(actor->bytes,cursor,records[i].state_bytes);cursor+=records[i].state_bytes;
        actor->storage=storage[i];actor->execution=execution[i];actor->execution.storage=&actor->storage;actor->active=true;
    }
    return true;
}
bool nativeScriptFighter_View(Fighter_GObj* object,uint32_t generation,NativeScriptReceiverView* result)
{
    ScriptFighterActor* actor=find(object);
    if(!result || !actor || actor->generation!=generation || status(actor)==NativeScriptVM_SourceRetired)return false;
    const NativeScriptFighterProgram* program=nativeScriptFighter_Program((FighterKind)actor->kind);
    if(!program || actor->execution.declarations!=&program->bank->declarations)return false;
    *result=(NativeScriptReceiverView){actor->execution.declarations,&actor->storage};return true;
}
NativeScriptVMStatus nativeScriptFighter_Helper(Fighter_GObj* object,uint32_t generation,
    uint32_t function,const NativeScriptValue* args,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    NativeScriptReceiverView view;ScriptFighterActor* actor=find(object);
    if(!actor || actor->faulted || !nativeScriptFighter_View(object,generation,&view) ||
        function>=view.program->function_count || view.program->functions[function].owner || view.program->functions[function].callback) {
        if(diagnostic) {
            // The VM is shared by all actors. Its current diagnostic belongs
            // to this delivery, not to the recipient's earlier failure.
            memset(diagnostic,0,sizeof(*diagnostic));
            snprintf(diagnostic->message,sizeof(diagnostic->message),actor && actor->faulted ?
                "Fighter helper recipient is faulted" :
                "Fighter helper was not delivered to its live owned root function");
        }
        return NativeScriptVM_Error;
    }
    const NativeScriptFunctionDeclaration* declaration=&view.program->functions[function];
    if(count!=declaration->parameter_count || (count&&!args)) {
        if(diagnostic) {
            diagnostic->source=view.program->tree->nodes[declaration->source_node].source;
            snprintf(diagnostic->message,sizeof(diagnostic->message),"Fighter helper arguments do not match its signature");
        }
        return NativeScriptVM_Error;
    }
    NativeScriptVMStatus state=runFunction(actor,function,UINT32_MAX,actor->tick,
        view.program->functions[function].result_type,args,count,NULL,result);
    if(state!=NativeScriptVM_Ok&&diagnostic)*diagnostic=actor->execution.vm->diagnostic;
    return state;
}
