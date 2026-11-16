#include "ftdynamics.h"

#include <Runtime/platform.h>

#include <placeholder.h>
#include <stddef.h>

#include "forward.h"
#include "ftcoll.h"
#include "inlines.h"
#include "native_fighter_runtime.h"
#ifdef MELEE_NATIVE
#include "native_fighter_lifetime.h"
#include "native_source_package.h"
#include "native_script_bindings.h"
#include <sysdolphin/baselib/class.h>
#endif
#include "kinds/ftKirby/ftkirby.h"
#include "types.h"
#include <melee/gr/ground.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lb_00F9.h>
#include <melee/lb/dynamics_state_internal.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/jobj.h>

/* 09CB40 */ static void ftCo_8009CB40(Fighter* fp, ssize_t bone_idx, bool,
                                       FigaTree*);
/* 09DD94 */ static void ftCo_8009DD94(Fighter_GObj*, bool);
/* 09E1D4 */ static HSD_JObj* ftCo_8009E1D4(HSD_JObj*, HSD_JObj*, HSD_JObj*);
/* 09E4A8 */ static void ftCo_8009E4A8(Fighter* fp);
/* 09E614 */ static void ftCo_8009E614(Fighter* fp);

#ifdef MELEE_NATIVE
typedef struct NativeDynamicsLifetime {
    NativeFighterLifetime actor;
    HSD_JObj* root;
    HSD_ClassInfo* root_class;
    BoneDynamicsDesc sets[Ft_Dynamics_NumMax];
    // The actual native dynamics allocator contains this many nodes globally.
    struct { struct DynamicsData* data; struct DynamicsData* next; HSD_JObj* joint; HSD_ClassInfo* joint_class; }
        nodes[DYNAMICS_NODE_COUNT];
    uint32_t node_count,attachment_mask;
    int count;
    uint8_t object_kind,collider_count;
} NativeDynamicsLifetime;

static bool nativeDynamics_RootCurrent(const NativeDynamicsLifetime* saved)
{
    const NativeFighterLifetime* actor=&saved->actor;
    if(nativeMatchPool_Resolve(actor->reference)!=actor->object ||
       actor->reference.lifetime!=nativeGObjPool.lifetime ||
       actor->object->classifier!=HSD_GOBJ_CLASS_FIGHTER ||
       actor->object->user_data!=actor->data ||
       !nativeCallbackSource_Alive(actor->object,actor->generation,NativeCallbackSource_Fighter) ||
       actor->data->kind!=actor->kind || actor->data->ft_data!=actor->fighter_data ||
       actor->object->hsd_obj!=saved->root || actor->object->obj_kind!=saved->object_kind)
        return false;
    return !saved->count || HSD_CLASS_METHOD(saved->root)==saved->root_class;
}

static bool nativeDynamics_Current(const NativeDynamicsLifetime* saved)
{
    if(!nativeFighterLifetime_Current(&saved->actor) || !nativeDynamics_RootCurrent(saved))return false;
    Fighter* fp=saved->actor.data;
    if(fp->dynamics_num!=saved->count || fp->x166C!=saved->collider_count ||
       fp->native_attachment_dynamic_mask!=saved->attachment_mask)return false;
    for(int i=0;i<saved->count;++i)
        if(fp->dynamic_bone_sets[i].bone_id!=saved->sets[i].bone_id ||
           fp->dynamic_bone_sets[i].dyn_desc.data!=saved->sets[i].dyn_desc.data ||
           fp->dynamic_bone_sets[i].dyn_desc.count!=saved->sets[i].dyn_desc.count)return false;
    for(uint32_t i=0;i<saved->node_count;++i)
        if(saved->nodes[i].data->next!=saved->nodes[i].next ||
           saved->nodes[i].data->desc.lb_unk0.jobj!=saved->nodes[i].joint ||
           HSD_CLASS_METHOD(saved->nodes[i].joint)!=saved->nodes[i].joint_class)return false;
    return true;
}

static bool nativeDynamics_CaptureActor(Fighter* fp,NativeFighterLifetime* actor)
{
    if(!nativeFighterLifetime_Capture(fp,actor))return false;
    if(!actor->source_owned && nativeFighterRuntime_SourcePackage(fp->kind))return false;
    if(actor->source_owned) {
        NativeScriptRuntimeContext context={.source=fp->gobj,.source_class=NativeCallbackSource_Fighter,
            .generation=actor->generation,.scene_epoch=actor->reference.lifetime};
        if(nativeScriptBindings_SourceFighterLayout(&context,NULL)!=&actor->program->bank->motion_layout)return false;
    }
    return nativeFighterLifetime_Current(actor);
}

static bool nativeDynamics_Capture(Fighter* fp,NativeDynamicsLifetime* saved)
{
    NativeDynamicsLifetime value={0};
    if(!nativeDynamics_CaptureActor(fp,&value.actor))return false;
    value.count=fp->dynamics_num;
    if(value.count<0 || value.count>=Ft_Dynamics_NumMax)return false;
    value.root=fp->gobj->hsd_obj;value.object_kind=fp->gobj->obj_kind;
    if(value.count) {
        if(!value.root || value.object_kind!=HSD_GObj_JObjKind)return false;
        value.root_class=HSD_CLASS_METHOD(value.root);
        if(!value.root_class || value.root_class->head.obj_size<(int)sizeof(HSD_JObj) ||
           !hsdObjIsDescendantOf((HSD_Obj*)value.root,HSD_CLASS_INFO(&hsdJObj)))return false;
    }
    value.collider_count=fp->x166C;value.attachment_mask=fp->native_attachment_dynamic_mask;
    for(int i=0;i<value.count;++i) {
        value.sets[i]=fp->dynamic_bone_sets[i];
        struct DynamicsData* node=value.sets[i].dyn_desc.data;
        if(stage_info.grkind==Gr_Kind_Flatzone && !node)return false;
        for(uint32_t n=0;n<value.sets[i].dyn_desc.count;++n) {
            if(!node || !node->desc.lb_unk0.jobj || value.node_count==ARRAY_SIZE(value.nodes))return false;
            value.nodes[value.node_count].data=node;value.nodes[value.node_count].next=node->next;
            value.nodes[value.node_count].joint=node->desc.lb_unk0.jobj;
            HSD_ClassInfo* joint_class=HSD_CLASS_METHOD(node->desc.lb_unk0.jobj);
            if(!joint_class || joint_class->head.obj_size<(int)sizeof(HSD_JObj) ||
               !hsdObjIsDescendantOf((HSD_Obj*)node->desc.lb_unk0.jobj,HSD_CLASS_INFO(&hsdJObj)))return false;
            value.nodes[value.node_count++].joint_class=joint_class;
            node=node->next;
        }
        if(node)return false;
    }
    if(!nativeDynamics_Current(&value))return false;*saved=value;return true;
}
#endif

static inline void ftCo_8009CB40_inline(struct DynamicsData* data)
{
    data->desc.lb_unk0.unk_2C.x = data->desc.lb_unk0.jobj->mtx[0][3];
    data->desc.lb_unk0.unk_2C.y = data->desc.lb_unk0.jobj->mtx[1][3];
    data->desc.lb_unk0.unk_2C.z = data->desc.lb_unk0.jobj->mtx[2][3];
    data->desc.lb_unk0.unk_44 = 0.0f;
    HSD_JObjSetTranslate(data->desc.lb_unk0.jobj,
                         &data->desc.lb_unk0.translate);
    HSD_JObjSetScale(data->desc.lb_unk0.jobj, &data->desc.lb_unk0.scale);
}

void ftCo_8009CB40(Fighter* fp, ssize_t bone_idx, bool arg2, FigaTree* arg3)
{
    ftDynamics* dyn = fp->ft_data->x2C;
#ifdef MELEE_NATIVE
    if (fp->native_attachment_dynamic_mask & (1u << bone_idx)) return;
#endif
    PAD_STACK(8);
    if (dyn->dynamicsNum != 0) {
        s32 bone_id;
        s32 flag;
        HSD_JObj* var_r30;
        s32 var_r29;
        struct DynamicsData* data;
        s32 inverse_flag;
        bone_id = dyn->ftDynamicBones->array[bone_idx].bone_id;
        flag = arg2 == 0 ? 0 : 1;
        var_r30 = fp->parts[bone_id].joint;
        var_r29 = 0;
        if (arg2 == 1) {
            fp->dynamic_bone_sets[bone_idx].bone_id = (enum_t) (ssize_t) arg3;
        } else {
            fp->dynamic_bone_sets[bone_idx].bone_id = 0x100;
        }
        data = fp->dynamic_bone_sets[bone_idx].dyn_desc.data;
        {
            s32 tmp = 1 - flag;
            inverse_flag = tmp;
        }
        while (data != NULL) {
            if (var_r29 < (s32) arg3) {
                if (inverse_flag != 0 && !(fp->parts[bone_id].flags_b0)) {
                    ftCo_8009CB40_inline(data);
                }
                fp->parts[bone_id].flags_b0 = inverse_flag;
            } else {
                if (flag != 0 && !(fp->parts[bone_id].flags_b0)) {
                    ftCo_8009CB40_inline(data);
                }
                fp->parts[bone_id].flags_b0 = flag;
            }
            if (fp->parts[bone_id].flags_b0) {
                HSD_JObjClearFlags(var_r30, 0x20000U);
            }
            var_r30 = HSD_JObjGetChild(var_r30);
            data = data->next;
            var_r29++;
            bone_id++;
        }
    }
}

void ftCo_8009CF84(Fighter* fp)
{
    ftData* data = fp->ft_data;
    fp->dynamics_num = data->x2C->dynamicsNum;
    if (fp->dynamics_num >= Ft_Dynamics_NumMax) {
        HSD_ASSERTREPORT(109, 0, "fighter dynamics num over!\n");
    }
    {
        ssize_t i;
        for (i = 0; i < data->x2C->dynamicsNum; i++) {
            BoneDynamicsDesc* bones = &data->x2C->ftDynamicBones->array[i];
            lb_8000FD48(fp->parts[bones->bone_id].joint,
                        &fp->dynamic_bone_sets[i].dyn_desc,
                        bones->dyn_desc.count);
            fp->dynamic_bone_sets[i].bone_id = 0;
            ftCo_8009CB40(fp, i, 1, NULL);
            lb_80011710(&data->x2C->ftDynamicBones->array[i].dyn_desc,
                        &fp->dynamic_bone_sets[i].dyn_desc);
        }
    }
}

/// Attach one hat bone dynamics descriptor to @p fp's dynamic bone sets.
static inline void ftCo_SetupKirbyHatBone(Fighter* fp, KirbyCopyData* hat,
                                          int dyn_idx, ssize_t i)
{
    BoneDynamicsDesc* article =
        &hat->resources[dyn_idx].dynamics->ftDynamicBones->array[i];
    HSD_JObj* cur = fp->u.kb.hat.jobj;
    ssize_t j;
    for (j = 0; j < (signed) article->bone_id; j++) {
        if (cur->child != NULL) {
            cur = cur->child;
            continue;
        }
        while (cur->next == NULL) {
            cur = cur->parent;
        }
        cur = cur->next;
    }
    lb_8000FD48(cur, &fp->dynamic_bone_sets[i].dyn_desc,
                article->dyn_desc.count);
    fp->dynamic_bone_sets[i].bone_id = 0;
    lb_80011710(
        &hat->resources[dyn_idx].dynamics->ftDynamicBones->array[i].dyn_desc,
        &fp->dynamic_bone_sets[i].dyn_desc);
}

void ftCo_8009D074(Fighter* fp)
{
    KirbyCopyData* hat = ft_80459B88.hats[Ft_Kind_Koopa];
    fp->dynamics_num = hat->resources[2].dynamics->dynamicsNum;
    HSD_ASSERTREPORT(135, fp->dynamics_num < Ft_Dynamics_NumMax,
                     "fighter dynamics num over!\n");
    {
        ssize_t i;
        for (i = 0; i < fp->dynamics_num; i++) {
            ftCo_SetupKirbyHatBone(fp, hat, 2, i);
        }
    }
}

void ftCo_8009D18C(Fighter* fp)
{
    KirbyCopyData* hat = ft_80459B88.hats[Ft_Kind_Zelda];
    fp->dynamics_num = hat->resources[2].dynamics->dynamicsNum;
    HSD_ASSERTREPORT(167, fp->dynamics_num < Ft_Dynamics_NumMax,
                     "fighter dynamics num over!\n");
    {
        ssize_t i;
        for (i = 0; i < fp->dynamics_num; i++) {
            ftCo_SetupKirbyHatBone(fp, hat, 2, i);
        }
    }
}

void ftCo_8009D2A4(Fighter* fp)
{
    KirbyCopyData* hat = ft_80459B88.hats[Ft_Kind_Nana];
    fp->dynamics_num = hat->resources[2].dynamics->dynamicsNum;
    HSD_ASSERTREPORT(199, fp->dynamics_num < Ft_Dynamics_NumMax,
                     "fighter dynamics num over!\n");
    {
        ssize_t i;
        for (i = 0; i < fp->dynamics_num; i++) {
            ftCo_SetupKirbyHatBone(fp, hat, 2, i);
        }
    }
}

void ftCo_8009D3BC(Fighter* fp)
{
    KirbyCopyData* hat = ft_80459B88.hats[Ft_Kind_Falco];
    fp->dynamics_num = hat->resources[2].dynamics->dynamicsNum;
    HSD_ASSERTREPORT(232, fp->dynamics_num < Ft_Dynamics_NumMax,
                     "fighter dynamics num over!\n");
    {
        ssize_t i;
        for (i = 0; i < fp->dynamics_num; i++) {
            ftCo_SetupKirbyHatBone(fp, hat, 2, i);
        }
    }
}

void ftCo_8009D4D4(Fighter* fp)
{
    KirbyCopyData* hat = ft_80459B88.hats[Ft_Kind_Kirby];
    fp->dynamics_num = hat->resources[1].dynamics->dynamicsNum;
    HSD_ASSERTREPORT(265, fp->dynamics_num < Ft_Dynamics_NumMax,
                     "fighter dynamics num over!\n");
    {
        ssize_t i;
        for (i = 0; i < fp->dynamics_num; i++) {
            ftCo_SetupKirbyHatBone(fp, hat, 1, i);
        }
    }
}

void ftCo_8009D5EC(Fighter* fp)
{
    KirbyCopyData* hat = ft_80459B88.hats[Ft_Kind_Mars];
    fp->dynamics_num = hat->resources[0].dynamics->dynamicsNum;
    HSD_ASSERTREPORT(298, fp->dynamics_num < Ft_Dynamics_NumMax,
                     "fighter dynamics num over!\n");
    {
        ssize_t i;
        for (i = 0; i < fp->dynamics_num; i++) {
            ftCo_SetupKirbyHatBone(fp, hat, 0, i);
        }
    }
}

void ftCo_8009D704(Fighter* fp)
{
    KirbyCopyData* hat = ft_80459B88.hats[Ft_Kind_Link];
    fp->dynamics_num = hat->resources[2].dynamics->dynamicsNum;
    HSD_ASSERTREPORT(331, fp->dynamics_num < Ft_Dynamics_NumMax,
                     "fighter dynamics num over!\n");
    {
        ssize_t i;
        for (i = 0; i < fp->dynamics_num; i++) {
            ftCo_SetupKirbyHatBone(fp, hat, 2, i);
        }
    }
}

void ftCo_8009D81C(Fighter* fp)
{
    KirbyCopyData* hat = ft_80459B88.hats[Ft_Kind_Yoshi];
    PAD_STACK(2 * 4);
    fp->dynamics_num = hat->resources[3].dynamics->dynamicsNum;
    HSD_ASSERTREPORT(364, fp->dynamics_num < Ft_Dynamics_NumMax,
                     "fighter dynamics num over!\n");
    {
        ssize_t i;
        for (i = 0; i < fp->dynamics_num; i++) {
            s32 bone_id =
                hat->resources[3].dynamics->ftDynamicBones->array[i].bone_id;
            fp->parts[bone_id].flags_b0 = true;
            lb_8000FD48(fp->parts[bone_id].joint,
                        &fp->dynamic_bone_sets[i].dyn_desc,
                        hat->resources[3]
                            .dynamics->ftDynamicBones->array[i]
                            .dyn_desc.count);
            fp->dynamic_bone_sets[i].bone_id = FtPart_TopN;
            lb_80011710(
                &hat->resources[3].dynamics->ftDynamicBones->array[i].dyn_desc,
                &fp->dynamic_bone_sets[i].dyn_desc);
        }
    }
}

void ftCo_8009D920(Fighter* fp)
{
    KirbyCopyData* hat = ft_80459B88.hats[Ft_Kind_Luigi];
    fp->dynamics_num = hat->resources[1].dynamics->dynamicsNum;
    HSD_ASSERTREPORT(388, fp->dynamics_num < Ft_Dynamics_NumMax,
                     "fighter dynamics num over!\n");
    {
        ssize_t i;
        for (i = 0; i < fp->dynamics_num; i++) {
            ftCo_SetupKirbyHatBone(fp, hat, 1, i);
        }
    }
}

void ftCo_8009DA38(Fighter* fp)
{
    KirbyCopyData* hat = ft_80459B88.hats[Ft_Kind_Ganon];
    fp->dynamics_num = hat->resources[1].dynamics->dynamicsNum;
    HSD_ASSERTREPORT(421, fp->dynamics_num < Ft_Dynamics_NumMax,
                     "fighter dynamics num over!\n");
    {
        ssize_t i;
        for (i = 0; i < fp->dynamics_num; i++) {
            ftCo_SetupKirbyHatBone(fp, hat, 1, i);
        }
    }
}

void ftCo_8009DB50(Fighter* fp)
{
    KirbyCopyData* hat = ft_80459B88.hats[Ft_Kind_Purin];
    PAD_STACK(2 * 4);
    fp->dynamics_num = hat->resources[4].dynamics->dynamicsNum;
    HSD_ASSERTREPORT(455, fp->dynamics_num < Ft_Dynamics_NumMax,
                     "fighter dynamics num over!\n");
    {
        ssize_t i;
        for (i = 0; i < fp->dynamics_num; i++) {
            s32 bone_id =
                hat->resources[4].dynamics->ftDynamicBones->array[i].bone_id;
            fp->parts[bone_id].flags_b0 = true;
            lb_8000FD48(fp->parts[bone_id].joint,
                        &fp->dynamic_bone_sets[i].dyn_desc,
                        hat->resources[4]
                            .dynamics->ftDynamicBones->array[i]
                            .dyn_desc.count);
            fp->dynamic_bone_sets[i].bone_id = FtPart_TopN;
            lb_80011710(
                &hat->resources[4].dynamics->ftDynamicBones->array[i].dyn_desc,
                &fp->dynamic_bone_sets[i].dyn_desc);
        }
    }
}

void ftCo_8009DC54(Fighter* fp)
{
    DynamicsDesc* desc;
    ssize_t i;
    ssize_t idx;
    ftData* data = fp->ft_data;
    if (fp->x619_costume_id == 2) {
        idx = 0;
    } else if (fp->x619_costume_id == 3) {
        idx = 1;
    } else {
        return;
    }
    fp->dynamics_num = 3;
    HSD_ASSERTREPORT(490, fp->dynamics_num < Ft_Dynamics_NumMax,
                     "fighter dynamics num over!\n");
    {
        ssize_t bone_idx = idx * 2 + 1;
        ssize_t dyn_idx = 0;
        i = 0;
        do {
            // The original 32-bit Kirby/Purin layouts aliased this field.
            // Their pointer offsets differ on the native 64-bit build.
            HSD_JObj* cur = fp->u.pr.x223C;
            ftDynamics* dynamics = data->x2C;
            ArticleDynamicBones* bones = dynamics->ftDynamicBones;
            ssize_t j;
            for (j = 0;
                 j <
                 (signed) dynamics->ftDynamicBones->array[bone_idx].bone_id;
                 j++)
            {
                if (cur->child != NULL) {
                    cur = cur->child;
                    continue;
                }
                while (cur->next == NULL) {
                    cur = cur->parent;
                }
                cur = cur->next;
            }
            {
                desc = &fp->dynamic_bone_sets[i + 1].dyn_desc;
                lb_8000FD48(cur, desc, bones->array[bone_idx].dyn_desc.count);
                fp->dynamic_bone_sets[dyn_idx + 1].bone_id = FtPart_TopN;
                lb_80011710(
                    &data->x2C->ftDynamicBones->array[bone_idx].dyn_desc,
                    desc);
            }
            i++;
            bone_idx++;
            dyn_idx++;
        } while (i < 2);
    }
}

void ftCo_8009DD94(Fighter_GObj* gobj, bool arg1)
{
    Fighter* fp = GET_FIGHTER(gobj);
#ifdef MELEE_NATIVE
    NativeDynamicsLifetime saved;
    if(!nativeDynamics_Capture(fp,&saved))return;
    bool restore_scale=false;
#endif
    ssize_t dynamics_num = fp->dynamics_num;
    HSD_JObj* cur;
    ssize_t i;
    BoneDynamicsDesc* dyn = fp->dynamic_bone_sets;
    bool ret_B0;
    f32 scale_x;
    PAD_STACK(8);

    if (dynamics_num != 0 && stage_info.grkind == Gr_Kind_Flatzone) {
        HSD_JObj* jobj = GET_JOBJ(gobj);
        scale_x = jobj->scale.x;
        jobj->scale.x = fp->x34_scale.y;
#ifdef MELEE_NATIVE
        restore_scale=true;
#endif
        HSD_JObjSetMtxDirty(jobj);
    }
    {
        FighterKind kind;
        if (fp->x34_scale.y < 1.0f || fp->x34_scale.y > 1.0f) {
            ret_B0 = false;
        } else {
            kind = fp->kind;
            if (
#ifdef MELEE_NATIVE
                !saved.actor.source_owned &&
#endif
                kind == Ft_Kind_Mewtwo && fp->motion_id >= 0xDF &&
                fp->motion_id <= 0xE8)
            {
                ret_B0 = true;
            } else if (fp->ground_or_air == GA_Air ||
#ifdef MELEE_NATIVE
                       (!saved.actor.source_owned &&
#endif
                       (kind == Ft_Kind_Koopa || kind == Ft_Kind_GKoops)
#ifdef MELEE_NATIVE
                       )
#endif
                       )
            {
                ret_B0 = false;
            } else {
                ret_B0 = true;
            }
        }
#ifdef MELEE_NATIVE
        const NativeFighterDefinition* definition = saved.actor.native_definition;
        if(saved.actor.source_owned && nativeScriptFighter_HasHook(gobj,NativeScriptFighter_DynamicsGroundCollision)) {
            NativeScriptValue result;
            if(nativeScriptFighter_Run(gobj,NativeScriptFighter_DynamicsGroundCollision,
                    NULL,0,NULL,&result)!=NativeScriptVM_Ok || result.type!=NativeScriptType_Bool ||
               !nativeDynamics_Current(&saved))goto dynamics_abort;
            ret_B0=result.as.u64!=0;
        } else if (!saved.actor.source_owned && definition != NULL && definition->on_dynamics_ground_collision != NULL) {
            ret_B0 = definition->on_dynamics_ground_collision(gobj);
            if(!nativeDynamics_Current(&saved))goto dynamics_abort;
        }
#endif
        ftColl_8007AF60(gobj);
#ifdef MELEE_NATIVE
        if(!nativeDynamics_Current(&saved))goto dynamics_abort;
#endif
        ftCo_8009E614(fp);
#ifdef MELEE_NATIVE
        if(!nativeDynamics_Current(&saved))goto dynamics_abort;
        if(saved.actor.source_owned) {
            for(i=0;i<dynamics_num;i++,dyn++) {
                NativeScriptValue arguments[2]={
                    {.type=NativeScriptType_I32,.as.i64=i},
                    {.type=NativeScriptType_I32,.as.i64=dynamics_num}};
                NativeScriptValue result;
                NativeScriptVMStatus status=nativeScriptFighter_Run(gobj,
                    NativeScriptFighter_DynamicsInteractionStart,arguments,2, NULL,&result);
                if(status!=NativeScriptVM_Ok || result.type!=NativeScriptType_I32 ||
                   !nativeDynamics_Current(&saved))goto dynamics_abort;
                lb_8001044C(&dyn->dyn_desc,fp->x1670,fp->x166C,fp->cur_pos.y,
                           arg1,dyn->bone_id,(int32_t)result.as.i64,ret_B0);
                if(!nativeDynamics_Current(&saved))goto dynamics_abort;
            }
        } else if (definition != NULL && definition->on_dynamics_interaction_start != NULL) {
            for (i = 0; i < dynamics_num; i++, dyn++) {
                int first_active = definition->on_dynamics_interaction_start(
                    gobj, (int)i, (int)dynamics_num);
                if(!nativeDynamics_Current(&saved))goto dynamics_abort;
                lb_8001044C(&dyn->dyn_desc, fp->x1670, fp->x166C,
                            fp->cur_pos.y, arg1, dyn->bone_id, first_active, ret_B0);
                if(!nativeDynamics_Current(&saved))goto dynamics_abort;
            }
        } else
#endif
        if (fp->kind == Ft_Kind_Peach) {
            for (i = 0; i < dynamics_num - 1; i++, dyn++) {
                lb_8001044C(&dyn->dyn_desc, fp->x1670, fp->x166C,
                            fp->cur_pos.y, arg1, dyn->bone_id, 3, ret_B0);
            }
            lb_8001044C(&dyn->dyn_desc, fp->x1670, fp->x166C, fp->cur_pos.y,
                        arg1, dyn->bone_id, 0, ret_B0);
        } else if (fp->kind == Ft_Kind_Mewtwo) {
            for (i = 0; i < dynamics_num; i++, dyn++) {
                lb_8001044C(&dyn->dyn_desc, fp->x1670, fp->x166C,
                            fp->cur_pos.y, arg1, dyn->bone_id, 8, ret_B0);
            }
        } else {
            for (i = 0; i < dynamics_num; i++, dyn++) {
                lb_8001044C(&dyn->dyn_desc, fp->x1670, fp->x166C,
                            fp->cur_pos.y, arg1, dyn->bone_id, 0, ret_B0);
            }
        }
    }
#ifdef MELEE_NATIVE
    if(!nativeDynamics_Current(&saved))goto dynamics_abort;
#endif
    if (dynamics_num != 0 && stage_info.grkind == Gr_Kind_Flatzone) {
        HSD_JObj* jobj = GET_JOBJ(gobj);
        jobj->scale.x = scale_x;
        HSD_JObjSetMtxDirty(jobj);
        dyn = fp->dynamic_bone_sets;
        i = 0;
        while (i < dynamics_num) {
            cur = dyn->dyn_desc.data->desc.lb_unk0.jobj;
            while (cur->child != NULL) {
                cur = cur->child;
            }
            HSD_JObjSetupMatrix(cur);
            i++;
            dyn++;
        }
    }
#ifdef MELEE_NATIVE
    return;
dynamics_abort:
    // A logical VM fault leaves the native object alive. Undo only the scale
    // borrowed from the exact retained root; never touch a retired replacement.
    if(restore_scale && nativeDynamics_RootCurrent(&saved) &&
       (!saved.actor.source_owned || nativeScriptFighter_Status(gobj)!=NativeScriptVM_SourceRetired)) {
        saved.root->scale.x=scale_x;
        HSD_JObjSetMtxDirty(saved.root);
    }
#endif
}

void ftCo_8009E0A8(Fighter_GObj* gobj)
{
    ftCo_8009DD94(gobj, GET_FIGHTER(gobj)->x2228_b1);
}

void ftCo_UnloadDynamicBones(Fighter* fp)
{
    ssize_t i;
    for (i = 0; i < fp->dynamics_num; i++) {
        lb_8000FD18(&fp->dynamic_bone_sets[i].dyn_desc);
    }
    fp->dynamics_num = 0;
}

void ftCo_8009E140(Fighter* fp, bool arg1)
{
    ssize_t i;
    PAD_STACK(2 * 4);
    if (fp->kind == Ft_Kind_Kirby) {
        return;
    }
    if (fp->kind == Ft_Kind_Purin) {
        ftCo_8009CB40(fp, 0, arg1, NULL);
        return;
    }
    for (i = 0; i < fp->dynamics_num; i++) {
#ifdef MELEE_NATIVE
        if (fp->native_attachment_dynamic_mask & (1u << i)) continue;
#endif
        ftCo_8009CB40(fp, i, arg1, NULL);
    }
}

HSD_JObj* ftCo_8009E1D4(HSD_JObj* arg0, HSD_JObj* arg1, HSD_JObj* arg2)
{
    HSD_JObj* temp;
    if (arg0 == arg2) {
        return arg1;
    }
    if (arg0->child != NULL) {
        temp = ftCo_8009E1D4(arg0->child, arg1->child, arg2);
        if (temp != NULL) {
            return temp;
        }
    }
    if (arg0->next != NULL) {
        temp = ftCo_8009E1D4(arg0->next, arg1->next, arg2);
        if (temp != NULL) {
            return temp;
        }
    }
    return NULL;
}

enum_t ftCo_8009E318(Fighter_GObj* gobj, enum Fighter_Part arg1, f32 arg2)
{
    Fighter* fp = GET_FIGHTER(gobj);
    HSD_JObj* part_jobj = fp->parts[arg1].joint;
    ssize_t i;
    for (i = 0; i < fp->dynamics_num; i++) {
        ssize_t j;
        struct DynamicsData* cur;
        for (cur = fp->dynamic_bone_sets[i].dyn_desc.data, j = 0; cur != NULL;
             cur = cur->next, j++)
        {
            if (cur->desc.lb_unk0.jobj == part_jobj) {
                if (fp->parts[arg1].flags_b0) {
                    fp->parts[arg1].flags_b0 = false;
                    fp->dynamic_bone_sets[i].bone_id = j + 1;
                    goto exit_true;
                } else {
                    fp->parts[arg1].flags_b0 = true;
                    fp->dynamic_bone_sets[i].bone_id = j;
                    goto exit_false;
                }
            }
        }
    }
    return -1;
exit_true: {
    if (fp->x590 != 0) {
        ftAnim_8006EED4(fp, arg1, fp->x590, arg2, fp->frame_speed_mul);
    }
    return 0;
}
exit_false: {
    HSD_JObj* walk_jobj =
        ftCo_8009E1D4(GET_JOBJ(gobj), fp->x8AC_animSkeleton, part_jobj);
    ftAnim_80070758(part_jobj);
    ftAnim_80070758(walk_jobj);
    return 0;
}
}

void ftCo_8009E4A8(Fighter* fp)
{
    float frame = fp->cur_anim_frame;
    float speed = fp->frame_speed_mul;
    int i;

    if (fp->x594_b4) {
        FigaTree** tree = fp->ft_data->x2C->x10[fp->x28[fp->anim_id][1]];
        if (tree != NULL) {
            for (i = 0; i < fp->dynamics_num; i++) {
#ifdef MELEE_NATIVE
                if (fp->native_attachment_dynamic_mask & (1u << i)) continue;
#endif
                ftCo_8009CB40(fp, i, 1, tree[i]);
                if (fp->x590 != NULL) {
                    ftAnim_8006EED4(
                        fp, fp->ft_data->x2C->ftDynamicBones->array[i].bone_id,
                        fp->x590, frame, speed);
                }
            }
        }
    } else if (fp->x594_b3) {
        for (i = 0; i < fp->dynamics_num; i++) {
#ifdef MELEE_NATIVE
            if (fp->native_attachment_dynamic_mask & (1u << i)) continue;
#endif
            ftCo_8009CB40(fp, i, 0, NULL);
            if (fp->x590 != NULL) {
                ftAnim_8006EED4(
                    fp, fp->ft_data->x2C->ftDynamicBones->array[i].bone_id,
                    fp->x590, frame, speed);
            }
        }
    }
}

void ftCo_8009E614(Fighter* fp)
{
    bool follows_wind = fp->kind == Ft_Kind_Mars || fp->kind == Ft_Kind_Emblem;
#ifdef MELEE_NATIVE
    NativeFighterLifetime lifetime;
    if(!nativeDynamics_CaptureActor(fp,&lifetime))return;
    const NativeFighterDefinition* definition=lifetime.native_definition;
    if(lifetime.source_owned) {
        follows_wind=false;
        if(nativeScriptFighter_HasHook(fp->gobj,NativeScriptFighter_DynamicsWind)) {
            NativeScriptValue result;
            if(nativeScriptFighter_Run(fp->gobj,NativeScriptFighter_DynamicsWind,
                    NULL,0,NULL,&result)!=NativeScriptVM_Ok ||
               !nativeFighterLifetime_Current(&lifetime))return;
            follows_wind=result.as.u64!=0;
        }
    }
    else if (definition != NULL && definition->on_dynamics_wind != NULL) {
        follows_wind = definition->on_dynamics_wind(fp->gobj);
        if(!nativeFighterLifetime_Current(&lifetime))return;
    }
#endif
    if (follows_wind && !fp->x2227_b6)
    {
        int x = lb_80011ABC();
        if (x == -1) {
            ftCo_8009E4A8(fp);
            return;
        }
        if (x == +1) {
            ssize_t i;
            ftData* data = fp->ft_data;
            float frame_speed_mul, cur_anim_frame;
            cur_anim_frame = fp->cur_anim_frame;
            frame_speed_mul = fp->frame_speed_mul;
            if (fp->x594_b4 || fp->x594_b3) {
                for (i = 0; i < data->x2C->dynamicsNum; i++) {
                    ftCo_8009CB40(fp, i, 1, NULL);
                }
                if (fp->x590 != 0) {
                    ftAnim_8006EBE8(fp->gobj, cur_anim_frame, frame_speed_mul,
                                    0.0f);
#ifdef MELEE_NATIVE
                    if(!nativeFighterLifetime_Current(&lifetime))return;
#endif
                    ftAnim_8006E9B4(fp->gobj);
                }
            }
        }
    }
}

bool ftCo_8009E714(Fighter_GObj* gobj, Fighter_Part bone_id, int arg2, float x,
                   float y, float arg5, float arg6, float arg7)
{
    Fighter* fp = GET_FIGHTER(gobj);
    HSD_JObj* jobj = fp->parts[bone_id].joint;
    int var_r31 = arg2;
    Vec3 out_vec;
    Vec3 in_vec;
    in_vec.x = x;
    in_vec.y = y;
    in_vec.z = 0.0f;
    lb_8000B1CC(jobj, &in_vec, &out_vec);
    if (var_r31 < 0) {
        var_r31 = 120;
    }
    lb_800119DC(&out_vec, var_r31, arg5, arg6, arg7);
    return 0;
}

void ftCo_8009E7B4(Fighter* fp, u8 (*arg1)[2])
{
#ifdef MELEE_NATIVE
    NativeFighterLifetime lifetime;
    if(!nativeDynamics_CaptureActor(fp,&lifetime))return;
#endif
    bool source_owned=false;
#ifdef MELEE_NATIVE
    source_owned=lifetime.source_owned;
#endif
    s32 i;
    if (fp->anim_id != -1) {
        s32 var_r3;
        u32 cached_b6;
        if ((cached_b6 = fp->x2227_b6)) {
            if ((source_owned || fp->kind != Ft_Kind_Kirby)) {
                if ((!source_owned && fp->kind == Ft_Kind_Purin)) {
                    ftCo_8009CB40(fp, 0, 0, NULL);
                    return;
                }
                for (i = 0; i < fp->dynamics_num; i++) {
                    ftCo_8009CB40(fp, i, 0, NULL);
                }
            }
        } else {
            bool follows_wind = fp->kind == Ft_Kind_Mars || fp->kind == Ft_Kind_Emblem;
#ifdef MELEE_NATIVE
            const NativeFighterDefinition* definition=lifetime.native_definition;
            if(lifetime.source_owned) {
                follows_wind=false;
                if(nativeScriptFighter_HasHook(fp->gobj,NativeScriptFighter_DynamicsWind)) {
                    NativeScriptValue result;
                    if(nativeScriptFighter_Run(fp->gobj,NativeScriptFighter_DynamicsWind,
                            NULL,0,NULL,&result)!=NativeScriptVM_Ok ||
                       !nativeFighterLifetime_Current(&lifetime))return;
                    follows_wind=result.as.u64!=0;
                }
            }
            else if (definition != NULL && definition->on_dynamics_wind != NULL) {
                follows_wind = definition->on_dynamics_wind(fp->gobj);
                if(!nativeFighterLifetime_Current(&lifetime))return;
            }
#endif
            if (!follows_wind) {
                var_r3 = 0;
            } else if (!cached_b6) {
                if (lb_80011ABC() > 0) {
                    var_r3 = 1;
                } else {
                    var_r3 = 0;
                }
            }
            if (var_r3) {
                if ((source_owned || fp->kind != Ft_Kind_Kirby)) {
                    if ((!source_owned && fp->kind == Ft_Kind_Purin)) {
                        ftCo_8009CB40(fp, 0, 1, NULL);
                        return;
                    }
                    for (i = 0; i < fp->dynamics_num; i++) {
                        ftCo_8009CB40(fp, i, 1, NULL);
                    }
                }
            } else {
                if (fp->x594_b4) {
                    FigaTree*** dyn;
                    FigaTree** tree;
                    u8 blend_slot = arg1[0][1];
                    dyn = fp->ft_data->x2C->x10;
                    if (dyn == NULL) {
                        for (i = 0; i < fp->dynamics_num; i++) {
                            ftCo_8009CB40(fp, i, 0, NULL);
                        }
                        return;
                    }
                    tree = dyn[blend_slot];
                    if (tree == NULL) {
                        for (i = 0; i < fp->dynamics_num; i++) {
                            ftCo_8009CB40(fp, i, 0, NULL);
                        }
                        return;
                    }
                    {
                        s32 j;
                        FigaTree** cursor;
                        j = (var_r3 = 0);
                        cursor = &tree[j];
                        for (; j < fp->dynamics_num; j++) {
#ifdef MELEE_NATIVE
                            if (fp->native_attachment_dynamic_mask & (1u << j)) continue;
#endif
                            ftCo_8009CB40(fp, j, 1, *cursor);
                            cursor++;
                        }
                    }
                    return;
                } else {
                    if (fp->x594_b3) {
                        if ((source_owned || fp->kind != Ft_Kind_Kirby)) {
                            if ((!source_owned && fp->kind == Ft_Kind_Purin)) {
                                ftCo_8009CB40(fp, 0, 0, NULL);
                                return;
                            }
                            for (i = 0; i < fp->dynamics_num; i++) {
                                ftCo_8009CB40(fp, i, 0, NULL);
                            }
                        }
                    } else {
                        if ((source_owned || fp->kind != Ft_Kind_Kirby)) {
                            if ((!source_owned && fp->kind == Ft_Kind_Purin)) {
                                ftCo_8009CB40(fp, 0, 1, NULL);
                                return;
                            }
                            for (i = 0; i < fp->dynamics_num; i++) {
                                ftCo_8009CB40(fp, i, 1, NULL);
                            }
                        }
                    }
                }
            }
        }
    } else if (fp->x2227_b6 && (source_owned || fp->kind != Ft_Kind_Kirby)) {
        if ((!source_owned && fp->kind == Ft_Kind_Purin)) {
            ftCo_8009CB40(fp, 0, 0, NULL);
            return;
        }
        for (i = 0; i < fp->dynamics_num; i++) {
            ftCo_8009CB40(fp, i, 0, NULL);
        }
    }
}

void ftCo_8009EAF8(Fighter_GObj* gobj)
{
    Ground_801C4E20();
}
