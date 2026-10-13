#include "ftCo_AerialAction.h"
#include <melee/ft/fighter.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/kinds/ftPeach/ftpeachfloat.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/controller.h>

bool ftCo_AerialAction_CheckManual(Fighter_GObj* gobj)
{
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(gobj, NativeCallbackSource_Fighter, &generation)) return true;
    Fighter* fighter = GET_FIGHTER(gobj);
    NativeFighterLifetime lifetime;
    if (!nativeFighterLifetime_Capture(fighter, &lifetime)) return true;
    if (lifetime.source_owned) {
        NativeScriptValue result;
        return nativeScriptFighter_Run(gobj, NativeScriptFighter_AerialActionManual,
                   NULL, 0, NULL, &result) != NativeScriptVM_Ok ||
            !nativeFighterLifetime_Current(&lifetime) || result.as.u64 != 0;
    }
    FighterKind kind = fighter->kind;
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(kind);
    if (definition != NULL && definition->on_aerial_action_manual != NULL) {
        bool consumed = definition->on_aerial_action_manual(gobj);
        return consumed || !nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter) ||
            fighter->kind != kind || nativeFighterRuntime_Definition(kind) != definition;
    }
    bool requested = fighter->input.lstick[0].y <= -p_ftCommonData->x88 &&
        (fighter->input.held_buttons[0] & HSD_PAD_XY);
    if (kind == Ft_Kind_Peach && fighter->u.pe.has_float && requested) {
        ftPe_8011BB6C(gobj, true);
        return true;
    }
    return false;
}

bool ftCo_AerialAction_CheckAutomatic(Fighter_GObj* gobj)
{
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(gobj, NativeCallbackSource_Fighter, &generation)) return true;
    Fighter* fighter = GET_FIGHTER(gobj);
    NativeFighterLifetime lifetime;
    if (!nativeFighterLifetime_Capture(fighter, &lifetime)) return true;
    if (lifetime.source_owned) {
        NativeScriptValue result;
        return nativeScriptFighter_Run(gobj, NativeScriptFighter_AerialActionAutomatic,
                   NULL, 0, NULL, &result) != NativeScriptVM_Ok ||
            !nativeFighterLifetime_Current(&lifetime) || result.as.u64 != 0;
    }
    FighterKind kind = fighter->kind;
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(kind);
    if (definition != NULL && definition->on_aerial_action_automatic != NULL) {
        bool consumed = definition->on_aerial_action_automatic(gobj);
        return consumed || !nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter) ||
            fighter->kind != kind || nativeFighterRuntime_Definition(kind) != definition;
    }
    if (kind == Ft_Kind_Peach && fighter->self_vel.y <= 0 && fighter->u.pe.has_float &&
        (fighter->input.lstick[0].y >= p_ftCommonData->tap_jump_threshold ||
         (fighter->input.held_buttons[0] & HSD_PAD_XY))) {
        ftPe_8011BB6C(gobj, true);
        return true;
    }
    return false;
}
