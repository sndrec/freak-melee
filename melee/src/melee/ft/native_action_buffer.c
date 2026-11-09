#include "native_action_buffer.h"

#include <math.h>
#include <melee/ft/fighter.h>
#include <melee/ft/kinds/ftCommon/ftCo_Attack100.h>
#include <melee/ft/kinds/ftCommon/ftCo_SpecialS.h>
#include <melee/ft/types.h>

enum {
    ActionButtons = HSD_PAD_A | HSD_PAD_B | HSD_PAD_XY | HSD_PAD_Z |
                    HSD_PAD_L | HSD_PAD_R | HSD_PAD_DPADUP | HSD_PAD_DPADDOWN |
                    HSD_PAD_PROFILE_SHORT_HOP,
};

static int direction(float value, float threshold)
{
    if (value >= threshold) {
        return 1;
    }
    return value <= -threshold ? -1 : 0;
}

void nativeActionBuffer_Record(Fighter* fp, unsigned frames)
{
    NativeActionBuffer* pending = &fp->action_buffer;
    if (frames == 0) {
        pending->direction.frames = pending->movement.frames = 0;
        return;
    }
    if (pending->direction.frames) {
        --pending->direction.frames;
    }
    if (pending->movement.frames) {
        --pending->movement.frames;
    }

    const Vec2 stick = fp->input.lstick[0];
    const Vec2 last = fp->input.lstick[1];
    const NativeActionStick now = {
        stick, fp->x670_timer_lstick_tilt_x, fp->x671_timer_lstick_tilt_y,
        frames + 1,
    };
    const int walk = direction(stick.x, p_ftCommonData->walk_stick_threshold);
    const float dash_threshold = p_ftCommonData->dash_smash_stick_threshold;
    const int dash_direction = direction(stick.x, dash_threshold);
    // A stick can leave its dead zone one sample before it reaches dash strength.
    const bool dash = fp->x670_timer_lstick_tilt_x == 0 ||
        (dash_direction && dash_direction != direction(last.x, dash_threshold));
    const bool vertical = fp->x671_timer_lstick_tilt_y == 0 ||
        (stick.y < -p_ftCommonData->x90 && last.y >= -p_ftCommonData->x90);
    if (dash || vertical ||
        (walk && walk != direction(last.x, p_ftCommonData->walk_stick_threshold)))
    {
        pending->movement = now;
    }

    // During hitlag the engine accumulates presses. Capture only new edges.
    const HSD_Pad held = fp->input.held_buttons[0];
    const HSD_Pad pressed = held & ~fp->input.held_buttons[1];
    const Vec2 cstick = fp->input.cstick[0];
    const Vec2 last_cstick = fp->input.cstick[1];
    const float cstick_x_threshold = p_ftCommonData->xDC;
    const float cstick_y_threshold = p_ftCommonData->xE0;
    const bool cstick_edge =
        (fabsf(cstick.x) >= cstick_x_threshold &&
         fabsf(last_cstick.x) < cstick_x_threshold) ||
        (fabsf(cstick.y) >= cstick_y_threshold &&
         fabsf(last_cstick.y) < cstick_y_threshold);
    if ((pressed & ActionButtons) || fp->trigger_analog_timer == 0 || cstick_edge) {
        pending->direction = now;
        // Use a pending direction with a button when the main stick is neutral.
        if (pending->movement.frames && !walk &&
            fabsf(stick.y) < p_ftCommonData->vertical_stick_smash_deadzone)
        {
            pending->direction = pending->movement;
            pending->direction.frames = now.frames;
        }
        pending->cstick = cstick;
        pending->cstick_command = cstick_edge;
        pending->held = held & (ActionButtons | HSD_PAD_LR);
        pending->pressed = pressed & (ActionButtons | HSD_PAD_LR);
        pending->trigger = fp->input.triggers[0];
    }
}

bool nativeActionBuffer_Begin(Fighter* fp, NativeActionInput* live,
                             NativeActionInput* offered)
{
    NativeActionBuffer* pending = &fp->action_buffer;
    if (!pending->direction.frames && !pending->movement.frames) {
        return false;
    }
    *live = (NativeActionInput){
        fp->input.lstick[0], fp->input.cstick[0], fp->input.cstick[1],
        fp->input.pressed_buttons, fp->input.held_buttons[0], fp->input.triggers[0],
        fp->x670_timer_lstick_tilt_x, fp->x671_timer_lstick_tilt_y,
        fp->x686, fp->x687, fp->x688, fp->x689,
    };
    *offered = *live;
    if (pending->direction.frames) {
        // The engine can carry several old presses out of hitlag. Give the
        // latest action the same priority it has during ordinary recovery.
        offered->pressed =
            (live->pressed & ~(ActionButtons | HSD_PAD_LR)) | pending->pressed;
        offered->held |= pending->held;
        if (pending->trigger > offered->trigger) {
            offered->trigger = pending->trigger;
        }
        if (pending->cstick_command) {
            offered->cstick = pending->cstick;
            offered->previous_cstick = (Vec2){ 0 };
        }
    }
    const NativeActionStick* stick = &pending->direction;
    if (!stick->frames ||
        (pending->cstick_command && pending->movement.frames) ||
        (!(pending->pressed & (HSD_PAD_A | HSD_PAD_B | HSD_PAD_Z)) &&
         pending->movement.frames))
    {
        stick = &pending->movement;
    }
    offered->lstick = stick->position;
    offered->tilt_x = stick->tilt_x;
    offered->tilt_y = stick->tilt_y;

    fp->input.lstick[0] = offered->lstick;
    fp->input.cstick[0] = offered->cstick;
    fp->input.cstick[1] = offered->previous_cstick;
    fp->input.pressed_buttons = offered->pressed;
    fp->input.held_buttons[0] = offered->held;
    fp->input.triggers[0] = offered->trigger;
    fp->x670_timer_lstick_tilt_x = offered->tilt_x;
    fp->x671_timer_lstick_tilt_y = offered->tilt_y;
    if (pending->direction.frames) {
        // Grounded special checks read the results of these input tests from
        // timers. Reclassify the offered sample without aging the real timers.
        offered->special_up = !ftCo_800D6928(fp);
        offered->special_down = !ftCo_800D688C(fp);
        offered->special_side = !ftCo_SpecialS_HasInput(fp);
        offered->special_neutral = !ftCo_800D67C4(fp);
        fp->x686 = offered->special_up;
        fp->x687 = offered->special_down;
        fp->x688 = offered->special_side;
        fp->x689 = offered->special_neutral;
    }
    pending->live_stick_x = live->lstick.x;
    pending->checking = true;
    return true;
}

void nativeActionBuffer_End(Fighter* fp, const NativeActionInput* live,
                           const NativeActionInput* offered)
{
    // Undo the temporary input while retaining writes made by the action,
    // such as the stick timer reset when dropping through a platform.
    if (fp->input.lstick[0].x == offered->lstick.x)
        fp->input.lstick[0].x = live->lstick.x;
    if (fp->input.lstick[0].y == offered->lstick.y)
        fp->input.lstick[0].y = live->lstick.y;
    if (fp->input.cstick[0].x == offered->cstick.x)
        fp->input.cstick[0].x = live->cstick.x;
    if (fp->input.cstick[0].y == offered->cstick.y)
        fp->input.cstick[0].y = live->cstick.y;
    if (fp->input.cstick[1].x == offered->previous_cstick.x)
        fp->input.cstick[1].x = live->previous_cstick.x;
    if (fp->input.cstick[1].y == offered->previous_cstick.y)
        fp->input.cstick[1].y = live->previous_cstick.y;
    const HSD_Pad replaced_presses = offered->pressed ^ live->pressed;
    fp->input.pressed_buttons =
        (fp->input.pressed_buttons & ~replaced_presses) |
        (live->pressed & replaced_presses);
    fp->input.held_buttons[0] &= ~(offered->held & ~live->held);
    if (fp->input.triggers[0] == offered->trigger)
        fp->input.triggers[0] = live->trigger;
    if (fp->x670_timer_lstick_tilt_x == offered->tilt_x)
        fp->x670_timer_lstick_tilt_x = live->tilt_x;
    if (fp->x671_timer_lstick_tilt_y == offered->tilt_y)
        fp->x671_timer_lstick_tilt_y = live->tilt_y;
    if (fp->x686 == offered->special_up)
        fp->x686 = live->special_up;
    if (fp->x687 == offered->special_down)
        fp->x687 = live->special_down;
    if (fp->x688 == offered->special_side)
        fp->x688 = live->special_side;
    if (fp->x689 == offered->special_neutral)
        fp->x689 = live->special_neutral;
    fp->action_buffer.checking = false;
    fp->action_buffer.live_stick_x = 0.0f;
}
