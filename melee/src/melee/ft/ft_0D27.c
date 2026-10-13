#include "ft_0D27.h"

#include <Runtime/platform.h>

#include <stdbool.h>

#include "fighter.h"
#include "forward.h"
#include "ft_081B.h"
#include "ft_0877.h"
#include "ftcamera.h"
#include "ftchangeparam.h"
#include "ftcommon.h"
#include "ftlib.h"
#include "inlines.h"
#include "kinds/ftCommon/forward.h"
#include "kinds/ftCommon/ftCo_Attack100.h"
#include "kinds/ftCommon/ftCo_CaptureCut.h"
#include "kinds/ftCommon/ftCo_Fall.h"
#include "kinds/ftCommon/ftCo_FallAerial.h"
#include "kinds/ftCommon/ftCo_FallSpecial.h"
#include "kinds/ftCommon/ftCo_Lift.h"
#include "types.h"
#include "native_fighter_runtime.h"
#include "native_callback_lifetime.h"
#include "native_fighter_lifetime.h"
#include <melee/it/kinds/itkinoko.h>
#include <sysdolphin/baselib/jobj.h>

void ftCo_800D2770(Fighter_GObj* gobj, f32 arg8)
{
    Fighter* fp = GET_FIGHTER(gobj);
    ftLib_SetScale(gobj, arg8);
    ftCo_800D105C(gobj);
    ftCamera_80076064(fp);
    ft_80081C88(gobj, fp->x34_scale.y);
}

bool ftCo_800D27C4(Fighter* fp)
{
    bool var_r0 = fp->motion_id == ftCo_MS_DamageIce ? false : true;
    if (!var_r0) {
        return false;
    }
    switch (fp->x2070.x2071_b0_3) {
    case 12:
    case 13:
        return false;
    }
    return true;
}

bool fn_800D2818(Fighter* fp)
{
    int tmp;
    bool var_r0 = fp->motion_id == ftCo_MS_DamageIce ? false : true;
    if (!var_r0) {
        return false;
    }
    if (fp->motion_id >= ftCo_MS_KinokoGiantStart &&
        fp->motion_id <= ftCo_MS_KinokoSmallEndAir)
    {
        return false;
    }
    tmp = fp->x2070.x2071_b0_3;
    if (tmp != 5 && (tmp >= 5 || tmp != 0) && !fp->x2070.x2071_b4) {
        return false;
    } else {
        return true;
    }
}

bool ftCo_Mushroom_SelectAirResume(Fighter_GObj* gobj, int ms_id)
{
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(gobj, NativeCallbackSource_Fighter, &generation)) return false;
    Fighter* fp = GET_FIGHTER(gobj);
    NativeFighterLifetime lifetime;
    if (!nativeFighterLifetime_Capture(fp, &lifetime)) return false;
    const FighterKind kind = fp->kind;
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(kind);
    if (lifetime.source_owned) {
        if (nativeScriptFighter_HasHook(gobj, NativeScriptFighter_AirResumeMotion)) {
            NativeScriptValue argument = {.type=NativeScriptType_I32,.as.i64=ms_id}, result;
            if (nativeScriptFighter_Run(gobj, NativeScriptFighter_AirResumeMotion,
                    &argument, 1, NULL, &result) != NativeScriptVM_Ok ||
                !nativeFighterLifetime_Current(&lifetime)) return false;
            int32_t mapped = (int32_t)result.as.i64;
            if (mapped >= 0 && lifetime.program->bank->has_motion_layout &&
                nativeScriptMoves_Contains(&lifetime.program->bank->motion_layout,
                                          (uint32_t)mapped)) ms_id = mapped;
        }
    } else if (definition != NULL && definition->on_air_resume_motion != NULL) {
        if (!nativeCallbackDispatch_Enter()) return false;
        int32_t mapped = definition->on_air_resume_motion(gobj, ms_id);
        nativeCallbackDispatch_Leave();
        if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter) ||
            fp->kind != kind || nativeFighterRuntime_Definition(kind) != definition) return false;
        if (mapped >= 0 && (mapped < ftCo_MS_Count ||
            (mapped >= definition->motion_state_first &&
             (uint32_t)(mapped - definition->motion_state_first) < definition->motion_state_count))) ms_id = mapped;
    } else if (fp->kind == Ft_Kind_Peach && ms_id == 0x172) {
        fp->mv.co.mushroom.resume_air = ftCo_800968C8;
        return true;
    }
    switch (ms_id) {
    case ftCo_MS_FallAerial:
        fp->mv.co.mushroom.resume_air = ftCo_FallAerial_Enter;
        return true;
    case ftCo_MS_FallSpecial:
    case ftCo_MS_ItemParasolFallSpecial:
        fp->mv.co.mushroom.resume_air = ftCo_800968C8;
        return true;
    default:
        fp->mv.co.mushroom.resume_air = ftCo_Fall_Enter;
        return true;
    }
}

void fn_800D290C(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    Fighter_GObj* victim;

    ftCommon_8007DB58(gobj);
    ftCo_8009750C(gobj);

    victim = getFtVictim(fp);
    if (victim != NULL) {
        if (fp->x221B_b5) {
            ftCo_CaptureCut_Enter(victim);
        } else {
            HSD_GObj* tmp = victim;
            ftCo_CaptureCut_Enter(gobj);
            ftCo_800DA698(tmp, false);
        }
        ftCommon_8007DB58(victim);
    }
}

void fn_800D299C(Fighter_GObj* gobj, int idx, f32 arg8, f32 arg9)
{
    static HSD_Joint lbl_803C6FF0 = {
        NULL,
        8,
        NULL,
        NULL,
        { NULL },
        { 0.0F, 0.0F, 0.0F },
        { 1.0F, 1.0F, 1.0F },
        { 0.0F, 0.0F, 0.0F },
    };

    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->x2184 == NULL) {
        fp->x2184 = HSD_JObjLoadJoint(&lbl_803C6FF0);
    }
    fp->mv.co.mushroom.start_scale = arg8;
    fp->mv.co.mushroom.target_scale = arg9;
    HSD_JObjAddAnim(fp->x2184, it_80293660(idx), NULL, NULL);
    ftAnim_80070734(fp->x2184, 0.0F);
    HSD_JObjAnim(fp->x2184);
}

bool fn_800D2A3C(HSD_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    HSD_JObjAnim(fp->x2184);
    {
        f32 scale = HSD_JObjGetScaleX(fp->x2184);
        f32 mid = fp->mv.co.mushroom.start_scale;
        f32 fast = fp->mv.co.mushroom.target_scale;
        ftLib_SetScale(gobj, (fast - mid) * scale + mid);
    }
    if (HSD_AObjGetFlags(fp->x2184->aobj) & 0x40000000) {
        return false;
    } else {
        return true;
    }
}

void fn_800D2AD8(Fighter* fp)
{
    ft_PlaySFX(fp, ftCo_MS_CaptureDamageKoopaAir, 0x7F, 0x40);
}

void fn_800D2B04(Fighter* fp)
{
    ft_PlaySFX(fp, ftCo_MS_CaptureWaitKoopaAir, 0x7F, 0x40);
}

void fn_800D2B30(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->mv.co.mushroom.clear_velocity = 0;
    fp->mv.co.mushroom.ground_accel_1 = fp->xE4_ground_accel_1;
    fp->mv.co.mushroom.ground_accel_2 = fp->xE8_ground_accel_2;
    fp->mv.co.mushroom.self_accel = fp->x74_self_accel;
    fp->mv.co.mushroom.ground_velocity = fp->gr_vel;
    fp->mv.co.mushroom.self_velocity = fp->self_vel;
    fp->mv.co.mushroom.ground_knockback = fp->xF0_ground_kb_vel;
    fp->mv.co.mushroom.knockback = fp->x8c_kb_vel;
    fp->mv.co.mushroom.ground_shield_knockback = fp->xF4_ground_attacker_shield_kb_vel;
    fp->mv.co.mushroom.shield_knockback = fp->x98_atk_shield_kb;
    fp->mv.co.mushroom.other_velocity = fp->xD4_unk_vel;
    ftCommon_8007E2FC(fp->gobj);
}

void fn_800D2BF8(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->xE4_ground_accel_1 = fp->mv.co.mushroom.ground_accel_1;
    fp->xE8_ground_accel_2 = fp->mv.co.mushroom.ground_accel_2;
    fp->x74_self_accel = fp->mv.co.mushroom.self_accel;
    fp->gr_vel = fp->mv.co.mushroom.ground_velocity;
    fp->self_vel = fp->mv.co.mushroom.self_velocity;
    fp->xF0_ground_kb_vel = fp->mv.co.mushroom.ground_knockback;
    fp->x8c_kb_vel = fp->mv.co.mushroom.knockback;
    fp->xF4_ground_attacker_shield_kb_vel = fp->mv.co.mushroom.ground_shield_knockback;
    fp->x98_atk_shield_kb = fp->mv.co.mushroom.shield_knockback;
    fp->xD4_unk_vel = fp->mv.co.mushroom.other_velocity;
}

void fn_800D2CA0(Fighter* fp)
{
    ftCommon_8007D7FC(fp);
    fp->mv.co.mushroom.ground_velocity = fp->mv.co.mushroom.self_velocity.x;
}

void fn_800D2CD4(Fighter* fp)
{
    ftCommon_8007D5D4(fp);
    fp->mv.co.mushroom.ground_velocity = 0.0F;
    fp->mv.co.mushroom.self_accel.y = 0.0F;
}

bool ft_800D2D0C(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->item_gobj != NULL && itGetKind(fp->item_gobj) == It_Kind_ScBall) {
        return true;
    }
    return false;
}
