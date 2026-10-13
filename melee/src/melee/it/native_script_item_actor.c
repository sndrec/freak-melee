#include "native_script_item_actor_internal.h"
#include <melee/ft/native_script_actor_access.h>
#include "types.h"
#include "native_item_owner.h"
#include <melee/ft/fighter.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_fighter_runtime.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/gobj.h>
#include <dolphin/os.h>
#include <string.h>
#include <math.h>

// Actual mutable entity owners, deliberately not host-excluded from .mstate.
NativeScriptItemActor nativeScriptItemActors[NATIVE_SCRIPT_ITEM_ACTORS];
uint32_t nativeScriptItemInitializedCount;
uint64_t nativeScriptItemSceneEpoch;
uint16_t nativeScriptItemActorByObject[NATIVE_GOBJ_CAPACITY];

typedef struct HookContract { const char* name; uint8_t context, kind, result, count, args[3]; } HookContract;
#define ARTICLE(name,result,count,...) {name,NativeScriptEvent_Article,NativeScriptEvent_ArticleLogic,result,count,{__VA_ARGS__}}
#define WORKSHOP(name,result) {name,NativeScriptEvent_WorkshopItem,NativeScriptEvent_ItemHook,result,0,{0}}
static const HookContract hooks[NativeScriptItemHook_Count]={
    ARTICLE("spawned",NativeScriptType_Void,0,0),ARTICLE("destroyed",NativeScriptType_Void,0,0),
    ARTICLE("picked_up",NativeScriptType_Void,0,0),ARTICLE("dropped",NativeScriptType_Void,0,0),
    ARTICLE("thrown",NativeScriptType_Void,0,0),ARTICLE("damage_dealt",NativeScriptType_Bool,0,0),
    ARTICLE("damage_received",NativeScriptType_Bool,0,0),ARTICLE("entered_air",NativeScriptType_Void,0,0),
    ARTICLE("reflected",NativeScriptType_Bool,0,0),ARTICLE("clanked",NativeScriptType_Bool,0,0),
    ARTICLE("absorbed",NativeScriptType_Bool,0,0),ARTICLE("shield_bounced",NativeScriptType_Bool,0,0),
    ARTICLE("hit_shield",NativeScriptType_Bool,0,0),ARTICLE("interaction",NativeScriptType_Void,1,NativeScriptType_Object),
    ARTICLE("created",NativeScriptType_Void,0,0),ARTICLE("out_of_bounds",NativeScriptType_Void,0,0),
    ARTICLE("accessory",NativeScriptType_Void,0,0),ARTICLE("entered_hitlag",NativeScriptType_Void,0,0),
    ARTICLE("exited_hitlag",NativeScriptType_Void,0,0),ARTICLE("touched",NativeScriptType_Bool,0,0),
    ARTICLE("jumped_on",NativeScriptType_Bool,0,0),ARTICLE("grab_dealt",NativeScriptType_Void,0,0),
    ARTICLE("grabbed_for_victim",NativeScriptType_Void,1,NativeScriptType_Object),ARTICLE("launched",NativeScriptType_Void,0,0),
    ARTICLE("stage_contact",NativeScriptType_Void,0,0),ARTICLE("stage_landed",NativeScriptType_Void,0,0),
    ARTICLE("stage_left",NativeScriptType_Void,0,0),ARTICLE("held_status",NativeScriptType_I32,0,0),
    ARTICLE("held_request",NativeScriptType_Bool,3,NativeScriptType_I32,NativeScriptType_F32,NativeScriptType_F32),
    ARTICLE("holder_neutral",NativeScriptType_Void,0,0),
    WORKSHOP("thrown",NativeScriptType_Void),WORKSHOP("tick",NativeScriptType_Void),
    WORKSHOP("hit",NativeScriptType_Bool),WORKSHOP("ground_collision",NativeScriptType_Bool)
};
#undef ARTICLE
#undef WORKSHOP
static const HookContract phases[NativeScriptItemPhase_Count]={
    {"animation",NativeScriptEvent_Article,NativeScriptEvent_ArticleState,NativeScriptType_Bool,0,{0}},
    {"physics",NativeScriptEvent_Article,NativeScriptEvent_ArticleState,NativeScriptType_Void,0,{0}},
    {"collision",NativeScriptEvent_Article,NativeScriptEvent_ArticleState,NativeScriptType_Bool,0,{0}}
};
static bool function_valid(const NativeScriptItemProgram* p,uint32_t id,const HookContract* expected,bool helper)
{
    if(id==UINT32_MAX)return true;
    const NativeScriptCompileBank* b=p->bank;
    if(id>=b->declarations.function_count || id>=b->code.function_count)return false;
    const NativeScriptFunctionDeclaration* f=&b->declarations.functions[id];
    const NativeScriptExecutionFunction* e=&b->execution_functions[id];
    const NativeScriptEventDescriptor* event=nativeScriptEvents_Descriptor(e->event_id);
    if(f->parameter_first>b->declarations.parameter_count || f->parameter_count>b->declarations.parameter_count-f->parameter_first ||
       f->owner!=p->owner || e->owner!=p->owner || e->source_class!=NativeCallbackSource_Item ||
       f->result_type!=expected->result || f->parameter_count!=expected->count)return false;
    if(f->callback || e->callback){
        if(!f->callback || !e->callback || !event || !event->signature_supported ||
           event->result_type!=expected->result || event->argument_count!=expected->count ||
           event->context!=expected->context || event->kind!=expected->kind || strcmp(event->name,expected->name))return false;
    }else if(!helper)return false;
    for(unsigned i=0;i<expected->count;++i)
        if((f->callback&&event->arguments[i].type!=expected->args[i]) ||
           b->declarations.parameters[f->parameter_first+i].type!=expected->args[i])return false;
    return true;
}
bool nativeScriptItem_ProgramValid(const NativeScriptItemProgram* p)
{
    if(!p || !p->bank || !p->bank->valid || !p->identity ||
       (uint32_t)p->author_kind>=Ft_Kind_RuntimeCapacity || p->author_kind==Ft_Kind_None ||
       nativeScriptItem_Program(p->author_kind,p->owner)!=p)return false;
    const NativeScriptCompileBank* b=p->bank;
    if(!p->owner || p->owner>=b->tree.count ||
       (b->tree.nodes[p->owner].kind!=NativeScriptSource_Article && b->tree.nodes[p->owner].kind!=NativeScriptSource_Item) ||
       !p->state_count || p->state_count>NATIVE_SCRIPT_ITEM_STATES || !p->states ||
       p->signal_count>NATIVE_SCRIPT_ITEM_SIGNALS || (p->signal_count&&!p->signals) ||
       b->maximum_call_depth>NATIVE_SCRIPT_RUNTIME_FRAMES || b->maximum_call_registers>NATIVE_SCRIPT_RUNTIME_REGISTERS)return false;
    if(b->tree.nodes[p->owner].kind==NativeScriptSource_Item){
        if(p->kind!=It_Kind_NativeHoldable || p->recipe_index>UINT16_MAX || !p->recipe ||
           p->recipe->on_thrown || p->recipe->on_tick || p->recipe->on_ground_collision || p->recipe->on_hit)return false;
    }else if(p->recipe_index!=UINT32_MAX || p->recipe)return false;
    const NativeScriptStorageOwner* owner=nativeScriptStorage_Owner(&b->storage_layout,p->owner);
    NativeScriptDiagnostic d={0};
    if(!owner || owner->state_bytes>NATIVE_SCRIPT_ITEM_STATE_BYTES ||
       !nativeScriptStorage_ParametersReady(&b->storage_layout,&b->storage_parameters,p->owner,&d))return false;
    for(unsigned i=0;i<NativeScriptItemHook_Count;++i)if(!function_valid(p,p->hooks[i],&hooks[i],false))return false;
    for(unsigned i=0;i<p->state_count;++i){
        if(p->states[i].animation_id < -1 || p->states[i].animation_id>1023)return false;
        for(unsigned phase=0;phase<NativeScriptItemPhase_Count;++phase)
            if(!function_valid(p,p->states[i].functions[phase],&phases[phase],false))return false;
    }
    const HookContract signal={"signal",NativeScriptEvent_Article,NativeScriptEvent_ArticleSignal,NativeScriptType_Void,1,{NativeScriptType_F32}};
    for(unsigned i=0;i<p->signal_count;++i)
        if((i && p->signals[i-1].signal_id>=p->signals[i].signal_id) ||
           p->signals[i].function==UINT32_MAX || !function_valid(p,p->signals[i].function,&signal,false))return false;
    return true;
}
static NativeScriptItemActor* find(Item_GObj* object)
{
    NativeMatchPoolReference reference;
    if(!nativeMatchPool_Reference(&nativeGObjPool,object,&reference) || reference.slot>=NATIVE_GOBJ_CAPACITY)return NULL;
    unsigned slot=nativeScriptItemActorByObject[reference.slot];if(slot>=nativeScriptItemInitializedCount)return NULL;
    NativeScriptItemActor* a=&nativeScriptItemActors[slot];
    return a->active && a->object==object && a->object_reference.generation==reference.generation &&
        a->object_reference.lifetime==reference.lifetime?a:NULL;
}
bool nativeScriptItem_NativeIdentity(const Item_GObj* object,const NativeScriptItemProgram* p,uint32_t generation)
{
    if(!nativeCallbackSource_Alive((Item_GObj*)object,generation,NativeCallbackSource_Item))return false;
    const Item* item=object->user_data;
    return item->entity==object && item->kind==p->kind && item->native_article_author_kind==p->author_kind &&
        item->native_script_owner==p->owner &&
        (!p->article || item->xC4_article_data==p->article) &&
        (!p->recipe ||
            (item->xDD4_itemVar.native_holdable.owner_kind==(uint32_t)p->author_kind &&
             item->xDD4_itemVar.native_holdable.definition_index==p->recipe_index));
}
static bool alive(const NativeScriptItemActor* a)
{
    if(!a || !a->active || !a->storage.live || nativeMatchPool_Resolve(a->object_reference)!=a->object ||
       !nativeScriptItem_NativeIdentity(a->object,a->program,a->generation))return false;
    return a->program_identity==a->program->identity &&
        nativeScriptItem_Program(a->program->author_kind,a->program->owner)==a->program;
}
bool nativeScriptItem_IsAttached(Item_GObj* object){return find(object)!=NULL;}
NativeScriptVMStatus nativeScriptItem_Status(Item_GObj* object)
{
    NativeScriptItemActor* actor=find(object);
    if(!alive(actor))return NativeScriptVM_SourceRetired;
    return actor->faulted?NativeScriptVM_Error:NativeScriptVM_Ok;
}
bool nativeScriptItem_View(Item_GObj* object,uint32_t generation,NativeScriptReceiverView* result)
{
    NativeScriptItemActor* actor=find(object);
    if(!result || !alive(actor) || actor->generation!=generation ||
        actor->execution.declarations!=&actor->program->bank->declarations)return false;
    *result=(NativeScriptReceiverView){actor->execution.declarations,&actor->storage};return true;
}
const NativeScriptItemProgram* nativeScriptItem_Spec(Item_GObj* object)
{
    NativeScriptItemActor* a=find(object);
    return a && nativeMatchPool_Resolve(a->object_reference)==object && a->program_identity==a->program->identity &&
        nativeScriptItem_NativeIdentity(object,a->program,a->generation) &&
        nativeScriptItem_Program(a->program->author_kind,a->program->owner)==a->program?a->program:NULL;
}

static bool capture_relation(Fighter_GObj* object,NativeScriptItemRelation* result)
{
    NativeScriptItemRelation relation={0};
    if(object){
        if(!nativeMatchPool_Reference(&nativeGObjPool,object,&relation.object) ||
           relation.object.lifetime!=nativeScriptItemSceneEpoch ||
           !nativeCallbackSource_Snapshot(object,NativeCallbackSource_Fighter,&relation.generation))return false;
        const Fighter* fighter=object->user_data;
        if(!fighter || fighter->gobj!=object || (uint32_t)fighter->kind>=Ft_Kind_RuntimeCapacity || fighter->kind==Ft_Kind_None)return false;
        relation.data=fighter;relation.kind=(uint32_t)fighter->kind;
    }
    *result=relation;return true;
}
static Fighter_GObj* resolve_relation(const NativeScriptItemRelation* relation)
{
    if(!relation->object.pool || relation->object.pool!=&nativeGObjPool ||
       relation->object.lifetime!=nativeScriptItemSceneEpoch)return NULL;
    Fighter_GObj* object=nativeMatchPool_Resolve(relation->object);
    if(!object || object->user_data!=relation->data ||
       !nativeCallbackSource_Alive(object,relation->generation,NativeCallbackSource_Fighter))return NULL;
    const Fighter* fighter=object->user_data;
    return fighter->gobj==object && (uint32_t)fighter->kind==relation->kind?object:NULL;
}
Fighter_GObj* nativeScriptItem_Creator(Item_GObj* object)
{
    NativeScriptItemActor* actor=find(object);
    return alive(actor)?resolve_relation(&actor->creator):NULL;
}
Fighter_GObj* nativeScriptItem_AttachmentOwner(Item_GObj* object)
{
    NativeScriptItemActor* actor=find(object);
    return alive(actor)?resolve_relation(&actor->attachment_owner):NULL;
}
bool nativeScriptItem_SetAttachmentOwner(Item_GObj* object,Fighter_GObj* owner)
{
    NativeScriptItemActor* actor=find(object);NativeScriptItemRelation relation;
    if(!alive(actor) || actor->faulted || !capture_relation(owner,&relation) || !alive(actor))return false;
    actor->attachment_owner=relation;return true;
}
bool nativeScriptItem_ClearAttachmentOwner(Item_GObj* object)
{
    NativeScriptItemActor* actor=find(object);
    if(!alive(actor) || actor->faulted)return false;
    actor->attachment_owner=(NativeScriptItemRelation){0};return true;
}
bool nativeScriptItem_GetCreationValue(Item_GObj* object,float* output)
{
    NativeScriptItemActor* actor=find(object);
    if(!output || !alive(actor) || actor->faulted)return false;
    *output=actor->creation_value;return true;
}
bool nativeScriptItem_SetCreationValue(Item_GObj* object,float value)
{
    NativeScriptItemActor* actor=find(object);
    if(!isfinite(value) || !alive(actor) || actor->faulted)return false;
    actor->creation_value=value;return true;
}
bool nativeScriptItem_HasActors(FighterKind author)
{
    for(unsigned i=0;i<nativeScriptItemInitializedCount;++i){const NativeScriptItemActor* a=&nativeScriptItemActors[i];
        if((a->active||a->running) && a->program && a->program->author_kind==author)return true;}
    return false;
}
bool nativeScriptItem_ReferencesProgram(const NativeScriptCompileBank* bank)
{
    for(unsigned i=0;i<nativeScriptItemInitializedCount;++i){const NativeScriptItemActor* a=&nativeScriptItemActors[i];
        if((a->active||a->running) && a->program && a->program->bank==bank)return true;}
    return false;
}
bool nativeScriptItem_Attach(Item_GObj* object,const NativeScriptItemProgram* p)
{
    uint32_t generation;NativeMatchPoolReference reference;
    if(!nativeScriptItemSceneEpoch || nativeScriptItemSceneEpoch!=nativeGObjPool.lifetime || find(object) ||
       !nativeScriptItem_ProgramValid(p) || !nativeCallbackSource_Snapshot(object,NativeCallbackSource_Item,&generation) ||
       !nativeMatchPool_Reference(&nativeGObjPool,object,&reference) || reference.slot>=NATIVE_GOBJ_CAPACITY)return false;
    if(!nativeScriptItem_NativeIdentity(object,p,generation))return false;
    NativeScriptItemActor* a=NULL;
    for(unsigned i=0;i<nativeScriptItemInitializedCount;++i)
        if(!nativeScriptItemActors[i].active&&!nativeScriptItemActors[i].running){a=&nativeScriptItemActors[i];break;}
    if(!a){
        if(nativeScriptItemInitializedCount==NATIVE_SCRIPT_ITEM_ACTORS)return false;
        a=&nativeScriptItemActors[nativeScriptItemInitializedCount++];
    }
    memset(a,0,sizeof(*a));NativeScriptDiagnostic d={0};const NativeScriptCompileBank* b=p->bank;
    Fighter_GObj* creator=nativeItem_Creator(object);
    if((((Item*)object->user_data)->native_creator && !creator) || !capture_relation(creator,&a->creator))return false;
    a->attachment_owner=a->creator;
    if(!nativeScriptStorage_Create(&b->storage_layout,&b->storage_parameters,p->owner,generation,generation,
        a->bytes,sizeof(a->bytes),&a->storage,&d))return false;
    a->execution=(NativeScriptExecution){.declarations=&b->declarations,.code=&b->code,.storage=&a->storage,
        .vm=nativeScriptRuntime_VM(),.payloads=nativeScriptRuntime_Payloads(),
        .functions=b->execution_functions,.function_count=b->declarations.function_count,.receiver_lookup=nativeScriptActors_Lookup,.receiver_helper=nativeScriptActors_Helper,.fighter_kind=p->author_kind};
    if(!nativeScriptExecution_Bind(&a->execution,&d)){nativeScriptStorage_Destroy(&a->storage);return false;}
    a->object=object;a->object_reference=reference;a->program=p;a->program_identity=p->identity;a->generation=generation;
    a->selected_state=UINT32_MAX;for(unsigned i=0;i<NativeScriptItemPhase_Count;++i)a->phases[i]=UINT32_MAX;
    for(unsigned i=0;i<NativeScriptItemCallback_Count;++i)a->callbacks[i]=UINT32_MAX;
    a->active=true;nativeScriptItemActorByObject[reference.slot]=(uint16_t)(a-nativeScriptItemActors);return true;
}
void nativeScriptItem_Detach(Item_GObj* object)
{
    NativeScriptItemActor* a=find(object);if(!a)return;
    nativeScriptStorage_Destroy(&a->storage);a->active=false;
    nativeScriptItemActorByObject[a->object_reference.slot]=UINT16_MAX;
    // Running frames retain this slot and program pin until their stack unwinds.
}
bool nativeScriptItem_StateAnimation(Item_GObj* object,uint32_t state,int32_t* animation)
{
    NativeScriptItemActor* a=find(object);
    if(!animation || !alive(a) || a->faulted || state>=a->program->state_count)return false;
    *animation=a->program->states[state].animation_id;return true;
}
bool nativeScriptItem_SelectState(Item_GObj* object,uint32_t state)
{
    NativeScriptItemActor* a=find(object);
    if(!alive(a) || a->faulted || state>=a->program->state_count || a->revision==UINT64_MAX)return false;
    a->selected_state=state;++a->revision;memcpy(a->phases,a->program->states[state].functions,sizeof(a->phases));
    if(((Item*)object->user_data)->native_capture_inert)
        for(unsigned phase=0;phase<NativeScriptItemPhase_Count;++phase)a->phases[phase]=UINT32_MAX;
    return true;
}
uint64_t nativeScriptItem_Revision(Item_GObj* object){NativeScriptItemActor* a=find(object);return a?a->revision:0;}
void nativeScriptItem_BeginTick(Item_GObj* object)
{
    NativeScriptItemActor* a=find(object);if(alive(a))++a->tick;
}
bool nativeScriptItem_HookBound(Item_GObj* object,uint32_t hook)
{
    NativeScriptItemActor* a=find(object);if(!a || hook>=NativeScriptItemHook_Count)return false;
    if(hook>=NativeScriptItemHook_Accessory && hook<=NativeScriptItemHook_GrabbedForVictim){
        unsigned slot=hook-NativeScriptItemHook_Accessory;
        if(!alive(a))return false;Item* item=object->user_data;bool native=false;
        switch(slot){
        case NativeScriptItemCallback_Accessory:native=item->on_accessory!=NULL;break;
        case NativeScriptItemCallback_EnteredHitlag:native=item->entered_hitlag!=NULL;break;
        case NativeScriptItemCallback_ExitedHitlag:native=item->exited_hitlag!=NULL;break;
        case NativeScriptItemCallback_Touched:native=item->touched!=NULL;break;
        case NativeScriptItemCallback_JumpedOn:native=item->jumped_on!=NULL;break;
        case NativeScriptItemCallback_GrabDealt:native=item->grab_dealt!=NULL;break;
        case NativeScriptItemCallback_GrabbedForVictim:native=item->grabbed_for_victim!=NULL;break;
        }
        if(native)a->callbacks[slot]=UINT32_MAX;return a->callbacks[slot]!=UINT32_MAX;
    }
    return a->program->hooks[hook]!=UINT32_MAX;
}
bool nativeScriptItem_CallbackValid(const NativeScriptItemProgram* p,uint32_t slot,uint32_t function)
{
    if(!p || slot>=NativeScriptItemCallback_Count)return false;
    unsigned hook=NativeScriptItemHook_Accessory+slot;
    if(!function_valid(p,function,&hooks[hook],true))return false;
    return function==UINT32_MAX || !p->bank->declarations.functions[function].callback || p->hooks[hook]==function;
}
uint32_t nativeScriptItem_CallbackFunction(Item_GObj* object,uint32_t slot)
{
    NativeScriptItemActor* a=find(object);return a&&slot<NativeScriptItemCallback_Count?a->program->hooks[NativeScriptItemHook_Accessory+slot]:UINT32_MAX;
}
bool nativeScriptItem_BindCallback(Item_GObj* object,uint32_t slot,uint32_t function)
{
    NativeScriptItemActor* a=find(object);
    if(!alive(a) || a->faulted || !nativeScriptItem_CallbackValid(a->program,slot,function))return false;
    a->callbacks[slot]=function;Item* item=object->user_data;
    switch(slot){
    case NativeScriptItemCallback_Accessory:item->on_accessory=NULL;break;
    case NativeScriptItemCallback_EnteredHitlag:item->entered_hitlag=NULL;break;
    case NativeScriptItemCallback_ExitedHitlag:item->exited_hitlag=NULL;break;
    case NativeScriptItemCallback_Touched:item->touched=NULL;break;
    case NativeScriptItemCallback_JumpedOn:item->jumped_on=NULL;break;
    case NativeScriptItemCallback_GrabDealt:item->grab_dealt=NULL;break;
    case NativeScriptItemCallback_GrabbedForVictim:item->grabbed_for_victim=NULL;break;
    }
    return true;
}
void nativeScriptItem_ResetStateCallbacks(Item_GObj* object)
{
    NativeScriptItemActor* a=find(object);if(!a)return;
    for(unsigned slot=0;slot<=NativeScriptItemCallback_JumpedOn;++slot)a->callbacks[slot]=UINT32_MAX;
}
bool nativeScriptItem_PhaseBound(Item_GObj* object,uint32_t phase)
{
    NativeScriptItemActor* a=find(object);if(!alive(a) || phase>=NativeScriptItemPhase_Count)return false;
    Item* item=object->user_data;
    if((phase==NativeScriptItemPhase_Animation&&item->animated) ||
       (phase==NativeScriptItemPhase_Physics&&item->physics_updated) ||
       (phase==NativeScriptItemPhase_Collision&&item->collided))a->phases[phase]=UINT32_MAX;
    return a->phases[phase]!=UINT32_MAX;
}
static NativeScriptVMStatus run(NativeScriptItemActor* a,uint32_t function,const HookContract* contract,
    const NativeScriptValue* args,uint32_t count,NativeScriptValue* result)
{
    if(!result || !contract || count!=contract->count || (count&&!args))return NativeScriptVM_Error;
    for(unsigned i=0;i<count;++i)if(args[i].type!=contract->args[i])return NativeScriptVM_Error;
    if(!alive(a))return NativeScriptVM_SourceRetired;
    if(a->faulted)return NativeScriptVM_Error;
    if(function==UINT32_MAX){NativeScriptValue neutral={.type=contract->result};
        if(contract->result==NativeScriptType_I32)neutral.as.i64=-1;*result=neutral;return NativeScriptVM_Ok;}
    ++a->running;NativeScriptValue output;
    NativeScriptVMStatus status=nativeScriptExecution_Run(&a->execution,a->object,NativeCallbackSource_Item,
        a->generation,a->tick,a->object_reference.lifetime,function,args,count, NULL,&output);
    --a->running;
    if(!alive(a))return NativeScriptVM_SourceRetired;
    if(status==NativeScriptVM_Ok && output.type!=contract->result)status=NativeScriptVM_Error;
    if(status==NativeScriptVM_Error){a->faulted=true;const NativeScriptVM* vm=a->execution.vm;
        OSReport("[script] Item %u function %u frame %u source %u:%u: %s\n",a->generation,function,a->tick,
            vm->diagnostic.source.line,vm->diagnostic.source.column,vm->diagnostic.message);}
    if(status==NativeScriptVM_Ok){++a->completed_callbacks;*result=output;}
    return status;
}
NativeScriptVMStatus nativeScriptItem_RunHook(Item_GObj* object,uint32_t hook,
    const NativeScriptValue* args,uint32_t count,NativeScriptValue* result)
{
    NativeScriptItemActor* a=find(object);if(!a || hook>=NativeScriptItemHook_Count)return NativeScriptVM_Error;
    uint32_t function=hook>=NativeScriptItemHook_Accessory && hook<=NativeScriptItemHook_GrabbedForVictim?
        a->callbacks[hook-NativeScriptItemHook_Accessory]:a->program->hooks[hook];
    NativeScriptVMStatus status=run(a,function,&hooks[hook],args,count,result);
    // Destroyed may perform nested native removal or fail. Never re-enable its
    // view; retain attachment/pin until actual userdata removal invokes Detach.
    if(hook==NativeScriptItemHook_Destroyed)nativeScriptStorage_Destroy(&a->storage);
    return status;
}
NativeScriptVMStatus nativeScriptItem_RunPhase(Item_GObj* object,uint32_t phase,NativeScriptValue* result)
{
    NativeScriptItemActor* a=find(object);if(!a || phase>=NativeScriptItemPhase_Count)return NativeScriptVM_Error;
    return run(a,a->phases[phase],&phases[phase],NULL,0,result);
}
NativeScriptVMStatus nativeScriptItem_RunSignal(Item_GObj* object,uint32_t signal_id,float value,NativeScriptValue* result)
{
    NativeScriptItemActor* a=find(object);if(!a)return NativeScriptVM_Error;
    unsigned low=0,high=a->program->signal_count;
    while(low<high){unsigned mid=low+(high-low)/2;if(a->program->signals[mid].signal_id<signal_id)low=mid+1;else high=mid;}
    if(low==a->program->signal_count || a->program->signals[low].signal_id!=signal_id)return NativeScriptVM_Error;
    const HookContract signal={"signal",NativeScriptEvent_Article,NativeScriptEvent_ArticleSignal,NativeScriptType_Void,1,{NativeScriptType_F32}};
    NativeScriptValue arg={.type=NativeScriptType_F32,.as.f32=value};
    return run(a,a->program->signals[low].function,&signal,&arg,1,result);
}
bool nativeScriptItem_Boundary(void)
{
    if(!nativeScriptRuntime_Idle() || HSD_GObj_CurrentInvokedProcGObj || HSD_GObj_CurrentInvokedProc ||
       HSD_GObj_804D7818 || HSD_GObj_DelayedProcInfo.flags)return false;
    for(unsigned i=0;i<nativeScriptItemInitializedCount;++i)if(nativeScriptItemActors[i].running)return false;
    return true;
}
bool nativeScriptItem_CanBeginScene(uint64_t epoch)
{
    if(!epoch || nativeScriptItemSceneEpoch || !nativeScriptItem_Boundary())return false;
    for(unsigned i=0;i<nativeScriptItemInitializedCount;++i)if(nativeScriptItemActors[i].active)return false;
    return true;
}
bool nativeScriptItem_BeginScene(uint64_t epoch)
{
    if(!nativeScriptItem_CanBeginScene(epoch) || nativeGObjPool.lifetime!=epoch)return false;
    memset(nativeScriptItemActorByObject,0xFF,sizeof(nativeScriptItemActorByObject));
    nativeScriptItemInitializedCount=0;nativeScriptItemSceneEpoch=epoch;return true;
}
bool nativeScriptItem_RetireScene(void)
{
    if(!nativeScriptItem_Boundary())return false;
    for(unsigned i=0;i<nativeScriptItemInitializedCount;++i){nativeScriptStorage_Destroy(&nativeScriptItemActors[i].storage);nativeScriptItemActors[i].active=false;}
    memset(nativeScriptItemActorByObject,0xFF,sizeof(nativeScriptItemActorByObject));
    nativeScriptItemSceneEpoch=0;return true;
}
