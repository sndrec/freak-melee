#include "ftCo_ItemParasolOpen.h"

#include <melee/ft/forward.h>
#include <melee/ft/kinds/ftPeach/forward.h>

#include "forward.h"
#include "ftCo_AirCatch.h"
#include "ftCo_AttackAir.h"
#include "ftCo_ItemParasolFall.h"
#include "ftCo_ItemParasolFallSpecial.h"
#include "ftCo_ItemThrow.h"
#include "ftCo_JumpAerial.h"
#include "ftCo_Landing.h"
#include "ftCo_SpecialAir.h"
#include "inlines.h"
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/inlines.h>
#include <melee/ft/types.h>
#include <melee/ft/native_fighter_held_motion.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_lifetime.h>

bool ftCo_800CEE70(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->x2221_b5) {
        return false;
    }
    if ((fp->x2221_b7 ||
         fp->input.lstick[0].y >= p_ftCommonData->open_parasol_threshold) &&
        fp->self_vel.y <= 0.0f && (ftGetParasolStatus(gobj) == 6))
    {
        ftCo_ItemParasolOpen_Enter(gobj, 0.0f);
        return true;
    }
    return false;
}

static inline void ft_800CEF08_inline1(Fighter* fp, int x2104)
{
    fp->x2221_b7 = false;
    fp->x2221_b4 = true;
    if (fp->x2221_b6) {
        fp->x2104 = x2104;
        fp->x2221_b6 = false;
    }
}

void ftCo_ItemParasolOpen_Enter(Fighter_GObj* gobj, float blend)
{
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(gobj, NativeCallbackSource_Fighter, &generation)) return;
    Fighter* fp = GET_FIGHTER(gobj);
    NativeFighterLifetime lifetime;
    if (!nativeFighterLifetime_Capture(fp, &lifetime)) return;
    const ftCommonData* common = p_ftCommonData;
    if (!common) return;
    fp->mv.co.parasol_open.prev_msid = fp->motion_id;
    int32_t selected = nativeFighterHeldMotion_Select(gobj, ftCo_MS_ItemParasolOpen,
                                                     fp->motion_id);
    if (selected < 0 || !nativeFighterLifetime_Current(&lifetime) ||
        p_ftCommonData != common) return;
    Fighter_ChangeMotionState(gobj, selected, 0, 0.0f, 1.0f, blend, NULL);
    if (!nativeFighterLifetime_Current(&lifetime) || p_ftCommonData != common) return;
    ftAnim_8006EBA4(gobj);
    if (!nativeFighterLifetime_Current(&lifetime) || p_ftCommonData != common) return;
    ft_800CEF08_inline1(fp, common->x59C);
}

void ftCo_ItemParasolOpen_Anim(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (!ftAnim_IsFramesRemaining(gobj)) {
        if (fp->mv.co.parasol_open.prev_msid == ftCo_MS_FallSpecial) {
            ftCo_800CF3C8(gobj);
        } else {
            ftCo_800CF280(gobj);
        }
    }
}

void ftCo_ItemParasolOpen_IASA(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->mv.co.parasol_open.prev_msid != ftCo_MS_FallSpecial) {
        RETURN_IF(ftCo_SpecialAir_CheckInput(gobj));
        RETURN_IF(ftCo_80095328(gobj, NULL, NULL));
        RETURN_IF(ftCo_800C3B10(gobj));
        RETURN_IF(ftCo_AttackAir_CheckItemThrowInput(gobj));
    }
    RETURN_IF(ftCo_800CB870(gobj));
}

void ftCo_ItemParasolOpen_Phys(Fighter_GObj* gobj)
{
    ftCo_DatAttrs* ca;
    Fighter* fp = GET_FIGHTER(gobj);
    ca = &fp->co_attrs;

    {
        float mul = p_ftCommonData->x590;
        float grav = ca->gravity * mul;
        float terminal_vel = ca->terminal_velocity * mul;
        ftCommon_Fall(fp, grav, terminal_vel);
        {
            float drift, target_vel;
            if (ABS(fp->input.lstick[0].x) >= p_ftCommonData->x258) {
                drift = p_ftCommonData->x58C *
                        (fp->input.lstick[0].x * ca->air_drift_stick_mul);
                target_vel = p_ftCommonData->x58C *
                             (fp->input.lstick[0].x * ca->air_drift_max);
            } else {
                target_vel = 0.0F;
                drift = 0.0F;
            }
            ftCommon_CalcSelfAccel_AccelToVelClamped(fp, drift, target_vel,
                                                     ca->aerial_friction);
        }
    }
}

void ftCo_ItemParasolOpen_Coll(Fighter_GObj* gobj)
{
    ft_8008370C(gobj, ftCo_Landing_Enter_Basic);
}
