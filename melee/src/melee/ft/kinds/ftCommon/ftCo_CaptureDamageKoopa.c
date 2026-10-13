#include "ftCo_CaptureDamageKoopa.h"

#include <Runtime/platform.h>

#include <melee/ft/forward.h>
#include <sysdolphin/baselib/forward.h>

#include "ftCo_Attack100.h"
#include "ftCo_CaptureCut.h"
#include "ftCo_CaptureKoopa.h"
#include "ftCo_CaptureWaitKoopa.h"
#include <melee/ft/fighter.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/native_fighter_capture.h>

void ftCo_EnterAttachedCaptureMotion(Fighter_GObj* gobj, FtMotionId motion)
{
    Fighter* fp = GET_FIGHTER(gobj);
    NativeFighterCapturePair pair;
    if (!nativeFighterCapture_Retain(fp->victim_gobj, &pair) ||
        pair.victim.object != gobj || !nativeFighterCapture_Attached(&pair)) return;
    Fighter_ChangeMotionState(gobj, motion,
                              fp->x2222_b6 ? Ft_MF_FreezeState : Ft_MF_None, 0,
                              1, 0, fp->victim_gobj);
    if (!nativeFighterCapture_Attached(&pair)) return;
    fp->accessory1_cb = ftCo_800DB464;
    ftCommon_8007E2F4(fp, 0x1FF);
    ftAnim_8006EBA4(gobj);
}

static inline void inlineA0(Fighter_GObj* gobj, HSD_GObjEvent cb)
{
    u8 _[8] = { 0 };
    Fighter* fp = GET_FIGHTER(gobj);
    if (!ftAnim_IsFramesRemaining(gobj)) {
        cb(gobj);
        return;
    }
    /// @todo #ftCo_CaptureWaitKoopa_Anim
    ftCo_800BC458(gobj);
    if (fp->grab_timer <= 0) {
        ftCo_800DA698(fp->victim_gobj, false);
        fp->facing_dir = -fp->facing_dir;
        ftCo_CaptureCut_Enter(gobj);
    } else {
        ftCo_800BC4A8(gobj);
    }
}

void ftCo_CaptureDamageKoopa_Anim(Fighter_GObj* gobj)
{
    inlineA0(gobj, ftCo_800BCC20);
}

void ftCo_CaptureDamageKoopa_IASA(Fighter_GObj* gobj) {}

void ftCo_CaptureDamageKoopa_Phys(Fighter_GObj* gobj) {}

void ftCo_CaptureDamageKoopa_Coll(Fighter_GObj* gobj) {}

void ftCo_CaptureDamageKoopaAir_Anim(Fighter_GObj* gobj)
{
    inlineA0(gobj, ftCo_800BCD00);
}

void ftCo_CaptureDamageKoopaAir_IASA(Fighter_GObj* gobj) {}

void ftCo_CaptureDamageKoopaAir_Phys(Fighter_GObj* gobj) {}

void ftCo_CaptureDamageKoopaAir_Coll(Fighter_GObj* gobj) {}
