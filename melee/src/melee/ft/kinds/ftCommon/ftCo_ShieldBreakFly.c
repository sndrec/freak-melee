#include "ftCo_ShieldBreakFly.h"

#include <Runtime/platform.h>

#include <melee/ft/forward.h>
#include <sysdolphin/baselib/forward.h>

#include "ftCo_ShieldBreakDown.h"
#include "ftCo_ShieldBreakFall.h"
#include "inlines.h"
#include <dolphin/mtx.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0877.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/types.h>
#ifdef MELEE_NATIVE
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_fighter_lifetime.h>
#endif

void ftCo_80098B20(Fighter_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
#ifdef MELEE_NATIVE
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(gobj, NativeCallbackSource_Fighter,
                                       &generation)) return;
    NativeFighterLifetime lifetime;
    bool retained = nativeFighterLifetime_Capture(fp, &lifetime);
    if (!retained && nativeScriptFighter_Program(fp->kind)) return;
#endif
    ftCommon_8007D5D4(fp);
#ifdef MELEE_NATIVE
    if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter) ||
        (retained && !nativeFighterLifetime_Current(&lifetime))) return;
#endif
    Fighter_ChangeMotionState(gobj, 205, 64, 0, 1, 0, NULL);
#ifdef MELEE_NATIVE
    if (!nativeCallbackSource_Alive(gobj, generation,
                                    NativeCallbackSource_Fighter) ||
        (retained && !nativeFighterLifetime_Current(&lifetime))) return;
#endif
    ftAnim_8006EBA4(gobj);
#ifdef MELEE_NATIVE
    if (!nativeCallbackSource_Alive(gobj, generation,
                                    NativeCallbackSource_Fighter) ||
        (retained && !nativeFighterLifetime_Current(&lifetime))) return;
#endif
    fp->self_vel.x = 0;
    fp->self_vel.y = fp->co_attrs.shield_break_initial_velocity;
#ifdef MELEE_NATIVE
    const NativeFighterDefinition* definition =
        nativeFighterRuntime_Definition(fp->kind);
    if (nativeScriptFighter_IsAttached(gobj)) {
        if (!retained ||
            (nativeScriptFighter_HasHook(gobj, NativeScriptFighter_ShieldBreak) &&
             nativeScriptFighter_Run(gobj, NativeScriptFighter_ShieldBreak,
                 NULL, 0, NULL, NULL) != NativeScriptVM_Ok) ||
            !nativeFighterLifetime_Current(&lifetime)) return;
        fp = GET_FIGHTER(gobj);
    } else if (definition != NULL && definition->on_shield_break != NULL) {
        definition->on_shield_break(gobj);
        if (!nativeCallbackSource_Alive(gobj, generation,
                                        NativeCallbackSource_Fighter) ||
            (retained && !nativeFighterLifetime_Current(&lifetime))) return;
        fp = GET_FIGHTER(gobj);
    } else
#endif
    if (fp->kind == Ft_Kind_Purin) {
        fp->x2222_b3 = true;
    }
    ftCo_SpawnEf(gobj, fp->parts[fp->ft_data->x8->x11].joint, 0, 1051);
#ifdef MELEE_NATIVE
    if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter) ||
        (retained && !nativeFighterLifetime_Current(&lifetime))) return;
#endif
    ft_PlaySFX(fp, 130, 127, 64);
#ifdef MELEE_NATIVE
    if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter) ||
        (retained && !nativeFighterLifetime_Current(&lifetime))) return;
#endif
    ftCommon_8007EBAC(fp, 24, 0U);
#ifdef MELEE_NATIVE
    if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter) ||
        (retained && !nativeFighterLifetime_Current(&lifetime))) return;
#endif
    ftColl_8007B62C(gobj, 2);
}

void ftCo_ShieldBreakFly_Anim(Fighter_GObj* gobj)
{
    if (!ftAnim_IsFramesRemaining(gobj)) {
        ftCo_80098D90(gobj);
    }
}

void ftCo_ShieldBreakFly_IASA(HSD_GObj* gobj) {}

void ftCo_ShieldBreakFly_Phys(Fighter_GObj* gobj)
{
    ft_80084EEC(gobj);
}

void ftCo_ShieldBreakFly_Coll(Fighter_GObj* gobj)
{
    ft_80082C74(gobj, ftCo_80098E3C);
}

void ftCo_80098C9C(Fighter_GObj* gobj)
{
    u8 _[8] = { 0 };
    ftCo_80098B20(gobj);
}
