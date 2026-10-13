#include "ftCo_JumpAerial.h"

#include <melee/ft/forward.h>

#include <placeholder.h>

#include "forward.h"
#include "ftCo_AirCatch.h"
#include "ftCo_Attack100.h"
#include "ftCo_AttackAir.h"
#include "ftCo_EscapeAir.h"
#include "ftCo_FallAerial.h"
#include "ftCo_FallSpecial.h"
#include "ftCo_HammerWait.h"
#include "ftCo_ItemParasolOpen.h"
#include "ftCo_ItemScrew.h"
#include "ftCo_ItemThrow.h"
#include "ftCo_SpecialAir.h"
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0877.h>
#include <melee/ft/ft_0881.h>
#include <melee/ft/ft_0C8C.h>
#include <melee/ft/ft_0D27.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/inlines.h>
#include "ftCo_AerialAction.h"
#include <melee/ft/types.h>
#ifdef MELEE_NATIVE
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_script_fighter.h>
#endif
#include <sysdolphin/baselib/jobj.h>

/* 0CC3C4 */ static void ftYs_JumpAerial_Anim_Cb(Fighter_GObj* gobj);
/* 0CC654 */ static void ftNs_JumpAerial_Phys_Cb(Fighter_GObj* gobj);

void ft_800CB6EC(Fighter* fp, s32 arg1)
{
    if (fp->mv.co.jumpaerial.x0 != 0) {
        fp->mv.co.jumpaerial.x0 -= 1;
        HSD_JObjAddRotationY(fp->parts->joint, -MTXDegToRad(180.0F / arg1));
        if (fp->mv.co.jumpaerial.x0 == (arg1 / 2)) {
            fp->facing_dir = -fp->facing_dir;
        }
    }
}

bool ft_did_jump(Fighter* fp, bool arg1)
{
    if (fp->x1968_jumpsUsed < fp->co_attrs.max_jumps &&
        (((HSD_PadProfileTapJumpEnabled(fp->input.held_buttons[0])) &&
          fp->input.lstick[0].y >= p_ftCommonData->tap_jump_threshold &&
          fp->x671_timer_lstick_tilt_y < p_ftCommonData->tap_jump_window) ||
         HSD_PadProfileHasJumpButton(fp->input.pressed_buttons)) &&
        !(arg1 && (fp->x68A < p_ftCommonData->x1C)))
    {
        return true;
    }

    return false;
}

bool ftCo_800CB870(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);

    if (ftCo_800C5240(gobj)) {
        return false;
    }
    if (fp->can_multijump) {
        return ftCo_800D730C(gobj, false);
    }
    return ftCo_JumpAerial_CheckInput(gobj, false);
}

bool ftCo_800CB8E0(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);

    if (ftCo_800C5240(gobj)) {
        return false;
    }
    if (fp->can_multijump) {
        return ftCo_800D730C(gobj, true);
    }
    return ftCo_JumpAerial_CheckInput(gobj, true);
}

bool ftCo_JumpAerial_CheckInput(Fighter_GObj* gobj, bool arg1)
{
    ftCo_DatAttrs* co_attrs;
    Fighter* fp = GET_FIGHTER(gobj);
    PAD_STACK(8);
    co_attrs = &fp->co_attrs;
    if (ft_did_jump(fp, arg1)) {
        if (ft_800D2D0C(gobj)) {
            Vec3 v;
            PAD_STACK(8);
            v.x = fp->input.lstick[0].x * co_attrs->air_jump_h_multiplier;
            v.y = co_attrs->jump_v_initial_velocity *
                  co_attrs->air_jump_v_multiplier;
            v.z = 0.0F;
            ft_800D2E7C(gobj, &v);
            return true;
        }
#ifdef MELEE_NATIVE
        if(nativeScriptFighter_IsAttached(gobj)) {
            if(nativeScriptFighter_Status(gobj)!=NativeScriptVM_Ok)return false;
            if(nativeScriptFighter_HasHook(gobj,NativeScriptFighter_AerialJump)) {
                (void)nativeScriptFighter_Run(gobj,NativeScriptFighter_AerialJump,NULL,0, NULL,NULL);
                return true; // Source execution is terminal, including failure.
            }
            // Absent custom entry retains the general engine jump. An owned
            // source never selects Ness/Yoshi/Peach/Mewtwo native handlers.
            ftCo_JumpAerial_Enter_Basic(gobj);
            return true;
        }
#endif
        switch (fp->kind) {
        case Ft_Kind_Ness:
            ftNs_JumpAerial_Enter(gobj);
            break;
        case Ft_Kind_Yoshi:
            ftYs_JumpAerial_Enter(gobj);
            break;
        case Ft_Kind_Peach:
            ftPe_JumpAerial_Enter(gobj);
            break;
        case Ft_Kind_Mewtwo:
            ftMt_JumpAerial_Enter(gobj);
            break;
        default:
            ftCo_JumpAerial_Enter_Basic(gobj);
            break;
        }
        return true;
    }
    return false;
}

static inline void ft_SetVec(Vec3* dst, Vec3* src)
{
    dst->x = src->x;
    dst->y = src->y;
    dst->z = src->z;
}

void ftCo_JumpAerial_Begin(Fighter_GObj* gobj, FtMotionId msid, Vec3* vel,
                          bool reset_input_timer, bool animation_driven,
                          bool worn_item_sound)
{
    Fighter* fp = GET_FIGHTER(gobj);
#ifdef MELEE_NATIVE
    NativeFighterLifetime source;
    if(!nativeFighterLifetime_Capture(fp,&source))return;
    uint32_t jump_physics=UINT32_MAX;
    if(source.source_owned && animation_driven) {
        jump_physics=source.program->hooks[NativeScriptFighter_AnimationDrivenJumpPhysics];
        if(jump_physics==UINT32_MAX)return;
    }
#endif

    Fighter_ChangeMotionState(gobj, msid, Ft_MF_SkipNametagVis, 0.0F, 1.0F,
                              0.0F, NULL);
#ifdef MELEE_NATIVE
    if(!nativeFighterLifetime_Current(&source))return;
#endif
    fp->x2221_b7 = true;
    ft_SetVec(&fp->self_vel, vel);
    if (reset_input_timer) {
        fp->x671_timer_lstick_tilt_y = 0xFE;
    }
    fp->x1968_jumpsUsed += 1;
    ft_80088770(fp);
#ifdef MELEE_NATIVE
    if(!nativeFighterLifetime_Current(&source))return;
#endif
    ft_80088328(fp, fp->ft_data->x4C_sfx->x14, SFX_VOLUME_MAX, SFX_PAN_MID);
#ifdef MELEE_NATIVE
    if(!nativeFighterLifetime_Current(&source))return;
#endif
    if (worn_item_sound && fp->x197C != NULL) {
        ft_PlaySFX(fp, 0x11B, SFX_VOLUME_MAX, SFX_PAN_MID);
    }
#ifdef MELEE_NATIVE
    if(!nativeFighterLifetime_Current(&source))return;
    if(animation_driven && source.source_owned) {
        // Preserve the native late override, using only an owned prepared ID.
        (void)nativeScriptFighter_BindMotionPhase(gobj,NativeScriptMotion_Physics,jump_physics,false);
        return;
    }
#endif
    if (animation_driven) fp->phys_cb = ftCo_JumpAerial_Phys_Cb;
}

void ftCo_JumpAerial_Enter_Basic(Fighter_GObj* gobj)
{
    ftCo_DatAttrs* co_attrs;
    Fighter* fp = GET_FIGHTER(gobj);
    FtMotionId msid;
    Vec3 vel;
    co_attrs = &fp->co_attrs;
    PAD_STACK(8);

    ftCommon_8007D5D4(fp);
    fp->cmd_vars[0] = 1;
    msid = (fp->input.lstick[0].x * fp->facing_dir) > -p_ftCommonData->x78
               ? ftCo_MS_JumpAerialF
               : ftCo_MS_JumpAerialB;
    vel.x = fp->input.lstick[0].x * co_attrs->air_jump_h_multiplier;
    vel.y =
        co_attrs->jump_v_initial_velocity * co_attrs->air_jump_v_multiplier;
    vel.z = 0.0F;
    ftCo_JumpAerial_Begin(gobj, msid, &vel, true, false,
                          fp->kind != Ft_Kind_Mewtwo && fp->kind != Ft_Kind_Ness);
}

void ftNs_JumpAerial_Enter(Fighter_GObj* gobj)
{
    ftCo_DatAttrs* co_attrs_r31;
    Fighter* fp = GET_FIGHTER(gobj);
    Vec3 vel = { 0.0F, 0.0F, 0.0F };
    FtMotionId msid;
    PAD_STACK(4);
    co_attrs_r31 = &fp->co_attrs;

    ftCommon_8007D5D4(fp);
    fp->cmd_vars[0] = 1;
    msid = (fp->input.lstick[0].x * fp->facing_dir) > -p_ftCommonData->x78
               ? ftCo_MS_JumpAerialF
               : ftCo_MS_JumpAerialB;

    ftCo_JumpAerial_Begin(gobj, msid, &vel, true, false, false);
    fp->phys_cb = (HSD_GObjEvent) ftNs_JumpAerial_Phys_Cb;
    fp->mv.co.jumpaerial.init_h_vel =
        fp->input.lstick[0].x * co_attrs_r31->air_jump_h_multiplier;
}

void ftYs_JumpAerial_Enter(Fighter_GObj* gobj)
{
    // mv.co.jumpaerial.x0 could also be mv.ys.jumpaerial.x0
    Fighter* fp = GET_FIGHTER(gobj);
    ftYoshiAttributes* ys_attrs = fp->dat_attrs;
    Vec3 vel;
    int arg1;
    PAD_STACK(16);

    ftCommon_8007D5D4(fp);
    fp->cmd_vars[0] = 1;
    vel.x = fp->input.lstick[0].x * fp->co_attrs.air_jump_h_multiplier;
    vel.y = 0.0F;
    vel.z = 0.0F;
    ftCo_JumpAerial_Begin(gobj, ftCo_MS_JumpAerialF, &vel, true, true, true);
    fp->anim_cb = (HSD_GObjEvent) ftYs_JumpAerial_Anim_Cb;
    fp->dmg.armor1 = ys_attrs->x8;
    if ((fp->input.lstick[0].x * fp->facing_dir) < -ys_attrs->x4) {
        fp->mv.co.jumpaerial.x0 = ys_attrs->x0;
    } else {
        fp->mv.co.jumpaerial.x0 = 0;
    }

    arg1 = ys_attrs->x0;
    ft_800CB6EC(fp, arg1);
}

void ftPe_JumpAerial_Enter(Fighter_GObj* gobj)
{
    ftCo_DatAttrs* co_attrs;
    Fighter* fp_r31 = GET_FIGHTER(gobj);
    FtMotionId msid;
    Vec3 vel;
    co_attrs = &fp_r31->co_attrs;
    PAD_STACK(8);

    ftCommon_8007D5D4(fp_r31);
    fp_r31->cmd_vars[0] = 1;
    msid =
        (fp_r31->input.lstick[0].x * fp_r31->facing_dir) > -p_ftCommonData->x78
            ? ftCo_MS_JumpAerialF
            : ftCo_MS_JumpAerialB;
    vel.x = fp_r31->input.lstick[0].x * co_attrs->air_jump_h_multiplier;
    vel.y = 0.0F;
    vel.z = 0.0F;
    ftCo_JumpAerial_Begin(gobj, msid, &vel, true, true, true);
}

void ftMt_JumpAerial_Enter(Fighter_GObj* gobj)
{
    ftCo_DatAttrs* co_attrs;
    Fighter* fp_r31 = GET_FIGHTER(gobj);
    FtMotionId msid;
    Vec3 vel;
    co_attrs = &fp_r31->co_attrs;
    PAD_STACK(8);

    ftCommon_8007D5D4(fp_r31);
    fp_r31->cmd_vars[0] = 1;
    msid =
        (fp_r31->input.lstick[0].x * fp_r31->facing_dir) > -p_ftCommonData->x78
            ? ftCo_MS_JumpAerialF
            : ftCo_MS_JumpAerialB;
    vel.x = fp_r31->input.lstick[0].x * co_attrs->air_jump_h_multiplier;
    vel.y = 0.0F;
    vel.z = 0.0F;
    ftCo_JumpAerial_Begin(gobj, msid, &vel, true, true, false);
}

void ftCo_JumpAerial_Anim(Fighter_GObj* gobj)
{
    if (!ftAnim_IsFramesRemaining(gobj)) {
        ftCo_FallAerial_Enter(gobj);
    }
}

void ftYs_JumpAerial_Anim_Cb(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    ftYoshiAttributes* ys_attrs = fp->dat_attrs;
    PAD_STACK(8);
    ftCo_JumpAerial_Anim(gobj);
    ft_800CB6EC(fp, ys_attrs->x0);
}

void ftCo_JumpAerial_IASA(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
#ifdef MELEE_NATIVE
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(gobj, NativeCallbackSource_Fighter,
                                       &generation)) return;
    bool owned=nativeScriptFighter_IsAttached(gobj);
    if(owned && nativeScriptFighter_Status(gobj)!=NativeScriptVM_Ok)return;
#endif
    RETURN_IF(ftCo_SpecialAir_CheckInput(gobj));
#ifdef MELEE_NATIVE
    if (!nativeCallbackSource_Alive(gobj, generation,
                                    NativeCallbackSource_Fighter) ||
        (owned && nativeScriptFighter_Status(gobj)!=NativeScriptVM_Ok)) return;
#endif
    RETURN_IF(ftCo_80095328(gobj, NULL, NULL));
#ifdef MELEE_NATIVE
    if (!nativeCallbackSource_Alive(gobj, generation,
                                    NativeCallbackSource_Fighter) ||
        (owned && nativeScriptFighter_Status(gobj)!=NativeScriptVM_Ok)) return;
#endif
    RETURN_IF(ftCo_800D7100(gobj));
#ifdef MELEE_NATIVE
    if (!nativeCallbackSource_Alive(gobj, generation,
                                    NativeCallbackSource_Fighter) ||
        (owned && nativeScriptFighter_Status(gobj)!=NativeScriptVM_Ok)) return;
#endif
    RETURN_IF(ftCo_800C3B10(gobj));
#ifdef MELEE_NATIVE
    if (!nativeCallbackSource_Alive(gobj, generation,
                                    NativeCallbackSource_Fighter) ||
        (owned && nativeScriptFighter_Status(gobj)!=NativeScriptVM_Ok)) return;
#endif
    RETURN_IF(ftCo_80099A58(gobj));
#ifdef MELEE_NATIVE
    if (!nativeCallbackSource_Alive(gobj, generation,
                                    NativeCallbackSource_Fighter) ||
        (owned && nativeScriptFighter_Status(gobj)!=NativeScriptVM_Ok)) return;
#endif
    RETURN_IF(ftCo_AttackAir_CheckItemThrowInput(gobj));
#ifdef MELEE_NATIVE
    if (!nativeCallbackSource_Alive(gobj, generation,
                                    NativeCallbackSource_Fighter) ||
        (owned && nativeScriptFighter_Status(gobj)!=NativeScriptVM_Ok)) return;
#endif
    RETURN_IF(ftCo_800D705C(gobj));
#ifdef MELEE_NATIVE
    if (!nativeCallbackSource_Alive(gobj, generation,
                                    NativeCallbackSource_Fighter) ||
        (owned && nativeScriptFighter_Status(gobj)!=NativeScriptVM_Ok)) return;
#endif
    RETURN_IF(ftCo_AerialAction_CheckManual(gobj));
#ifdef MELEE_NATIVE
    if (!nativeCallbackSource_Alive(gobj, generation,
                                    NativeCallbackSource_Fighter) ||
        (owned && nativeScriptFighter_Status(gobj)!=NativeScriptVM_Ok)) return;
#endif
    RETURN_IF(ftCo_800CB870(gobj));
#ifdef MELEE_NATIVE
    if (!nativeCallbackSource_Alive(gobj, generation,
                                    NativeCallbackSource_Fighter) ||
        (owned && nativeScriptFighter_Status(gobj)!=NativeScriptVM_Ok)) return;
#endif
    RETURN_IF(fp->cmd_vars[0] && ftCo_AerialAction_CheckAutomatic(gobj));
#ifdef MELEE_NATIVE
    if (!nativeCallbackSource_Alive(gobj, generation,
                                    NativeCallbackSource_Fighter) ||
        (owned && nativeScriptFighter_Status(gobj)!=NativeScriptVM_Ok)) return;
#endif
    RETURN_IF(!fp->cmd_vars[0]);
#ifdef MELEE_NATIVE
    if (!nativeCallbackSource_Alive(gobj, generation,
                                    NativeCallbackSource_Fighter) ||
        (owned && nativeScriptFighter_Status(gobj)!=NativeScriptVM_Ok)) return;
#endif
    RETURN_IF(ftCo_800CEE70(gobj));
}

void ftCo_JumpAerial_Phys(Fighter_GObj* gobj)
{
    ft_80084DB0(gobj);
}

void ftNs_JumpAerial_Phys_Cb(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    ftCommon_CalcSelfAccel_DriftFrom(fp, fp->mv.co.jumpaerial.init_h_vel);
    fp->mv.co.jumpaerial.init_h_vel += fp->x74_self_accel.x;
    fp->x74_self_accel.x = 0.0F;
    fp->self_vel.x = fp->x6A4_transNOffset.z * fp->facing_dir +
                     fp->mv.co.jumpaerial.init_h_vel;
    ft_800851D0(gobj);
}

void ftCo_JumpAerial_Phys_Cb(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    ftCommon_CalcSelfAccel_Drift(fp);
    ft_800851D0(gobj);
}

void ftCo_JumpAerial_Coll(Fighter_GObj* gobj)
{
    ft_800835B0(gobj, ftCo_80096CC8, (HSD_GObjEvent) ft_80082B1C);
}
