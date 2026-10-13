#include "ftCo_Attack100.h"
#include "ftCo_Fall.h"
#include "ftCo_Throw.h"
#include <melee/ef/efasync.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/inlines.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/it/kinds/itlinkhookshot.h>
#include <melee/it/kinds/itsamusgrapple.h>

void fn_800DA1D8(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    HSD_JObj* jobj;
    PAD_STACK(8);
    u32 generation = fp->x8_spawnNum;
    FighterKind kind = fp->kind;
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(kind);
    Fighter_GObj* victim = fp->victim_gobj;
    uint32_t victim_generation = 0;
#ifdef MELEE_NATIVE
    const bool retained = nativeScriptFighter_IsAttached(gobj) ||
        (victim != NULL && nativeScriptFighter_IsAttached(victim));
    NativeFighterLifetime source_lifetime, peer_lifetime;
    if (retained) {
        uint32_t peer_generation;
        if (!nativeFighterLifetime_Capture(fp, &source_lifetime) ||
            (victim != NULL && (!nativeCallbackSource_Snapshot(victim, NativeCallbackSource_Fighter, &peer_generation) ||
                !nativeFighterLifetime_Capture(GET_FIGHTER(victim), &peer_lifetime) ||
                peer_lifetime.data->victim_gobj != gobj))) return;
    }
#endif

    if (victim != NULL && !nativeCallbackSource_Snapshot(victim,
            NativeCallbackSource_Fighter, &victim_generation)) return;
    fp->gr_vel = 0.0F;
    Fighter_ChangeMotionState(gobj, ftCo_MS_CatchWait, 0, 0.0F, 1.0F, 0.0F,
                              NULL);
#ifdef MELEE_NATIVE
    if (retained && (!nativeFighterLifetime_Current(&source_lifetime) ||
        fp->victim_gobj != victim || (victim != NULL &&
            (!nativeFighterLifetime_Current(&peer_lifetime) || peer_lifetime.data->victim_gobj != gobj)))) return;
#endif
    if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter)) return;
    fp = GET_FIGHTER(gobj);
    if (fp->kind != kind || nativeFighterRuntime_Definition(kind) != definition) return;
    if (victim != NULL && (!nativeCallbackSource_Alive(victim, victim_generation,
            NativeCallbackSource_Fighter) || fp->victim_gobj != victim)) return;
    fp->accessory1_cb = fn_800DA4A0;
    fp->take_dmg_cb = fn_800DA490;
    fp->x221B_b7 = false;
    jobj = fp->parts[fp->ft_data->x8->x11].joint;
    fp->mv.co.capturedamage.x18 = jobj;
    efAsync_Spawn(gobj, &GET_FIGHTER(gobj)->x60C, 1, 0x41D, jobj);
    ftCommon_8007EBAC(fp, 3, 0);
    ftCommon_8007E2F4(fp, 0x1FF);
    fn_800DB6C8(fp->victim_gobj);
}

void fn_800DA2B0(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    Fighter_GObj* victim = fp->victim_gobj;
#ifdef MELEE_NATIVE
    const bool retained = nativeScriptFighter_IsAttached(gobj) ||
        (victim != NULL && nativeScriptFighter_IsAttached(victim));
    NativeFighterLifetime source_lifetime, peer_lifetime;
    if (retained) {
        uint32_t peer_generation;
        if (!nativeFighterLifetime_Capture(fp, &source_lifetime) ||
            (victim != NULL && (!nativeCallbackSource_Snapshot(victim, NativeCallbackSource_Fighter, &peer_generation) ||
                !nativeFighterLifetime_Capture(GET_FIGHTER(victim), &peer_lifetime) ||
                peer_lifetime.data->victim_gobj != gobj))) return;
    }
#endif

    u32 generation = fp->x8_spawnNum;
    FighterKind kind = fp->kind;
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(kind);
    Fighter_ChangeMotionState(gobj, ftCo_MS_CatchWait, 0, 0.0F, 1.0F, 0.0F,
                              NULL);
#ifdef MELEE_NATIVE
    if (retained && (!nativeFighterLifetime_Current(&source_lifetime) ||
        fp->victim_gobj != victim || (victim != NULL &&
            (!nativeFighterLifetime_Current(&peer_lifetime) || peer_lifetime.data->victim_gobj != gobj)))) return;
#endif
    if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter)) return;
    fp = GET_FIGHTER(gobj);
    if (fp->kind != kind || nativeFighterRuntime_Definition(kind) != definition) return;
    fp->accessory1_cb = fn_800DA4A0;
    fp->take_dmg_cb = fn_800DA490;
    ftCommon_8007E2F4(fp, 0x1FF);
}

void ftCo_CatchWait_Anim(Fighter_GObj* gobj) {}

void ftCo_CatchWait_IASA(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    Fighter_GObj* victim = fp->victim_gobj;
#ifdef MELEE_NATIVE
    const bool retained = nativeScriptFighter_IsAttached(gobj) ||
        (victim != NULL && nativeScriptFighter_IsAttached(victim));
    NativeFighterLifetime source_lifetime, peer_lifetime;
    if (retained) {
        uint32_t peer_generation;
        if (!nativeFighterLifetime_Capture(fp, &source_lifetime) ||
            (victim != NULL && (!nativeCallbackSource_Snapshot(victim, NativeCallbackSource_Fighter, &peer_generation) ||
                !nativeFighterLifetime_Capture(GET_FIGHTER(victim), &peer_lifetime) ||
                peer_lifetime.data->victim_gobj != gobj))) return;
    }
#endif

    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(fp->kind);
    u32 generation = fp->x8_spawnNum;
    FighterKind kind = fp->kind;
    if (!fn_800DA4C0(gobj) && !ftCo_800DD1E4(gobj)) return;
#ifdef MELEE_NATIVE
    if (retained && (!nativeFighterLifetime_Current(&source_lifetime) ||
        fp->victim_gobj != victim || (victim != NULL &&
            (!nativeFighterLifetime_Current(&peer_lifetime) || peer_lifetime.data->victim_gobj != gobj)))) return;
#endif
    if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter)) return;
    fp = GET_FIGHTER(gobj);
    if (fp->kind != kind || nativeFighterRuntime_Definition(kind) != definition) return;
#ifdef MELEE_NATIVE
    if (nativeScriptFighter_IsAttached(gobj) && nativeScriptFighter_HasHook(gobj, NativeScriptFighter_CatchWaitRelease)) {
        NativeScriptValue result;
        if (nativeScriptFighter_Run(gobj, NativeScriptFighter_CatchWaitRelease, NULL, 0, NULL, &result) != NativeScriptVM_Ok) return;
        if (retained && (!nativeFighterLifetime_Current(&source_lifetime) ||
            fp->victim_gobj != victim || (victim != NULL &&
                (!nativeFighterLifetime_Current(&peer_lifetime) || peer_lifetime.data->victim_gobj != gobj)))) return;
        return;
    }
#endif
    if (definition != NULL && definition->on_catch_wait_release != NULL) {
        definition->on_catch_wait_release(gobj);
        return;
    }
    if (fp->kind == Ft_Kind_Link || fp->kind == Ft_Kind_CLink) {
        it_802A7AAC(gobj);
    } else if (fp->kind == Ft_Kind_Samus) {
        it_802BAC3C(gobj);
    }
}

void ftCo_CatchWait_Phys(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);

    ftCommon_CalcGroundAccel_Deaccel(fp, p_ftCommonData->x64 *
                                             fp->co_attrs.ground_friction);
    ftCommon_SetSelfMovementFromGroundedMovement(gobj);
}

void ftCo_CatchWait_Coll(Fighter_GObj* gobj)
{
    ft_800841B8(gobj, fn_800DA440);
}

void fn_800DA440(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    Fighter_GObj* victim = fp->victim_gobj;
    ftCo_800DC920(gobj, victim);
    ftCo_Fall_Enter(gobj);
    ftCo_Fall_Enter(victim);
}

void fn_800DA490(Fighter_GObj* gobj)
{
    GET_FIGHTER(gobj)->take_dmg_cb = NULL;
}

void fn_800DA4A0(Fighter_GObj* gobj)
{
    fn_800DA054(gobj);
}

bool fn_800DA4C0(Fighter_GObj* gobj)
{
    if (GET_FIGHTER(gobj)->input.pressed_buttons & 0x100) {
        fn_800DA4FC(gobj);
        return true;
    }
    return false;
}
