#include "ftCo_Attack100.h"
#include "ftCo_CapturePulled.h"
#include "ftCo_CatchPull.h"
#include "ftCo_Fall.h"
#include "ftCo_ItemThrow.h"
#include "ftpickupitem.h"
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/inlines.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_fighter_runtime.h>

/* 0D8BFC */ static void fn_800D8BFC(Fighter_GObj* arg0);

bool ftCo_Catch_CheckInput(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
#ifdef MELEE_NATIVE
    const bool owned = nativeScriptFighter_IsAttached(gobj);
    NativeFighterLifetime lifetime;
    if (owned && !nativeFighterLifetime_Capture(fp, &lifetime)) return true;
#endif


    if (ftCo_800951D0(gobj)) {
        return true;
    }
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(fp->kind);
#ifdef MELEE_NATIVE
    if (owned && nativeScriptFighter_HasHook(gobj, NativeScriptFighter_CatchAllowed)) {
        NativeScriptValue result;
        if (nativeScriptFighter_Run(gobj, NativeScriptFighter_CatchAllowed, NULL, 0, NULL, &result) != NativeScriptVM_Ok ||
            !nativeFighterLifetime_Current(&lifetime)) return true;
        if (!result.as.u64) return false;
    } else
#endif
    if (definition != NULL && definition->on_catch_allowed != NULL) {
        u32 generation = fp->x8_spawnNum;
        FighterKind kind = fp->kind;
        bool allowed = definition->on_catch_allowed(gobj);
        if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter)) return true;
        fp = GET_FIGHTER(gobj);
        if (fp->kind != kind || nativeFighterRuntime_Definition(kind) != definition) return true;
        if (!allowed) return false;
    } else {
        if (!fn_800D8E94(gobj)) return false;
        if (!fn_800D952C(gobj)) return false;
    }
    if ((fp->input.held_buttons[0] & HSD_PAD_LR) &&
        (fp->input.pressed_buttons & HSD_PAD_A))
    {
        ftCo_800D8C54(gobj, ftCo_MS_Catch);
        return true;
    }
    return false;
}

bool ftCo_800D8A38(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
#ifdef MELEE_NATIVE
    const bool owned = nativeScriptFighter_IsAttached(gobj);
    NativeFighterLifetime lifetime;
    if (owned && !nativeFighterLifetime_Capture(fp, &lifetime)) return true;
#endif


    if (ftCo_80095254(gobj)) {
        return true;
    }
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(fp->kind);
#ifdef MELEE_NATIVE
    if (owned && nativeScriptFighter_HasHook(gobj, NativeScriptFighter_CatchAllowed)) {
        NativeScriptValue result;
        if (nativeScriptFighter_Run(gobj, NativeScriptFighter_CatchAllowed, NULL, 0, NULL, &result) != NativeScriptVM_Ok ||
            !nativeFighterLifetime_Current(&lifetime)) return true;
        if (!result.as.u64) return false;
    } else
#endif
    if (definition != NULL && definition->on_catch_allowed != NULL) {
        u32 generation = fp->x8_spawnNum;
        FighterKind kind = fp->kind;
        bool allowed = definition->on_catch_allowed(gobj);
        if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter)) return true;
        fp = GET_FIGHTER(gobj);
        if (fp->kind != kind || nativeFighterRuntime_Definition(kind) != definition) return true;
        if (!allowed) return false;
    } else {
        if (!fn_800D8E94(gobj)) return false;
        if (!fn_800D952C(gobj)) return false;
    }
    if ((fp->input.held_buttons[0] & HSD_PAD_LR) &&
        (fp->input.pressed_buttons & HSD_PAD_A))
    {
        ftCo_800D8C54(gobj, ftCo_MS_CatchDash);
        return true;
    }
    return false;
}

bool ftCo_800D8AE0(Fighter_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
#ifdef MELEE_NATIVE
    const bool owned = nativeScriptFighter_IsAttached(gobj);
    NativeFighterLifetime lifetime;
    if (owned && !nativeFighterLifetime_Capture(fp, &lifetime)) return true;
#endif


    if (ftCo_800952DC(gobj)) {
        return true;
    }
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(fp->kind);
#ifdef MELEE_NATIVE
    if (owned && nativeScriptFighter_HasHook(gobj, NativeScriptFighter_CatchAllowed)) {
        NativeScriptValue result;
        if (nativeScriptFighter_Run(gobj, NativeScriptFighter_CatchAllowed, NULL, 0, NULL, &result) != NativeScriptVM_Ok ||
            !nativeFighterLifetime_Current(&lifetime)) return true;
        if (!result.as.u64) return false;
    } else
#endif
    if (definition != NULL && definition->on_catch_allowed != NULL) {
        u32 generation = fp->x8_spawnNum;
        FighterKind kind = fp->kind;
        bool allowed = definition->on_catch_allowed(gobj);
        if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter)) return true;
        fp = GET_FIGHTER(gobj);
        if (fp->kind != kind || nativeFighterRuntime_Definition(kind) != definition) return true;
        if (!allowed) return false;
    } else {
        if (!fn_800D8E94(gobj)) return false;
        if (!fn_800D952C(gobj)) return false;
    }
    if ((fp->input.held_buttons[0] & HSD_PAD_LR) && fp->mv.co.common.x0 != 0) {
        ftCo_800D8C54(gobj, ftCo_MS_CatchDash);
        return true;
    }
    if (fp->mv.co.common.x0 != 0) {
        fp->mv.co.common.x0--;
    }
    return false;
}

bool ftCo_800D8B9C(Fighter_GObj* gobj)
{
    Fighter* fp = gobj->user_data;

    if ((fp->input.pressed_buttons & HSD_PAD_A) && fp->mv.co.guard.x24 != 0) {
        ftCo_800D8C54(gobj, ftCo_MS_CatchDash);
        return true;
    }
    if (fp->mv.co.guard.x24 != 0) {
        fp->mv.co.guard.x24--;
    }
    return false;
}

void fn_800D8BFC(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    ftpickupitem_800948A8(gobj, fp->target_item_gobj);
    fp->x1A64 = NULL;
    fp->target_item_gobj = NULL;
    ftpickupitem_80094694(gobj, 0x5D, 1);
}

void ftCo_800D8C54(Fighter_GObj* gobj, FtMotionId msid)
{
    Fighter* fp = GET_FIGHTER(gobj);
#ifdef MELEE_NATIVE
    const bool owned = nativeScriptFighter_IsAttached(gobj);
    NativeFighterLifetime lifetime;
    if (owned && !nativeFighterLifetime_Capture(fp, &lifetime)) return;
#endif

    u32 generation = fp->x8_spawnNum;
    FighterKind kind = fp->kind;

    fp->x74_self_accel.z = 0.0F;
    fp->x74_self_accel.y = 0.0F;
    fp->x74_self_accel.x = 0.0F;
    fp->mv.co.catch.x0 = 0.0F;
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(fp->kind);
#ifdef MELEE_NATIVE
    if (owned && nativeScriptFighter_HasHook(gobj, NativeScriptFighter_CatchStart)) {
        NativeScriptValue result;
        if (nativeScriptFighter_Run(gobj, NativeScriptFighter_CatchStart, NULL, 0, NULL, &result) != NativeScriptVM_Ok ||
            !nativeFighterLifetime_Current(&lifetime)) return;
    } else
#endif
    if (definition != NULL && definition->on_catch_start != NULL) {
        definition->on_catch_start(gobj);
        if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter)) return;
        fp = GET_FIGHTER(gobj);
        if (fp->kind != kind || nativeFighterRuntime_Definition(kind) != definition) return;
    }
    Fighter_ChangeMotionState(gobj, msid, 0, 0.0F, 1.0F, 0.0F, NULL);
#ifdef MELEE_NATIVE
    if (owned && !nativeFighterLifetime_Current(&lifetime)) return;
#endif
    if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter)) return;
    fp = GET_FIGHTER(gobj);
    if (fp->kind != kind || nativeFighterRuntime_Definition(kind) != definition) return;
    ftCommon_8007E2D0(fp, 1, fn_800D9CE8, fn_800D8BFC, fn_800DAADC);
}

void ftCo_Catch_Anim(Fighter_GObj* gobj)
{
    u8 _[8];

    Fighter* fp = GET_FIGHTER(gobj);
#ifdef MELEE_NATIVE
    const bool owned = nativeScriptFighter_IsAttached(gobj);
    NativeFighterLifetime lifetime;
    if (owned && !nativeFighterLifetime_Capture(fp, &lifetime)) return;
#endif

    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(fp->kind);
#ifdef MELEE_NATIVE
    if (owned && nativeScriptFighter_HasHook(gobj, NativeScriptFighter_CatchFrame)) {
        NativeScriptValue result;
        if (nativeScriptFighter_Run(gobj, NativeScriptFighter_CatchFrame, NULL, 0, NULL, &result) != NativeScriptVM_Ok ||
            !nativeFighterLifetime_Current(&lifetime)) return;
        if (result.as.u64) return;
    } else
#endif
    if (definition != NULL && definition->on_catch_frame != NULL) {
        u32 generation = fp->x8_spawnNum;
        FighterKind kind = fp->kind;
        bool handled = definition->on_catch_frame(gobj);
        if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter)) return;
        fp = GET_FIGHTER(gobj);
        if (fp->kind != kind || nativeFighterRuntime_Definition(kind) != definition || handled) return;
    } else {
        if (fn_800D8EC8(gobj) || fn_800D9558(gobj)) return;
    }
    if (!ftAnim_IsFramesRemaining(gobj)) {
        ft_8008A2BC(gobj);
    }
}

void ftCo_CatchDash_Anim(Fighter_GObj* gobj)
{
    PAD_STACK(8);
    Fighter* fp = GET_FIGHTER(gobj);
#ifdef MELEE_NATIVE
    const bool owned = nativeScriptFighter_IsAttached(gobj);
    NativeFighterLifetime lifetime;
    if (owned && !nativeFighterLifetime_Capture(fp, &lifetime)) return;
#endif

    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(fp->kind);
#ifdef MELEE_NATIVE
    if (owned && nativeScriptFighter_HasHook(gobj, NativeScriptFighter_CatchFrame)) {
        NativeScriptValue result;
        if (nativeScriptFighter_Run(gobj, NativeScriptFighter_CatchFrame, NULL, 0, NULL, &result) != NativeScriptVM_Ok ||
            !nativeFighterLifetime_Current(&lifetime)) return;
        if (result.as.u64) return;
    } else
#endif
    if (definition != NULL && definition->on_catch_frame != NULL) {
        u32 generation = fp->x8_spawnNum;
        FighterKind kind = fp->kind;
        bool handled = definition->on_catch_frame(gobj);
        if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter)) return;
        fp = GET_FIGHTER(gobj);
        if (fp->kind != kind || nativeFighterRuntime_Definition(kind) != definition || handled) return;
    } else {
        if (fn_800D9228(gobj) || fn_800D9930(gobj)) return;
    }
    if (!ftAnim_IsFramesRemaining(gobj)) {
        ft_8008A2BC(gobj);
    }
}

void ftCo_Catch_IASA(Fighter_GObj* gobj) {}

void ftCo_CatchDash_IASA(Fighter_GObj* gobj) {}

void ftCo_Catch_Phys(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);

    ftCommon_CalcGroundAccel_Deaccel(fp, p_ftCommonData->x64 *
                                             fp->co_attrs.ground_friction);
    ftCommon_SetSelfMovementFromGroundedMovement(gobj);
}

void ftCo_CatchDash_Phys(Fighter_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    ft_80085030(gobj, p_ftCommonData->x64 * fp->co_attrs.ground_friction,
                fp->facing_dir);
}

void ftCo_Catch_Coll(Fighter_GObj* gobj)
{
    ft_800841B8(gobj, fn_800D8E30);
}

void fn_800D8E30(Fighter_GObj* gobj)
{
    PAD_STACK(8);
    Fighter* fp = GET_FIGHTER(gobj);
#ifdef MELEE_NATIVE
    const bool owned = nativeScriptFighter_IsAttached(gobj);
    NativeFighterLifetime lifetime;
    if (owned && !nativeFighterLifetime_Capture(fp, &lifetime)) return;
#endif

    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(fp->kind);
#ifdef MELEE_NATIVE
    if (owned && nativeScriptFighter_HasHook(gobj, NativeScriptFighter_CatchExit)) {
        NativeScriptValue result;
        if (nativeScriptFighter_Run(gobj, NativeScriptFighter_CatchExit, NULL, 0, NULL, &result) != NativeScriptVM_Ok ||
            !nativeFighterLifetime_Current(&lifetime)) return;
    } else
#endif
    if (definition != NULL && definition->on_catch_exit != NULL) {
        u32 generation = fp->x8_spawnNum;
        FighterKind kind = fp->kind;
        definition->on_catch_exit(gobj);
        if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter)) return;
        fp = GET_FIGHTER(gobj);
        if (fp->kind != kind || nativeFighterRuntime_Definition(kind) != definition) return;
    } else {
        fn_800D949C(gobj);
        fn_800D9C64(gobj);
    }
    ftCo_Fall_Enter(gobj);
}

void ftCo_CatchDash_Coll(Fighter_GObj* gobj)
{
    ft_800841B8(gobj, fn_800D8E30);
}

bool fn_800D8E94(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    FighterKind kind = fp->kind;
    if (((kind == Ft_Kind_Link) || (kind == Ft_Kind_CLink)) &&
        (fp->u.lk.xC != NULL))
    {
        return false;
    }
    return true;
}
