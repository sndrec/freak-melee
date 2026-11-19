#include "native_script_effect_internal.h"
#include "native_model_effect.h"
#include "efsync.h"
#include "eflib.h"
#include "efasync.h"
#include <melee/ft/native_script_actor_access.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/fighter.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/ftdat.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/objalloc.h>
#include <string.h>
#include <stdio.h>

NativeScriptEffectActor nativeScriptEffectActors[NATIVE_SCRIPT_EFFECT_ACTORS];
uint32_t nativeScriptEffectInitializedCount;
uint16_t nativeScriptEffectByObject[NATIVE_GOBJ_CAPACITY];
uint64_t nativeScriptEffectEpoch;
static EF_Effect* physical(NativeMatchPoolReference reference,uint32_t generation)
{
    HSD_GObj* object=nativeMatchPool_Resolve(reference);
    if(!object||object->classifier!=HSD_GOBJ_CLASS_EFFECT||object->user_data_kind!=8||!object->user_data||
       object->user_data_remove_func!=efLib_remove_user_data)return NULL;
    EF_Effect* effect=object->user_data;
    return effect->gobj==object&&effect->native_generation==generation&&!effect->native_cancelled?effect:NULL;
}
static NativeScriptEffectActor* find(HSD_GObj* object)
{
    NativeMatchPoolReference reference;
    if(!nativeMatchPool_Reference(&nativeGObjPool,object,&reference)||reference.slot>=NATIVE_GOBJ_CAPACITY)return NULL;
    unsigned slot=nativeScriptEffectByObject[reference.slot];if(slot>=nativeScriptEffectInitializedCount)return NULL;
    NativeScriptEffectActor* actor=&nativeScriptEffectActors[slot];if(!actor->active)return NULL;
    for(unsigned i=0;i<actor->member_count;++i)if(actor->member_live[i]&&actor->members[i].slot==reference.slot&&
       actor->members[i].generation==reference.generation&&actor->members[i].lifetime==reference.lifetime)return actor;
    return NULL;
}
bool nativeScriptEffect_IsAttached(HSD_GObj* object){return find(object)!=NULL;}
const NativeScriptEffectProgram* nativeScriptEffect_Spec(HSD_GObj* object){NativeScriptEffectActor* a=find(object);return a?a->program:NULL;}
static bool origin_current(const NativeScriptEffectOrigin* identity,HSD_JObj* origin,Fighter_GObj* creator)
{
    if(creator->hsd_obj!=identity->creator_root)return false;
    if(!identity->on_part)return origin==identity->creator_root;
    Fighter* fighter=creator->user_data;
    const NativeSourcePackage* package=identity->package;
    if(!package||nativeFighterRuntime_SourcePackage(fighter->kind)!=package||!package->assets.ready||
       fighter->ft_data!=&package->assets.character.data||fighter->parts!=identity->parts||
       !ftPartsTable||ftPartsTable[fighter->kind]!=&package->assets.parts||
       package->assets.parts.parts_num!=package->assets.facts.joints+package->assets.facts.omission_count||
       identity->part>=package->assets.parts.parts_num||
       fighter->parts[identity->part].joint!=origin)return false;
    return HSD_CLASS_METHOD(origin)==identity->info&&
        identity->info&&identity->info->head.obj_size>=(int)sizeof(HSD_JObj);
}
bool nativeScriptEffect_Peers(const NativeScriptEffectActor* actor)
{
    if(!actor||!actor->program||actor->program_identity!=actor->program->identity||
       nativeScriptEffect_Program(actor->program->author_kind,actor->program->owner)!=actor->program||
       actor->creator.lifetime!=nativeScriptEffectEpoch||nativeGObjPool.lifetime!=nativeScriptEffectEpoch||
       !actor->member_count||actor->member_count!=actor->program->member_count||actor->remaining!=actor->member_count)return false;
    Fighter_GObj* creator=nativeMatchPool_Resolve(actor->creator);NativeScriptReceiverView view;
    if(!creator||!nativeCallbackSource_Alive(creator,actor->creator_generation,NativeCallbackSource_Fighter)||
       ((Fighter*)creator->user_data)->kind!=actor->program->author_kind||!origin_current(&actor->origin_identity,actor->origin,creator)||
       !nativeScriptFighter_View(creator,actor->creator_generation,&view)||view.program!=actor->creator_view.program||
       view.storage!=actor->creator_view.storage||!view.storage||!view.storage->live||
       view.storage->layout->identity!=actor->creator_layout||!view.storage->parameters||
       view.storage->parameters->identity!=actor->creator_parameters)return false;
    for(unsigned i=0;i<actor->member_count;++i) {
        EF_Effect* effect=physical(actor->members[i],actor->generations[i]);
        if(!actor->member_live[i]||!effect||effect->parent_gobj!=creator||effect->native_policy||effect->update||
           effect->native_origin_joint!=actor->origin||effect->native_creator_generation!=actor->creator_generation||
           effect->native_member_count!=actor->member_count||effect->native_member_index!=i||
           (actor->program->family==EF_MODEL_JOINT&&effect->attach_jobj!=actor->origin))return false;
        for(unsigned j=0;j<actor->member_count;++j)if(effect->native_members[j].object!=nativeMatchPool_Resolve(actor->members[j])||
            effect->native_members[j].generation!=actor->generations[j])return false;
    }
    return true;
}
NativeScriptVMStatus nativeScriptEffect_Status(HSD_GObj* object)
{NativeScriptEffectActor* a=find(object);if(!a||!a->storage.live||!nativeScriptEffect_Peers(a))return NativeScriptVM_SourceRetired;return a->faulted?NativeScriptVM_Error:NativeScriptVM_Ok;}
EF_Effect* nativeScriptEffect_Resolve(HSD_GObj* object,uint32_t generation)
{
    NativeScriptEffectActor* a=find(object);if(!a||!a->storage.live||!nativeScriptEffect_Peers(a))return NULL;
    for(unsigned member=0;member<a->member_count;++member)if(a->generations[member]==generation&&nativeMatchPool_Resolve(a->members[member])==object)
        return physical(a->members[member],generation);
    return NULL;
}
bool nativeScriptEffect_Snapshot(HSD_GObj* object,uint32_t* generation)
{
    NativeScriptEffectActor* a=find(object);if(!generation||!a||!a->storage.live||!nativeScriptEffect_Peers(a))return false;
    for(unsigned member=0;member<a->member_count;++member)if(nativeMatchPool_Resolve(a->members[member])==object){*generation=a->generations[member];return true;}
    return false;
}
bool nativeScriptEffect_View(HSD_GObj* object,uint32_t generation,NativeScriptReceiverView* result)
{
    NativeScriptEffectActor* a=find(object);if(!result||!a||!a->storage.live||!nativeScriptEffect_Peers(a)||
       nativeMatchPool_Resolve(a->members[0])!=object||a->generations[0]!=generation)return false;
    *result=(NativeScriptReceiverView){a->execution.declarations,&a->storage};return true;
}
Fighter_GObj* nativeScriptEffect_Creator(HSD_GObj* object,uint32_t generation)
{
    NativeScriptEffectActor* a=find(object);if(!a||!a->storage.live||!nativeScriptEffect_Peers(a))return NULL;
    for(unsigned i=0;i<a->member_count;++i)if(nativeMatchPool_Resolve(a->members[i])==object&&a->generations[i]==generation)return nativeMatchPool_Resolve(a->creator);
    return NULL;
}
HSD_JObj* nativeScriptEffect_Joint(HSD_GObj* object,uint32_t generation,unsigned member)
{
    NativeScriptEffectActor* a=find(object);if(!nativeScriptEffect_Creator(object,generation)||!a||member>=a->member_count)return NULL;
    return ((HSD_GObj*)nativeMatchPool_Resolve(a->members[member]))->hsd_obj;
}
bool nativeScriptEffect_Bind(NativeScriptEffectActor* a,const NativeScriptEffectProgram* p,uint32_t generation,NativeScriptDiagnostic* d)
{
    const NativeScriptCompileBank* b=p->bank;
    a->storage=(NativeScriptStorageInstance){.layout=&b->storage_layout,.parameters=&b->storage_parameters,.state=a->bytes,
        .capacity=sizeof(a->bytes),.owner=p->owner,.identity=generation,.generation=generation,.live=true};
    a->execution=(NativeScriptExecution){.declarations=&b->declarations,.code=&b->code,.storage=&a->storage,
        .vm=nativeScriptRuntime_VM(),.payloads=nativeScriptRuntime_Payloads(),.functions=b->execution_functions,
        .function_count=b->declarations.function_count,.receiver_lookup=nativeScriptActors_Lookup,
        .receiver_helper=nativeScriptActors_Helper,.fighter_kind=p->author_kind};
    return nativeScriptExecution_Bind(&a->execution,d);
}
static bool current_caller(NativeScriptRuntimeContext* context,NativeMatchPoolReference reference,NativeScriptReceiverView view)
{
    NativeScriptReceiverView current;
    return reference.lifetime==context->scene_epoch&&nativeMatchPool_Resolve(reference)==context->source&&
        nativeScriptBindings_SourceAlive(context)&&nativeScriptFighter_View(context->source,context->generation,&current)&&
        current.program==view.program&&current.storage==view.storage&&current.storage&&current.storage->live;
}
NativeScriptVMStatus nativeScriptEffect_Spawn(NativeScriptRuntimeContext* context,uint32_t owner,bool on_part,int32_t raw_part,bool deferred,NativeScriptDiagnostic* d)
{
    NativeMatchPoolReference creator;NativeScriptReceiverView view;
    if(!context||context->source_class!=NativeCallbackSource_Fighter||context->scene_epoch!=nativeScriptEffectEpoch||
       !nativeMatchPool_Reference(&nativeGObjPool,context->source,&creator)||
       !nativeScriptFighter_View(context->source,context->generation,&view)||!current_caller(context,creator,view))return NativeScriptVM_SourceRetired;
    Fighter_GObj* object=context->source;Fighter* fp=object->user_data;HSD_JObj* origin=object->hsd_obj;
    const NativeScriptEffectProgram* p=nativeScriptEffect_Program(fp->kind,owner);
    if(!p||!origin||!view.storage->parameters||&p->bank->declarations!=view.program)return NativeScriptVM_Error;
    NativeScriptEffectOrigin selected={.creator_root=origin,.on_part=on_part};
    if(on_part) {
        const NativeSourcePackage* package=nativeFighterRuntime_SourcePackage(fp->kind);
        if((p->family!=EF_MODEL_JOINT&&p->family!=EF_MODEL_POSITION)||!package||!package->assets.ready||!fp->parts||
           fp->ft_data!=&package->assets.character.data||!ftPartsTable||ftPartsTable[fp->kind]!=&package->assets.parts||
           package->assets.parts.parts_num!=package->assets.facts.joints+package->assets.facts.omission_count||
           !package->assets.parts.parts_num||package->assets.parts.parts_num>MAX_FT_PARTS) {
            if(d)snprintf(d->message,sizeof(d->message),"Named part effect requires its exact owned Fighter rig and joint/position recipe");
            return NativeScriptVM_Error;
        }
        // Match the engine part-spawn contract: a valid rig with an absent
        // optional slot is a healthy no-op, without allocating effect storage.
        int part=nativeFighterRig_Resolve(&package->assets.rig,raw_part);
        if(part<0||(uint32_t)part>=package->assets.parts.parts_num||!fp->parts[part].joint)return NativeScriptVM_Ok;
        origin=fp->parts[part].joint;
        selected.part=(uint32_t)part;
        selected.parts=fp->parts;selected.package=package;
        selected.info=HSD_CLASS_METHOD(origin);
        if(object->obj_kind!=HSD_GObj_JObjKind||!selected.info||
           selected.info->head.obj_size<(int)sizeof(HSD_JObj)||
           !hsdObjIsDescendantOf((HSD_Obj*)origin,HSD_CLASS_INFO(&hsdJObj))||
           !current_caller(context,creator,view)||!origin_current(&selected,origin,object))return NativeScriptVM_Error;
    }
    if(deferred) {
        if(!on_part||p->family!=EF_MODEL_POSITION)return NativeScriptVM_Error;
        EF_QueuedEffect* queued=HSD_ObjAlloc(&efAsync_AllocData);
        if(!queued){if(d)snprintf(d->message,sizeof(d->message),"Deferred effect queue capacity exhausted");return NativeScriptVM_Error;}
        *queued=(EF_QueuedEffect){.spawn_kind=EF_SPAWN_POS,.gfx_id=p->gfx_id,.jobj=origin,
            .native_creator_generation=context->generation,.native_sample_part=(s16)selected.part,
            .native_script_owner=owner,.native_script_epoch=context->scene_epoch,
            .native_script_program_identity=p->identity,
            .native_script_layout_identity=view.storage->layout->identity,
            .native_script_parameter_identity=view.storage->parameters->identity,
            .native_script_root=object->hsd_obj};
        if(HSD_GObj_CurrentInvokedProc&&HSD_GObj_CurrentInvokedProc->s_link<9U) {
            EF_QueuedEffect* head=(EF_QueuedEffect*)&fp->x60C;
            queued->next=head->next;head->next=queued;
        } else {
            NativeScriptVMStatus status=nativeScriptEffect_ProcessDeferred(object,queued);
            HSD_ObjFree(&efAsync_AllocData,queued);
            return status;
        }
        return NativeScriptVM_Ok;
    }
    NativeScriptEffectActor* a=NULL;
    for(unsigned i=0;i<nativeScriptEffectInitializedCount;++i)if(!nativeScriptEffectActors[i].active&&!nativeScriptEffectActors[i].running){a=&nativeScriptEffectActors[i];break;}
    if(!a && nativeScriptEffectInitializedCount<NATIVE_SCRIPT_EFFECT_ACTORS){
        a=&nativeScriptEffectActors[nativeScriptEffectInitializedCount++];memset(a,0,sizeof(*a));
    }
    if(!a){if(d)snprintf(d->message,sizeof(d->message),"Interpreted effect capacity exhausted");return NativeScriptVM_Error;}
    if (!p->model_path.length) {
        const EF_ModelRecipe* recipe = efSync_ModelRecipe(p->gfx_id);
        if (!recipe || recipe->family != p->family || recipe->model_count != p->member_count)
            return NativeScriptVM_Error;
    } else if (!p->model.model_desc.joint) {
        return NativeScriptVM_Error;
    }
    Vec3 position=fp->cur_pos;
    const bool owned_follow = p->model_path.length && p->family == EF_MODEL_JOINT;
    if ((p->family == EF_MODEL_POSITION && on_part) || owned_follow) {
        lb_8000B1CC(origin,NULL,&position);
        if(!current_caller(context,creator,view)||!origin_current(&selected,origin,object))
            return NativeScriptVM_SourceRetired;
    }
    EF_SpawnResult spawned = {0};
    if (p->model_path.length) {
        EF_Effect* effect = efLib_Create(p->gfx_id, object, &p->model);
        if (effect) {
            HSD_JObj* root = effect->gobj->hsd_obj;
            HSD_JObjSetTranslate(root, &position);
            if (owned_follow) {
                lb_8000C1C0(root, origin);
                effect->attach_jobj = origin;
            }
            spawned.kind = EF_SPAWN_MODEL;
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){effect->gobj, effect->native_generation};
            spawned.model_generation = effect->native_generation;
            spawned.object.model = effect;
        }
    } else {
        spawned = p->family == EF_MODEL_POSITION
            ? efSync_Spawn(p->gfx_id, object, &position)
            : efSync_Spawn(p->gfx_id, object, origin);
    }
    bool caller=current_caller(context,creator,view)&&origin_current(&selected,origin,object);
    if(spawned.kind!=EF_SPAWN_MODEL||spawned.member_count!=p->member_count) {
        // A partial native model group is not a viable source instance. Retire
        // only the returned generations, never unrelated recycled objects.
        for(unsigned i=0;i<spawned.member_count&&i<EF_MODEL_GROUP_MAX;++i)
            nativeModelEffect_CancelGeneration(spawned.members[i].object,spawned.members[i].generation);
        return caller?NativeScriptVM_Error:NativeScriptVM_SourceRetired;
    }
    memset(a,0,sizeof(*a));a->creator=creator;a->creator_generation=context->generation;a->creator_view=view;
    a->creator_layout=view.storage->layout->identity;a->creator_parameters=view.storage->parameters->identity;
    a->origin=origin;a->program=p;a->program_identity=p->identity;a->member_count=a->remaining=p->member_count;
    a->origin_identity=selected;
    bool valid=true;
    for(unsigned i=0;i<spawned.member_count;++i) {
        a->generations[i]=spawned.members[i].generation;a->member_live[i]=true;
        if(!nativeMatchPool_Reference(&nativeGObjPool,spawned.members[i].object,&a->members[i])){valid=false;break;}
        EF_Effect* member=physical(a->members[i],a->generations[i]);
        if(!member||member->parent_gobj!=object||(p->family==EF_MODEL_JOINT&&member->attach_jobj!=origin)){valid=false;break;}
        for(unsigned j=0;j<i;++j)if(a->members[j].slot==a->members[i].slot)valid=false;
    }
    if(valid)valid=nativeScriptStorage_Create(&p->bank->storage_layout,&p->bank->storage_parameters,p->owner,a->generations[0],a->generations[0],
        a->bytes,sizeof(a->bytes),&a->storage,d)&&nativeScriptEffect_Bind(a,p,a->generations[0],d);
    if(!valid) {
        nativeScriptStorage_Destroy(&a->storage);
        for(unsigned i=0;i<spawned.member_count;++i)nativeModelEffect_CancelGeneration(spawned.members[i].object,spawned.members[i].generation);
        return caller?NativeScriptVM_Error:NativeScriptVM_SourceRetired;
    }
    a->active=true;
    for(unsigned i=0;i<a->member_count;++i) {
        EF_Effect* member=physical(a->members[i],a->generations[i]);member->update=NULL;member->native_author=NULL;member->native_policy=NULL;
        member->native_creator_generation=a->creator_generation;member->native_attachment_part=on_part?raw_part:-1;member->native_origin_joint=origin;
        member->native_member_count=(u8)a->member_count;member->native_member_index=(u8)i;memcpy(member->native_members,spawned.members,sizeof(member->native_members));
        nativeScriptEffectByObject[a->members[i].slot]=(uint16_t)(a-nativeScriptEffectActors);
    }
    // Preserve completed native creation when the caller retires. The owner is
    // bound/pinned for coherent cleanup; its next native tick will cancel it.
    return caller?NativeScriptVM_Ok:NativeScriptVM_SourceRetired;
}
NativeScriptVMStatus nativeScriptEffect_Run(HSD_GObj* object)
{
    NativeScriptEffectActor* a=find(object);NativeScriptVMStatus status=nativeScriptEffect_Status(object);
    if(status!=NativeScriptVM_Ok||!a)return status;
    if(nativeMatchPool_Resolve(a->members[0])!=object)return NativeScriptVM_Ok;
    ++a->tick;if(a->program->update==UINT32_MAX)return NativeScriptVM_Ok;
    ++a->running;NativeScriptValue result;
    status=nativeScriptExecution_Run(&a->execution,object,NativeCallbackSource_ModelEffect,a->generations[0],a->tick,
        a->members[0].lifetime,a->program->update,NULL,0, NULL,&result);--a->running;
    if(status==NativeScriptVM_Error)a->faulted=true;
    if(status==NativeScriptVM_Ok){++a->completed;status=nativeScriptEffect_Status(object);}return status;
}
void nativeScriptEffect_Detach(HSD_GObj* object)
{
    NativeScriptEffectActor* a=find(object);if(!a)return;nativeScriptStorage_Destroy(&a->storage);
    for(unsigned i=0;i<a->member_count;++i)if(a->member_live[i]&&nativeMatchPool_Resolve(a->members[i])==object) {
        a->member_live[i]=false;nativeScriptEffectByObject[a->members[i].slot]=UINT16_MAX;if(a->remaining)--a->remaining;break;
    }
    if(!a->remaining)a->active=false;
}
bool nativeScriptEffect_HasActors(FighterKind author)
{for(unsigned i=0;i<nativeScriptEffectInitializedCount;++i)if((nativeScriptEffectActors[i].active||nativeScriptEffectActors[i].running)&&nativeScriptEffectActors[i].program->author_kind==author)return true;return false;}
bool nativeScriptEffect_ReferencesProgram(const NativeScriptCompileBank* bank)
{
    if(!bank)return false;
    for(unsigned i=0;i<nativeScriptEffectInitializedCount;++i) {
        const NativeScriptEffectActor* a=&nativeScriptEffectActors[i];
        if((a->active||a->running)&&(a->execution.declarations==&bank->declarations||
           (a->storage.layout&&a->storage.layout->program==&bank->declarations)))return true;
    }
    return false;
}
bool nativeScriptEffect_Boundary(void)
{
    if(!nativeScriptRuntime_Idle()||HSD_GObj_CurrentInvokedProc||HSD_GObj_CurrentInvokedProcGObj||HSD_GObj_804D7818||HSD_GObj_DelayedProcInfo.flags)return false;
    for(unsigned i=0;i<nativeScriptEffectInitializedCount;++i)if(nativeScriptEffectActors[i].running)return false;return true;
}
bool nativeScriptEffect_CanBeginScene(uint64_t epoch)
{
    if(!epoch||nativeScriptEffectEpoch||!nativeScriptEffect_Boundary())return false;
    for(unsigned i=0;i<nativeScriptEffectInitializedCount;++i)if(nativeScriptEffectActors[i].active)return false;return true;
}
bool nativeScriptEffect_BeginScene(uint64_t epoch)
{if(!nativeScriptEffect_CanBeginScene(epoch)||nativeGObjPool.lifetime!=epoch)return false;memset(nativeScriptEffectByObject,0xFF,sizeof(nativeScriptEffectByObject));nativeScriptEffectInitializedCount=0;nativeScriptEffectEpoch=epoch;return true;}
bool nativeScriptEffect_RetireScene(void)
{
    if(!nativeScriptEffect_Boundary())return false;
    for(unsigned i=0;i<nativeScriptEffectInitializedCount;++i){nativeScriptStorage_Destroy(&nativeScriptEffectActors[i].storage);nativeScriptEffectActors[i].active=false;}
    memset(nativeScriptEffectByObject,0xFF,sizeof(nativeScriptEffectByObject));nativeScriptEffectEpoch=0;return true;
}
