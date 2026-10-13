#include "ftCo_ThrownKoopa.h"

#include <Runtime/platform.h>

#include <melee/ft/forward.h>

#include "ftCo_Thrown.h"
#include "types.h"
#include <melee/ft/fighter.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/types.h>
#include <melee/ft/native_fighter_capture.h>

void ftCo_EnterAttachedThrow(Fighter_GObj* gobj, FtMotionId motion,
                             MotionFlags flags, float start, bool initialize)
{
    Fighter* fp = GET_FIGHTER(gobj);
    NativeFighterCapturePair pair;
    if (!nativeFighterCapture_Retain(fp->victim_gobj, &pair) ||
        pair.victim.object != gobj || !nativeFighterCapture_Attached(&pair)) return;
    fp->facing_dir = pair.holder.data->facing_dir;
    fp->mv.co.capturekoopa.x0 = false;
    Fighter_ChangeMotionState(gobj, motion, flags, start, 1, 0, pair.holder.object);
    if (!nativeFighterCapture_Attached(&pair)) return;
    fp->accessory1_cb = ftCo_800DE508;
    ftCommon_8007E2F4(fp, 0x1FF);
    if (initialize) ftAnim_8006EBA4(gobj);
}

void ftCo_ThrownKoopaF_Anim(Fighter_GObj* gobj) {}

void ftCo_ThrownKoopaF_IASA(Fighter_GObj* gobj) {}

void ftCo_ThrownKoopaF_Phys(Fighter_GObj* gobj) {}

void ftCo_ThrownKoopaF_Coll(Fighter_GObj* gobj) {}

void ftCo_ThrownKoopaB_Anim(Fighter_GObj* gobj) {}

void ftCo_ThrownKoopaB_IASA(Fighter_GObj* gobj) {}

void ftCo_ThrownKoopaB_Phys(Fighter_GObj* gobj) {}

void ftCo_ThrownKoopaB_Coll(Fighter_GObj* gobj) {}

void ftCo_ThrownKoopaAirF_Anim(Fighter_GObj* gobj) {}

void ftCo_ThrownKoopaAirF_IASA(Fighter_GObj* gobj) {}

void ftCo_ThrownKoopaAirF_Phys(Fighter_GObj* gobj) {}

void ftCo_ThrownKoopaAirF_Coll(Fighter_GObj* gobj) {}

void ftCo_ThrownKoopaAirB_Anim(Fighter_GObj* gobj) {}

void ftCo_ThrownKoopaAirB_IASA(Fighter_GObj* gobj) {}

void ftCo_ThrownKoopaAirB_Phys(Fighter_GObj* gobj) {}

void ftCo_ThrownKoopaAirB_Coll(Fighter_GObj* gobj) {}
