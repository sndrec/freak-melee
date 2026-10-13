#include "native_script_binding_presentation.h"
#include "native_script_bindings.h"
#include "native_fighter_runtime.h"
#include "native_source_package.h"
#include "native_fighter_lifetime.h"
#include "fighter.h"
#include "ft_0877.h"
#include "ft_0881.h"
#include "ftparts.h"
#include "ftdat.h"
#include "types.h"
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <math.h>
#include <stdio.h>

static bool error(NativeScriptDiagnostic* diagnostic,const char* message)
{if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message);return false;}

static FighterPartsTable* parts(Fighter* fighter,NativeScriptDiagnostic* diagnostic)
{
    if((unsigned)fighter->kind>=Ft_Kind_RuntimeCapacity||!ftPartsTable||!ftPartsTable[fighter->kind]||!fighter->parts) {
        error(diagnostic,"Fighter part table or instance joints are absent.");return NULL;
    }
    FighterPartsTable* table=ftPartsTable[fighter->kind];
    const NativeSourcePackage* source=nativeFighterRuntime_SourcePackage(fighter->kind);
    if(source) {
        if(!source->assets.ready || fighter->ft_data!=&source->assets.character.data || table!=&source->assets.parts ||
           table->parts_num!=source->assets.facts.joints+source->assets.facts.omission_count ||
           !table->parts_num || table->parts_num>MAX_FT_PARTS) {
            error(diagnostic,"Fighter part table differs from the selected source rig.");return NULL;
        }
        return table;
    }
    if(!table->parts_num||table->parts_num>MAX_FT_PARTS) {
        error(diagnostic,"Fighter joint count disagrees with owned model storage.");return NULL;
    }
    return table;
}

static bool joint(Fighter* fighter,int64_t selector,bool semantic_part,bool rotation,
    uint32_t* result,NativeScriptDiagnostic* diagnostic)
{
    FighterPartsTable* table=parts(fighter,diagnostic);if(!table)return false;
    if(semantic_part) {
        if(selector<0||selector>=FT_COMMON_PART_COUNT||!table->part_to_joint)
            return error(diagnostic,"Semantic fighter part is outside the native part schema.");
        selector=ftParts_GetBoneIndex(fighter,(Fighter_Part)selector);
    }
    if(selector<0||(uint64_t)selector>=table->parts_num||!fighter->parts[selector].joint)
        return error(diagnostic,"Requested fighter part is unmapped or its joint is absent.");
    if(rotation&&(fighter->parts[selector].joint->flags&JOBJ_USE_QUATERNION)&&
       (!fighter->parts[selector].x4_jobj2||
        (fighter->parts[selector].x4_jobj2->flags&JOBJ_USE_QUATERNION)))
        return error(diagnostic,"Quaternion fighter rotation has no Euler copy joint.");
    *result=(uint32_t)selector;return true;
}

typedef struct BoneQuery {
    NativeFighterLifetime lifetime;
    NativeScriptRuntimeContext* context;
    const NativeSourcePackage* package;
    FighterPartsTable* table;
    FighterBone* parts;
    const u8* forward;
    const u8* reverse;
    uint32_t count,index;
    HSD_JObj* selected;
    HSD_JObj* parent;
    HSD_ClassInfo* info;
    void* root;
    HSD_ClassInfo* root_info;
    uint8_t root_kind;
} BoneQuery;

static bool query_current(const BoneQuery* query,NativeScriptDiagnostic* diagnostic)
{
    const NativeFighterLifetime* life=&query->lifetime;
    if(query->context->source!=life->object||query->context->source_class!=NativeCallbackSource_Fighter||
       query->context->generation!=life->generation||query->context->scene_epoch!=life->reference.lifetime||
       !nativeFighterLifetime_Current(life)||!nativeScriptBindings_SourceAlive(query->context)||
       nativeFighterRuntime_SourcePackage(life->kind)!=query->package||
       !ftPartsTable||ftPartsTable[life->kind]!=query->table||life->data->parts!=query->parts||
       query->table->parts_num!=query->count||query->table->part_to_joint!=query->forward||
       query->table->joint_to_part!=query->reverse||life->object->hsd_obj!=query->root||
       life->object->obj_kind!=query->root_kind)
        return error(diagnostic,"Bone query source, program, storage or rig changed");
    if(query->package && (!life->source_owned||
       nativeScriptBindings_SourceFighterLayout(query->context,diagnostic)!=&life->program->bank->motion_layout||
       query->table!=&query->package->assets.parts||
       query->forward!=query->package->assets.part_to_joint||query->reverse!=query->package->assets.joint_to_part||
       query->count!=query->package->assets.facts.joints+query->package->assets.facts.omission_count))
        return error(diagnostic,"Bone query no longer matches its selected source rig");
    if(query->selected && (query->parts[query->index].joint!=query->selected||
       HSD_CLASS_METHOD(query->selected)!=query->info||query->info->head.obj_size<(int)sizeof(*query->selected)||
       query->selected->parent!=query->parent))
        return error(diagnostic,"Bone query selected joint changed");
    if(query->root_info && (HSD_CLASS_METHOD((HSD_JObj*)query->root)!=query->root_info||
       query->root_info->head.obj_size<(int)sizeof(HSD_JObj)))
        return error(diagnostic,"Bone query root class or extent changed");
    return true;
}

static bool query_capture(NativeScriptRuntimeContext* context,BoneQuery* query,NativeScriptDiagnostic* diagnostic)
{
    if(!context||context->source_class!=NativeCallbackSource_Fighter||!context->scene_epoch||
       !nativeScriptBindings_SourceAlive(context))return error(diagnostic,"Bone query needs a live Fighter source");
    *query=(BoneQuery){.context=context};
    if(!nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&query->lifetime))
        return error(diagnostic,"Bone query Fighter lifetime is unavailable");
    query->package=nativeFighterRuntime_SourcePackage(query->lifetime.kind);
    query->table=parts(query->lifetime.data,diagnostic);if(!query->table)return false;
    query->parts=query->lifetime.data->parts;query->count=query->table->parts_num;
    query->forward=query->table->part_to_joint;query->reverse=query->table->joint_to_part;
    query->root=query->lifetime.object->hsd_obj;query->root_kind=query->lifetime.object->obj_kind;
    return query_current(query,diagnostic);
}

static bool query_root(NativeScriptRuntimeContext* context,BoneQuery* query,NativeScriptDiagnostic* diagnostic)
{
    if(!query_capture(context,query,diagnostic))return false;
    HSD_JObj* root=query->root;
    if(!root||query->root_kind!=HSD_GObj_JObjKind)
        return error(diagnostic,"Root scale requires a live Fighter JObj root");
    query->root_info=HSD_CLASS_METHOD(root);
    if(!query->root_info||query->root_info->head.obj_size<(int)sizeof(*root)||
       !hsdObjIsDescendantOf((HSD_Obj*)root,HSD_CLASS_INFO(&hsdJObj)))
        return error(diagnostic,"Root scale requires an admitted native JObj class and extent");
    // Native class admission can initialize a class and run nested code.
    // Retain both the Fighter rig and its actual GObj root, not a mapped TopN.
    return query_current(query,diagnostic);
}

static bool query_select(BoneQuery* query,const NativeScriptValue* value,bool semantic_part,
    NativeScriptDiagnostic* diagnostic)
{
    if(!value||value->type!=NativeScriptType_I32||value->as.i64<INT32_MIN||value->as.i64>INT32_MAX)
        return error(diagnostic,"Bone query selector needs a native signed Int");
    int64_t selected=value->as.i64;
    if(semantic_part) {
        if(selected<0||selected>=FT_COMMON_PART_COUNT||!query->forward)
            return error(diagnostic,"Semantic fighter part is outside the native part schema");
        selected=ftParts_GetBoneIndex(query->lifetime.data,(Fighter_Part)selected);
    }
    if(selected<0||(uint64_t)selected>=query->count||!query->parts[selected].joint)
        return error(diagnostic,"Requested fighter part is unmapped or its joint is absent");
    query->index=(uint32_t)selected;query->selected=query->parts[selected].joint;
    query->parent=query->selected->parent;query->info=HSD_CLASS_METHOD(query->selected);
    if(!query->info||query->info->head.obj_size<(int)sizeof(*query->selected)||
       !hsdObjIsDescendantOf((HSD_Obj*)query->selected,HSD_CLASS_INFO(&hsdJObj)))
        return error(diagnostic,"Bone query joint lacks a native JObj class and extent");
    return query_current(query,diagnostic);
}

bool nativeScriptBindingPresentation_Read(NativeScriptRuntimeContext* context,uint32_t semantic,
    NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    if(semantic>=NativeScriptBinding_BonesRootScaleX&&semantic<=NativeScriptBinding_BonesRootScaleZ) {
        BoneQuery query;if(!result||!query_root(context,&query,diagnostic))return false;
        HSD_JObj* root=query.root;
        float value=semantic==NativeScriptBinding_BonesRootScaleX?root->scale.x:
            semantic==NativeScriptBinding_BonesRootScaleY?root->scale.y:root->scale.z;
        if(!query_current(&query,diagnostic))return false;
        *result=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=value};return true;
    }
    if(semantic==NativeScriptBinding_BonesCount) {
        BoneQuery query;if(!result||!query_capture(context,&query,diagnostic))return false;
        *result=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=query.count};return true;
    }
    if(semantic==NativeScriptBinding_BonesRootRotationX||semantic==NativeScriptBinding_BonesRootRotationY) {
        if(!context||!result||context->source_class!=NativeCallbackSource_Fighter||
           !nativeScriptBindings_SourceAlive(context))
            return error(diagnostic,"Root rotation requires its live Fighter and an output");
        Fighter* fighter=((HSD_GObj*)context->source)->user_data;
        uint32_t index;if(!joint(fighter,0,false,true,&index,diagnostic))return false;
        float angle=semantic==NativeScriptBinding_BonesRootRotationX?
            ftPartGetRotX(fighter,(int)index):ftPartGetRotY(fighter,(int)index);
        if(!nativeScriptBindings_SourceAlive(context))return error(diagnostic,"Root rotation source retired before publication");
        *result=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=angle};return true;
    }
    return error(diagnostic,"Unsupported fighter presentation property.");
}

bool nativeScriptBindingPresentation_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    NativeScriptValue* values,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    if(semantic==NativeScriptBinding_BonesSetTranslation) {
        if(!values||count!=4||!result||values[0].type!=NativeScriptType_I32||
           values[0].as.i64<INT32_MIN||values[0].as.i64>INT32_MAX||
           values[1].type!=NativeScriptType_F32||values[2].type!=NativeScriptType_F32||
           values[3].type!=NativeScriptType_F32)
            return error(diagnostic,"Joint translation requires a signed Int raw joint and three Floats");
        BoneQuery query;if(!query_capture(context,&query,diagnostic))return false;
        if(!query.lifetime.source_owned)return error(diagnostic,"Joint translation requires its owned Fighter rig");
        // Native source operations ignore absent numeric slots. Do not remap
        // an exact costume index through the semantic role table a second time.
        if(values[0].as.i64<0||(uint64_t)values[0].as.i64>=query.count) {
            if(!query_current(&query,diagnostic))return false;
            *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
        }
        if(!query_select(&query,&values[0],false,diagnostic))return false;
        Vec3 translation={values[1].as.f32,values[2].as.f32,values[3].as.f32};
        HSD_JObjSetTranslate(query.selected,&translation);
        if(!query_current(&query,diagnostic))return false;
        *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    if(semantic==NativeScriptBinding_BonesRootSetScale) {
        if(!values||count!=3||!result||values[0].type!=NativeScriptType_F32||
           values[1].type!=NativeScriptType_F32||values[2].type!=NativeScriptType_F32)
            return error(diagnostic,"Root scale requires three typed Float values");
        BoneQuery query;if(!query_root(context,&query,diagnostic))return false;
        Vec3 scale={values[0].as.f32,values[1].as.f32,values[2].as.f32};
        HSD_JObjSetScale(query.root,&scale);
        if(!query_current(&query,diagnostic))return false;
        *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    if(semantic==NativeScriptBinding_BonesIndex||semantic==NativeScriptBinding_BonesWorldPosition||
       semantic==NativeScriptBinding_BonesWorldPositionWithOffset) {
        bool index_query=semantic==NativeScriptBinding_BonesIndex;
        bool offset_query=semantic==NativeScriptBinding_BonesWorldPositionWithOffset;
        if(!values||!result||count!=(index_query?1u:offset_query?7u:4u))
            return error(diagnostic,"Bone query argument count is invalid");
        if(!index_query)for(unsigned i=1;i<count;++i)
            if(values[i].type!=NativeScriptType_F32)return error(diagnostic,"Bone world-position arguments need Float slots");
        BoneQuery query;if(!query_capture(context,&query,diagnostic)||
           !query_select(&query,&values[0],index_query,diagnostic))return false;
        if(index_query){*result=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=query.index};return true;}
        Vec3 offset={0},position;
        if(offset_query)offset=(Vec3){values[4].as.f32,values[5].as.f32,values[6].as.f32};
        lb_8000B1CC(query.selected,offset_query?&offset:NULL,&position);
        if(!query_current(&query,diagnostic))return false;
        values[1]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=position.x};
        values[2]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=position.y};
        values[3]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=position.z};
        *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    Fighter* fighter=((HSD_GObj*)context->source)->user_data;
    if(semantic==NativeScriptBinding_SoundPlay||semantic==NativeScriptBinding_SoundLoop||semantic==NativeScriptBinding_SoundChannel) {
        unsigned first=semantic==NativeScriptBinding_SoundChannel?1:0;
        if(count!=first+3)return error(diagnostic,"Sound call argument count is invalid.");
        int32_t sound=(int32_t)values[first].as.i64;
        u8 volume=(u8)values[first+1].as.i64,pan=(u8)values[first+2].as.i64;
        int64_t channel=semantic==NativeScriptBinding_SoundPlay?0:semantic==NativeScriptBinding_SoundLoop?3:values[0].as.i64;
        switch(channel) {
        case 0:ft_PlaySFX(fighter,sound,volume,pan);break;
        case 1:ft_80088478(fighter,sound,volume,pan);break;
        case 2:ft_800881D8(fighter,sound,volume,pan);break;
        case 3:ft_80088510(fighter,sound,volume,pan);break;
        case 4:ft_800885A8(fighter,sound,volume,pan);break;
        case 5:ft_80088640(fighter,sound,volume,pan);break;
        case 6:ft_80088328(fighter,sound,volume,pan);break;
        default:return error(diagnostic,"Sound channel must be in 0..6.");
        }
        return true;
    }
    uint32_t index;
    if(semantic==NativeScriptBinding_BonesSetScale||semantic==NativeScriptBinding_BonesSetDirectAnimationCopy) {
        bool scale=semantic==NativeScriptBinding_BonesSetScale;
        if(!joint(fighter,values[0].as.i64,scale,false,&index,diagnostic))return false;
        if(scale){Vec3 value={values[1].as.f32,values[2].as.f32,values[3].as.f32};HSD_JObjSetScale(fighter->parts[index].joint,&value);}
        else fighter->parts[index].flags_b4=values[1].as.u64!=0;
        return true;
    }
    bool part=semantic==NativeScriptBinding_BonesRotateX||semantic==NativeScriptBinding_BonesRotateY||semantic==NativeScriptBinding_BonesRotateZ;
    bool explicit_joint=semantic==NativeScriptBinding_BonesRootRotateXWithJoint||semantic==NativeScriptBinding_BonesRootRotateYWithJoint||semantic==NativeScriptBinding_BonesRootRotateZWithJoint;
    int axis=semantic==NativeScriptBinding_BonesRootRotateX||semantic==NativeScriptBinding_BonesRootRotateXWithJoint||semantic==NativeScriptBinding_BonesRotateX?0:
        semantic==NativeScriptBinding_BonesRootRotateY||semantic==NativeScriptBinding_BonesRootRotateYWithJoint||semantic==NativeScriptBinding_BonesRotateY?1:
        semantic==NativeScriptBinding_BonesRootRotateZ||semantic==NativeScriptBinding_BonesRootRotateZWithJoint||semantic==NativeScriptBinding_BonesRotateZ||semantic==NativeScriptBinding_BonesRootResetZ?2:-1;
    if(axis<0)return error(diagnostic,"Unsupported fighter presentation operation.");
    if(!joint(fighter,part||explicit_joint?values[0].as.i64:0,part,true,&index,diagnostic))return false;
    float angle=semantic==NativeScriptBinding_BonesRootResetZ?0.0f:values[part||explicit_joint?1:0].as.f32;
    if(axis==0)ftPartSetRotX(fighter,(int)index,angle);
    else if(axis==1)ftPartSetRotY(fighter,(int)index,angle);
    else ftPartSetRotZ(fighter,(int)index,angle);
    return true;
}


static void geometry_nonlooping_rate(HSD_AObj* animation, float rate)
{
    if (animation && !(HSD_AObjGetFlags(animation) & AOBJ_LOOP))
        HSD_AObjSetRate(animation, rate);
}

bool nativeScriptBindingPresentation_AccessoryInvoke(NativeScriptRuntimeContext* context,
    uint32_t semantic, const NativeScriptValue* values, uint32_t count,
    NativeScriptValue* result, NativeScriptDiagnostic* diagnostic)
{
    bool create = semantic == NativeScriptBinding_AccessoryCreate;
    bool add = semantic == NativeScriptBinding_AccessoryAddAnimation;
    bool scale = semantic == NativeScriptBinding_AccessorySetScale;
    bool rate = semantic == NativeScriptBinding_AccessorySetGeometryRate;
    bool request = semantic == NativeScriptBinding_AccessoryRequestFrame;
    bool evaluate = semantic == NativeScriptBinding_AccessoryEvaluate;
    bool follow = semantic == NativeScriptBinding_AccessoryFollowBone;
    bool clear = semantic == NativeScriptBinding_AccessoryClear;
    uint32_t expected = scale ? 3 : (add || rate) ? 2 :
        (create || request || follow) ? 1 : 0;
    if ((!create && !add && !scale && !rate && !request && !evaluate && !follow && !clear) ||
        !result || count != expected || (count && !values))
        return error(diagnostic, "Accessory operation has an invalid signature");
    for (uint32_t i = 0; i < count; ++i) {
        if (create || add || follow) {
            if (values[i].type != NativeScriptType_I32 || values[i].as.i64 < INT32_MIN ||
                values[i].as.i64 > INT32_MAX)
                return error(diagnostic, "Accessory selectors require native signed Int values");
        } else if (rate && i == 1) {
            if (values[i].type != NativeScriptType_Bool || values[i].as.u64 > 1)
                return error(diagnostic, "Accessory loop policy requires Bool");
        } else if (values[i].type != NativeScriptType_F32 || !isfinite(values[i].as.f32))
            return error(diagnostic, "Accessory transform and timing require finite Float values");
    }
    BoneQuery query;
    if (!query_capture(context, &query, diagnostic)) return false;
    if (!query.lifetime.source_owned || !query.package)
        return error(diagnostic, "Accessory controls require an owned interpreted fighter");
    Fighter* fighter = query.lifetime.data;
    HSD_JObj* accessory = fighter->x20A0_accessory;
    const FtAccessoryAsset* recipe = NULL;
    unsigned slot = 0;
    if (create || add) {
        int64_t selected = values[0].as.i64;
        if (selected < 0 || selected >= FT_CHARACTER_ITEM_CAPACITY)
            return error(diagnostic, "Accessory asset slot is outside the owned table");
        slot = (unsigned)selected;
        const FtCharacterAssets* assets = &query.package->assets.character;
        recipe = assets->accessory_assets[slot];
        if (!recipe || assets->fighter_items[slot] != recipe ||
            !ftAccessoryAssets_Validate(&assets->import, recipe))
            return error(diagnostic, "Accessory asset has no owned typed recipe");
        if (add && (values[1].as.i64 < -1 ||
            values[1].as.i64 >= (int64_t)recipe->variant_count))
            return error(diagnostic, "Accessory animation selection is outside its recipe");
    }
    NativeScriptValue value = { .type = create ? NativeScriptType_Bool : NativeScriptType_Void };
    if (create) {
        if (accessory) { *result = value; return true; }
        HSD_JObj* loaded = HSD_JObjLoadJoint(recipe->root);
        if (!query_current(&query, diagnostic) || fighter->x20A0_accessory != NULL ||
            query.package->assets.character.accessory_assets[slot] != recipe) {
            if (loaded) HSD_JObjRemoveAll(loaded);
            return error(diagnostic, "Accessory source or slot changed during construction");
        }
        fighter->x20A0_accessory = loaded;
        value.as.u64 = loaded != NULL;
        *result = value;
        return true;
    }
    if (!accessory) {
        if (clear) { *result = value; return true; }
        return error(diagnostic, "Accessory operation requires a live model");
    }
    HSD_ClassInfo* info = HSD_CLASS_METHOD(accessory);
    if (!info || info->head.obj_size < (int)sizeof(*accessory) ||
        !hsdObjIsDescendantOf((HSD_Obj*)accessory, HSD_CLASS_INFO(&hsdJObj)) ||
        !query_current(&query, diagnostic) || fighter->x20A0_accessory != accessory)
        return error(diagnostic, "Accessory source, class or ownership changed");
    if (follow && !query_select(&query, &values[0], false, diagnostic)) return false;
    if (clear) {
        // Unpublish before disposal so nested fighter cleanup cannot free it twice.
        fighter->x20A0_accessory = NULL;
        HSD_JObjRemoveAll(accessory);
        if (!query_current(&query, diagnostic) || fighter->x20A0_accessory != NULL)
            return error(diagnostic, "Accessory source changed during disposal");
    } else {
        if (add) {
            const FtAccessoryAnimation* animation = values[1].as.i64 < 0 ?
                &recipe->base : &recipe->variants[(unsigned)values[1].as.i64];
            HSD_JObjAddAnimAll(accessory, animation->joint, animation->material, animation->shape);
        } else if (scale) {
            Vec3 size = { values[0].as.f32, values[1].as.f32, values[2].as.f32 };
            HSD_JObjSetScale(accessory, &size);
        } else if (rate) {
            // Spatial channels, including joint/shape/reference animation;
            // material and texture clocks are deliberately independent.
            HSD_ForeachAnim(accessory, JOBJ_TYPE, ALL_TYPE_MASK & ~TOBJ_MASK & ~MOBJ_MASK,
                values[1].as.u64 ? (void*)HSD_AObjSetRate : (void*)geometry_nonlooping_rate,
                AOBJ_ARG_AF, values[0].as.f32);
        } else if (request) HSD_JObjReqAnimAll(accessory, values[0].as.f32);
        else if (evaluate) HSD_JObjAnimAll(accessory);
        else if (follow) lb_8000C2F8(accessory, query.selected);
        if (!query_current(&query, diagnostic) || fighter->x20A0_accessory != accessory ||
            HSD_CLASS_METHOD(accessory) != info ||
            (recipe && query.package->assets.character.accessory_assets[slot] != recipe))
            return error(diagnostic, "Accessory identity changed during its operation");
    }
    *result = value;
    return true;
}
