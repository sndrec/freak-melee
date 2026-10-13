#include "ftCo_ItemParasolFallSpecial.h"

#include <melee/ft/forward.h>
#include <melee/ft/kinds/ftPeach/forward.h>

#include "forward.h"
#include "ftCo_FallSpecial.h"
#include "ftCo_JumpAerial.h"
#include "ftCo_Landing.h"
#include "inlines.h"
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_0877.h>
#include <melee/ft/inlines.h>
#include <melee/ft/types.h>
#include <melee/ft/native_fighter_held_motion.h>
#include <melee/ft/native_callback_lifetime.h>

void ftCo_800CF3C8(Fighter_GObj* gobj)
{
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(gobj, NativeCallbackSource_Fighter, &generation)) return;
    Fighter* fp = GET_FIGHTER(gobj);
    int32_t selected = nativeFighterHeldMotion_Select(gobj,
        ftCo_MS_ItemParasolFallSpecial, fp->motion_id);
    if (selected < 0) return;
    Fighter_ChangeMotionState(gobj, selected,
        Ft_MF_SkipHit | Ft_MF_SkipParasol, 0.0f, 1.0f, 0.0f, NULL);
}

void ftCo_ItemParasolFallSpecial_Anim(Fighter_GObj* gobj) {}

void ftCo_ItemParasolFallSpecial_IASA(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->input.lstick[0].y <= p_ftCommonData->close_parasol_threshold &&
        ft_80087988(gobj))
    {
        ftCo_800968C8(gobj);
    } else {
        RETURN_IF(ftCo_800CB870(gobj));
    }
}

void ftCo_ItemParasolFallSpecial_Phys(Fighter_GObj* gobj)
{
    ftCo_ItemParasol_Phys(gobj);
}

void ftCo_ItemParasolFallSpecial_Coll(Fighter_GObj* gobj)
{
    ft_8008370C(gobj, ftCo_LandingFallSpecial_Enter_Basic);
}
