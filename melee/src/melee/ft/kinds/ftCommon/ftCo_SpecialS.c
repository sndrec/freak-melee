#include "ftCo_SpecialS.h"

#include <Runtime/platform.h>

#include <dolphin/mtx.h>
#include <melee/ft/fighter.h>
#ifdef MELEE_NATIVE
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_callback_lifetime.h>
#endif
#include <melee/ft/ft_081B.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/types.h>

/* 0960CC */ static void doEnter(Fighter_GObj* gobj);

/// Check @c SpecialS input without entering the state.
bool ftCo_SpecialS_HasInput(Fighter* fp)
{
    if (fp->input.pressed_buttons & HSD_PAD_B &&
        ABS(fp->input.lstick[0].x) >= p_ftCommonData->x218)
    {
        return true;
    }
    return false;
}

bool ftCo_SpecialS_CheckInput(Fighter_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    if (ftData_SpecialS[fp->kind] == NULL
#ifdef MELEE_NATIVE
            && !nativeScriptFighter_IsAttached(gobj)
#endif
        ) {
        return false;
    }
    if (fp->x688 == 0) {
        if (fp->input.lstick[0].x * fp->facing_dir < -p_ftCommonData->x220) {
            ftCommon_UpdateFacing(fp);
        }
        doEnter(gobj);
        return true;
    }
    return false;
}

static void doEnter(Fighter_GObj* gobj)
{
    u8 _[8];
    Fighter* fp = gobj->user_data;
    fp->gr_vel +=
        -(fp->gr_vel * (1 - fp->co_attrs.specials_ground_speed_retention)) *
        ft_GetGroundFrictionMultiplier(fp);
#ifdef MELEE_NATIVE
    if (nativeScriptFighter_IsAttached(gobj)) {
        if (nativeScriptFighter_Run(gobj,NativeScriptFighter_SpecialS,NULL,0, NULL,NULL)!=NativeScriptVM_Ok) return;
        uint32_t generation;
        if (!nativeCallbackSource_Snapshot(gobj,NativeCallbackSource_Fighter,&generation)) return;
    } else
#endif
    ftData_SpecialS[fp->kind](gobj);
}
