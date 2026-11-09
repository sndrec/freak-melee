#include "ftCo_LandingAir.h"

#include <Runtime/platform.h>

#include <melee/ft/forward.h>

#include "forward.h"
#include "ftCo_Landing.h"
#include "ftCo_Wait.h"
#include <melee/ft/fighter.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/types.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>

void ftCo_LandingAir_EnterWithLag(Fighter_GObj* gobj)
{
    u8 _[20] = { 0 };
    float lag;
    FtMotionId msid = ftCo_MS_None;
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->cmd_vars[0]) {
        switch (fp->motion_id) {
        case ftCo_MS_AttackAirN:
            lag = fp->co_attrs.landingairn_lag;
            msid = ftCo_MS_LandingAirN;
            break;
        case ftCo_MS_AttackAirF:
            lag = fp->co_attrs.landingairf_lag;
            msid = ftCo_MS_LandingAirF;
            break;
        case ftCo_MS_AttackAirB:
            lag = fp->co_attrs.landingairb_lag;
            msid = ftCo_MS_LandingAirB;
            break;
        case ftCo_MS_AttackAirHi:
            lag = fp->co_attrs.landingairhi_lag;
            msid = ftCo_MS_LandingAirHi;
            break;
        case ftCo_MS_AttackAirLw:
            msid = ftCo_MS_LandingAirLw;
            lag = fp->co_attrs.landingairlw_lag;
            break;
        }
    }
    if (msid != ftCo_MS_None) {
        ftCo_LandingAir_EnterWithMsidLag(gobj, msid, lag);
    } else {
        ftCo_Landing_Enter_Basic(gobj);
    }
}

void ftCo_LandingAir_EnterWithMsidLag(Fighter_GObj* gobj, FtMotionId msid,
                                      float lag)
{
    u8 _[8];
#ifdef MELEE_NATIVE
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(gobj, NativeCallbackSource_Fighter, &generation)) return;
    Fighter* fp = GET_FIGHTER(gobj);
    const FighterKind kind = fp->kind;
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(kind);
#endif
    ftCommon_8007D7FC(GET_FIGHTER(gobj));
#ifdef MELEE_NATIVE
    if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter) ||
        fp->kind != kind || nativeFighterRuntime_Definition(kind) != definition) return;
#endif
    Fighter_ChangeMotionState(gobj, msid, Ft_MF_None, 0, 1, 0, NULL);
#ifdef MELEE_NATIVE
    if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter) ||
        fp->kind != kind || nativeFighterRuntime_Definition(kind) != definition) return;
#endif
    // Keep the uncanceled clip timing, including fighter-specific landing lag.
    // The first interrupt is at the old shortened landing's end.
    if (lag > 0.0f) {
        const float speed = (ftAnim_8006F484(gobj) + 0.1f) / lag;
        const int shortened_lag = (int) (lag / p_ftCommonData->xE8);
        GET_FIGHTER(gobj)->aerial_landing_interrupt_frame =
            speed * (shortened_lag > 0 ? shortened_lag : 1);
        ftAnim_SetAnimRate(gobj, speed);
    }
}

void ftCo_LandingAir_Anim(Fighter_GObj* gobj)
{
    ftCo_Landing_Anim(gobj);
}

void ftCo_LandingAir_IASA(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->aerial_landing_interrupt_frame > 0.0f &&
        fp->cur_anim_frame + 0.0001f >= fp->aerial_landing_interrupt_frame)
    {
        ftCo_Wait_IASA(gobj);
    }
}

void ftCo_LandingAir_Phys(Fighter_GObj* gobj)
{
    ftCo_Landing_Phys(gobj);
}

void ftCo_LandingAir_Coll(Fighter_GObj* gobj)
{
    ftCo_Landing_Coll(gobj);
}
