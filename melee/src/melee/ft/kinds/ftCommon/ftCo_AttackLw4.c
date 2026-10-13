#include "ftCo_AttackLw4.h"

#include <Runtime/platform.h>

#include <melee/ft/forward.h>

#include "forward.h"
#include "ftCo_ItemThrow.h"
#include "ftCo_Wait.h"
#include <dolphin/mtx.h>
#include <melee/ft/fighter.h>
#ifdef MELEE_NATIVE
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_script_fighter.h>
#endif
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ft_0DF1.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/kinds/ftNess/ftnessattacklw4.h>
#include <melee/ft/types.h>

/* 08CC5C */ static void doEnter(Fighter_GObj* gobj);

static bool checkLStick(Fighter* fp)
{
    if (fp->input.pressed_buttons & HSD_PAD_A &&
        fp->input.lstick[0].y <= p_ftCommonData->xD4 &&
        fp->x671_timer_lstick_tilt_y < p_ftCommonData->xD8)
    {
        return true;
    }
    return false;
}

bool ftCo_AttackLw4_CheckInput(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (checkLStick(fp) ||
        (!HSD_PadProfileUsesTiltStick(fp->input.held_buttons[0]) &&
         ftCo_800DF3A8(fp)))
    {
        if (fp->item_gobj != NULL && (ftCo_80094E54(fp) || ftCo_800DF3DC(fp)))
        {
            ftCo_800957F4(gobj, ftCo_MS_LightThrowLw4);
            return true;
        }
#ifdef MELEE_NATIVE
        if (nativeScriptFighter_IsAttached(gobj)) {
            if (nativeScriptFighter_HasHook(gobj, NativeScriptFighter_DownSmashEnter)) {
                (void) nativeScriptFighter_Run(gobj, NativeScriptFighter_DownSmashEnter, NULL, 0, NULL, NULL);
            } else {
                doEnter(gobj);
            }
            return true;
        }
        const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(fp->kind);
        if (definition != NULL && definition->on_down_smash_enter != NULL) {
            definition->on_down_smash_enter(gobj);
            return true;
        }
#endif
        switch (fp->kind) {
        case Ft_Kind_Ness:
            ftNs_AttackLw4_Enter(gobj);
            break;
        default:
            doEnter(gobj);
        }
        return true;
    }
    return false;
}

void doEnter(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
#ifdef MELEE_NATIVE
    NativeFighterLifetime lifetime;
    if (!nativeFighterLifetime_Capture(fp, &lifetime)) return;
#endif
    fp->allow_interrupt = false;
    Fighter_ChangeMotionState(gobj, ftCo_MS_AttackLw4, Ft_MF_None, 0, 1, 0,
                              NULL);
#ifdef MELEE_NATIVE
    if (!nativeFighterLifetime_Current(&lifetime)) return;
#endif
    ftAnim_8006EBA4(gobj);
}

void ftCo_AttackLw4_Anim(Fighter_GObj* gobj)
{
    if (!ftAnim_IsFramesRemaining(gobj)) {
        ft_8008A2BC(gobj);
    }
}

void ftCo_AttackLw4_IASA(Fighter_GObj* gobj)
{
    if (GET_FIGHTER(gobj)->allow_interrupt) {
        ftCo_Wait_IASA(gobj);
    }
}

void ftCo_AttackLw4_Phys(Fighter_GObj* gobj)
{
    ft_80084F3C(gobj);
}

void ftCo_AttackLw4_Coll(Fighter_GObj* gobj)
{
    ft_80084104(gobj);
}
