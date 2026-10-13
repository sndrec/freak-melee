#include "efsync.h"

#include <math.h>

#include "efalt.h"
#include "efasync.h"
#include "efdata.h"
#include "eflib.h"
#include "types.h"
#include <melee/ft/inlines.h>
#include <sysdolphin/baselib/generator.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/psstructs.h>
#include <sysdolphin/baselib/random.h>

extern EF_DAT_Entry efAsync_DatEntries[51];

// Immutable argument family and complete model count for named construction.
// Other typed dispatcher recipes retain their own argument/result contracts.
const EF_ModelRecipe* efSync_ModelRecipe(s32 gfx_id)
{
    static const EF_ModelRecipe joint = { EF_MODEL_JOINT, 1 };
    static const EF_ModelRecipe pair = { EF_MODEL_JOINT, 2 };
    static const EF_ModelRecipe position = { EF_MODEL_POSITION, 1 };
    switch (gfx_id) {
    case 0x4DA: return &pair;
    case 0x4CC: return &position;
    case 0x449:
    case 0x47C:
    case 0x482:
    case 0x487:
    case 0x488:
    case 0x489:
    case 0x48A:
    case 0x48B:
    case 0x48C:
    case 0x491:
    case 0x496:
    case 0x497:
    case 0x498:
    case 0x499:
    case 0x4B4:
    case 0x4B5:
    case 0x4B6:
    case 0x4B7:
    case 0x4B8:
    case 0x4B9:
    case 0x4C0:
    case 0x4C6:
    case 0x4C7:
    case 0x4CA:
    case 0x4CB:
    case 0x4CD:
    case 0x4D6:
    case 0x4E0:
    case 0x4ED:
    case 0x4EE:
    case 0x4EF:
    case 0x4F0:
    case 0x4F2:
    case 0x4F3:
    case 0x4F4:
    case 0x4F5:
    case 0x4F6:
    case 0x4F7:
    case 0x4FE:
    case 0x509:
    case 0x50D:
    case 0x511:
    case 0x512:
        return &joint;
    default:
        return NULL;
    }
}

// Routes gfx_id to a specific spawn handler:
//
// .--------------------------------------------.
// | gfx_id range       | handler               |
// |--------------------+-----------------------|
// | 0x0000 - 0x024F    | efLib_CreateGenerator |
// | 0x7530 - 0x7917    | efLib_CreateGenerator |
// | 0x0250 - 0x0477    | efAsync_Dispatch      |
// | 0x0479 - 0x04BA    | efAlt_Spawn           |
// | 0x04BB - 0x0512    | cases in this switch  |
// *--------------------------------------------*
//
// Effects attach to a parent gobj and optionally a jobj for
// position/rotation inheritance. Note there's variadic args!
EF_SpawnResult efSync_Spawn(s32 gfx_id, HSD_GObj* gobj, ...)
{
    va_list vlist;
    Vec3 translate;
    Vec3 scale;
    HSD_psAppSRT* psAppSRT;
    EF_SpawnResult spawned = { 0 };
    EF_Effect* ret_eff;
    HSD_JObj* jobj_2;
    HSD_JObj* jobj_1;
    Vec3* va_vec3;
    f64 half_pi;
    f32 va_f32_1;
    f32 rand_f32;
    f32 rand_param_x;
    f32 rand_param_y;
    f32 rand_rot_y;
    Fighter* fp;
    s32 cnt_2;
    f32 rand_rot_x;
    PAD_STACK(0x2C);

    efLib_AnimCount = efLib_LoadKind = 0;
    va_start(vlist, gobj);
    if ((gfx_id == 0x479) && (efAsync_DatEntries[1].data == NULL)) {
        gfx_id = 0x506;
    }
    if (gfx_id < 0x250) {
        va_vec3 = va_arg(vlist, Vec3*);
        spawned.object.generator = efLib_CreateGenerator(gfx_id, va_vec3);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        va_end(vlist);
        return spawned;
    }
    if (gfx_id / 1000 == 0x1E) {
        va_vec3 = va_arg(vlist, Vec3*);
        spawned.object.generator = efLib_CreateGenerator(gfx_id, va_vec3);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        va_end(vlist);
        return spawned;
    }
    if (gfx_id < 0x478) {
        spawned = efAsync_Dispatch(gfx_id, gobj, vlist);
        va_end(vlist);
        return spawned;
    }
    if (gfx_id < 0x4BA) {
        spawned = efAlt_Spawn(gfx_id, gobj, vlist);
        va_end(vlist);
        return spawned;
    }
    efLib_LoadKind = EF_LOADKIND_SYNC;
    switch (gfx_id) {
    case 0x4BB:
        spawned.object.model = efLib_Create_Attach_Scale_FacingDir(
            0x1770, gobj, va_arg(vlist, void*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        if (spawned.object.model != NULL) {
            spawned.object.model->next = efLib_Create_AttachChild_Scale(
                0x1772, gobj, va_arg(vlist, HSD_JObj*));
        }
        break;
    case 0x4BC:
        spawned.object.model = efLib_Create_Attach_Scale_FacingDir(
            0x1771, gobj, va_arg(vlist, void*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        if (spawned.object.model != NULL) {
            spawned.object.model->next = efLib_Create_AttachChild_Scale(
                0x1773, gobj, va_arg(vlist, HSD_JObj*));
        }
        break;
    case 0x4BD:
        spawned.object.generator =
            hsd_8039EFAC(0, 7, 0x1B58, va_arg(vlist, HSD_JObj*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4BE:
        spawned.object.generator =
            hsd_8039EFAC(0, 7, 0x1B5C, va_arg(vlist, HSD_JObj*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4BF:
        jobj_1 = va_arg(vlist, HSD_JObj*);
        spawned.object.generator = hsd_8039EFAC(0, 7, 0x1B5D, jobj_1);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        hsd_8039EFAC(0, 0, 0x5F, jobj_1);
        break;
    case 0x4C0:
        spawned.object.model =
            efLib_Create_Attach(0x1B58U, gobj, va_arg(vlist, HSD_JObj*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        break;
    case 0x4C1:
        spawned.object.model = efLib_Create_Attach_Scale_FacingDir(
            0x1B59, gobj, va_arg(vlist, void*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        if (spawned.object.model != NULL) {
            va_f32_1 = *va_arg(vlist, f32*);
            HSD_JObjSetRotationZ(GET_JOBJ(spawned.object.model->gobj),
                                 va_f32_1);
        }
        break;
    case 0x4C2:
        spawned.object.model = efLib_Create_Attach_Scale_FacingDir(
            0x1B5A, gobj, va_arg(vlist, void*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        if (spawned.object.model != NULL) {
            va_f32_1 = *va_arg(vlist, f32*);
            HSD_JObjSetRotationZ(GET_JOBJ(spawned.object.model->gobj),
                                 va_f32_1);
        }
        break;
    case 0x4C3: {
        HSD_Generator* gen = efLib_CreateGenerator_AddAppSRT(0x24CU);
        if (gen != NULL) {
            va_vec3 = va_arg(vlist, Vec3*);
            spawned.object.generator = gen;
            spawned.kind = spawned.object.generator != NULL
                               ? EF_SPAWN_GENERATOR
                               : EF_SPAWN_NONE;
            spawned.model_generation = 0;
            psAppSRT = gen->appsrt;
            psAppSRT->translate = *va_vec3;
        }
        break;
    }
    case 0x4C4:
        spawned.object.model = efLib_Create_Attach_Scale_FacingDir(
            0x1B5B, gobj, va_arg(vlist, void*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        if (spawned.object.model != NULL) {
            va_f32_1 = *va_arg(vlist, f32*);
            HSD_JObjSetRotationZ(GET_JOBJ(spawned.object.model->gobj),
                                 va_f32_1);
        }
        break;
    case 0x4C5:
        spawned.object.model = efLib_Create_Attach_Scale_FacingDir(
            0x1B5C, gobj, va_arg(vlist, void*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        if (spawned.object.model != NULL) {
            va_f32_1 = *va_arg(vlist, f32*);
            HSD_JObjSetRotationZ(GET_JOBJ(spawned.object.model->gobj),
                                 va_f32_1);
        }
        break;
    case 0x4C6:
        spawned.object.model = efLib_Create_Attach_Scale_FacingDir(
            0x1F40, gobj, va_arg(vlist, void*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        break;
    case 0x4C7:
        spawned.object.model = efLib_Create_Attach_Scale_FacingDir(
            0x1F41, gobj, va_arg(vlist, void*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        break;
    case 0x4C8:
        spawned.object.model =
            efLib_Create_Attach_Scale(0x1F42, gobj, va_arg(vlist, HSD_JObj*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        ret_eff = spawned.object.model;
        if (ret_eff != NULL) {
            if (*va_arg(vlist, f32*) < 0.0f) {
                half_pi = -M_PI_2;
            } else {
                half_pi = M_PI_2;
            }
            va_f32_1 = half_pi;
            HSD_JObjSetRotationY(GET_JOBJ(ret_eff->gobj), va_f32_1);
            jobj_2 = GET_JOBJ(ret_eff->gobj);
            HSD_JObjAnimAll(jobj_2);
        }
        break;
    case 0x4C9:
        spawned.object.model =
            efLib_Create_Attach_Scale(0x1F43, gobj, va_arg(vlist, HSD_JObj*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        ret_eff = spawned.object.model;
        if (ret_eff != NULL) {
            if (*va_arg(vlist, f32*) < 0.0f) {
                half_pi = -M_PI_2;
            } else {
                half_pi = M_PI_2;
            }
            va_f32_1 = half_pi;
            HSD_JObjSetRotationY(GET_JOBJ(ret_eff->gobj), va_f32_1);
            jobj_2 = GET_JOBJ(ret_eff->gobj);
            HSD_JObjAnimAll(jobj_2);
        }
        break;
    case 0x4CA:
        spawned.object.model =
            efLib_Create_Attach_Scale(0x1F44, gobj, va_arg(vlist, HSD_JObj*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        break;
    case 0x4CB:
        spawned.object.model =
            efLib_Create_Attach_Scale(0x1F45, gobj, va_arg(vlist, HSD_JObj*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        break;
    case 0x4CC:
        spawned.object.model =
            efLib_Create_Attach_Pos(0x1F46U, gobj, va_arg(vlist, Vec3*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        if (spawned.object.model != NULL) {
            jobj_1 = gobj->hsd_obj;
            (void) jobj_1;
            HSD_JObjGetScale(jobj_1, &scale);
            HSD_JObjSetScale(GET_JOBJ(spawned.object.model->gobj), &scale);
            spawned.object.model->update = efLib_Cb_SetRotY_FromFighterDir;
        }
        break;
    case 0x4CD:
        spawned.object.model =
            efLib_Create_Attach(0x2328U, gobj, va_arg(vlist, HSD_JObj*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        break;
    case 0x4CE:
        spawned.object.generator =
            efLib_CreateGenerator(0x2328U, va_arg(vlist, Vec3*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4CF:
    case 0x4D0: {
        EF_Effect* effect;
        EF_Effect* eff_1;
        s32 cnt_1;
        f32 scale_f32;
        scale_f32 = 1.0f;
        va_vec3 = va_arg(vlist, Vec3*);
        translate = *va_vec3;
        if (gfx_id == 0x4CF) {
            scale_f32 = *va_arg(vlist, f32*);
        }
        cnt_1 = 0;
    loop_141:
        rand_f32 = HSD_Randf();
        if (rand_f32 < 0.5f) {
            if (gfx_id == 0x4D0) {
                effect = efLib_Create_Attach_Pos(0x1CU, gobj, &translate);
            } else {
                effect = efLib_Create_Attach_Pos(0x1EU, gobj, &translate);
            }
        } else if (gfx_id == 0x4D0) {
            effect = efLib_Create_Attach_Pos(0x1DU, gobj, &translate);
        } else {
            effect = efLib_Create_Attach_Pos(0x1FU, gobj, &translate);
        }
        if (effect != NULL) {
            effect->update = efLib_Cb_SetOffset_FromParams;
            effect->lifetime = 0x32;
            rand_rot_y = M_TAU * HSD_Randf();
            rand_rot_x = M_TAU * HSD_Randf();
            jobj_1 = GET_JOBJ(effect->gobj);
            HSD_JObjSetScaleX(jobj_1, scale_f32);
            jobj_1 = GET_JOBJ(effect->gobj);
            HSD_JObjSetScaleY(jobj_1, scale_f32);
            jobj_1 = GET_JOBJ(effect->gobj);
            HSD_JObjSetScaleZ(jobj_1, scale_f32);
            jobj_1 = GET_JOBJ(effect->gobj);
            HSD_JObjSetRotationY(jobj_1, rand_rot_y);
            jobj_1 = GET_JOBJ(effect->gobj);
            HSD_JObjSetRotationX(jobj_1, rand_rot_x);
            rand_param_x = sinf(rand_rot_y);
            {
                f32 tmp = 2.0f * cosf(rand_rot_x);
                effect->params.x = tmp * rand_param_x;
            }
            effect->params.y = 2.0f * sinf(rand_rot_x);
            rand_param_y = cosf(rand_rot_y);
            {
                f32 tmp = 2.0f * cosf(rand_rot_x);
                effect->params.z = tmp * rand_param_y;
            }
            if (cnt_1 != 0) {
                eff_1->next = effect;
                eff_1 = eff_1->next;
            } else {
                spawned.object.model = eff_1 = effect;
                spawned.kind = spawned.object.model != NULL ? EF_SPAWN_MODEL
                                                            : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
                spawned.model_generation =
                    spawned.object.model != NULL
                        ? spawned.object.model->native_generation
                        : 0;
#endif
            }
            if (++cnt_1 < 0xC) {
                goto loop_141;
            }
        }
        break;
    }
    case 0x4EE:
        spawned.object.model =
            efLib_Create_Attach(0x2710U, gobj, va_arg(vlist, HSD_JObj*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        break;
    case 0x4EF:
        spawned.object.model =
            efLib_Create_Attach(0x2711U, gobj, va_arg(vlist, HSD_JObj*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        break;
    case 0x4F0:
        spawned.object.model =
            efLib_Create_Attach_Scale(0x2712, gobj, va_arg(vlist, HSD_JObj*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        break;
    case 0x4D5:
        spawned.object.generator =
            hsd_8039EFAC(0, 0xB, 0x2AF8, va_arg(vlist, HSD_JObj*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4D6:
        spawned.object.model =
            efLib_Create_Attach_Scale(0x2AF8, gobj, va_arg(vlist, HSD_JObj*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        break;
    case 0x4D7:
        spawned.object.generator =
            hsd_8039EFAC(0, 0xB, 0x2AF9, va_arg(vlist, HSD_JObj*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4D8: {
        HSD_Generator* generator = efLib_CreateGenerator_AddAppSRT(0x61U);
        if (generator != NULL) {
            va_vec3 = va_arg(vlist, Vec3*);
            psAppSRT = generator->appsrt;
            psAppSRT->translate = *va_vec3;
            HSD_JObjGetScale((jobj_2 = GET_JOBJ(gobj)), &scale);
            spawned.object.generator = generator;
            spawned.kind = spawned.object.generator != NULL
                               ? EF_SPAWN_GENERATOR
                               : EF_SPAWN_NONE;
            spawned.model_generation = 0;
            generator->appsrt->scale.x = generator->appsrt->scale.y =
                generator->appsrt->scale.z = scale.y;
            generator->appsrt->rot.y = M_PI_2;
        }
        break;
    }
    case 0x4D9:
        spawned.object.model =
            efLib_Create_Attach_Pos(0x2EE0U, gobj, va_arg(vlist, Vec3*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        if (spawned.object.model != NULL) {
            if (*va_arg(vlist, f32*) < 0.0f) {
                half_pi = -M_PI_2;
            } else {
                half_pi = M_PI_2;
            }
            va_f32_1 = half_pi;
            ret_eff = spawned.object.model;
            jobj_2 = GET_JOBJ(ret_eff->gobj);
            HSD_JObjSetRotationY(jobj_2, va_f32_1);
            HSD_JObjGetScale((jobj_2 = GET_JOBJ(gobj)), &scale);
            jobj_2 = GET_JOBJ(ret_eff->gobj);
            HSD_JObjSetScale(jobj_2, &scale);
        }
        break;
    case 0x4DA: {
        EF_Effect* eff_1;
        jobj_2 = va_arg(vlist, HSD_JObj*);
        spawned.object.model = efLib_Create_Attach_Scale(0x2EE1, gobj, jobj_2);
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        if (spawned.object.model != NULL) {
            eff_1 = efLib_Create_Attach_Scale(0x2EE2, gobj, jobj_2);
#ifdef MELEE_NATIVE
            if (eff_1 != NULL) {
                spawned.member_count = 2;
                spawned.members[1] = (EF_ModelMember){ eff_1->gobj, eff_1->native_generation };
            }
#endif
            spawned.object.model->next = eff_1;
            if (eff_1 != NULL) {
                spawned.object.model->update = efLib_Cb_ftKp_SpecialHi;
            }
        }
        break;
    }
    case 0x4DB:
        spawned.object.generator =
            efLib_CreateGenerator_Attach_Scale(0x2EE5, vlist, gobj);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4DC:
        spawned.object.generator =
            efLib_CreateGenerator_Attach_Scale(0x2EE6, vlist, gobj);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4DD:
        spawned.object.generator =
            efLib_CreateGenerator_Attach_Scale(0x2EE7, vlist, gobj);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4DE:
        spawned.object.generator =
            efLib_CreateGenerator_Attach_Scale(0x2EE8, vlist, gobj);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4DF:
        spawned.object.generator =
            efLib_CreateGenerator_Attach_Scale(0x143, vlist, gobj);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4E0:
        spawned.object.model =
            efLib_Create_Attach(0x32C8U, gobj, va_arg(vlist, HSD_JObj*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        break;
    case 0x4E1: {
        HSD_JObj* cursor;
        spawned.object.model = efLib_Create(0x32C9, gobj);
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        if (spawned.object.model != NULL) {
            jobj_1 = va_arg(vlist, HSD_JObj*);
            spawned.object.model->attach_jobj = jobj_1;
            spawned.object.model->update = efLib_Cb_SetOffsetY_FromParamY;
            cursor = jobj_1;
            while (HSD_JObjGetParent(cursor) != NULL) {
                jobj_1 = HSD_JObjGetParent(cursor);
                cursor = jobj_1;
            }
            HSD_JObjGetScale(cursor, &scale);
            spawned.object.model->params = *va_arg(vlist, Vec3*);
            spawned.object.model->params.y *= scale.y;
        }
        break;
    }
    case 0x4E2:
        spawned.object.model =
            efLib_Create_Attach_Pos(0x32CAU, gobj, va_arg(vlist, Vec3*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        break;
    case 0x4E3:
        spawned.object.generator =
            efLib_CreateGenerator(0x18AU, va_arg(vlist, Vec3*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4E4:
        spawned.object.generator =
            efLib_CreateGenerator(0x194U, va_arg(vlist, Vec3*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4E5:
        spawned.object.generator =
            hsd_8039EFAC(0, 0, 0x17D, va_arg(vlist, HSD_JObj*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4E6:
        spawned.object.generator =
            hsd_8039EFAC(0, 0, 0x17E, va_arg(vlist, HSD_JObj*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4E7:
        spawned.object.generator =
            efLib_CreateGenerator(0x196U, va_arg(vlist, Vec3*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4E8:
        spawned.object.model =
            efLib_Create_Attach_Pos(0x32CBU, gobj, va_arg(vlist, Vec3*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        break;
    case 0x4E9:
        spawned.object.generator =
            hsd_8039EFAC(0, 0xE, 0x36B0, va_arg(vlist, HSD_JObj*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4EA:
        spawned.object.generator =
            hsd_8039EFAC(0, 0xE, 0x36B1, va_arg(vlist, HSD_JObj*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4EB:
        spawned.object.generator =
            efLib_CreateGenerator_AppSRT_SetFacingDir(0x36B6, vlist);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4EC:
        spawned.object.generator =
            hsd_8039EFAC(0, 0xE, 0x36B7, va_arg(vlist, HSD_JObj*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4ED:
        spawned.object.model =
            efLib_Create_AttachChild(0x36B0U, gobj, va_arg(vlist, HSD_JObj*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        break;
    case 0x4D1:
        spawned.object.generator =
            hsd_8039EFAC(0, 0, 0x64, va_arg(vlist, HSD_JObj*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4D2:
        spawned.object.model =
            efLib_Create_Attach_Pos(0x3A98U, gobj, va_arg(vlist, Vec3*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        break;
    case 0x4D3:
        jobj_2 = va_arg(vlist, HSD_JObj*);
        spawned.object.generator =
            efLib_CreateGenerator_Attach_AddAppSRT(0x172, jobj_2);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        if (spawned.object.generator != NULL) {
            efLib_CreateGenerator_Attach_AddAppSRT(0x173, jobj_2);
        }
        break;
    case 0x4D4:
        spawned.object.generator =
            efLib_CreateGenerator_Attach(0x11E, va_arg(vlist, HSD_JObj*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4F1:
        spawned.object.generator =
            efLib_CreateGenerator_AppSRT_SetFacingDir(0x3E80, vlist);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4F2:
        spawned.object.model = efLib_Create_Attach_Scale_FacingDir(
            0x3E80, gobj, va_arg(vlist, void*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        break;
    case 0x4F3:
        spawned.object.model = efLib_Create_Attach_Scale_FacingDir(
            0x3E81, gobj, va_arg(vlist, void*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        break;
    case 0x4F4:
        spawned.object.model =
            efLib_Create_AttachChild(0x4268U, gobj, va_arg(vlist, HSD_JObj*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        goto block_297;
        break;
    case 0x4F5:
        spawned.object.model =
            efLib_Create_AttachChild(0x4269U, gobj, va_arg(vlist, HSD_JObj*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
    block_297:
        if (spawned.object.model != NULL) {
            HSD_JObjGetScale((jobj_2 = GET_JOBJ(gobj)), &scale);
            ret_eff = spawned.object.model;
            if (GET_JOBJ(ret_eff->gobj) == NULL) {
                __assert("jobj.h", 823, "jobj");
            }
            jobj_2 = GET_JOBJ(ret_eff->gobj);
            HSD_JObjSetScale(jobj_2, &scale);
        }
        break;
    case 0x4F6:
        spawned.object.model =
            efLib_Create_Attach_Scale(0x426A, gobj, va_arg(vlist, HSD_JObj*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        break;
    case 0x4F7:
        spawned.object.model =
            efLib_Create_Attach_Scale(0x426B, gobj, va_arg(vlist, HSD_JObj*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        break;
    case 0x4F8:
        spawned.object.generator =
            efLib_CreateGenerator_Attach_Scale(0x6E, vlist, gobj);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4F9:
        spawned.object.generator =
            efLib_CreateGenerator_Attach_Scale(0x1C8, vlist, gobj);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4FA:
        spawned.object.generator =
            efLib_CreateGenerator_Attach_Scale(0x166, vlist, gobj);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4FB:
        spawned.object.generator =
            hsd_8039EFAC(0, 0, 0x71, va_arg(vlist, HSD_JObj*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4FE:
        spawned.object.model =
            efLib_Create_Attach(0x426CU, gobj, va_arg(vlist, HSD_JObj*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        break;
    case 0x4FF:
    case 0x500:
        spawned.object.model = efLib_Create_Attach_Scale(
            0x426D, gobj, ((Fighter*) gobj->user_data)->parts[1].joint);
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        break;
    case 0x501:
        fp = GET_FIGHTER(gobj);
        spawned.object.model = efLib_Create_Attach_Scale(
            0x426E, gobj, ((Fighter*) gobj->user_data)->parts[1].joint);
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        if (spawned.object.model != NULL) {
            if (fp->facing_dir < 0.0f) {
                half_pi = -M_PI_2;
            } else {
                half_pi = M_PI_2;
            }
            va_f32_1 = half_pi;
            jobj_2 = GET_JOBJ(spawned.object.model->gobj);
            HSD_JObjSetRotationY(jobj_2, va_f32_1);
            spawned.object.model->attach_jobj = fp->parts[85].joint;
            spawned.object.model->update = efLib_Cb_LifetimeEndSpawn;
            spawned.object.model->lifetime = 6;
        }
        break;
    case 0x502:
        spawned.object.generator =
            hsd_8039EFAC(0, 0, 0x1A6, va_arg(vlist, HSD_JObj*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4FC:
        spawned.object.generator =
            hsd_8039EFAC(0, 0x11, 0x426D, va_arg(vlist, HSD_JObj*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4FD:
        spawned.object.generator =
            hsd_8039EFAC(0, 0x11, 0x4271, va_arg(vlist, HSD_JObj*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x503:
        spawned.object.generator =
            efLib_CreateGenerator(0x6AU, va_arg(vlist, Vec3*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x504: {
        HSD_Generator* generator = efLib_CreateGenerator_AddAppSRT(0x6DU);
        if (generator != NULL) {
            va_vec3 = va_arg(vlist, Vec3*);
            psAppSRT = generator->appsrt;
            psAppSRT->translate = *va_vec3;
            HSD_JObjGetScale((jobj_2 = GET_JOBJ(gobj)), &scale);
            spawned.object.generator = generator;
            spawned.kind = spawned.object.generator != NULL
                               ? EF_SPAWN_GENERATOR
                               : EF_SPAWN_NONE;
            spawned.model_generation = 0;
            generator->appsrt->scale.x = generator->appsrt->scale.y =
                generator->appsrt->scale.z = scale.y;
        }
        break;
    }
    case 0x505: {
        HSD_Generator* generator = efLib_CreateGenerator_AddAppSRT(0x79U);
        if (generator != NULL) {
            va_vec3 = va_arg(vlist, Vec3*);
            psAppSRT = generator->appsrt;
            psAppSRT->translate = *va_vec3;
            HSD_JObjGetScale((jobj_2 = gobj->hsd_obj), &scale);
            spawned.object.generator = generator;
            spawned.kind = spawned.object.generator != NULL
                               ? EF_SPAWN_GENERATOR
                               : EF_SPAWN_NONE;
            spawned.model_generation = 0;
            generator->appsrt->scale.x = generator->appsrt->scale.y =
                generator->appsrt->scale.z = scale.y;
        }
        break;
    }
    case 0x506:
        spawned.object.generator =
            efLib_CreateGenerator(0x4658U, va_arg(vlist, Vec3*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x507:
        jobj_1 = va_arg(vlist, HSD_JObj*);
        spawned.object.model = efLib_Create_Attach(0x4650U, gobj, jobj_1);
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        if (spawned.object.model != NULL) {
            if (*va_arg(vlist, f32*) < 0.0f) {
                half_pi = -M_PI_2;
            } else {
                half_pi = M_PI_2;
            }
            va_f32_1 = half_pi;
            jobj_2 = GET_JOBJ(spawned.object.model->gobj);
            HSD_JObjSetRotationY(jobj_2, va_f32_1);
        }
        hsd_8039EFAC(0, 0x12, 0x4650, jobj_1);
        break;
    case 0x508:
        spawned.object.generator =
            efLib_CreateGenerator(0x4652U, va_arg(vlist, Vec3*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x509:
        spawned.object.model =
            efLib_Create_Attach_Scale(0x4651, gobj, va_arg(vlist, HSD_JObj*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        if (spawned.object.model != NULL) {
            spawned.object.model->update = efLib_Cb_ftLg_SpecialLw;
        }
        break;
    case 0x50A:
        spawned.object.generator =
            hsd_8039EFAC(0, 0, 0x5F, va_arg(vlist, HSD_JObj*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x50B:
        spawned.object.model = efLib_Create_Attach_Scale_FacingDir(
            0x4A38, gobj, va_arg(vlist, void*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        if (spawned.object.model != NULL) {
            spawned.object.model->next = efLib_Create_Attach_Scale_FacingDir(
                0x4A39, gobj, va_arg(vlist, void*));
        }
        break;
    case 0x50C:
        spawned.object.model =
            efLib_Create_Attach(0x4A3AU, gobj, va_arg(vlist, HSD_JObj*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        if (spawned.object.model != NULL) {
            spawned.object.model->params.z = *va_arg(vlist, f32*);
            spawned.object.model->update =
                efLib_Cb_SetRotYZ_FromParamZ_FighterDir;
        }
        break;
    case 0x50D:
        spawned.object.model =
            efLib_Create_Attach(0x4A3BU, gobj, va_arg(vlist, HSD_JObj*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        break;
    case 0x50E:
        spawned.object.model =
            efLib_Create_Attach_Scale(0x4A3C, gobj, va_arg(vlist, HSD_JObj*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        if (spawned.object.model != NULL) {
            if (*va_arg(vlist, f32*) < 0.0f) {
                half_pi = -M_PI_2;
            } else {
                half_pi = M_PI_2;
            }
            va_f32_1 = half_pi;
            jobj_2 = GET_JOBJ(spawned.object.model->gobj);
            HSD_JObjSetRotationY(jobj_2, va_f32_1);
        }
        break;
    case 0x50F:
        spawned.object.model =
            efLib_Create_Attach_Scale(0x4A3D, gobj, va_arg(vlist, HSD_JObj*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        if (spawned.object.model != NULL) {
            if (*va_arg(vlist, f32*) < 0.0f) {
                half_pi = -M_PI_2;
            } else {
                half_pi = M_PI_2;
            }
            va_f32_1 = half_pi;
            jobj_2 = GET_JOBJ(spawned.object.model->gobj);
            HSD_JObjSetRotationY(jobj_2, va_f32_1);
        }
        break;
    case 0x510:
        spawned.object.generator =
            efLib_CreateGenerator_AppSRT_SetFacingDir(0xBF68, vlist);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x511:
        spawned.object.model = efLib_Create_Attach_Scale_FacingDir(
            0xBF68, gobj, va_arg(vlist, void*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        break;
    case 0x512:
        spawned.object.model = efLib_Create_Attach_Scale_FacingDir(
            0xBF69, gobj, va_arg(vlist, void*));
        spawned.kind =
            spawned.object.model != NULL ? EF_SPAWN_MODEL : EF_SPAWN_NONE;
#if defined(MELEE_NATIVE)
        spawned.model_generation =
            spawned.object.model != NULL
                ? spawned.object.model->native_generation
                : 0;
        if (spawned.object.model != NULL) {
            spawned.member_count = 1;
            spawned.members[0] = (EF_ModelMember){ spawned.object.model->gobj, spawned.model_generation };
        }
#endif
        break;
    }
    while (efLib_AnimCount != 0) {
        cnt_2 = efLib_AnimCount - 1;
        efLib_AnimCount = cnt_2;
#ifdef MELEE_NATIVE
        HSD_JObjAnimAll(efLib_AnimQueue[cnt_2]);
#else
        HSD_JObjAnimAll(
            ((EF_ParamEntry*) (((u32*) efLib_AnimQueue) + cnt_2))->gobj);
#endif
    }

    va_end(vlist);
    return spawned;
}
