#include "ftCo_AttackAir.h"

#include <Runtime/platform.h>

#include <melee/ft/forward.h>

#include "forward.h"
#include "ftCo_AirCatch.h"
#include "ftCo_Attack100.h"
#include "ftCo_Fall.h"
#include "ftCo_ItemThrow.h"
#include "ftCo_JumpAerial.h"
#include "ftCo_LandingAir.h"
#include <dolphin/mtx.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_0CDD.h>
#include <melee/ft/ft_0DF1.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/kinds/ftGameWatch/ftgamewatchattackair.h>
#include <melee/ft/kinds/ftLink/ftlinkattackair.h>
#include <melee/ft/types.h>
#include <melee/ft/native_fighter_runtime.h>
#ifdef MELEE_NATIVE
#include <melee/ft/native_fighter_lifetime.h>
#endif
#include <melee/ft/native_callback_lifetime.h>
#include <melee/it/it_26B1.h>

/* 08CE0C */ static void decideFighter(Fighter_GObj* gobj);

/// @todo This probably doesn't exist; see #ftCo_AttackAir_CheckItemThrowInput.
static bool checkItemThrowInput(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->input.pressed_buttons & HSD_PAD_A || ftCo_800DF478(fp)) {
        if ((unsigned) ftCo_AttackAir_GetMsidFromCStick(fp) -
                    ftCo_MS_AttackAirN <=
                1 &&
            fp->item_gobj != NULL && itGetUseKind(fp->item_gobj) == 3)
        {
            ftCo_800CDDA0(gobj);
            return true;
        } else {
            decideFighter(gobj);
            return true;
        }
    } else {
        return false;
    }
}

/**
 * @todo This is exactly #checkItemThrowInput but it uses too much stack for
 *       #ftCo_AttackAirLw_IASA to call it.
 */
bool ftCo_AttackAir_CheckItemThrowInput(Fighter_GObj* gobj)
{
    u8 _[8];
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->input.pressed_buttons & HSD_PAD_A || ftCo_800DF478(fp)) {
        if ((unsigned) ftCo_AttackAir_GetMsidFromCStick(fp) -
                    ftCo_MS_AttackAirN <=
                1 &&
            fp->item_gobj != NULL && itGetUseKind(fp->item_gobj) == 3)
        {
            ftCo_800CDDA0(gobj);
            return true;
        } else {
            decideFighter(gobj);
            return true;
        }
    } else {
        return false;
    }
}

static void decideFighter(Fighter_GObj* gobj)
{
#ifdef MELEE_NATIVE
    Fighter* fp = GET_FIGHTER(gobj);
    if (nativeScriptFighter_IsAttached(gobj)) {
        NativeFighterLifetime lifetime;
        if (!nativeFighterLifetime_Capture(fp, &lifetime)) return;
        int32_t selected = ftCo_AttackAir_GetMsidFromCStick(fp);
        int32_t motion = selected;
        if (nativeScriptFighter_HasHook(gobj, NativeScriptFighter_AerialAttackMotion)) {
            NativeScriptValue argument = { .type=NativeScriptType_I32, .as.i64=selected }, result;
            if (nativeScriptFighter_Run(gobj, NativeScriptFighter_AerialAttackMotion,
                                       &argument, 1, NULL, &result) != NativeScriptVM_Ok ||
                !nativeFighterLifetime_Current(&lifetime)) return;
            motion = (int32_t) result.as.i64;
            const NativeScriptMoveLayout* layout = &lifetime.program->bank->motion_layout;
            if (!lifetime.program->bank->has_motion_layout ||
                !nativeScriptMoves_Contains(layout, (uint32_t) motion)) motion = selected;
        }
        if (!nativeFighterLifetime_Current(&lifetime)) return;
        ftCo_AttackAir_EnterFromMsid(gobj, motion);
        return;
    }
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(fp->kind);
    if (definition != NULL && definition->on_aerial_attack_motion != NULL) {
        uint32_t generation;
        if (!nativeCallbackSource_Snapshot(gobj, NativeCallbackSource_Fighter, &generation)) return;
        const FighterKind kind = fp->kind;
        int32_t selected = ftCo_AttackAir_GetMsidFromCStick(fp);
        int32_t motion = definition->on_aerial_attack_motion(gobj, selected);
        if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter) ||
            fp->kind != kind || nativeFighterRuntime_Definition(kind) != definition) return;
        if (motion != selected && (motion < definition->motion_state_first ||
            (uint32_t)(motion - definition->motion_state_first) >= definition->motion_state_count))
            motion = selected;
        ftCo_AttackAir_EnterFromMsid(gobj, motion);
        return;
    }
#endif
    switch (GET_FIGHTER(gobj)->kind) {
    case Ft_Kind_Link:
    case Ft_Kind_CLink:
        ftLk_AttackAir_Enter(gobj);
        return;
    case Ft_Kind_GameWatch:
        ftGw_AttackAirN_DecideAction(gobj);
        return;
    default:
        ftCo_AttackAir_EnterFromCStick(gobj);
        return;
    }
}

FtMotionId ftCo_AttackAir_GetMsidFromCStick(Fighter* fp)
{
    float stick_angle, stick_x, stick_y;
    if (ftCo_800DF478(fp)) {
        stick_x = fp->input.cstick[0].x;
        stick_y = fp->input.cstick[0].y;
        stick_angle = ftCo_GetCStickAngle(fp);
    } else {
        stick_x = fp->input.lstick[0].x;
        stick_y = fp->input.lstick[0].y;
        stick_angle = ftCo_GetLStickAngle(fp);
    }
    if (ABS(stick_x) < p_ftCommonData->xDC &&
        ABS(stick_y) < p_ftCommonData->xE0)
    {
        return ftCo_MS_AttackAirN;
    }
    if (stick_angle > p_ftCommonData->x20_radians) {
        return ftCo_MS_AttackAirHi;
    }
    if (stick_angle < -p_ftCommonData->x20_radians) {
        return ftCo_MS_AttackAirLw;
    }
    if (stick_x * fp->facing_dir >= 0) {
        return ftCo_MS_AttackAirF;
    }
    return ftCo_MS_AttackAirB;
}

void ftCo_AttackAir_EnterFromCStick(Fighter_GObj* gobj)
{
    ftCo_AttackAir_EnterFromMsid(
        gobj, ftCo_AttackAir_GetMsidFromCStick(GET_FIGHTER(gobj)));
}

void ftCo_AttackAir_EnterFromMsid(Fighter_GObj* gobj, FtMotionId msid)
{
    Fighter* fp = GET_FIGHTER(gobj);
#ifdef MELEE_NATIVE
    NativeFighterLifetime lifetime;
    if (!nativeFighterLifetime_Capture(fp, &lifetime)) return;
#endif
    fp->allow_interrupt = false;
    fp->cmd_vars[0] = 0;
    fp->throw_flags = 0;
    Fighter_ChangeMotionState(gobj, msid, Ft_MF_KeepFastFall, 0, 1, 0, NULL);
#ifdef MELEE_NATIVE
    if (!nativeFighterLifetime_Current(&lifetime)) return;
#endif
    ftAnim_8006EBA4(gobj);
#ifdef MELEE_NATIVE
    if (!nativeFighterLifetime_Current(&lifetime)) return;
    if (lifetime.source_owned) {
        (void) nativeScriptFighter_Run(gobj, NativeScriptFighter_AerialAttackEnter, NULL, 0, NULL, NULL);
        return;
    }
    const NativeFighterDefinition* definition = lifetime.native_definition;
    if (definition != NULL && definition->on_aerial_attack_enter != NULL)
        definition->on_aerial_attack_enter(gobj);
#endif
}

void ftCo_AttackAir_Anim(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (ftCheckThrowB3(fp)) {
        fp->facing_dir = -fp->facing_dir;
    }
    if (!ftAnim_IsFramesRemaining(gobj)) {
        ftCo_Fall_Enter(gobj);
    }
}

/// @remarks Trying to use an @c inline function breaks inlining.
#define DO_IASA(gobj)                                                         \
    if (GET_FIGHTER(gobj)->allow_interrupt) {                                 \
        RETURN_IF(ftCo_80095328(gobj, false, NULL));                          \
        RETURN_IF(ftCo_800D7100(gobj));                                       \
        RETURN_IF(ftCo_800C3B10(gobj));                                       \
        RETURN_IF(checkItemThrowInput(gobj));                                 \
        RETURN_IF(ftCo_800D705C(gobj));                                       \
        RETURN_IF(ftCo_800CB870(gobj));                                       \
    }

void ftCo_AttackAirN_IASA(Fighter_GObj* gobj)
{
    DO_IASA(gobj);
}

void ftCo_AttackAirF_IASA(Fighter_GObj* gobj)
{
    DO_IASA(gobj);
}

void ftCo_AttackAirB_IASA(Fighter_GObj* gobj)
{
    DO_IASA(gobj);
}

void ftCo_AttackAirHi_IASA(Fighter_GObj* gobj)
{
    DO_IASA(gobj);
}

void ftCo_AttackAirLw_IASA(Fighter_GObj* gobj)
{
    DO_IASA(gobj);
}

#undef DO_IASA

void ftCo_AttackAir_Phys(Fighter_GObj* gobj)
{
    ft_80084DB0(gobj);
}

void ftCo_AttackAir_Coll(Fighter_GObj* gobj)
{
    ft_80082C74(gobj, ftCo_LandingAir_EnterWithLag);
}
