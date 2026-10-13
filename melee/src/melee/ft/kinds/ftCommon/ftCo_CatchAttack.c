#include "ftCo_Attack100.h"
#include "ftCo_Fall.h"
#include "ftCo_Throw.h"
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/inlines.h>
#include <melee/ft/native_fighter_lifetime.h>

void fn_800DA054(Fighter_GObj* gobj);

void fn_800DA4FC(Fighter_GObj* gobj)
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

    fp->gr_vel = 0.0F;
    Fighter_ChangeMotionState(gobj, ftCo_MS_CatchAttack, 0, 0.0F, 1.0F, 0.0F,
                              NULL);
#ifdef MELEE_NATIVE
    if (retained && (!nativeFighterLifetime_Current(&source_lifetime) ||
        fp->victim_gobj != victim || (victim != NULL &&
            (!nativeFighterLifetime_Current(&peer_lifetime) || peer_lifetime.data->victim_gobj != gobj)))) return;
#endif
    fp->accessory1_cb = fn_800DA678;
    fp->take_dmg_cb = fn_800DA668;
    ftCommon_8007E2F4(fp, 0x1FF);
}

void ftCo_CatchAttack_Anim(Fighter_GObj* gobj)
{
    if (!ftAnim_IsFramesRemaining(gobj)) {
        fn_800DA2B0(gobj);
    }
}

void ftCo_CatchAttack_IASA(Fighter_GObj* gobj) {}

void ftCo_CatchAttack_Phys(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    ftCommon_CalcGroundAccel_Deaccel(fp, p_ftCommonData->x64 *
                                             fp->co_attrs.ground_friction);
    ftCommon_SetSelfMovementFromGroundedMovement(gobj);
}

void ftCo_CatchAttack_Coll(Fighter_GObj* gobj)
{
    ft_800841B8(gobj, fn_800DA618);
}

void fn_800DA618(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    Fighter_GObj* victim = fp->victim_gobj;

    ftCo_800DC920(gobj, victim);
    ftCo_Fall_Enter(gobj);
    ftCo_Fall_Enter(victim);
}

void fn_800DA668(Fighter_GObj* gobj)
{
    GET_FIGHTER(gobj)->take_dmg_cb = NULL;
}

void fn_800DA678(Fighter_GObj* gobj)
{
    fn_800DA054(gobj);
}
