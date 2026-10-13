#include "native_script_binding_cpu_control.h"
#include "native_script_bindings.h"
#include "native_source_package.h"
#include "native_fighter_lifetime.h"
#include "native_cpu_history.h"
#include <melee/it/native_item_lifetime.h>
#include "fighter.h"
#include "types.h"
#include "kinds/ftCommon/ftCo_0A01.h"
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

typedef struct CPUCaller {
    NativeScriptRuntimeContext context;
    NativeMatchPoolReference reference;
    Fighter* fighter;
    FighterKind kind;
    const ftData* fighter_data;
    const NativeSourcePackage* package;
    const NativeScriptFighterProgram* program;
    NativeScriptReceiverView view;
    const NativeScriptStorageParameters* parameter_object;
    uint64_t layout,parameters,program_identity;
} CPUCaller;
static bool error(NativeScriptDiagnostic* d,const char* message)
{if(d)snprintf(d->message,sizeof(d->message),"%s",message);return false;}
static bool current(const CPUCaller* s,NativeScriptRuntimeContext* c,NativeScriptDiagnostic* d)
{
    NativeScriptReceiverView view;
    if(c->source!=s->context.source||c->source_class!=s->context.source_class||c->generation!=s->context.generation||c->scene_epoch!=s->context.scene_epoch||
       nativeMatchPool_Resolve(s->reference)!=c->source||s->reference.lifetime!=c->scene_epoch||
       ((HSD_GObj*)c->source)->classifier!=HSD_GOBJ_CLASS_FIGHTER||((HSD_GObj*)c->source)->user_data!=s->fighter||
       s->fighter->kind!=s->kind||s->fighter->ft_data!=s->fighter_data||
       nativeFighterRuntime_SourcePackage(s->fighter->kind)!=s->package||nativeScriptFighter_Program(s->fighter->kind)!=s->program||
       s->program->identity!=s->program_identity||nativeScriptBindings_SourceFighterLayout(c,d)!=&s->program->bank->motion_layout||
       !nativeScriptFighter_View(c->source,c->generation,&view)||view.program!=s->view.program||view.storage!=s->view.storage||
       !view.storage||!view.storage->live||!view.storage->layout||view.storage->layout->identity!=s->layout||
       view.storage->parameters!=s->parameter_object||!view.storage->parameters||view.storage->parameters->identity!=s->parameters)
        return error(d,"CPU caller program, pool or storage retired before continuation");
    return true;
}
static bool capture(NativeScriptRuntimeContext* c,CPUCaller* s,NativeScriptDiagnostic* d)
{
    if(!c||c->source_class!=NativeCallbackSource_Fighter||!c->scene_epoch||!nativeMatchPool_Reference(&nativeGObjPool,c->source,&s->reference)||
       s->reference.lifetime!=c->scene_epoch||((HSD_GObj*)c->source)->classifier!=HSD_GOBJ_CLASS_FIGHTER||
       !nativeScriptBindings_SourceFighterLayout(c,d))return error(d,"CPU control requires its live owned Fighter source");
    s->context=*c;s->fighter=((HSD_GObj*)c->source)->user_data;s->package=nativeFighterRuntime_SourcePackage(s->fighter->kind);s->program=nativeScriptFighter_Program(s->fighter->kind);
    if(!s->package||!s->program||!nativeScriptFighter_View(c->source,c->generation,&s->view)||!s->view.storage||!s->view.storage->layout||!s->view.storage->parameters)
        return error(d,"CPU control owned source view is absent");
    s->kind=s->fighter->kind;s->fighter_data=s->fighter->ft_data;
    s->program_identity=s->program->identity;s->layout=s->view.storage->layout->identity;s->parameter_object=s->view.storage->parameters;s->parameters=s->parameter_object->identity;
    // Admission above invokes no native/authored callback. The stored identities
    // are rechecked by current() after kernels, not again while capturing them.
    return true;
}
// Scalar property operations invoke no engine or authored callback. Validate
// their selected source once; continuation snapshots belong to Invoke kernels.
static Fighter* scalar_source(NativeScriptRuntimeContext* c,NativeScriptDiagnostic* d)
{
    NativeMatchPoolReference reference;
    if(!c || c->source_class!=NativeCallbackSource_Fighter || !c->scene_epoch ||
       !nativeMatchPool_Reference(&nativeGObjPool,c->source,&reference) ||
       reference.lifetime!=c->scene_epoch ||
       ((HSD_GObj*)c->source)->classifier!=HSD_GOBJ_CLASS_FIGHTER ||
       !nativeScriptBindings_SourceFighterLayout(c,d)) {
        error(d,"CPU property requires its live owned Fighter source");return NULL;
    }
    // SourceFighterLayout already proves generation, payload backlink, selected
    // program and owned storage/parameters. No callback runs before this scalar
    // access, so there is no continuation to capture or revalidate here.
    return ((HSD_GObj*)c->source)->user_data;
}
bool nativeScriptBindingCPUControl_Read(NativeScriptRuntimeContext* c,uint32_t id,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    if(!out)return error(d,"CPU property output is missing");Fighter* fighter=scalar_source(c,d);if(!fighter)return false;NativeScriptValue value={0};
    switch(id){
    case NativeScriptBinding_CPUPendingAction:value=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=fighter->cpu.xF8_b5!=0};break;
    case NativeScriptBinding_CPUBehavior:value=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=fighter->cpu.x18};break;
    case NativeScriptBinding_CPUPreviousBehavior:value=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=fighter->cpu.x1C};break;
    case NativeScriptBinding_CPUTravelFrames:value=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=fighter->cpu.x90};break;
    case NativeScriptBinding_CPUSelectedCommand:value=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=fighter->cpu.xA4};break;
    case NativeScriptBinding_CPUElapsedFrames:value=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=fighter->cpu.x7C};break;
    case NativeScriptBinding_CPUFloorInBounds:value=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=fighter->cpu.xFA_b5!=0};break;
    case NativeScriptBinding_CPUTargetID:{
        CPUCaller s;if(!capture(c,&s,d))return false;
        // The planner stores Fighter data, not a GObj or a persistent script
        // reference. Find physical membership before dereferencing that data.
        Fighter* selected=fighter->cpu.x44;
        value.type=NativeScriptType_U32;
        if(!selected)break;
        if(!HSD_GObjPLinkHead)return error(d,"CPU target registry is unavailable");
        HSD_GObj* target=NULL;NativeMatchPoolReference reference={0};
        uint32_t generation=0;FighterKind kind=Ft_Kind_None;unsigned visited=0;
        for(HSD_GObj* object=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];object;object=object->next){
            NativeMatchPoolReference candidate;
            if(visited++>=nativeGObjPool.capacity||!nativeMatchPool_Reference(&nativeGObjPool,object,&candidate)||
               candidate.lifetime!=c->scene_epoch||nativeMatchPool_Resolve(candidate)!=object||
               object->classifier!=HSD_GOBJ_CLASS_FIGHTER||!object->user_data)
                return error(d,"CPU target registry contains a stale or foreign actor");
            if(object->user_data!=selected)continue;
            if(!nativeCallbackSource_Snapshot(object,NativeCallbackSource_Fighter,&generation)||selected->gobj!=object)
                return error(d,"CPU target physical identity is invalid");
            target=object;reference=candidate;kind=selected->kind;break;
        }
        if(!target)return error(d,"CPU selected target has left the physical Fighter registry");
        if(!current(&s,c,d)||fighter->cpu.x44!=selected||nativeMatchPool_Resolve(reference)!=target||
           target->classifier!=HSD_GOBJ_CLASS_FIGHTER||target->user_data!=selected||
           !nativeCallbackSource_Alive(target,generation,NativeCallbackSource_Fighter)||
           selected->gobj!=target||selected->kind!=kind)
            return error(d,"CPU target retired before creation-ID publication");
        value.as.u64=generation;break;
    }
    case NativeScriptBinding_CPUDestinationX:value=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=fighter->cpu.x54.x};break;
    case NativeScriptBinding_CPUDestinationY:value=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=fighter->cpu.x54.y};break;
    case NativeScriptBinding_CPUApproachSpacing:value.type=NativeScriptType_F32;memcpy(&value.as.f32,&fighter->cpu.x56C,4);break;
    case NativeScriptBinding_CPUMode:value=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=fighter->cpu.xC};break;
    case NativeScriptBinding_CPUFollowDistance:value.type=NativeScriptType_F32;memcpy(&value.as.f32,&fighter->cpu.x3C,4);break;
    case NativeScriptBinding_CPUCombatCandidatesEnabled:value=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=fighter->cpu.xF9_b2!=0};break;
    case NativeScriptBinding_CPURangedGateEnabled:value=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=fighter->cpu.xF9_b4!=0};break;
    case NativeScriptBinding_CPUOutputLeftX:value=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=fighter->cpu.lstick.x};break;
    case NativeScriptBinding_CPUOutputLeftY:value=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=fighter->cpu.lstick.y};break;
    case NativeScriptBinding_CPUOutputLeftTrigger:value=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=fighter->cpu.ltrigger};break;
    case NativeScriptBinding_CPUOutputRightTrigger:value=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=fighter->cpu.rtrigger};break;
    case NativeScriptBinding_CPUOutputButtons:{int32_t buttons;memcpy(&buttons,&fighter->cpu.buttons,4);value=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=buttons};break;}
    default:return error(d,"Unsupported CPU control property");}
    *out=value;return true;
}
bool nativeScriptBindingCPUControl_Write(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* value,NativeScriptDiagnostic* d)
{
    NativeScriptIRType type;
    switch(id){
    case NativeScriptBinding_CPUPendingAction:case NativeScriptBinding_CPUCombatCandidatesEnabled:case NativeScriptBinding_CPURangedGateEnabled:type=NativeScriptType_Bool;break;
    case NativeScriptBinding_CPUBehavior:case NativeScriptBinding_CPUMode:case NativeScriptBinding_CPULevel:
    case NativeScriptBinding_CPUTravelFrames:case NativeScriptBinding_CPUSelectedCommand:
    case NativeScriptBinding_CPUOutputLeftX:case NativeScriptBinding_CPUOutputLeftY:case NativeScriptBinding_CPUOutputLeftTrigger:case NativeScriptBinding_CPUOutputRightTrigger:case NativeScriptBinding_CPUOutputButtons:type=NativeScriptType_I32;break;
    case NativeScriptBinding_CPUFollowDistance:type=NativeScriptType_F32;break;
    default:return error(d,"CPU property is not writable");}
    if(!value||value->type!=type)return error(d,"CPU write requires its exact writable property type");
    if((id==NativeScriptBinding_CPUTravelFrames||id==NativeScriptBinding_CPUSelectedCommand)&&
       (value->as.i64<INT32_MIN||value->as.i64>INT32_MAX))return error(d,"CPU planner field exceeds its native Int width");
    if(id>=NativeScriptBinding_CPUOutputLeftX&&id<=NativeScriptBinding_CPUOutputButtons&&
       (value->as.i64<INT32_MIN||value->as.i64>INT32_MAX))return error(d,"CPU output exceeds its declared Int width");
    Fighter* fighter=scalar_source(c,d);if(!fighter)return false;
    switch(id){
    case NativeScriptBinding_CPUPendingAction:fighter->cpu.xF8_b5=value->as.u64!=0;break;
    case NativeScriptBinding_CPUBehavior:fighter->cpu.x18=(int32_t)value->as.i64;break;
    case NativeScriptBinding_CPUTravelFrames:fighter->cpu.x90=(int32_t)value->as.i64;break;
    case NativeScriptBinding_CPUSelectedCommand:fighter->cpu.xA4=(int32_t)value->as.i64;break;
    case NativeScriptBinding_CPUMode:fighter->cpu.xC=(int32_t)value->as.i64;break;
    case NativeScriptBinding_CPULevel:fighter->cpu.level=(int32_t)value->as.i64;break;
    case NativeScriptBinding_CPUFollowDistance:if(isfinite(value->as.f32)&&value->as.f32>=0.0f)memcpy(&fighter->cpu.x3C,&value->as.f32,4);break;
    case NativeScriptBinding_CPUCombatCandidatesEnabled:fighter->cpu.xF9_b2=value->as.u64!=0;break;
    case NativeScriptBinding_CPURangedGateEnabled:fighter->cpu.xF9_b4=value->as.u64!=0;break;
    case NativeScriptBinding_CPUOutputLeftX:if(value->as.i64>=-128&&value->as.i64<=127)fighter->cpu.lstick.x=(int8_t)value->as.i64;break;
    case NativeScriptBinding_CPUOutputLeftY:if(value->as.i64>=-128&&value->as.i64<=127)fighter->cpu.lstick.y=(int8_t)value->as.i64;break;
    case NativeScriptBinding_CPUOutputLeftTrigger:if(value->as.i64>=0&&value->as.i64<=255)fighter->cpu.ltrigger=(uint8_t)value->as.i64;break;
    case NativeScriptBinding_CPUOutputRightTrigger:if(value->as.i64>=0&&value->as.i64<=255)fighter->cpu.rtrigger=(uint8_t)value->as.i64;break;
    case NativeScriptBinding_CPUOutputButtons:fighter->cpu.buttons=(uint32_t)value->as.i64;break;
    default:return false;}
    return true;
}
static bool queue(CPUCaller* s,uint32_t id,const NativeScriptValue* args,uint32_t count,NativeScriptDiagnostic* d)
{
    if(!args||args[0].type!=NativeScriptType_I32||args[0].as.i64<0||args[0].as.i64>255)return error(d,"CPU queue command type/range is invalid");
    int operands=nativeScriptCPUControl_CommandOperands((uint32_t)args[0].as.i64);
    if(operands<0||count!=(uint32_t)operands+1||id!=NativeScriptBinding_CPUQueue+(uint32_t)operands)
        return error(d,"CPU queue opcode and operand arity differ");
    NativeScriptDiagnostic local;uint8_t bytes[2];for(int i=0;i<operands;++i){NativeScriptValue converted;
        if(!nativeScriptValue_Convert(&args[i+1],NativeScriptType_I32,&converted,d?d:&local))return false;
        bytes[i]=(uint8_t)(uint32_t)converted.as.i64;}
    uintptr_t first=(uintptr_t)s->fighter->cpu.buffer,pos=(uintptr_t)s->fighter->cpu.write_pos;
    if(pos<first||pos-first>sizeof(s->fighter->cpu.buffer)||(uintptr_t)count>sizeof(s->fighter->cpu.buffer)-(pos-first))
        return error(d,"CPU command buffer is uninitialized, invalid or exhausted");
    if(operands==0)ftCo_800B463C(s->fighter,(uint8_t)args[0].as.i64);
    else if(operands==1)ftCo_800B46B8(s->fighter,(uint8_t)args[0].as.i64,bytes[0]);
    else ftCo_800B4778(s->fighter,(uint8_t)args[0].as.i64,bytes[0],bytes[1]);
    return true;
}
static bool nearest(CPUCaller* s,const NativeScriptValue* args,NativeScriptValue* value,NativeScriptDiagnostic* d)
{
    if(!HSD_GObjPLinkHead)return error(d,"CPU Fighter list is unavailable");
    // The native selector dereferences this list. Verify typed pool membership
    // before it runs; the actual eligibility/distance/order algorithm stays native.
    uint32_t visited=0;
    for(HSD_GObj* g=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];g;g=g->next){NativeMatchPoolReference reference;
        if(visited++==nativeGObjPool.capacity)return error(d,"CPU Fighter list is cyclic or exceeds its pool bound");
        if(!nativeMatchPool_Reference(&nativeGObjPool,g,&reference)||reference.lifetime!=s->context.scene_epoch||g->classifier!=HSD_GOBJ_CLASS_FIGHTER)
            return error(d,"CPU selector encountered a foreign pool/classifier");}
    Vec3 point={args[0].as.f32,args[1].as.f32,args[2].as.f32};uint32_t handle=0;
    bool found=ftCo_CpuNearestEligibleTarget(s->context.source,&point,&handle);
    if(found){HSD_GObj* selected=NULL;visited=0;
        for(HSD_GObj* g=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];g;g=g->next){NativeMatchPoolReference reference;
            if(visited++==nativeGObjPool.capacity)return error(d,"CPU selected-target list is cyclic or exceeds its pool bound");
            if(!nativeMatchPool_Reference(&nativeGObjPool,g,&reference)||reference.lifetime!=s->context.scene_epoch||g->classifier!=HSD_GOBJ_CLASS_FIGHTER)
                return error(d,"CPU selected-target scan encountered a foreign pool/classifier");
            if(!g->user_data)continue;
            Fighter* fp=g->user_data;if((uint32_t)fp->x8_spawnNum==handle){if(selected)return error(d,"CPU selected handle is ambiguous");selected=g;}}
        NativeMatchPoolReference reference;
        if(!selected||!nativeMatchPool_Reference(&nativeGObjPool,selected,&reference)||reference.lifetime!=s->context.scene_epoch||
           nativeMatchPool_Resolve(reference)!=selected||selected->classifier!=HSD_GOBJ_CLASS_FIGHTER||
           !nativeCallbackSource_Alive(selected,handle,NativeCallbackSource_Fighter)||((Fighter*)selected->user_data)->gobj!=selected)
            return error(d,"CPU selected Fighter retired before handle publication");
    }
    *value=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=found?handle:0};return true;
}
// These native kernels scan physical registries and retain raw target payloads.
// Admit the lists before native dereferences, then retain only actual cached
// peers. No registry copy, callback dispatch or alternate planner is introduced.
static bool registry(CPUCaller* s,unsigned link,uint16_t classifier,NativeScriptDiagnostic* d)
{
    if(!HSD_GObjPLinkHead)return error(d,"CPU native registry is unavailable");
    unsigned visited=0;
    // Membership is established by this walk. Searching the same registry
    // again for each node makes planner admission quadratic in live items.
    for(HSD_GObj* g=HSD_GObjPLinkHead[link];g;g=g->next){NativeMatchPoolReference ref;
        if(visited++>=nativeGObjPool.capacity||!nativeMatchPool_Reference(&nativeGObjPool,g,&ref)||ref.lifetime!=s->context.scene_epoch||
           g->classifier!=classifier||!g->user_data)
            return error(d,"CPU native registry is stale, cyclic or has a foreign class");
        if(classifier==HSD_GOBJ_CLASS_FIGHTER?((Fighter*)g->user_data)->gobj!=g:((Item*)g->user_data)->entity!=g)
            return error(d,"CPU native registry payload changed physical identity");
    }
    return true;
}
static HSD_GObj* fighter_object(Fighter* data,uint32_t generation)
{
    for(HSD_GObj* g=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];g;g=g->next)
        if(data?g->user_data==data:(uint32_t)((Fighter*)g->user_data)->x8_spawnNum==generation)return g;
    return NULL;
}
static HSD_GObj* item_object(Item* data)
{
    for(HSD_GObj* g=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];g;g=g->next)if(g->user_data==data)return g;
    return NULL;
}
typedef struct CPUPlannerPeers {
    NativeFighterLifetime fighter;NativeItemLifetime items[3];unsigned item_count;bool has_fighter;
} CPUPlannerPeers;
static bool planner_peers(CPUCaller* s,uint32_t id,bool after,CPUPlannerPeers* p,NativeScriptDiagnostic* d)
{
    Fighter* f=s->fighter;Item* items[]={f->cpu.x4C,f->cpu.special_item_target};bool targets=id==NativeScriptBinding_CPUPlanAttack||
        (after&&(id==NativeScriptBinding_CPUPrepareCombatPlan||id==NativeScriptBinding_CPUPlanWithoutTarget));
    if(!registry(s,HSD_GOBJ_PLINK_FIGHTER,HSD_GOBJ_CLASS_FIGHTER,d)||!registry(s,HSD_GOBJ_PLINK_ITEM,HSD_GOBJ_CLASS_ITEM,d))return false;
    *p=(CPUPlannerPeers){0};
    // x48 is an equality-only selection cache, never a dereferenced target.
    // Refresh operations overwrite previous targets before using them.
    if(targets&&f->cpu.x44){HSD_GObj* g=fighter_object(f->cpu.x44,0);
        if(!g||!nativeFighterLifetime_Capture(g->user_data,&p->fighter)||p->fighter.reference.lifetime!=s->context.scene_epoch)
            return error(d,"CPU cached Fighter target is not live in this scene");
        p->has_fighter=true;}
    for(unsigned i=0;i<3;++i){HSD_GObj* g=i<2?(targets&&items[i]?item_object(items[i]):NULL):f->item_gobj;
        if((i<2?(!targets||!items[i]):!f->item_gobj))continue;
        if(!g||!nativeItemLifetime_Capture(g,&p->items[p->item_count])||p->items[p->item_count].reference.lifetime!=s->context.scene_epoch)
            return error(d,"CPU cached Item target is not live in this scene");
        ++p->item_count;}
    return true;
}
static bool planner_current(const CPUPlannerPeers* p,NativeScriptDiagnostic* d)
{
    if(p->has_fighter&&!nativeFighterLifetime_Current(&p->fighter))return error(d,"CPU retained Fighter target retired during planning");
    for(unsigned i=0;i<p->item_count;++i)if(!nativeItemLifetime_Current(&p->items[i]))return error(d,"CPU retained Item target retired during planning");
    return true;
}
bool nativeScriptBindingCPUControl_Invoke(NativeScriptRuntimeContext* c,uint32_t id,NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    bool destination=id==NativeScriptBinding_CPUSetDestination,spacing=id==NativeScriptBinding_CPUSetApproachSpacing,prepare=id==NativeScriptBinding_CPUPrepareDestinationUpdate;
    bool history=id==NativeScriptBinding_CPUReadHistoryPose,record=id==NativeScriptBinding_CPURecordInput,apply=id==NativeScriptBinding_CPUApplyRecordedInput;
    bool planner=id>=NativeScriptBinding_CPUPrepareCombatPlan&&id<=NativeScriptBinding_CPUPlanWithoutTarget;
    if(!out||((id<NativeScriptBinding_CPUClearTarget||id>NativeScriptBinding_CPUNearestEnemyAt)&&!destination&&!spacing&&!prepare&&!history&&!record&&!apply&&!planner))
        return error(d,"CPU control operation is unsupported");
    if(((id==NativeScriptBinding_CPUClearTarget||prepare||apply||planner)&&count)||((id==NativeScriptBinding_CPUNearestEnemyAt||destination)&&count!=3)||((spacing||record)&&count!=1)||(history&&count!=4)||
       (count&&!args))return error(d,"CPU control argument count is invalid");
    if(id==NativeScriptBinding_CPUNearestEnemyAt||destination||spacing||history)for(unsigned i=0;i<count;++i)if(args[i].type!=NativeScriptType_F32)return error(d,"CPU scalar arguments must be exact F32");
    if(record&&(args[0].type!=NativeScriptType_I32||args[0].as.i64<INT32_MIN||args[0].as.i64>INT32_MAX))return error(d,"CPU input source must be an exact native Int");
    CPUCaller s;if(!capture(c,&s,d))return false;NativeScriptValue value={.type=NativeScriptType_Void};
    Vec3 pose={0};float facing=0;NativeFighterLifetime input={0};CPUPlannerPeers peers={0};
    if(record){uint32_t generation=(uint32_t)args[0].as.i64;if(generation){
        if(!registry(&s,HSD_GOBJ_PLINK_FIGHTER,HSD_GOBJ_CLASS_FIGHTER,d))return false;HSD_GObj* g=fighter_object(NULL,generation);
        if(g&&(!nativeFighterLifetime_Capture(g->user_data,&input)||input.reference.lifetime!=c->scene_epoch))return error(d,"CPU input Fighter source is retired");
    }}
    if(planner&&!planner_peers(&s,id,false,&peers,d))return false;
    if(id==NativeScriptBinding_CPUClearTarget)s.fighter->cpu.x44=NULL;
    else if(id==NativeScriptBinding_CPUNearestEnemyAt){if(!nearest(&s,args,&value,d))return false;}
    else if(prepare)value=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=ftCo_CpuPrepareDestinationUpdate(s.fighter)};
    else if(destination)ftCo_CpuSetDestination(s.fighter,args[0].as.f32,args[1].as.f32,args[2].as.f32);
    else if(spacing)memcpy(&s.fighter->cpu.x56C,&args[0].as.f32,4);
    else if(record)value=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=input.object&&nativeCpuHistory_Record(s.context.source,(uint32_t)args[0].as.i64)};
    else if(apply)value=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=nativeCpuHistory_Apply(s.context.source)};
    else if(history)value=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=nativeCpuHistory_ReadPose(s.context.source,&pose,&facing)};
    else if(id==NativeScriptBinding_CPUPrepareCombatPlan)ftCo_CpuPrepareCombatPlan(s.fighter);
    else if(id==NativeScriptBinding_CPUUpdateRecoveryDestination)ftCo_CpuUpdateRecoveryDestination(s.fighter);
    else if(id==NativeScriptBinding_CPUPlanAttack)value=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=ftCo_CpuPlanAttack(s.fighter)};
    else if(id==NativeScriptBinding_CPUPlanWithoutTarget)ftCo_CpuPlanWithoutTarget(s.fighter);
    else if(!queue(&s,id,args,count,d))return false;
    if(!current(&s,c,d)||(input.object&&!nativeFighterLifetime_Current(&input)))return error(d,"CPU source or input target retired before publication");
    if(planner){CPUPlannerPeers selected;if(!planner_current(&peers,d)||!planner_peers(&s,id,true,&selected,d))return false;}
    if(history&&value.as.u64){args[0]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=pose.x};args[1]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=pose.y};args[2]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=pose.z};args[3]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=facing};}
    *out=value;return true;
}
