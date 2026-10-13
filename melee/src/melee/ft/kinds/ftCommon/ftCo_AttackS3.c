#include "ftCo_AttackS3.h"

#include <Runtime/platform.h>

#include <melee/ft/forward.h>

#include "forward.h"
#include "ftCo_ItemThrow.h"
#include "ftCo_Wait.h"
#include "ftpickupitem.h"
#include <dolphin/mtx.h>
#include <melee/ft/fighter.h>
#ifdef MELEE_NATIVE
#include <melee/ft/native_fighter_lifetime.h>
#endif
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ft_0CDD.h>
#include <melee/ft/ft_0DF1.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftswing.h>
#include <melee/ft/types.h>
#include <melee/it/it_26B1.h>

/* 08B788 */ static void decideAngle(Fighter_GObj* gobj, float stick_angle);

bool ftCo_AttackS3_CheckInput(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    bool tilt_stick =
        HSD_PadProfileUsesTiltStick(fp->input.held_buttons[0]) &&
        ftCo_800DF1C8(fp) &&
        ABS(ftCo_GetCStickAngle(fp)) < p_ftCommonData->x20_radians;
    bool attack_button = fp->input.pressed_buttons & HSD_PAD_A &&
                         fp->input.lstick[0].x * fp->facing_dir >=
                             p_ftCommonData->x98 &&
                         ABS(ftCo_GetLStickAngle(fp)) <
                             p_ftCommonData->x20_radians;
    if (tilt_stick || attack_button) {
        float stick_angle = tilt_stick ? ftCo_GetCStickAngle(fp)
                                       : ftCo_GetLStickAngle(fp);
        if (tilt_stick) {
            fp->facing_dir = fp->input.cstick[0].x >= 0.0F ? 1.0F : -1.0F;
        }
        if (fp->item_gobj) {
            if (fp->input.held_buttons[0] & HSD_PAD_LR ||
                itGetUseKind(fp->item_gobj) == 0)
            {
                ftCo_800957F4(gobj, ftCo_MS_LightThrowF);
                return true;
            } else {
                switch (itGetUseKind(fp->item_gobj)) {
                case 2:
                    ftCo_Attack_800CCF58(gobj, 1);
                    return true;
                case 3:
                    ftCo_Attack_800CDD14(gobj);
                    return true;
                }
            }
        }
        decideAngle(gobj, stick_angle);
        return true;
    }
    return false;
}

static void decideAngle(Fighter_GObj* gobj, float stick_angle)
{
    Fighter* fp = GET_FIGHTER(gobj);
#ifdef MELEE_NATIVE
    NativeFighterLifetime lifetime;
    if (!nativeFighterLifetime_Capture(fp, &lifetime)) return;
#endif
    if (!ftpickupitem_80094790(gobj)) {
#ifdef MELEE_NATIVE
        if (!nativeFighterLifetime_Current(&lifetime)) return;
#endif
        FtMotionId msid;
        if (stick_angle > p_ftCommonData->x9C_radians &&
            ftData_80085FD4(fp, ftCo_MS_AttackS3S)->x8 != 0)
        {
            msid = ftCo_MS_AttackS3Hi;
        } else if (stick_angle > p_ftCommonData->xA0_radians &&
                   ftData_80085FD4(fp, ftCo_MS_AttackS3LwS)->x8 != 0)
        {
            msid = ftCo_MS_AttackS3HiS;
        } else if (stick_angle < p_ftCommonData->xA8_radians &&
                   ftData_80085FD4(fp, ftCo_MS_AttackLw3)->x8 != 0)
        {
            msid = ftCo_MS_AttackS3Lw;
        } else if (stick_angle < p_ftCommonData->xA4_radians &&
                   ftData_80085FD4(fp, ftCo_MS_AttackHi3)->x8 != 0)
        {
            msid = ftCo_MS_AttackS3LwS;
        } else {
            msid = ftCo_MS_AttackS3S;
        }
        fp->allow_interrupt = false;
        Fighter_ChangeMotionState(gobj, msid, Ft_MF_None, 0, 1, 0, NULL);
#ifdef MELEE_NATIVE
        if (!nativeFighterLifetime_Current(&lifetime)) return;
#endif
        ftAnim_8006EBA4(gobj);
    }
}

void ftCo_AttackS3_Anim(Fighter_GObj* gobj)
{
    if (!ftAnim_IsFramesRemaining(gobj)) {
        ft_8008A2BC(gobj);
    }
}

void ftCo_AttackS3_IASA(Fighter_GObj* gobj)
{
    if (GET_FIGHTER(gobj)->allow_interrupt) {
        ftCo_Wait_IASA(gobj);
    }
}

void ftCo_AttackS3_Phys(Fighter_GObj* gobj)
{
    ft_80084F3C(gobj);
}

void ftCo_AttackS3_Coll(Fighter_GObj* gobj)
{
    ft_80084104(gobj);
}
