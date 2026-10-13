#include "ftCo_Attack1.h"

#include <Runtime/platform.h>

#include "ftCo_Attack100.h"
#include "ftCo_AttackHi3.h"
#include "ftCo_AttackHi4.h"
#include "ftCo_AttackLw3.h"
#include "ftCo_AttackLw4.h"
#include "ftCo_AttackS3.h"
#include "ftCo_AttackS4.h"
#include "ftCo_Dash.h"
#include "ftCo_ItemThrow.h"
#include "ftCo_Jump.h"
#include "ftCo_Squat.h"
#include "ftCo_Turn.h"
#include "ftCo_Wait.h"
#include "ftCo_Walk.h"
#include "ftpickupitem.h"
#include "types.h"
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0881.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ft_0CDD.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftswing.h>
#include <melee/ft/native_fighter_runtime.h>
#ifdef MELEE_NATIVE
#include <melee/ft/native_fighter_lifetime.h>
#endif
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/kinds/ftGameWatch/ftgamewatchattack11.h>
#include <melee/ft/types.h>
#include <melee/it/it_26B1.h>

/* 08AB50 */ static void onPkPc21EC(Fighter_GObj* gobj);
/* 08AB84 */ static void decideAttack11(Fighter_GObj* gobj);
/* 08AE30 */ static void doAttack12(Fighter_GObj* gobj);
/* 08AF0C */ static bool checkAttack12(Fighter_GObj* gobj);
/* 08B194 */ static void doAttack13(Fighter_GObj* gobj);
/* 08B248 */ static bool checkAttack13(Fighter_GObj* gobj);

bool ftCo_Attack1_CheckInput(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->input.pressed_buttons & HSD_PAD_A) {
        if (fp->item_gobj != NULL) {
            if (itGetUseKind(fp->item_gobj) == 0) {
                ftCo_800957F4(gobj, ftCo_MS_LightThrowF);
                return true;
            }
            if (fp->input.held_buttons[0] & HSD_PAD_LR) {
                ftCo_800957F4(gobj, ftCo_MS_LightThrowDrop);
                return true;
            }
            switch (itGetUseKind(fp->item_gobj)) {
            case 2:
                ftCo_Attack_800CCF58(gobj, false);
                return true;
            case 3:
                ftCo_Attack_800CDD14(gobj);
                return true;
            }
        }
        if (fp->hitlag_mul > 0 && fp->x2218_b1) {
            switch (fp->unk_msid) {
            case 44:
                doAttack12(gobj);
                return true;
            case 45:
                doAttack13(gobj);
                return true;
            }
        } else {
            decideAttack11(gobj);
            return true;
        }
    }
    if (fp->hitlag_mul > 0) {
        fp->hitlag_mul -= 1;
    }
    return false;
}

static void onPkPc21EC(Fighter_GObj* gobj)
{
    ft_800892A0(gobj);
    ft_80089824(gobj);
}

static void decideAttack11(Fighter_GObj* gobj)
{
#ifdef MELEE_NATIVE
    if (nativeScriptFighter_IsAttached(gobj)) {
        NativeFighterLifetime lifetime;
        if (!nativeFighterLifetime_Capture(GET_FIGHTER(gobj), &lifetime)) return;
        if (nativeScriptFighter_HasHook(gobj, NativeScriptFighter_JabEnter)) {
            (void) nativeScriptFighter_Run(gobj, NativeScriptFighter_JabEnter, NULL, 0, NULL, NULL);
        } else {
            ftCo_Attack11_EnterMotion(gobj, ftCo_MS_Attack11);
        }
        return;
    }
    const NativeFighterDefinition* definition =
        nativeFighterRuntime_Definition(GET_FIGHTER(gobj)->kind);
    if (definition != NULL && definition->on_jab_enter != NULL) {
        definition->on_jab_enter(gobj);
        return;
    }
#endif
    switch (GET_FIGHTER(gobj)->kind) {
    case Ft_Kind_GameWatch:
        ftGw_Attack11_Enter(gobj);
        return;
    default:
        ftCo_Attack11_EnterMotion(gobj, ftCo_MS_Attack11);
        return;
    }
}

static MotionFlags getMotionFlags(Fighter* fp)
{
#ifdef MELEE_NATIVE
    if (nativeScriptFighter_IsAttached(fp->gobj)) {
        NativeScriptValue result = {0};
        if (nativeScriptFighter_Run(fp->gobj, NativeScriptFighter_JabStart,
                                    NULL, 0, NULL, &result) != NativeScriptVM_Ok ||
            result.type != NativeScriptType_I32) return Ft_MF_None;
        return (MotionFlags) (uint32_t) result.as.i64;
    }
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(fp->kind);
    if (definition != NULL && definition->on_jab_start != NULL)
        return (MotionFlags) definition->on_jab_start(fp->gobj);
#endif
    switch (fp->kind) {
    case Ft_Kind_Pichu:
    case Ft_Kind_Pikachu:
        fp->x21EC = onPkPc21EC;
        return Ft_MF_SkipAttackCount;
    default:
        return Ft_MF_None;
    }
}

bool ftCo_Attack11_EnterMotion(Fighter_GObj* gobj, FtMotionId motion)
{
    u8 _[8] = { 0 };
    Fighter* fp = GET_FIGHTER(gobj);
#ifdef MELEE_NATIVE
    NativeFighterLifetime lifetime;
    if (!nativeFighterLifetime_Capture(fp, &lifetime)) return false;
#endif
    if (!ftpickupitem_80094790(gobj)) {
#ifdef MELEE_NATIVE
        if (!nativeFighterLifetime_Current(&lifetime)) return false;
#endif
        fp->allow_interrupt = false;
        fp->x2218_b1 = false;
        MotionFlags flags = getMotionFlags(fp);
#ifdef MELEE_NATIVE
        if (!nativeFighterLifetime_Current(&lifetime)) return false;
#endif
        Fighter_ChangeMotionState(gobj, motion, flags,
                                  0, 1, 0, NULL);
#ifdef MELEE_NATIVE
        if (!nativeFighterLifetime_Current(&lifetime)) return false;
#endif
        ftAnim_8006EBA4(gobj);
#ifdef MELEE_NATIVE
        if (!nativeFighterLifetime_Current(&lifetime)) return false;
#endif
        fp->hitlag_mul = fp->co_attrs.jab_2_input_window;
        fp->unk_msid = ftCo_MS_Attack11;
        fp->x2218_b2 = false;
        fp->mv.co.attack1.x0 = 0;
        fp->x1A54 = 0;
        return true;
    }
    return false;
}

void ftCo_Attack11_Anim(Fighter_GObj* arg0)
{
    u8 _[8] = { 0 };
    if (!ftAnim_IsFramesRemaining(arg0)) {
        ft_8008A2BC(arg0);
    }
}

void ftCo_Attack11_IASA(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->allow_interrupt) {
        RETURN_IF(ftCo_AttackS4_CheckInput(gobj));
        RETURN_IF(ftCo_AttackHi4_CheckInput(gobj));
        RETURN_IF(ftCo_AttackLw4_CheckInput(gobj));
        RETURN_IF(ftCo_AttackS3_CheckInput(gobj));
        RETURN_IF(ftCo_AttackHi3_CheckInput(gobj));
        RETURN_IF(ftCo_AttackLw3_CheckInput(gobj));
    }
    RETURN_IF(ftCo_Attack_800D6A50(gobj));
    RETURN_IF(checkAttack12(gobj));
    if (fp->allow_interrupt) {
        RETURN_IF(ftCo_Jump_CheckInput(gobj));
        RETURN_IF(ftCo_Dash_CheckInput(gobj));
        RETURN_IF(ftCo_800D5FB0(gobj));
        RETURN_IF(ftCo_Turn_CheckInput(gobj));
        RETURN_IF(ftCo_Walk_CheckInput(gobj));
    }
}

void ftCo_Attack11_Phys(Fighter_GObj* gobj)
{
    ft_80084FA8(gobj);
}

void ftCo_Attack11_Coll(Fighter_GObj* gobj)
{
    ft_80084104(gobj);
}

static void doAttack12Rapid(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
#ifdef MELEE_NATIVE
    if (nativeScriptFighter_IsAttached(gobj)) {
        NativeFighterLifetime lifetime;
        if (!nativeFighterLifetime_Capture(GET_FIGHTER(gobj), &lifetime)) return;
        if (nativeScriptFighter_HasHook(gobj, NativeScriptFighter_JabEnter)) {
            (void) nativeScriptFighter_Run(gobj, NativeScriptFighter_JabEnter, NULL, 0, NULL, NULL);
        } else {
            ftCo_Attack11_EnterMotion(gobj, ftCo_MS_Attack11);
        }
        return;
    }
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(fp->kind);
    if (definition != NULL && definition->on_jab_enter != NULL) {
        definition->on_jab_enter(gobj);
        return;
    }
#endif
    switch (fp->kind) {
    case Ft_Kind_GameWatch:
        ftGw_Attack11_Enter(gobj);
        return;
    default:
        ftCo_Attack11_EnterMotion(gobj, ftCo_MS_Attack11);
        return;
    }
}

static void doAttack12Normal(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
#ifdef MELEE_NATIVE
    NativeFighterLifetime lifetime;
    if (!nativeFighterLifetime_Capture(fp, &lifetime)) return;
#endif
    if (!ftpickupitem_80094790(gobj)) {
#ifdef MELEE_NATIVE
        if (!nativeFighterLifetime_Current(&lifetime)) return;
#endif
        fp->allow_interrupt = false;
        fp->x2218_b1 = false;
        Fighter_ChangeMotionState(gobj, ftCo_MS_Attack12, Ft_MF_None, 0, 1, 0,
                                  NULL);
#ifdef MELEE_NATIVE
        if (!nativeFighterLifetime_Current(&lifetime)) return;
#endif
        fp->hitlag_mul = fp->co_attrs.jab_3_input_window;
        fp->unk_msid = ftCo_MS_Attack12;
        fp->mv.co.attack1.x0 = 0;
    }
}

static void doAttack12(Fighter_GObj* gobj)
{
#ifdef MELEE_NATIVE
    if (nativeScriptFighter_IsAttached(gobj)) {
        NativeFighterLifetime lifetime;
        if (!nativeFighterLifetime_Capture(GET_FIGHTER(gobj), &lifetime)) return;
        bool restart = false;
        if (nativeScriptFighter_HasHook(gobj, NativeScriptFighter_JabRestart)) {
            NativeScriptValue argument = { .type=NativeScriptType_I32,
                                          .as.i64=ftCo_MS_Attack12 }, result;
            if (nativeScriptFighter_Run(gobj, NativeScriptFighter_JabRestart,
                    &argument, 1, NULL, &result) != NativeScriptVM_Ok ||
                !nativeFighterLifetime_Current(&lifetime)) return;
            restart = result.as.u64 != 0;
        }
        if (restart) ftCo_Attack11_EnterMotion(gobj, ftCo_MS_Attack11);
        else doAttack12Normal(gobj);
        return;
    }
    const NativeFighterDefinition* definition =
        nativeFighterRuntime_Definition(GET_FIGHTER(gobj)->kind);
    if (definition != NULL && definition->on_jab_restart != NULL) {
        Fighter* fp = GET_FIGHTER(gobj);
        const FighterKind kind = fp->kind;
        uint32_t generation;
        if (!nativeCallbackSource_Snapshot(gobj, NativeCallbackSource_Fighter, &generation)) return;
        bool restart = definition->on_jab_restart(gobj, ftCo_MS_Attack12);
        if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter) ||
            fp->kind != kind || nativeFighterRuntime_Definition(kind) != definition) return;
        if (restart)
            ftCo_Attack11_EnterMotion(gobj, ftCo_MS_Attack11);
        else
            doAttack12Normal(gobj);
        return;
    }
#endif
    switch (GET_FIGHTER(gobj)->kind) {
    case Ft_Kind_Pikachu:
    case Ft_Kind_Pichu:
        doAttack12Rapid(gobj);
        return;
    default:
        doAttack12Normal(gobj);
        return;
    }
}

bool checkAttack12(Fighter_GObj* gobj)
{
    u8 _[16] = { 0 };
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->hitlag_mul > 0) {
        fp->hitlag_mul -= 1;
        if (fp->input.pressed_buttons & HSD_PAD_A) {
            fp->mv.co.attack1.x0 = true;
        }
    }
    if (fp->mv.co.attack1.x0 && fp->x2218_b1) {
        doAttack12(gobj);
        return true;
    }
    return false;
}

void ftCo_Attack12_Anim(Fighter_GObj* gobj)
{
    u8 _[8] = { 0 };
    if (!ftAnim_IsFramesRemaining(gobj)) {
        ft_8008A2BC(gobj);
    }
}

void ftCo_Attack12_IASA(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->allow_interrupt) {
        RETURN_IF(ftCo_AttackS4_CheckInput(gobj));
        RETURN_IF(ftCo_AttackHi4_CheckInput(gobj));
        RETURN_IF(ftCo_AttackLw4_CheckInput(gobj));
        RETURN_IF(ftCo_AttackS3_CheckInput(gobj));
        RETURN_IF(ftCo_AttackHi3_CheckInput(gobj));
        RETURN_IF(ftCo_AttackLw3_CheckInput(gobj));
    }
    RETURN_IF(ftCo_Attack_800D6A50(gobj));
    RETURN_IF(checkAttack13(gobj));
    if (fp->allow_interrupt) {
        RETURN_IF(ftCo_Jump_CheckInput(gobj));
        RETURN_IF(ftCo_Dash_CheckInput(gobj));
        RETURN_IF(ftCo_800D5FB0(gobj));
        RETURN_IF(ftCo_Turn_CheckInput(gobj));
        RETURN_IF(ftCo_Walk_CheckInput(gobj));
    }
}

static void doAttack13(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    bool restart = fp->kind == Ft_Kind_Mars;
#ifdef MELEE_NATIVE
    NativeFighterLifetime lifetime;
    if (!nativeFighterLifetime_Capture(fp, &lifetime)) return;
    if (lifetime.source_owned) {
        restart = false;
        if (nativeScriptFighter_HasHook(gobj, NativeScriptFighter_JabRestart)) {
            NativeScriptValue argument = { .type=NativeScriptType_I32,
                                          .as.i64=ftCo_MS_Attack13 }, result;
            if (nativeScriptFighter_Run(gobj, NativeScriptFighter_JabRestart,
                    &argument, 1, NULL, &result) != NativeScriptVM_Ok ||
                !nativeFighterLifetime_Current(&lifetime)) return;
            restart = result.as.u64 != 0;
        }
    }
#endif
#ifdef MELEE_NATIVE
    const NativeFighterDefinition* definition =
        lifetime.native_definition;
    if (!nativeScriptFighter_IsAttached(gobj) && definition != NULL && definition->on_jab_restart != NULL) {
        restart = definition->on_jab_restart(gobj, ftCo_MS_Attack13);
        if (!nativeFighterLifetime_Current(&lifetime)) return;
    }
#endif
    if (restart) {
        ftCo_Attack11_EnterMotion(gobj, ftCo_MS_Attack11);
        return;
    }
    if (!ftpickupitem_80094790(gobj)) {
#ifdef MELEE_NATIVE
        if (!nativeFighterLifetime_Current(&lifetime)) return;
#endif
        fp->allow_interrupt = false;
        fp->x2218_b1 = false;
        Fighter_ChangeMotionState(gobj, ftCo_MS_Attack13, Ft_MF_None, 0, 1,
                                  0, NULL);
    }
}

bool checkAttack13(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->hitlag_mul > 0) {
        fp->hitlag_mul -= 1;
        if (fp->input.pressed_buttons & HSD_PAD_A) {
            fp->mv.co.attack1.x0 = true;
        }
    }
    if (fp->mv.co.attack1.x0 && fp->x2218_b1) {
        doAttack13(gobj);
        return true;
    }
    return false;
}

void ftCo_Attack13_Anim(Fighter_GObj* gobj)
{
    if (!ftAnim_IsFramesRemaining(gobj)) {
        ft_8008A2BC(gobj);
    }
}

void ftCo_Attack13_IASA(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (!ftCo_Attack_800D6A50(gobj) && fp->allow_interrupt) {
        ftCo_Wait_IASA(gobj);
    }
}
