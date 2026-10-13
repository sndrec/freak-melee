#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <melee/gm/native_profile_input.h>
#include <sysdolphin/baselib/controller.h>

static bool native_buttons[PAD_MAX_CONTROLLERS][32];
static int16_t native_axes[PAD_MAX_CONTROLLERS][8];
static PADDeadZones dead_zones[PAD_MAX_CONTROLLERS];

BOOL PADGetNativeButtonState(u32 port, s32 button)
{
    return port < PAD_MAX_CONTROLLERS && button >= 0 && button < 32 &&
           native_buttons[port][button];
}

s16 PADGetNativeAxisState(u32 port, s32 axis)
{
    return port < PAD_MAX_CONTROLLERS && axis >= 0 && axis < 8
               ? native_axes[port][axis]
               : 0;
}

PADDeadZones* PADGetDeadZones(u32 port)
{
    return port < PAD_MAX_CONTROLLERS ? &dead_zones[port] : NULL;
}

static void require(bool condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

static NativeProfileBinding button(int16_t native_button)
{
    return (NativeProfileBinding){
        .native_input = native_button,
        .type = NativeProfileBinding_Button,
    };
}

int main(void)
{
    require(HSD_PadProfileTapJumpEnabled(0) &&
                !HSD_PadProfileTapJumpEnabled(
                    HSD_PAD_PROFILE_TAP_JUMP_DISABLED) &&
                HSD_PadProfileHasJumpButton(HSD_PAD_X) &&
                HSD_PadProfileHasJumpButton(HSD_PAD_PROFILE_SHORT_HOP) &&
                HSD_PadProfileForcesShortHop(
                    HSD_PAD_PROFILE_SHORT_HOP) &&
                HSD_PadProfileUsesTiltStick(
                    HSD_PAD_PROFILE_CSTICK_TILT),
            "fighter semantics preserve defaults and expose profile policy");

    NativeProfile profile;
    nativeProfile_Default(&profile, "Alice");
    dead_zones[0] = (PADDeadZones){
        .emulateTriggers = false,
        .useDeadzones = false,
        .stickDeadZone = 1234,
        .substickDeadZone = 2345,
        .leftTriggerActivationZone = 3456,
        .rightTriggerActivationZone = 4567,
    };
    require(nativeProfileInput_Assign(0, &profile) &&
                dead_zones[0].useDeadzones &&
                dead_zones[0].stickDeadZone == 8000 &&
                dead_zones[0].leftTriggerActivationZone == 31150,
            "assign profile-owned calibration to the controller port");
    PADStatus pads[4] = {
        { .button = PAD_BUTTON_A | PAD_BUTTON_B | PAD_BUTTON_Y,
          .err = PAD_ERR_NONE },
    };
    nativeProfileInput_Apply(pads);
    require((pads[0].button &
             (PAD_BUTTON_A | PAD_BUTTON_B | PAD_BUTTON_Y)) ==
                (PAD_BUTTON_A | PAD_BUTTON_B | PAD_BUTTON_Y),
            "default action bindings preserve canonical controller input");

    profile.actions[NativeProfileAction_Attack] =
        (NativeProfileActionBindings){
            .bindings = { button(4), button(5) },
            .count = 2,
        };
    profile.actions[NativeProfileAction_Jump] =
        (NativeProfileActionBindings){
            .bindings = { button(6) },
            .count = 1,
        };
    profile.actions[NativeProfileAction_Shield] =
        (NativeProfileActionBindings){
            .bindings = {
                { .native_input = 1,
                  .type = NativeProfileBinding_AxisPositive },
            },
            .count = 1,
        };
    profile.actions[NativeProfileAction_ShortHop] =
        (NativeProfileActionBindings){
            .bindings = { button(7) },
            .count = 1,
        };
    profile.tap_jump = false;
    profile.c_stick = NativeProfileCStick_Tilt;
    profile.calibration.main_stick_dead_zone = 4000;
    profile.calibration.c_stick_dead_zone = 5000;
    profile.axes[NativeProfileAxis_MoveX] = (NativeProfileAxisBinding){
        .native_axis = 2,
        .direction = 1,
        .assigned = true,
    };
    profile.axes[NativeProfileAxis_MoveY] = (NativeProfileAxisBinding){
        .native_axis = 3,
        .direction = -1,
        .assigned = true,
    };
    profile.axes[NativeProfileAxis_CStickX] = (NativeProfileAxisBinding){
        .native_axis = 4,
        .direction = 1,
        .assigned = true,
    };
    profile.axes[NativeProfileAxis_RightTrigger] =
        (NativeProfileAxisBinding){
            .native_axis = 5,
            .direction = 1,
            .assigned = true,
        };
    require(nativeProfileInput_Assign(0, &profile),
            "replace the port profile at an explicit transition");

    native_buttons[0][5] = true;
    native_buttons[0][7] = true;
    native_axes[0][1] = 20000;
    native_axes[0][2] = 30000;
    native_axes[0][3] = 30000;
    native_axes[0][4] = 1000;
    native_axes[0][5] = 32767;
    pads[0] = (PADStatus){
        .button = PAD_BUTTON_Y | PAD_TRIGGER_R,
        .triggerRight = 50,
        .err = PAD_ERR_NONE,
    };
    nativeProfileInput_Apply(pads);
    require((pads[0].button & PAD_BUTTON_A) != 0 &&
                (pads[0].button & (PAD_BUTTON_X | PAD_BUTTON_Y)) == 0 &&
                (pads[0].button & PAD_TRIGGER_L) != 0 &&
                pads[0].triggerLeft == UINT8_MAX &&
                (pads[0].extButton & NATIVE_PROFILE_EXT_SHORT_HOP) != 0 &&
                (pads[0].extButton &
                 NATIVE_PROFILE_EXT_TAP_JUMP_DISABLED) != 0 &&
                (pads[0].extButton & NATIVE_PROFILE_EXT_CSTICK_TILT) != 0,
            "combine several physical sources into semantic action outputs");
    require(pads[0].stickX == 117 && pads[0].stickY == -117 &&
                pads[0].substickX == 0 && pads[0].triggerRight == 255 &&
                (pads[0].button & PAD_TRIGGER_R) != 0,
            "apply assigned axes, inversion, dead zones, and trigger threshold");

    native_axes[0][5] = 0;
    pads[0] = (PADStatus){
        .button = PAD_TRIGGER_R,
        .triggerRight = 200,
        .err = PAD_ERR_NONE,
    };
    nativeProfileInput_Apply(pads);
    require(pads[0].triggerRight == 0 &&
                (pads[0].button & PAD_TRIGGER_R) == 0,
            "released SDL trigger must not produce a half-pressed shield");
    native_axes[0][5] = 16384;
    nativeProfileInput_Apply(pads);
    require(pads[0].triggerRight == 128 &&
                (pads[0].button & PAD_TRIGGER_R) == 0,
            "half-pressed SDL trigger preserves analog shield strength");

    memset(native_buttons, 0, sizeof(native_buttons));
    memset(native_axes, 0, sizeof(native_axes));
    pads[0] = (PADStatus){
        .button = PAD_BUTTON_A | PAD_BUTTON_X | PAD_TRIGGER_L,
        .triggerLeft = 200,
        .err = PAD_ERR_NONE,
    };
    nativeProfileInput_Apply(pads);
    require((pads[0].button &
             (PAD_BUTTON_A | PAD_BUTTON_X | PAD_TRIGGER_L)) == 0 &&
                pads[0].triggerLeft == 0 &&
                (pads[0].extButton & NATIVE_PROFILE_EXT_SHORT_HOP) == 0,
            "clear inactive actions while retaining per-frame profile policy");

    profile.tap_jump = true;
    profile.c_stick = NativeProfileCStick_Smash;
    require(nativeProfileInput_Assign(0, &profile),
            "replace per-frame profile policy");
    nativeProfileInput_Apply(pads);
    require((pads[0].extButton &
             (NATIVE_PROFILE_EXT_TAP_JUMP_DISABLED |
              NATIVE_PROFILE_EXT_CSTICK_TILT)) == 0,
            "clear policy bits when the replacement profile uses defaults");

    nativeProfileInput_Clear(0);
    pads[0] = (PADStatus){ .button = PAD_BUTTON_A, .err = PAD_ERR_NONE };
    nativeProfileInput_Apply(pads);
    require(nativeProfileInput_Assigned(0) == NULL &&
                pads[0].button == PAD_BUTTON_A &&
                !dead_zones[0].useDeadzones &&
                dead_zones[0].stickDeadZone == 1234 &&
                dead_zones[0].rightTriggerActivationZone == 4567 &&
                !nativeProfileInput_Assign(PAD_MAX_CONTROLLERS, &profile),
            "clearing restores canonical samples and controller calibration");
    nativeProfileInput_ClearAll();
    puts("Native profile actions combine physical controls before rollback");
    return 0;
}
