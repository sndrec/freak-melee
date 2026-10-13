#include "ftCo_CaptureKoopa.h"
#include <melee/ft/native_fighter_target.h>
#include <melee/ft/native_fighter_capture.h>

#include <Runtime/platform.h>

#include "ftCo_Attack100.h"
#include "ftCo_CaptureDamageKoopa.h"
#include "ftCo_Lift.h"
#include "ftCo_Throw.h"
#include "types.h"
#include <dolphin/mtx.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/kinds/ftCaptain/types.h>
#include <melee/ft/types.h>
#include <sysdolphin/baselib/jobj.h>

void ftCo_800BC438(Fighter_GObj* gobj)
{
    Fighter_UpdateModelScale(gobj);
}

void ftCo_800BC458(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->grab_timer -= GET_FIGHTER(fp->victim_gobj)->capture_settings.timer_decay;
    fp->mv.co.capturekoopa.x0 =
        ftCommon_GrabMash(fp, GET_FIGHTER(fp->victim_gobj)->capture_settings.mash_reduction);
}

void ftCo_800BC4A8(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->mv.ca.specialhi.vel.y) {
        HSD_JObj* jobj =
            fp->parts[ftParts_GetBoneIndex(fp, FtPart_YRotN)].joint;
        float mv_xC = fp->mv.co.capturekoopa.xC;
        float fp_x1A50 = fp->x1A50 * mv_xC;
        float fp_x1A51 = fp->x1A51 * mv_xC;
        if (ABS(fp_x1A50 + HSD_JObjGetTranslationZ(jobj)) <=
            fp->mv.co.capturekoopa.x10)
        {
            HSD_JObjAddTranslationZ(jobj, fp_x1A50);
        }
        if (ABS(fp_x1A51 + HSD_JObjGetTranslationY(jobj)) <=
            fp->mv.co.walk.fast_anim_frame)
        {
            HSD_JObjAddTranslationY(jobj, fp_x1A51);
        }
        fp->mv.co.capturekoopa.x8 -= 1;
        if (fp->mv.ca.specialhi.vel.y <= 0 && !fp->mv.co.capturekoopa.x0) {
            ftAnim_SetAnimRate(gobj, 1);
            fp->mv.co.capturekoopa.x8 = 0;
        }
    }
    if (fp->mv.co.capturekoopa.x8 <= 0 && fp->mv.co.capturekoopa.x0) {
        fp->mv.co.capturekoopa.x8 = GET_FIGHTER(fp->victim_gobj)->capture_settings.mash_pause_frames;
        ftAnim_SetAnimRate(gobj, GET_FIGHTER(fp->victim_gobj)->capture_settings.mash_animation_rate);
    }
}

void ftCo_InitializeAttachedCapture(Fighter_GObj* gobj, Fighter_GObj* vic_gobj,
                                    FtMotionId motion)
{
    NativeFighterCapturePair pair;
    if (!nativeFighterCapture_Retain(vic_gobj, &pair) ||
        pair.victim.object != gobj) return;
    Fighter* vic_fp = GET_FIGHTER(vic_gobj);
    Fighter* fp = GET_FIGHTER(gobj);
    ftCommon_8007DB58(gobj);
    if (!nativeFighterCapture_Current(&pair, true)) return;
    ftCo_8009750C(gobj);
    if (!nativeFighterCapture_Current(&pair, true)) return;
    ftCo_800DD168(gobj);
    if (!nativeFighterCapture_Current(&pair, true)) return;
    fp->x1A5C = vic_gobj;
    fp->native_hitlag_link_generation = nativeFighterTarget_Id(fp->x1A5C);
    fp->victim_gobj = vic_gobj;
    fp->x221B_b5 = false;
    fp->x221B_b7 = false;
    fp->facing_dir = vic_fp->facing_dir;
    ftCommon_InitGrab(fp, 0, GET_FIGHTER(fp->victim_gobj)->capture_settings.timer_damage_multiplier);
    fp->mv.ca.specialhi.vel.y = 0;
    ftCo_800DB368(vic_fp, fp);
    if (!nativeFighterCapture_Attached(&pair)) return;
    ftCo_EnterAttachedCaptureMotion(gobj, motion);
    if (!nativeFighterCapture_Attached(&pair)) return;
    ftCommon_8007D5D4(fp);
    if (!nativeFighterCapture_Attached(&pair)) return;
    fp->mv.co.capturekoopa.xC = GET_FIGHTER(fp->victim_gobj)->capture_settings.wiggle_scale;
    fp->mv.co.capturekoopa.x10 = GET_FIGHTER(fp->victim_gobj)->capture_settings.wiggle_limit;
    ftCommon_8007E2FC(gobj);
}

void ftCo_CaptureKoopa_Anim(HSD_GObj* gobj) {}

void ftCo_CaptureKoopa_IASA(HSD_GObj* gobj) {}

void ftCo_CaptureKoopa_Phys(HSD_GObj* gobj) {}

void ftCo_CaptureKoopa_Coll(HSD_GObj* gobj) {}

void ftCo_CaptureKoopaAir_Anim(HSD_GObj* gobj) {}

void ftCo_CaptureKoopaAir_IASA(HSD_GObj* gobj) {}

void ftCo_CaptureKoopaAir_Phys(HSD_GObj* gobj) {}

void ftCo_CaptureKoopaAir_Coll(HSD_GObj* gobj) {}
