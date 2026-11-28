#include "ftCo_KneeBend.h"

#include <melee/ft/forward.h>

#include <placeholder.h>

#include "forward.h"
#include "ftCo_0A01.h"
#include "ftCo_Attack100.h"
#include "ftCo_AttackHi4.h"
#include "ftCo_ItemScrew.h"
#include "ftCo_Jump.h"
#include <melee/db/db.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0D27.h>
#include <melee/ft/inlines.h>
#include <melee/ft/types.h>
#include <melee/gm/gmvs.h>
#include <melee/gm/native_profile_input.h>

void ftCo_KneeBend_Enter(Fighter_GObj* gobj, ftCo_JumpInput jump_input)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->mv.co.kneebend.jump_input = jump_input;
    fp->mv.co.kneebend.is_short_hop =
        HSD_PadProfileForcesShortHop(fp->input.held_buttons[0]);
    Fighter_ChangeMotionState(gobj, ftCo_MS_KneeBend, 0U, 0.0F, 1.0F, 0.0F,
                              NULL);
}

void ftCo_KneeBend_Anim(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    ftCo_DatAttrs* co_attrs = &fp->co_attrs;

    if (fp->cur_anim_frame >= co_attrs->jump_startup_time ||
        !ftAnim_IsFramesRemaining(gobj))
    {
        if (ft_800D2D0C(gobj)) {
            ftCo_ItemScrew_Enter(gobj);
        } else {
            ftCo_Jump_Enter(gobj);
        }
    }
}

static void check_short_hop(Fighter* fp, HSD_Pad held, float lstick_y,
                            float cstick_y)
{
    const ftCo_JumpInput jump_input = fp->mv.co.kneebend.jump_input;
    const float release = p_ftCommonData->tap_jump_release_threshold;

    if (HSD_PadProfileForcesShortHop(held) ||
        (jump_input == JumpInput_XY && !(held & HSD_PAD_XY)) ||
        (jump_input == JumpInput_LStick && lstick_y < release) ||
        (jump_input == JumpInput_CStick && cstick_y < release))
    {
        fp->mv.co.kneebend.is_short_hop = true;
    }
}

void ftCo_KneeBend_Check_ShortHop(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    // Queued commands can replay input from before the player released jump.
    const NativeActionBuffer* buffer = &fp->action_buffer;
    check_short_hop(fp,
                    buffer->checking ? buffer->live_held : fp->input.held_buttons[0],
                    buffer->checking ? buffer->live_lstick.y : fp->input.lstick[0].y,
                    buffer->checking ? buffer->live_cstick_y : fp->input.cstick[0].y);
}

void ftCo_KneeBend_CheckShortHopAtTakeoff(Fighter* fp)
{
    if (fp->mv.co.kneebend.is_short_hop ||
        fp->mv.co.kneebend.jump_input == JumpInput_None ||
        fp->x2224_b2 || fp->x618_player_id >= 4 || ftCo_800A2040(fp))
    {
        return;
    }

    // The pad sample is ready before animation advances, but the fighter
    // normally copies it afterward. Read it before committing jump velocity.
    const HSD_PadStatus* pad = &HSD_PadGameStatus[fp->x618_player_id];
    HSD_Pad held = pad->button;
    if (pad->extension & NATIVE_PROFILE_EXT_SHORT_HOP) {
        held |= HSD_PAD_PROFILE_SHORT_HOP;
    }
    if (gm_8016B0FC()) {
        held &= HSD_PAD_A;
    }
    const float lstick_y = pad->nml_stickY;
    const float cstick_y = DbLevel < DbLKind_DebugRom &&
                                  !gm_IsCurrently1PMode_inline()
                              ? pad->nml_subStickY
                              : 0.0f;
    const float deadzone = p_ftCommonData->vertical_stick_deadzone;
    check_short_hop(fp, held, ABS(lstick_y) <= deadzone ? 0.0f : lstick_y,
                   ABS(cstick_y) <= deadzone ? 0.0f : cstick_y);
}

void ftCo_KneeBend_IASA(Fighter_GObj* gobj)
{
    PAD_STACK(8);
    RETURN_IF(ftCo_Attack100_CheckInput(gobj));
    RETURN_IF(ftCo_Catch_CheckInput(gobj));
    RETURN_IF(ftCo_AttackHi4_CheckInputNoD0(gobj));
    ftCo_KneeBend_Check_ShortHop(gobj);
}

void ftCo_KneeBend_Phys(Fighter_GObj* gobj)
{
    ft_80084F3C(gobj);
}

void ftCo_KneeBend_Coll(Fighter_GObj* gobj)
{
    ft_80083F88(gobj);
}
