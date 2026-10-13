#include "ftCo_SpecialAir.h"

#include <Runtime/platform.h>

#include <dolphin/mtx.h>
#include <melee/ft/fighter.h>
#ifdef MELEE_NATIVE
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_fighter_lifetime.h>
#endif
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/types.h>

bool ftCo_SpecialAir_CheckInput(Fighter_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    if (!(fp->input.pressed_buttons & HSD_PAD_B)) return false;
#ifdef MELEE_NATIVE
    NativeFighterLifetime lifetime;
    if (!nativeFighterLifetime_Capture(fp, &lifetime)) return false;
    uint32_t hook;
#endif
    HSD_GObjEvent callback;
    bool side = false, neutral = false;
    if (fp->input.lstick[0].y >= p_ftCommonData->x21C) {
        callback =
#ifdef MELEE_NATIVE
            lifetime.source_owned ? NULL :
#endif
            ftData_SpecialAirHi[fp->kind];
#ifdef MELEE_NATIVE
        hook = NativeScriptFighter_SpecialAirHi;
#endif
    } else if (fp->input.lstick[0].y <= -p_ftCommonData->x21C) {
        callback =
#ifdef MELEE_NATIVE
            lifetime.source_owned ? NULL :
#endif
            ftData_SpecialAirLw[fp->kind];
#ifdef MELEE_NATIVE
        hook = NativeScriptFighter_SpecialAirLw;
#endif
    } else if (ABS(fp->input.lstick[0].x) >= p_ftCommonData->x218) {
        callback =
#ifdef MELEE_NATIVE
            lifetime.source_owned ? NULL :
#endif
            ftData_SpecialAirS[fp->kind]; side = true;
#ifdef MELEE_NATIVE
        hook = NativeScriptFighter_SpecialAirS;
#endif
    } else {
        callback =
#ifdef MELEE_NATIVE
            lifetime.source_owned ? NULL :
#endif
            ftData_SpecialAirN[fp->kind]; neutral = true;
#ifdef MELEE_NATIVE
        hook = NativeScriptFighter_SpecialAirN;
#endif
    }
#ifdef MELEE_NATIVE
    if (lifetime.source_owned) {
        if (!nativeScriptFighter_HasHook(gobj, hook)) return false;
    } else
#endif
    if (callback == NULL) return false;
    if (side && fp->input.lstick[0].x * fp->facing_dir < -p_ftCommonData->x220) {
        ftCommon_UpdateFacing(fp);
    }
    if (neutral && fp->x676_x < p_ftCommonData->x224 &&
        ((fp->facing_dir == -1 && fp->x2228_b7 == 1) ||
         (fp->facing_dir == +1 && fp->x2228_b7 == 0))) {
        fp->facing_dir = -fp->facing_dir;
    }
#ifdef MELEE_NATIVE
    if (lifetime.source_owned) {
        if (nativeScriptFighter_Run(gobj, hook, NULL, 0, NULL, NULL) != NativeScriptVM_Ok) return true;
    } else
#endif
    callback(gobj);
#ifdef MELEE_NATIVE
    if (!nativeFighterLifetime_Current(&lifetime)) return true;
#endif
    fp->x2227_b5 = true;
    return true;
}
