#include "efalt.h"

#include <math.h>
#include <placeholder.h>

#include "eflib.h"
#include "types.h"
#include <melee/ft/types.h>
#include <sysdolphin/baselib/generator.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>

#define EFALT_VA_ARG(t) va_arg(vlist_arg, t)

extern volatile u32 efLib_LoadKind;
extern volatile s32 efLib_AnimCount;

// There seems to be multiple spawners, one dispatcher

EF_SpawnResult efAlt_Spawn(s32 gfx_id, HSD_GObj* gobj, va_list vlist_arg)
{
    EF_Effect* effect;
    HSD_JObj* jobj;
    f32 rotate_y;
    Vec3 scale;
    f32* value_ptr;
    EF_SpawnResult spawned = { 0 };

    efLib_LoadKind = EF_LOADKIND_SYNC;
    PAD_STACK(80);

    switch (gfx_id) {
    case 0x479:
        spawned.object.generator =
            efLib_CreateGenerator(0x3F2, EFALT_VA_ARG(Vec3*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x47A: {
        f64 temp_d;
        f32 rotate_y_2;

        jobj = EFALT_VA_ARG(HSD_JObj*);
        spawned.object.model = efLib_Create_Attach(0x3E8U, gobj, jobj);
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
            value_ptr = EFALT_VA_ARG(f32*);
            if (*value_ptr < 0.0f) {
                temp_d = -M_PI_2;
            } else {
                temp_d = M_PI_2;
            }
            rotate_y_2 = temp_d;
            HSD_JObjSetRotationY(GET_JOBJ(spawned.object.model->gobj),
                                 rotate_y_2);
        }
        hsd_8039EFAC(0, 1, 0x3E9, jobj);
        break;
    }
    case 0x47B:
        spawned.object.generator =
            efLib_CreateGenerator(0x3EB, EFALT_VA_ARG(Vec3*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x47C:
        spawned.object.model =
            efLib_Create_Attach_Scale(0x3E9U, gobj, EFALT_VA_ARG(HSD_JObj*));
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
            spawned.object.model->update = efLib_Cb_ftMr_SpecialLw;
        }
        break;
    case 0x47D:
        spawned.object.generator =
            hsd_8039EFAC(0, 1, 0x3F0, EFALT_VA_ARG(HSD_JObj*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x47E:
        spawned.object.generator =
            hsd_8039EFAC(0, 1, 0x3F1, EFALT_VA_ARG(HSD_JObj*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x47F:
        spawned.object.generator =
            hsd_8039EFAC(0, 2, 0x7D4, EFALT_VA_ARG(HSD_JObj*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x480:
        spawned.object.generator =
            hsd_8039EFAC(0, 2, 0x7D2, EFALT_VA_ARG(HSD_JObj*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x481:
        spawned.object.generator =
            hsd_8039EFAC(0, 2, 0x7D3, EFALT_VA_ARG(HSD_JObj*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x482:
        spawned.object.model =
            efLib_Create_Attach(0x7D0U, gobj, EFALT_VA_ARG(HSD_JObj*));
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
            spawned.object.model->update = efLib_Cb_SetScaleRotY_FromFighter;
        }
        break;
    case 0x483:
        spawned.object.generator =
            efLib_CreateGenerator(0x7D7, EFALT_VA_ARG(Vec3*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x484:
        spawned.object.generator =
            hsd_8039EFAC(0, 2, 0x7DB, EFALT_VA_ARG(HSD_JObj*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x485:
        spawned.object.generator =
            hsd_8039EFAC(0, 2, 0x7DE, EFALT_VA_ARG(HSD_JObj*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x486: {
        f64 temp_d;
        f32 rotate_y_2;
        spawned.object.model =
            efLib_Create_Attach_Pos(0x7D1U, gobj, EFALT_VA_ARG(Vec3*));
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
            value_ptr = EFALT_VA_ARG(f32*);
            if (*value_ptr < 0.0f) {
                temp_d = -M_PI_2;
            } else {
                temp_d = M_PI_2;
            }
            rotate_y_2 = temp_d;
            HSD_JObjSetRotationY(GET_JOBJ(spawned.object.model->gobj),
                                 rotate_y_2);
        }
        break;
    }
    case 0x487:
        spawned.object.model = efLib_Create_AttachChild_Scale(
            0x7D2U, gobj, EFALT_VA_ARG(HSD_JObj*));
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
    case 0x488:
        spawned.object.model =
            efLib_Create_Attach_Scale(0xBB8U, gobj, EFALT_VA_ARG(HSD_JObj*));
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
    case 0x489:
        spawned.object.model =
            efLib_Create_Attach_Scale(0xBB9U, gobj, EFALT_VA_ARG(HSD_JObj*));
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
    case 0x48A:
        spawned.object.model =
            efLib_Create_Attach_Scale(0xBBAU, gobj, EFALT_VA_ARG(HSD_JObj*));
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
    case 0x48B:
        spawned.object.model =
            efLib_Create_Attach(0xBBBU, gobj, EFALT_VA_ARG(HSD_JObj*));
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
    case 0x48C:
        spawned.object.model =
            efLib_Create_Attach(0xBBCU, gobj, EFALT_VA_ARG(HSD_JObj*));
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
            spawned.object.model->update = efLib_Cb_SetRotYZ_FromFighter;
        }
        break;
    case 0x48D:
        spawned.object.generator =
            efLib_CreateGenerator_AppSRT_SetFacingDir(0xBC0, vlist_arg);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x48E: {
        spawned.object.model =
            efLib_Create_Attach_Pos(0xBBDU, gobj, EFALT_VA_ARG(Vec3*));
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
            effect = spawned.object.model;
            HSD_JObjSetRotationY(GET_JOBJ(effect->gobj), M_PI_2);
            HSD_JObjSetRotationZ(GET_JOBJ(effect->gobj), *EFALT_VA_ARG(f32*));
        }
        break;
    }
    case 0x48F: {
        spawned.object.model = efLib_Create_Attach_Scale_FacingDir(
            0xFA0U, gobj, EFALT_VA_ARG(HSD_JObj*));
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
                0xFA1U, gobj, EFALT_VA_ARG(HSD_JObj*));
        }
        break;
    }
    case 0x490: {
        spawned.object.model =
            efLib_Create_Attach(0xFA2U, gobj, EFALT_VA_ARG(HSD_JObj*));
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
            spawned.object.model->params.z = *EFALT_VA_ARG(f32*);
            spawned.object.model->update =
                efLib_Cb_SetRotYZ_FromParamZ_FighterDir;
        }
        break;
    }
    case 0x491:
        spawned.object.model =
            efLib_Create_Attach(0xFA4U, gobj, EFALT_VA_ARG(HSD_JObj*));
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
    case 0x492: {
        f64 temp_d;
        f32 rotate_y_2;
        spawned.object.model =
            efLib_Create_Attach_Scale(0xFA3U, gobj, EFALT_VA_ARG(HSD_JObj*));
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
            value_ptr = EFALT_VA_ARG(f32*);
            if (*value_ptr < 0.0f) {
                temp_d = -M_PI_2;
            } else {
                temp_d = M_PI_2;
            }
            rotate_y_2 = temp_d;
            HSD_JObjSetRotationY(GET_JOBJ(spawned.object.model->gobj),
                                 rotate_y_2);
        }
        break;
    }
    case 0x493: {
        f64 temp_d;
        f32 rotate_y_2;
        spawned.object.model =
            efLib_Create_Attach_Scale(0xFA5U, gobj, EFALT_VA_ARG(HSD_JObj*));
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
            value_ptr = EFALT_VA_ARG(f32*);
            if (*value_ptr < 0.0f) {
                temp_d = -M_PI_2;
            } else {
                temp_d = M_PI_2;
            }
            rotate_y_2 = temp_d;
            HSD_JObjSetRotationY(GET_JOBJ(spawned.object.model->gobj),
                                 rotate_y_2);
        }
        break;
    }
    case 0x494: {
        Fighter* fp;
        HSD_JObj* jobj;
        HSD_JObj* jobj_2;
        EF_Effect* effect_1;
        EF_Effect* effect_2;
        EF_Effect* next_eff;
        u16 effect_flags;

        effect_flags = 0x41;
        fp = gobj->user_data;
        jobj = fp->parts[FtPart_R3rdNa].joint;
        jobj_2 = fp->parts[FtPart_TransN].joint;
        spawned.object.model = efLib_Create_AttachChild(0x1388U, gobj, jobj);
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
            effect_1 = spawned.object.model;
            effect_1->update = efLib_Cb_SetRotYAndTransition;
            effect_1->lifetime = effect_flags;
            effect_1->user_data = NULL;
            next_eff = efLib_Create_Attach(0x1389U, gobj, jobj);
            effect_1->next = next_eff;
            if (next_eff != NULL) {
                effect_2 = effect_1->next;
                effect_2->update = efLib_Cb_SetRotYAndTransition;
                effect_2->lifetime = effect_flags;
                effect_2->user_data = fp;
                next_eff = efLib_Create_Attach(0x138AU, gobj, jobj_2);
                effect_2->next = next_eff;
                if (next_eff != NULL) {
                    effect_1 = effect_2->next;
                    effect_1->update = efLib_Cb_SetRotYAndTransition;
                    effect_1->lifetime = effect_flags;
                    effect_1->user_data = fp;
                }
            }
        }
        break;
    }
    case 0x495: {
        f64 temp_d;
        f32 rotate_y_2;
        spawned.object.model =
            efLib_Create_Attach_Scale(0x138BU, gobj, EFALT_VA_ARG(HSD_JObj*));
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
            value_ptr = EFALT_VA_ARG(f32*);
            if (*value_ptr < 0.0f) {
                temp_d = -M_PI_2;
            } else {
                temp_d = M_PI_2;
            }
            rotate_y_2 = temp_d;
            HSD_JObjSetRotationY(GET_JOBJ(spawned.object.model->gobj),
                                 rotate_y_2);
        }
        break;
    }
    case 0x496:
        spawned.object.model =
            efLib_Create_Attach(0x138CU, gobj, EFALT_VA_ARG(HSD_JObj*));
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
    case 0x497:
        spawned.object.model =
            efLib_Create_Attach(0x138DU, gobj, EFALT_VA_ARG(HSD_JObj*));
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
            spawned.object.model->update = efLib_Cb_SetRotY_FromFighterDir;
        }
        break;
    case 0x498:
        spawned.object.model =
            efLib_Create_AttachChild(0x138EU, gobj, EFALT_VA_ARG(HSD_JObj*));
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
    case 0x499:
        spawned.object.model =
            efLib_Create_AttachChild(0x138FU, gobj, EFALT_VA_ARG(HSD_JObj*));
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
    case 0x49A:
        spawned.object.generator =
            efLib_CreateGenerator_AppSRT_SetFacingDir(0x138B, vlist_arg);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x49B:
        spawned.object.generator =
            hsd_8039EFAC(0, 5, 0x138F, EFALT_VA_ARG(HSD_JObj*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x49C:
        spawned.object.generator =
            hsd_8039EFAC(0, 5, 0x1395, EFALT_VA_ARG(HSD_JObj*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x49D: {
        HSD_JObj* jobj;
        spawned.object.model =
            efLib_Create_Attach_Pos(0x1390U, gobj, EFALT_VA_ARG(Vec3*));
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
            HSD_JObj* gobj_jobj = GET_JOBJ(gobj);

            jobj = GET_JOBJ(spawned.object.model->gobj);
            HSD_JObjSetScaleX(jobj, HSD_JObjGetScaleY(gobj_jobj));
            HSD_JObjSetScaleY(jobj, HSD_JObjGetScaleY(gobj_jobj));
            HSD_JObjSetScaleZ(jobj, HSD_JObjGetScaleY(gobj_jobj));
            HSD_JObjSetTranslateZ(HSD_JObjGetChild(jobj),
                                  HSD_JObjGetTranslationZ(jobj) - 6.0f);
            if (*EFALT_VA_ARG(f32*) < 0.0f) {
                HSD_JObjSetRotationY(jobj, -M_PI_2);
                HSD_JObjSetRotationZ(jobj, -*EFALT_VA_ARG(f32*));
            } else {
                HSD_JObjSetRotationY(jobj, M_PI_2);
                HSD_JObjSetRotationZ(jobj, *EFALT_VA_ARG(f32*));
            }
            spawned.object.model->update = efLib_Cb_SetJObjOffsetZ;
        }
        break;
    }
    case 0x49E:
        spawned.object.generator =
            efLib_CreateGenerator(0x206, EFALT_VA_ARG(Vec3*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x49F: {
        f64 temp_d;
        f32 rotate_y_2;
        HSD_JObj* jobj_2;

        jobj_2 = EFALT_VA_ARG(HSD_JObj*);
        spawned.object.model = efLib_Create_Attach(0x7D00U, gobj, jobj_2);
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
            effect = spawned.object.model;
            value_ptr = EFALT_VA_ARG(f32*);
            if (*value_ptr < 0.0f) {
                temp_d = -M_PI_2;
            } else {
                temp_d = M_PI_2;
            }
            rotate_y_2 = temp_d;
            HSD_JObjSetRotationY(GET_JOBJ(effect->gobj), rotate_y_2);
        }
        hsd_8039EFAC(0, 0x20, 0x7D00, jobj_2);
        break;
    }
    case 0x4A0:
        spawned.object.generator =
            efLib_CreateGenerator(0x7D02, EFALT_VA_ARG(Vec3*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4A5:
        spawned.object.generator =
            efLib_CreateGenerator_Attach_Scale(0xA028, vlist_arg, gobj);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4A6:
        spawned.object.generator =
            efLib_CreateGenerator_Attach_Scale(0xA029, vlist_arg, gobj);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4A7:
        spawned.object.generator =
            efLib_CreateGenerator_Attach_Scale(0xA02A, vlist_arg, gobj);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4A8:
        spawned.object.generator =
            efLib_CreateGenerator_Attach_Scale(0xA02B, vlist_arg, gobj);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4A1:
        jobj = EFALT_VA_ARG(HSD_JObj*);
        spawned.object.generator = hsd_8039EFAC(0, 0x22, 0x84D4, jobj);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4A2:
        jobj = EFALT_VA_ARG(HSD_JObj*);
        spawned.object.generator = hsd_8039EFAC(0, 0x22, 0x84D2, jobj);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4A3:
        jobj = EFALT_VA_ARG(HSD_JObj*);
        spawned.object.generator = hsd_8039EFAC(0, 0x22, 0x84D3, jobj);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4A4: {
        f64 temp_d;
        f32 rotate_y_2;
        spawned.object.model =
            efLib_Create_Attach_Pos(0x84D0U, gobj, EFALT_VA_ARG(Vec3*));
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
            value_ptr = EFALT_VA_ARG(f32*);
            if (*value_ptr < 0.0f) {
                temp_d = -M_PI_2;
            } else {
                temp_d = M_PI_2;
            }
            rotate_y_2 = temp_d;
            HSD_JObjSetRotationY(GET_JOBJ(spawned.object.model->gobj),
                                 rotate_y_2);
        }
        break;
    }
    case 0x4A9: {
        f64 temp_d;
        f32 rotate_y_2;
        spawned.object.model =
            efLib_Create_Attach_Scale(0x9858U, gobj, EFALT_VA_ARG(HSD_JObj*));
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
            effect = spawned.object.model;
            value_ptr = EFALT_VA_ARG(f32*);
            if (*value_ptr < 0.0f) {
                temp_d = -M_PI_2;
            } else {
                temp_d = M_PI_2;
            }
            rotate_y_2 = temp_d;
            HSD_JObjSetRotationY(GET_JOBJ(effect->gobj), rotate_y_2);
            HSD_JObjAnimAll(GET_JOBJ(effect->gobj));
        }
        break;
    }
    case 0x4AA: {
        f64 temp_d;
        f32 rotate_y_2;
        spawned.object.model =
            efLib_Create_Attach_Scale(0x9859U, gobj, EFALT_VA_ARG(HSD_JObj*));
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
            effect = spawned.object.model;
            value_ptr = EFALT_VA_ARG(f32*);
            if (*value_ptr < 0.0f) {
                temp_d = -M_PI_2;
            } else {
                temp_d = M_PI_2;
            }
            rotate_y_2 = temp_d;
            HSD_JObjSetRotationY(GET_JOBJ(effect->gobj), rotate_y_2);
            HSD_JObjAnimAll(GET_JOBJ(effect->gobj));
        }
        break;
    }
    case 0x4AB: {
        spawned.object.model = efLib_Create_Attach_Scale_FacingDir(
            0x9471U, gobj, EFALT_VA_ARG(HSD_JObj*));
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
                0x9470U, gobj, EFALT_VA_ARG(HSD_JObj*));
        }
        break;
    }
    case 0x4AC: {
        spawned.object.model =
            efLib_Create_Attach_Pos(0x80E8U, gobj, EFALT_VA_ARG(Vec3*));
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
            effect = spawned.object.model;
            HSD_JObjSetRotationY(GET_JOBJ(effect->gobj), M_PI_2);
            HSD_JObjSetRotationZ(GET_JOBJ(effect->gobj), *EFALT_VA_ARG(f32*));
        }
        break;
    }
    case 0x4AD:
        jobj = EFALT_VA_ARG(HSD_JObj*);
        spawned.object.generator = hsd_8039EFAC(0, 0x24, 0x8CA0, jobj);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4AE:
        jobj = EFALT_VA_ARG(HSD_JObj*);
        spawned.object.generator = hsd_8039EFAC(0, 0x2E, 0xB3B0, jobj);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4AF:
        jobj = EFALT_VA_ARG(HSD_JObj*);
        spawned.object.generator = hsd_8039EFAC(0, 0x2E, 0xB3B1, jobj);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4B0:
        spawned.object.generator =
            efLib_CreateGenerator_AppSRT_SetFacingDir(0xB3B6, vlist_arg);
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4B1: {
        HSD_JObj* input_jobj;
        f64 temp_d;
        f32 rotate_y_2;

        input_jobj = EFALT_VA_ARG(HSD_JObj*);
        spawned.object.model = efLib_Create_Attach(0x9088U, gobj, input_jobj);
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
            value_ptr = EFALT_VA_ARG(f32*);
            if (*value_ptr < 0.0f) {
                temp_d = -M_PI_2;
            } else {
                temp_d = M_PI_2;
            }
            effect = spawned.object.model;
            rotate_y_2 = temp_d;
            HSD_JObjSetRotationY(GET_JOBJ(effect->gobj), rotate_y_2);
        }
        hsd_8039EFAC(0, 0x25, 0x9088, input_jobj);
        break;
    }
    case 0x4B2:
        spawned.object.generator =
            efLib_CreateGenerator(0x908A, EFALT_VA_ARG(Vec3*));
        spawned.kind = spawned.object.generator != NULL ? EF_SPAWN_GENERATOR
                                                        : EF_SPAWN_NONE;
        spawned.model_generation = 0;
        break;
    case 0x4B3: {
        spawned.object.model = efLib_Create_Attach_Scale_FacingDir(
            0xB799U, gobj, EFALT_VA_ARG(HSD_JObj*));
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
                0xB798U, gobj, EFALT_VA_ARG(HSD_JObj*));
        }
        break;
    }
    case 0x4B4:
        spawned.object.model = efLib_Create_Attach_Scale_FacingDir(
            0x4E20U, gobj, EFALT_VA_ARG(HSD_JObj*));
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
    case 0x4B5:
        spawned.object.model = efLib_Create_Attach_Scale_FacingDir(
            0x4E21U, gobj, EFALT_VA_ARG(HSD_JObj*));
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
    case 0x4B6:
        spawned.object.model =
            efLib_Create_AttachChild(0x5208U, gobj, EFALT_VA_ARG(HSD_JObj*));
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
        goto lbl_4B6_4B7_shared;
    case 0x4B7:
        spawned.object.model =
            efLib_Create_AttachChild(0x5209U, gobj, EFALT_VA_ARG(HSD_JObj*));
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
    lbl_4B6_4B7_shared:
        if (spawned.object.model != NULL) {
            effect = spawned.object.model;
            HSD_JObjGetScale(GET_JOBJ(gobj), &scale);
            if (GET_JOBJ(effect->gobj) == NULL) {
                __assert("jobj.h", 823, "jobj");
            }
            HSD_JObjSetScale(GET_JOBJ(effect->gobj), &scale);
        }
        break;
    case 0x4B8:
        spawned.object.model = efLib_Create_Attach_Scale_FacingDir(
            0xBB80U, gobj, EFALT_VA_ARG(HSD_JObj*));
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
    case 0x4B9:
        spawned.object.model = efLib_Create_Attach_Scale_FacingDir(
            0xBB81U, gobj, EFALT_VA_ARG(HSD_JObj*));
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
        s32 cnt;

        cnt = efLib_AnimCount - 1;
        efLib_AnimCount = cnt;
        // horrible. its actually just efLib_AnimQueue[cnt]
#ifdef MELEE_NATIVE
        HSD_JObjAnimAll(efLib_AnimQueue[cnt]);
#else
        HSD_JObjAnimAll(
            ((EF_ParamEntry*) (((u32*) efLib_AnimQueue) + cnt))->gobj);
#endif
    }

    return spawned;
}
