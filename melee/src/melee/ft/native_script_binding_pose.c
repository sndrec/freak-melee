#include "native_script_binding_pose.h"
#include "native_script_bindings.h"
#include "native_script_fighter.h"
#include "native_source_package.h"
#include "native_fighter_pose.h"
#include "native_callback_lifetime.h"
#include "fighter.h"
#include "ftanim.h"
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <math.h>
#include <stdio.h>

typedef struct PoseSource {
    NativeScriptRuntimeContext* context;
    NativeMatchPoolReference reference;
    const NativeSourcePackage* package;
    const NativeScriptFighterProgram* program;
    NativeScriptReceiverView view;
    NativeFighterPoseAssets assets;
    Fighter* fighter;
    FighterBone* parts;
    HSD_JObj* live;
    HSD_JObj* blend;
    uint64_t program_identity, layout_identity, parameter_identity;
} PoseSource;

static bool error(NativeScriptDiagnostic* d,const char* message)
{if(d)snprintf(d->message,sizeof(d->message),"%s",message);return false;}

static bool current(const PoseSource* source,NativeScriptDiagnostic* d)
{
    NativeScriptRuntimeContext* c=source->context;
    NativeScriptReceiverView view;NativeFighterPoseAssets assets;
    if(nativeMatchPool_Resolve(source->reference)!=c->source ||
       source->reference.lifetime!=c->scene_epoch || !nativeScriptBindings_SourceAlive(c) ||
       ((HSD_GObj*)c->source)->user_data!=source->fighter ||
       nativeFighterRuntime_SourcePackage(source->fighter->kind)!=source->package ||
       nativeScriptFighter_Program(source->fighter->kind)!=source->program ||
       source->program->identity!=source->program_identity ||
       source->program->bank!=&source->package->compiler.bank ||
       nativeScriptBindings_SourceFighterLayout(c,d)!=&source->program->bank->motion_layout ||
       source->fighter->parts!=source->parts || ((HSD_GObj*)c->source)->hsd_obj!=source->live ||
       source->fighter->x8AC_animSkeleton!=source->blend ||
       !nativeScriptFighter_View(c->source,c->generation,&view) ||
       view.program!=source->view.program || view.storage!=source->view.storage ||
       !view.storage || !view.storage->live ||
       view.storage->layout!=&source->program->bank->storage_layout ||
       view.storage->layout->identity!=source->layout_identity ||
       !view.storage->parameters || view.storage->parameters->identity!=source->parameter_identity ||
       view.storage->parameters!=&source->program->bank->storage_parameters ||
       !nativeFighterRuntime_PoseAssets(c->source,&assets) ||
       assets.character!=source->assets.character || assets.costume!=source->assets.costume ||
       assets.action_count!=source->assets.action_count || assets.pose_graph_count!=source->assets.pose_graph_count ||
       assets.animation_bytes!=source->assets.animation_bytes || assets.animation_size!=source->assets.animation_size ||
       assets.animation_arena!=source->assets.animation_arena || assets.animation_arena_size!=source->assets.animation_arena_size)
        return error(d,"Pose source, program, storage or owned skeleton retired");
    return true;
}

static bool capture(NativeScriptRuntimeContext* c,PoseSource* source,NativeScriptDiagnostic* d)
{
    if(!c || c->source_class!=NativeCallbackSource_Fighter || !c->scene_epoch ||
       !nativeMatchPool_Reference(&nativeGObjPool,c->source,&source->reference) ||
       source->reference.lifetime!=c->scene_epoch || !nativeScriptBindings_SourceFighterLayout(c,d))
        return error(d,"Pose operation requires its live bound source Fighter");
    source->context=c;source->fighter=((HSD_GObj*)c->source)->user_data;
    source->parts=source->fighter->parts;source->live=((HSD_GObj*)c->source)->hsd_obj;
    source->blend=source->fighter->x8AC_animSkeleton;
    source->package=nativeFighterRuntime_SourcePackage(source->fighter->kind);
    source->program=nativeScriptFighter_Program(source->fighter->kind);
    if(!source->package || !source->program || !source->parts || !source->live || !source->blend ||
       source->program->bank!=&source->package->compiler.bank ||
       !nativeScriptFighter_View(c->source,c->generation,&source->view) ||
       !source->view.storage || !source->view.storage->layout || !source->view.storage->parameters ||
       !nativeFighterRuntime_PoseAssets(c->source,&source->assets))
        return error(d,"Pose operation has no matching owned source assets");
    source->program_identity=source->program->identity;
    source->layout_identity=source->view.storage->layout->identity;
    source->parameter_identity=source->view.storage->parameters->identity;
    return current(source,d);
}

// IntegralAny preserves width/sign through native marshalling. Reject invalid
// indices rather than converting a large value modulo 32 bits into a valid one.
static bool index(const NativeScriptValue* value,uint32_t limit,int32_t* result)
{
    uint64_t bits;
    switch(value->type) {
    case NativeScriptType_I32:case NativeScriptType_I64:
        if(value->as.i64<0)return false;bits=(uint64_t)value->as.i64;break;
    case NativeScriptType_Bool:case NativeScriptType_U32:case NativeScriptType_U64:
        bits=value->as.u64;break;
    default:return false;
    }
    if(bits>=limit || bits>INT32_MAX)return false;*result=(int32_t)bits;return true;
}

static HSD_Joint* graph(const PoseSource* source,const NativeScriptValue* value,NativeScriptDiagnostic* d)
{
    const NativeScriptCompileBank* bank=source->program->bank;
    if(value->type!=NativeScriptType_PoseGraph || value->as.u64>=bank->pose_graph_count ||
       bank->pose_graph_count!=source->package->assets.facts.pose_count ||
       bank->pose_graph_mask!=source->package->assets.facts.layout.joint_graph_mask ||
       bank->pose_symbol_count>16) {error(d,"Pose graph selector does not belong to this source bank");return NULL;}
    bool declared=false;
    for(unsigned i=0;i<bank->pose_symbol_count;++i)
        if(bank->pose_symbols[i].row==value->as.u64){declared=true;break;}
    if(!declared){error(d,"Pose graph selector has no prepared declaration");return NULL;}
    HSD_Joint* selected=nativeFighterRuntime_PoseGraph(source->context->source,(int32_t)value->as.u64);
    if(!selected)error(d,"Owned pose graph is unavailable");return selected;
}

bool nativeScriptBindingPose_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* arguments,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* d)
{
    const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(semantic);
    if(!descriptor || descriptor->parent!=NativeScriptBinding_Pose || !result ||
       count!=descriptor->argument_count || (count && !arguments))return error(d,"Invalid pose operation signature");
    for(unsigned i=0;i<count;++i) {
        if(descriptor->arguments[i].numeric_any) {
            int32_t checked;if(!index(&arguments[i],INT32_MAX,&checked))return error(d,"Pose clip/start must be a bounded integral index");
        } else if(arguments[i].type!=descriptor->arguments[i].type)return error(d,"Pose argument differs from its closed typed signature");
        if(arguments[i].type==NativeScriptType_F32 && !isfinite(arguments[i].as.f32))return error(d,"Pose frame/weight is not finite");
    }
    PoseSource source;if(!capture(context,&source,d))return false;
    Fighter* fighter=source.fighter;HSD_GObj* object=context->source;
    int32_t destination=NativeFighterPose_Live,start=0,clip_index=0;
    HSD_Joint* selected=NULL;FigaTree* clip=NULL;NativeFighterPoseOperation operation;
    if(semantic!=NativeScriptBinding_PoseBlendToLive && semantic!=NativeScriptBinding_PoseCopyToLive) {
        if(arguments[0].type!=NativeScriptType_PoseDestination || arguments[0].as.u64>NativeFighterPose_Blend)
            return error(d,"Pose destination must be Live or Blend");
        destination=(int32_t)arguments[0].as.u64;
    }
    switch(semantic) {
    case NativeScriptBinding_PoseLoadAnimation:
        operation=NativeFighterPose_LoadAnimation;
        if(!index(&arguments[1],source.assets.action_count,&clip_index) ||
           !(clip=nativeFighterRuntime_PoseClip(object,clip_index)))return error(d,"Pose clip exceeds the owned animation bank");
        break;
    case NativeScriptBinding_PoseRequestFrame:operation=NativeFighterPose_RequestFrame;break;
    case NativeScriptBinding_PoseEvaluate:operation=NativeFighterPose_Evaluate;break;
    case NativeScriptBinding_PoseCopyCostume:operation=NativeFighterPose_CopyCostume;break;
    case NativeScriptBinding_PoseCopyGraph:operation=NativeFighterPose_CopyGraph;break;
    case NativeScriptBinding_PoseBlendGraph:operation=NativeFighterPose_BlendGraph;break;
    case NativeScriptBinding_PoseBlendToLive:operation=NativeFighterPose_BlendToLive;break;
    case NativeScriptBinding_PoseCopyToLive:operation=NativeFighterPose_CopyToLive;break;
    default:return error(d,"Unsupported pose operation");
    }
    if(operation>=NativeFighterPose_CopyCostume) {
        unsigned argument=operation==NativeFighterPose_BlendToLive || operation==NativeFighterPose_CopyToLive?0:1;
        unsigned id;
        if(!index(&arguments[argument],(unsigned)INT32_MAX+1u,&id))return error(d,"Pose start needs a bone ID");
        int part=nativeFighterRig_Resolve(&source.package->assets.rig,id);
        if(part<0||(unsigned)part>=source.package->assets.parts.parts_num)return error(d,"Pose start bone is absent from the owned rig");
        start=(unsigned)part;
    }
    if(operation==NativeFighterPose_CopyGraph || operation==NativeFighterPose_BlendGraph) {
        selected=graph(&source,&arguments[2],d);if(!selected)return false;
    }
    if(!current(&source,d) || !nativeFighterPose_Validate(object,operation,destination,start,selected,clip))
        return error(d,"Pose operation has invalid owned rig, clip or descriptor topology");
    if(!current(&source,d))return false;
    switch(operation) {
    case NativeFighterPose_LoadAnimation:ftAnim_8006F4C8(fighter,destination,clip);break;
    case NativeFighterPose_RequestFrame:ftAnim_80070710(destination?source.blend:source.live,arguments[1].as.f32);break;
    case NativeFighterPose_Evaluate:HSD_JObjAnimAll(destination?source.blend:source.live);break;
    case NativeFighterPose_CopyCostume:
        selected=ftAnim_8006F994(fighter,destination?fighter->parts[start].x4_jobj2:fighter->parts[start].joint,fighter->x108_costume_joint);
        if(!current(&source,d))return false;
        if(!selected)return error(d,"Costume pose start has no owned descriptor");
        if(destination)ftAnim_8006FB88(fighter,(Fighter_Part)start,selected);
        else ftAnim_8006FA58(fighter,(Fighter_Part)start,selected);
        break;
    case NativeFighterPose_CopyGraph:
        if(destination)ftAnim_8006FB88(fighter,(Fighter_Part)start,selected);
        else ftAnim_8006FA58(fighter,(Fighter_Part)start,selected);break;
    case NativeFighterPose_BlendGraph:
        if(destination)ftAnim_80070108(fighter,(Fighter_Part)start,arguments[3].as.f32,arguments[4].as.f32,selected);
        else ftAnim_80070010(fighter,(Fighter_Part)start,arguments[3].as.f32,arguments[4].as.f32,selected);break;
    case NativeFighterPose_BlendToLive:ftAnim_8006FE9C(fighter,(Fighter_Part)start,arguments[1].as.f32,arguments[2].as.f32);break;
    case NativeFighterPose_CopyToLive:ftAnim_8006FF74(fighter,(Fighter_Part)start);break;
    }
    if(!current(&source,d))return false;
    *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}
