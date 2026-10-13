#include "native_script_binding_actions.h"
#include "native_script_bindings.h"
#include "native_callback_lifetime.h"
#include "native_source_package.h"
#include "native_script_effect_dispatch.h"
#include "fighter.h"
#include "ftcoll.h"
#include "ftparts.h"
#include "ftdat.h"
#include "types.h"
#include <melee/ef/efsync.h>
#include <melee/ef/efasync.h>
#include <melee/it/types.h>
#include <melee/it/native_item_lifetime.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lb_00F9.h>
#include <melee/lb/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <math.h>
#include <stdio.h>

static bool error(NativeScriptDiagnostic* d,const char* text)
{if(d)snprintf(d->message,sizeof(d->message),"%s",text);return false;}
typedef struct FeedbackSource {
    NativeScriptRuntimeContext* context;
    NativeMatchPoolReference reference;
    const NativeSourcePackage* package;
    const NativeScriptFighterProgram* program;
    NativeScriptReceiverView view;
    Fighter* fighter;
    FighterBone* parts;
    uint64_t program_identity,layout_identity,parameter_identity;
} FeedbackSource;
static bool current(const FeedbackSource* source,NativeScriptDiagnostic* d)
{
    NativeScriptRuntimeContext* c=source->context;NativeScriptReceiverView view;
    if(nativeMatchPool_Resolve(source->reference)!=c->source||source->reference.lifetime!=c->scene_epoch||
       !nativeScriptBindings_SourceAlive(c)||((HSD_GObj*)c->source)->user_data!=source->fighter||
       nativeFighterRuntime_SourcePackage(source->fighter->kind)!=source->package||
       nativeScriptFighter_Program(source->fighter->kind)!=source->program||
       source->program->identity!=source->program_identity||source->fighter->parts!=source->parts||
       nativeScriptBindings_SourceFighterLayout(c,d)!=&source->program->bank->motion_layout||
       !nativeScriptFighter_View(c->source,c->generation,&view)||view.program!=source->view.program||
       view.storage!=source->view.storage||!view.storage||!view.storage->live||
       view.storage->layout->identity!=source->layout_identity||!view.storage->parameters||
       view.storage->parameters->identity!=source->parameter_identity||
       !ftPartsTable||ftPartsTable[source->fighter->kind]!=&source->package->assets.parts||
       !source->package->assets.parts.parts_num||source->package->assets.parts.parts_num>MAX_FT_PARTS||
       source->package->assets.parts.parts_num!=source->package->assets.facts.joints+source->package->assets.facts.omission_count||
       source->package->assets.parts.part_to_joint!=source->package->assets.part_to_joint||
       source->package->assets.parts.joint_to_part!=source->package->assets.joint_to_part)
        return error(d,"Feedback source, program, schema or imported rig retired");
    return true;
}
static bool capture(NativeScriptRuntimeContext* c,FeedbackSource* source,NativeScriptDiagnostic* d)
{
    if(!c||c->source_class!=NativeCallbackSource_Fighter||!c->scene_epoch||
       !nativeMatchPool_Reference(&nativeGObjPool,c->source,&source->reference)||source->reference.lifetime!=c->scene_epoch||
       !nativeScriptBindings_SourceFighterLayout(c,d))return error(d,"Feedback requires its live bound source Fighter");
    source->context=c;source->fighter=((HSD_GObj*)c->source)->user_data;source->parts=source->fighter->parts;
    source->package=nativeFighterRuntime_SourcePackage(source->fighter->kind);source->program=nativeScriptFighter_Program(source->fighter->kind);
    if(!source->package||!source->program||!source->parts||!nativeScriptFighter_View(c->source,c->generation,&source->view)||
       !source->view.storage||!source->view.storage->layout||!source->view.storage->parameters)return error(d,"Feedback owned view is absent");
    source->program_identity=source->program->identity;source->layout_identity=source->view.storage->layout->identity;
    source->parameter_identity=source->view.storage->parameters->identity;
    return current(source,d);
}
static HSD_JObj* bone(Fighter* fp,uint32_t index,bool semantic,NativeScriptDiagnostic* d)
{
    FighterPartsTable* table=(uint32_t)fp->kind<Ft_Kind_RuntimeCapacity && ftPartsTable?ftPartsTable[fp->kind]:NULL;
    if(!table || !fp->parts || !table->parts_num || table->parts_num>MAX_FT_PARTS){error(d,"Feedback fighter model schema is absent");return NULL;}
    if(semantic){if(index>=FT_COMMON_PART_COUNT || !table->part_to_joint){error(d,"Feedback semantic part is out of range");return NULL;}index=(uint32_t)ftParts_GetBoneIndex(fp,(Fighter_Part)index);}
    if(index>=table->parts_num || !fp->parts[index].joint){error(d,"Feedback native bone is unmapped");return NULL;}return fp->parts[index].joint;
}
static bool effect_id(const NativeScriptValue* value,int32_t* result)
{
    uint64_t id;
    if(value->type<NativeScriptType_Bool||value->type>NativeScriptType_U64)return false;
    if(value->type==NativeScriptType_I32||value->type==NativeScriptType_I64) {
        if(value->as.i64<0)return false;id=(uint64_t)value->as.i64;
    } else id=value->as.u64;
    if(id>UINT16_MAX)return false;*result=(int32_t)id;return true;
}
bool nativeScriptBindingFeedback_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,const NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(semantic);
    if(!out||!descriptor||descriptor->argument_count!=count||(count&&!args))return error(d,"Feedback argument contract is invalid");
    for(unsigned i=0;i<count;++i) {
        if(descriptor->arguments[i].numeric_any) {
            if(args[i].type<NativeScriptType_Bool||args[i].type>NativeScriptType_U64)return error(d,"Feedback selector requires an integral value");
        } else if(args[i].type!=descriptor->arguments[i].type)return error(d,"Feedback argument differs from its declared type");
    }
    if(!context || !nativeScriptBindings_SourceAlive(context))return error(d,"Feedback source is unavailable");
    HSD_GObj* object=context->source;
    if(semantic==NativeScriptBinding_ItemSpawnEffectOnBone || semantic==NativeScriptBinding_ItemSpawnEffectOnBoneMode) {
        if(context->source_class!=NativeCallbackSource_Item)return error(d,"Item bone effect requires an Item source");
        Item* item=object->user_data;uint32_t index=(uint32_t)args[1].as.u64,mode=semantic==NativeScriptBinding_ItemSpawnEffectOnBoneMode?(uint32_t)args[2].as.u64:0;
        if((uint64_t)args[0].as.i64>65535 || index>=100 || mode>1 || !item->xC4_article_data || !item->xC4_article_data->x10_modelDesc)
            return error(d,"Item effect ID/bone/mode or model metadata is invalid");
        HSD_JObj* joint=object->hsd_obj;uint32_t bones=item->xC4_article_data->x10_modelDesc->x4_bone_count;
        if(bones){if(bones>100 || !item->xBBC_dynamicBoneTable || index>=bones)return error(d,"Item effect bone exceeds its native indexed table");joint=item->xBBC_dynamicBoneTable->bones[index];}
        else for(unsigned i=0;i<index && joint;++i)joint=joint->child;
        if(!joint)return error(d,"Item effect joint is absent");
        efAsync_Spawn(object,&item->xBC0,(u32)mode,(int32_t)args[0].as.i64,joint);
    } else if(semantic==NativeScriptBinding_EffectsWindAtPosition && context->source_class==NativeCallbackSource_Item) {
        NativeItemLifetime source;
        if(!nativeItemLifetime_Capture(object,&source) || !source.source_owned ||
           source.generation!=context->generation || source.reference.lifetime!=context->scene_epoch)
            return error(d,"Wind requires its live owned Item source");
        for(unsigned i=0;i<7;++i)if(i!=3&&!isfinite(args[i].as.f32))return error(d,"Wind position/scalar is not finite");
        Vec3 position={args[0].as.f32,args[1].as.f32,args[2].as.f32};
        lb_800119DC(&position,(int32_t)args[3].as.i64,args[4].as.f32,args[5].as.f32,args[6].as.f32);
        if(!nativeItemLifetime_Current(&source))return error(d,"Wind Item source, program or storage retired");
    } else {
        FeedbackSource source;if(!capture(context,&source,d))return false;
        Fighter* fp=source.fighter;
        switch(semantic) {
        case NativeScriptBinding_ReflectorDisable:fp->reflecting=false;break;
        case NativeScriptBinding_ReflectorRetain:fp->reflecting=true;break;
        case NativeScriptBinding_ReflectorUpdate:ftColl_8007AEF8(object);break;
        case NativeScriptBinding_ReflectorCreate: {
            if(!bone(fp,(uint32_t)args[0].as.u64,false,d))return false;
            for(unsigned i=2;i<8;++i)if(!isfinite(args[i].as.f32))return error(d,"Reflector scalar is not finite");
            ReflectDesc shape={.x0_bone_id=(uint32_t)args[0].as.u64,.x4_max_damage=(int32_t)args[1].as.i64,
                .x8_offset={args[2].as.f32,args[3].as.f32,args[4].as.f32},.x14_size=args[5].as.f32,.x18_damage_mul=args[6].as.f32,.x1C_speed_mul=args[7].as.f32,.x20_behavior=args[8].as.u64!=0};
            ftColl_CreateReflectHit(object,&shape,NULL);break;
        }
        case NativeScriptBinding_EffectsSpawn:
        case NativeScriptBinding_EffectsSpawnWithScalar:case NativeScriptBinding_EffectsSpawnOnJointWithScalar: {
            bool scalar=semantic==NativeScriptBinding_EffectsSpawnWithScalar||semantic==NativeScriptBinding_EffectsSpawnOnJointWithScalar;
            bool semantic_part=semantic==NativeScriptBinding_EffectsSpawn||semantic==NativeScriptBinding_EffectsSpawnWithScalar;
            if(args[0].as.i64<0||args[0].as.i64>UINT16_MAX||nativeScriptEffectDispatch_Arguments((int32_t)args[0].as.i64)!=
                (scalar?NativeScriptEffectArguments_JointScalar:NativeScriptEffectArguments_Joint))return error(d,"Effect dispatch argument ABI is incompatible");
            HSD_JObj* joint=bone(fp,(uint32_t)args[1].as.u64,semantic_part,d);if(!joint)return false;
            if(!current(&source,d))return false;
            if(scalar){float value=args[2].as.f32;if(!isfinite(value))return error(d,"Effect scalar is not finite");
                efSync_Spawn((int32_t)args[0].as.i64,object,joint,&value);}
            else efSync_Spawn((int32_t)args[0].as.i64,object,joint);
            break;
        }
        case NativeScriptBinding_EffectsWind: {
            HSD_JObj* joint=bone(fp,(uint32_t)args[0].as.u64,true,d);if(!joint)return false;
            for(unsigned i=1;i<6;++i)if(i!=2 && !isfinite(args[i].as.f32))return error(d,"Wind scalar is not finite");
            Vec3 position;lb_8000B1CC(joint,NULL,&position);
            if(!current(&source,d))return false;
            position.x+=args[1].as.f32*fp->facing_dir;
            lb_800119DC(&position,(int32_t)args[2].as.i64,args[3].as.f32,args[4].as.f32,args[5].as.f32);break;
        }
        case NativeScriptBinding_EffectsWindAtPosition: {
            for(unsigned i=0;i<7;++i)if(i!=3&&!isfinite(args[i].as.f32))return error(d,"Wind position/scalar is not finite");
            Vec3 position={args[0].as.f32,args[1].as.f32,args[2].as.f32};
            lb_800119DC(&position,(int32_t)args[3].as.i64,args[4].as.f32,args[5].as.f32,args[6].as.f32);break;
        }
        case NativeScriptBinding_EffectsSpawnAtPosition: {
            int32_t id;
            if(!effect_id(&args[0],&id)||nativeScriptEffectDispatch_Arguments(id)!=NativeScriptEffectArguments_Position)
                return error(d,"Effect dispatch does not accept the exact copied-position ABI");
            for(unsigned i=1;i<4;++i)if(!isfinite(args[i].as.f32))return error(d,"Effect position is not finite");
            Vec3 position={args[1].as.f32,args[2].as.f32,args[3].as.f32};
            efSync_Spawn(id,object,&position);break;
        }
        case NativeScriptBinding_EffectsSpawnWithScalarAtPosition:
        case NativeScriptBinding_EffectsSpawnWithTwoScalarsAtPosition: {
            int32_t id;uint32_t requirements;
            bool two=semantic==NativeScriptBinding_EffectsSpawnWithTwoScalarsAtPosition;
            if(!effect_id(&args[0],&id)||
               !(two?nativeScriptEffectDispatch_PositionTwoScalarRequirements(id,&requirements):
                     nativeScriptEffectDispatch_PositionScalarRequirements(id,&requirements)))
                return error(d,"Effect dispatch does not accept the exact copied-position/scalar ABI");
            for(unsigned i=1;i<count;++i)if(!isfinite(args[i].as.f32))
                return error(d,"Effect position/scalar is not finite");
            HSD_JObj* root=object->hsd_obj;
            HSD_ClassInfo* root_class=NULL;
            if(requirements&NativeScriptEffectPosition_Root){
                root_class=root?HSD_CLASS_METHOD(root):NULL;
                if(object->obj_kind!=HSD_GObj_JObjKind||!root||!root_class||
                   root_class->head.obj_size<(int)sizeof(*root)||
                   !hsdObjIsDescendantOf((HSD_Obj*)root,HSD_CLASS_INFO(&hsdJObj)))
                    return error(d,"Position/scalar effect requires its native creator JObj");
                if(!current(&source,d)||object->hsd_obj!=root||HSD_CLASS_METHOD(root)!=root_class)
                    return error(d,"Position/scalar effect creator root changed during class admission");
            }
            Vec3 position={args[1].as.f32,args[2].as.f32,args[3].as.f32};
            float scalar=args[4].as.f32;
            if(two){float second=args[5].as.f32;efSync_Spawn(id,object,&position,&scalar,&second);}
            else efSync_Spawn(id,object,&position,&scalar);
            if(!current(&source,d)||((requirements&NativeScriptEffectPosition_Root)&&
               (object->hsd_obj!=root||HSD_CLASS_METHOD(root)!=root_class)))
                return error(d,"Position/scalar effect source or creator root retired");
            break;
        }
        case NativeScriptBinding_EffectsSpawnAsyncAttached:
        case NativeScriptBinding_EffectsSpawnAsyncWithScalar: {
            bool scalar=semantic==NativeScriptBinding_EffectsSpawnAsyncWithScalar;
            int32_t id;
            if(!effect_id(&args[0],&id)||nativeScriptEffectDispatch_Arguments(id)!=
               (scalar?NativeScriptEffectArguments_JointScalar:NativeScriptEffectArguments_Joint))
                return error(d,"Async effect requires its exact joint/scalar dispatch ABI");
            if(scalar&&!isfinite(args[2].as.f32))return error(d,"Async effect scalar is not finite");
            uint32_t part=(uint32_t)args[1].as.u64;
            HSD_JObj* joint=bone(fp,part,true,d);if(!joint)return false;
            uint32_t index=source.package->assets.part_to_joint[part];
            HSD_ClassInfo* info=HSD_CLASS_METHOD(joint);
            if(!info||info->head.obj_size<(int)sizeof(*joint)||
               !hsdObjIsDescendantOf((HSD_Obj*)joint,HSD_CLASS_INFO(&hsdJObj)))
                return error(d,"Async effect joint lacks a native JObj class and extent");
            if(!current(&source,d)||source.package->assets.part_to_joint[part]!=index||
               source.parts[index].joint!=joint||HSD_CLASS_METHOD(joint)!=info||
               info->head.obj_size<(int)sizeof(*joint))
                return error(d,"Async effect joint changed during native admission");
            if(scalar) {
                float value=args[2].as.f32;
                efAsync_Spawn(object,&fp->x60C,EF_SPAWN_ATTACH_PARAM,(u32)id,joint,&value);
            } else efAsync_Spawn(object,&fp->x60C,EF_SPAWN_ATTACH,(u32)id,joint);
            if(!current(&source,d)||source.package->assets.part_to_joint[part]!=index||
               source.parts[index].joint!=joint||HSD_CLASS_METHOD(joint)!=info||
               info->head.obj_size<(int)sizeof(*joint))
                return error(d,"Async effect joint or source retired before publication");
            break;
        }
        default:return error(d,"Unknown fighter feedback operation");
        }
        if(!current(&source,d))return false;
    }
    if(!nativeScriptBindings_SourceAlive(context))return error(d,"Feedback source retired before publication");
    *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}
