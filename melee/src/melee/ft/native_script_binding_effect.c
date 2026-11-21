#include "native_script_binding_effect.h"
#include "native_script_effect_program.h"
#include "native_script_effect_dispatch.h"
#include "native_fighter_lifetime.h"
#include "native_source_package.h"
#include <melee/ef/efsync.h>
#include <melee/ef/efasync.h>
#include <melee/ef/eflib.h>
#include <melee/it/native_item_lifetime.h>
#include <melee/ft/ftdat.h>
#include <melee/ft/ftparts.h>
#include <melee/ef/native_script_effect_actor.h>
#include <melee/ef/native_model_effect.h>
#include <melee/lb/native_model_joint.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdio.h>
#include <math.h>

static bool error(NativeScriptDiagnostic* d,const char* text)
{if(d)snprintf(d->message,sizeof(d->message),"%s",text);return false;}

typedef struct EffectPartSource {
    NativeFighterLifetime lifetime;
    NativeScriptRuntimeContext* context;
    const NativeSourcePackage* package;
    FighterBone* parts;
    uint32_t count;
} EffectPartSource;

static bool part_current(const EffectPartSource* source,NativeScriptDiagnostic* d)
{
    const NativeSourcePackage* package=source->package;
    const NativeFighterLifetime* lifetime=&source->lifetime;
    if(!nativeFighterLifetime_Current(lifetime)||
       !nativeScriptBindings_SourceAlive(source->context)||
       nativeScriptBindings_SourceFighterLayout(source->context,d)!=&lifetime->program->bank->motion_layout||
       nativeFighterRuntime_SourcePackage(lifetime->kind)!=package||
       lifetime->data->parts!=source->parts||!ftPartsTable||
       ftPartsTable[lifetime->kind]!=&package->assets.parts||
       package->assets.parts.parts_num!=source->count||
       package->assets.parts.part_to_joint!=package->assets.part_to_joint||
       package->assets.parts.joint_to_part!=package->assets.joint_to_part)
        return error(d,"Effect source, program, storage or owned rig changed");
    return true;
}

static bool spawn_parts(NativeScriptRuntimeContext* context,NativeScriptValue* args,
                        uint32_t count,uint32_t semantic,NativeScriptDiagnostic* d)
{
    bool between=semantic==NativeScriptBinding_EffectsSpawnBetween;
    bool scalar=semantic==NativeScriptBinding_EffectsSpawnAsyncPositionScalarOnPart;
    bool deferred=scalar||semantic==NativeScriptBinding_EffectsSpawnAsyncPositionOnPart;
    if(count!=((between||scalar)?3u:2u)||!args||!context||context->source_class!=NativeCallbackSource_Fighter||
       !context->scene_epoch||args[1].type!=NativeScriptType_I32||
       args[1].as.i64<INT32_MIN||args[1].as.i64>INT32_MAX||
       (between&&(args[2].type!=NativeScriptType_I32||args[2].as.i64<INT32_MIN||args[2].as.i64>INT32_MAX)))
        return error(d,"Effect part invocation requires an integral ID and native signed part");
    if(scalar&&(args[2].type!=NativeScriptType_F32||!isfinite(args[2].as.f32)))
        return error(d,"Deferred position effect requires a finite Float scalar");
    uint64_t id;
    switch(args[0].type) {
    case NativeScriptType_I32:case NativeScriptType_I64:
        if(args[0].type==NativeScriptType_I32&&args[0].as.i64>INT32_MAX)
            return error(d,"Effect ID Int exceeds its declared width");
        if(args[0].as.i64<0)return error(d,"Effect ID is outside its native range");
        id=(uint64_t)args[0].as.i64;break;
    case NativeScriptType_Bool:
        if(args[0].as.u64>1)return error(d,"Effect ID Bool is malformed");
        id=args[0].as.u64;break;
    case NativeScriptType_U32:case NativeScriptType_U64:
        if(args[0].type==NativeScriptType_U32&&args[0].as.u64>UINT32_MAX)
            return error(d,"Effect ID UInt exceeds its declared width");
        id=args[0].as.u64;break;
    default:return error(d,"Effect ID requires an integral value");
    }
    NativeScriptEffectArguments expected=between?NativeScriptEffectArguments_TwoJoint:
        deferred?(scalar?NativeScriptEffectArguments_PositionScalar:NativeScriptEffectArguments_Position):
        NativeScriptEffectArguments_Joint;
    if(id>UINT16_MAX||nativeScriptEffectDispatch_Arguments((int32_t)id)!=expected)
        return error(d,"Effect dispatch does not accept the exact selected joint/position ABI");
    NativeMatchPoolReference reference;
    if(!nativeMatchPool_Reference(&nativeGObjPool,context->source,&reference)||
       reference.lifetime!=context->scene_epoch||
       !nativeScriptBindings_SourceFighterLayout(context,d))
        return error(d,"Effect requires its live selected source Fighter");
    EffectPartSource source={.context=context};
    if(!nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&source.lifetime)||
       !source.lifetime.source_owned||source.lifetime.generation!=context->generation||
       source.lifetime.reference.lifetime!=context->scene_epoch)
        return error(d,"Effect source identity or owned program is unavailable");
    source.package=nativeFighterRuntime_SourcePackage(source.lifetime.kind);
    source.parts=source.lifetime.data->parts;
    if(!source.package||!source.parts)return error(d,"Effect source rig is absent");
    source.count=source.package->assets.parts.parts_num;
    if(!source.count||source.count>MAX_FT_PARTS||
       source.package->assets.facts.joints>source.count||
       source.package->assets.facts.omission_count!=source.count-source.package->assets.facts.joints||
       !part_current(&source,d))return error(d,"Effect source rig count or mapping is invalid");
    unsigned selected_count=between?2u:1u;
    int32_t indices[2]={(int32_t)args[1].as.i64,between?(int32_t)args[2].as.i64:0};
    HSD_JObj* selected[2];HSD_ClassInfo* infos[2];
    // The canonical raw-part policy treats absent slots as healthy no-ops.
    // Check every slot before admitting classes or constructing either effect.
    for(unsigned i=0;i<selected_count;++i) {
        indices[i]=nativeFighterRig_Resolve(&source.package->assets.rig,indices[i]);
        if(indices[i]<0||(uint32_t)indices[i]>=source.count)return true;
        selected[i]=source.parts[indices[i]].joint;
        if(!selected[i])return true;
    }
    HSD_JObj* root=NULL;HSD_ClassInfo* root_info=NULL;
    // Synchronous joint recipes may read creator scale and retain the actual
    // Fighter parent for their update. Validate that root for single joints
    // too, including Attach_Scale_FacingDir's erased but typed JObj argument.
    if(!deferred) {
        root=source.lifetime.object->hsd_obj;
        if(!root||source.lifetime.object->obj_kind!=HSD_GObj_JObjKind)
            return error(d,"Joint effect requires its creator JObj root");
        root_info=HSD_CLASS_METHOD(root);
        if(!root_info||root_info->head.obj_size<(int)sizeof(*root)||
           !hsdObjIsDescendantOf((HSD_Obj*)root,HSD_CLASS_INFO(&hsdJObj)))
            return error(d,"Effect creator root lacks a native JObj class and extent");
    }
    for(unsigned i=0;i<selected_count;++i) {
        if(!part_current(&source,d))return false;
        infos[i]=HSD_CLASS_METHOD(selected[i]);
        if(!infos[i]||infos[i]->head.obj_size<(int)sizeof(*selected[i])||
           !hsdObjIsDescendantOf((HSD_Obj*)selected[i],HSD_CLASS_INFO(&hsdJObj)))
            return error(d,"Effect part lacks a native JObj class and extent");
    }
    // Class initialization and the synchronous animation drain may change the
    // creator. Retain both joints and the scale root through actual dispatch.
    for(unsigned stage=0;stage<2;++stage) {
        if(!part_current(&source,d))return false;
        if(!deferred&&(source.lifetime.object->hsd_obj!=root||
           source.lifetime.object->obj_kind!=HSD_GObj_JObjKind||
           HSD_CLASS_METHOD(root)!=root_info||root_info->head.obj_size<(int)sizeof(*root)))
            return error(d,"Effect creator root changed before publication");
        for(unsigned i=0;i<selected_count;++i)
            if(source.parts[indices[i]].joint!=selected[i]||
               HSD_CLASS_METHOD(selected[i])!=infos[i]||infos[i]->head.obj_size<(int)sizeof(*selected[i]))
                return error(d,"Effect part changed before publication");
        if(stage==0) {
            if(deferred) {
                if(scalar){float value=args[2].as.f32;efAsync_Spawn(source.lifetime.object,
                    &source.lifetime.data->x60C,EF_SPAWN_POS_PARAM,(u32)id,selected[0],&value);}
                else efAsync_Spawn(source.lifetime.object,&source.lifetime.data->x60C,
                    EF_SPAWN_POS,(u32)id,selected[0]);
            } else if(between)efSync_Spawn((s32)id,source.lifetime.object,selected[0],selected[1]);
            else efSync_Spawn((s32)id,source.lifetime.object,selected[0]);
        }
    }
    return true;
}
static bool joint(NativeScriptRuntimeContext* context,uint32_t semantic,const NativeScriptValue* selectors,uint32_t count,
    HSD_JObj** output,const NativeScriptEffectSelector** recipe,NativeScriptDiagnostic* d)
{
    if(!context||context->source_class!=NativeCallbackSource_ModelEffect||nativeScriptEffect_Status(context->source)!=NativeScriptVM_Ok)
        return error(d,"Effect source retired/faulted");
    const NativeScriptEffectProgram* p=nativeScriptEffect_Spec(context->source);if(!p)return error(d,"Missing effect program");
    unsigned low=0,high=p->selector_count;while(low<high){unsigned mid=low+(high-low)/2;if(p->selectors[mid].semantic<semantic)low=mid+1;else high=mid;}
    if(low==p->selector_count||p->selectors[low].semantic!=semantic)return error(d,"Effect selector is not in its exact prepared owner");
    const NativeScriptEffectSelector* selected=&p->selectors[low];
    if(count!=selected->argument_count||!selectors||selectors[0].type!=NativeScriptType_Text||
       selectors[0].as.text.offset!=selected->path_offset||selectors[0].as.text.length!=selected->path_length)
        return error(d,"Effect selector differs from its prepared literal path");
    if(count==2&&(!nativeScriptExpression_IsIntegral(selectors[1].type)||selectors[1].as.u64!=selected->member))return error(d,"Effect member differs from its prepared ordinal");
    HSD_JObj* node=nativeScriptEffect_Joint(context->source,context->generation,selected->member);
    for(unsigned depth=0;node&&depth<selected->depth;++depth){node=node->child;for(unsigned sibling=0;node&&sibling<selected->path[depth];++sibling)node=node->next;}
    *output=node;*recipe=selected;return true;
}
bool nativeScriptBindingEffect_Read(NativeScriptRuntimeContext* context,uint32_t semantic,const NativeScriptValue* selectors,uint32_t count,NativeScriptValue* output,NativeScriptDiagnostic* d)
{
    HSD_JObj* node;const NativeScriptEffectSelector* selected;if(!output||!joint(context,semantic,selectors,count,&node,&selected,d))return false;
    float value=nativeModelJoint_Read(node,selected->channel);
    if(selected->channel==10)*output=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=value!=0};
    else *output=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=value};return true;
}
bool nativeScriptBindingEffect_Write(NativeScriptRuntimeContext* context,uint32_t semantic,const NativeScriptValue* selectors,uint32_t count,const NativeScriptValue* value,NativeScriptDiagnostic* d)
{
    HSD_JObj* node;const NativeScriptEffectSelector* selected;if(!value||!joint(context,semantic,selectors,count,&node,&selected,d))return false;
    NativeScriptValue converted;if(!nativeScriptValue_Convert(value,selected->channel==10?NativeScriptType_Bool:NativeScriptType_F32,&converted,d))return false;
    float scalar=selected->channel==10?(float)converted.as.u64:converted.as.f32;if(!isfinite(scalar))return error(d,"Effect joint write requires a finite scalar");
    nativeModelJoint_Write(node,selected->channel,scalar);return nativeScriptEffect_Status(context->source)==NativeScriptVM_Ok;
}
bool nativeScriptBindingEffect_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,const NativeScriptValue* selectors,uint32_t selector_count,
    NativeScriptValue* args,uint32_t count,NativeScriptValue* output,NativeScriptDiagnostic* d)
{
    (void)selectors;if(!context||!output||selector_count)return error(d,"Effect invocation has invalid native inputs");
    if(semantic==NativeScriptBinding_EffectsSpawnOnPart||semantic==NativeScriptBinding_EffectsSpawnBetween||
       semantic==NativeScriptBinding_EffectsSpawnAsyncPositionOnPart||
       semantic==NativeScriptBinding_EffectsSpawnAsyncPositionScalarOnPart) {
        if(!spawn_parts(context,args,count,semantic,d))return false;
    } else if ((semantic & NATIVE_SCRIPT_EFFECT_PROPERTY_MASK) == NATIVE_SCRIPT_EFFECT_SPAWN_ITEM_POSITION) {
        const uint32_t owner = semantic & NATIVE_SCRIPT_EFFECT_INDEX_MASK;
        if (!args || count != 5 || args[0].type != NativeScriptType_I32 || args[0].as.i64 != owner)
            return error(d, "Model effect differs from its prepared source owner");
        for (unsigned i = 1; i < count; ++i) {
            if (args[i].type != NativeScriptType_F32 || !isfinite(args[i].as.f32))
                return error(d, "Model effect position and roll must be finite");
        }
        NativeItemLifetime item;
        if (context->source_class != NativeCallbackSource_Item ||
            !nativeItemLifetime_Capture(context->source, &item) || !item.source_owned ||
            item.removing || item.generation != context->generation ||
            item.reference.lifetime != context->scene_epoch || !nativeScriptBindings_SourceAlive(context))
            return error(d, "Model effect requires a live authored article");
        const NativeScriptEffectProgram* effect = nativeScriptEffect_Program(item.program->author_kind, owner);
        if (!effect || effect->bank != item.bank || effect->family != EF_MODEL_POSITION ||
            effect->update != UINT32_MAX || !effect->model_path.length || !effect->model.model_desc.joint)
            return error(d, "Article model effect requires a packaged position effect with no update");
        EF_Effect* spawned = efLib_Create(effect->gfx_id, item.object, &effect->model);
        if (spawned) {
            HSD_JObj* root = spawned->gobj->hsd_obj;
            Vec3 position = {args[1].as.f32, args[2].as.f32, args[3].as.f32};
            HSD_JObjSetTranslate(root, &position);
            nativeModelJoint_SetRoll(root, args[4].as.f32);
        }
        if (!nativeItemLifetime_Current(&item) || !nativeScriptBindings_SourceAlive(context))
            return error(d, "Article retired during model effect spawn");
    } else if((semantic&NATIVE_SCRIPT_EFFECT_PROPERTY_MASK)==NATIVE_SCRIPT_EFFECT_SPAWN ||
              (semantic&NATIVE_SCRIPT_EFFECT_PROPERTY_MASK)==NATIVE_SCRIPT_EFFECT_SPAWN_ON_PART||
              (semantic&NATIVE_SCRIPT_EFFECT_PROPERTY_MASK)==NATIVE_SCRIPT_EFFECT_SPAWN_DEFERRED_POSITION) {
        uint32_t owner=semantic&NATIVE_SCRIPT_EFFECT_INDEX_MASK;
        bool deferred=(semantic&NATIVE_SCRIPT_EFFECT_PROPERTY_MASK)==NATIVE_SCRIPT_EFFECT_SPAWN_DEFERRED_POSITION;
        bool on_part=deferred||(semantic&NATIVE_SCRIPT_EFFECT_PROPERTY_MASK)==NATIVE_SCRIPT_EFFECT_SPAWN_ON_PART;
        if(count!=(on_part?2u:1u)||!args||args[0].type!=NativeScriptType_I32||args[0].as.i64!=owner)return error(d,"Effect spawn differs from its prepared source owner");
        if(on_part&&(args[1].type!=NativeScriptType_I32||args[1].as.i64<INT32_MIN||args[1].as.i64>INT32_MAX))return error(d,"Effect origin requires a signed raw rig part");
        if(nativeScriptEffect_Spawn(context,owner,on_part,on_part?(int32_t)args[1].as.i64:0,deferred,d)!=NativeScriptVM_Ok)return false;
    } else if(semantic==NATIVE_SCRIPT_EFFECT_CANCEL) {
        if(count||context->source_class!=NativeCallbackSource_ModelEffect)return error(d,"Effect cancel requires its direct source view");
        EF_Effect* effect=nativeModelEffect_Resolve(context->source,context->generation);if(!effect)return error(d,"Effect source retired before cancel");nativeModelEffect_Cancel(effect);
    } else return error(d,"Unknown direct effect operation");
    *output=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}
