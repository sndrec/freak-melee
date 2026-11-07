#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <melee/gm/native_profile_input.h>
#include <sysdolphin/baselib/controller.h>
#include <melee/gm/native_controller_calibration.h>

static bool native_buttons[PAD_MAX_CONTROLLERS][32];
static int16_t native_axes[PAD_MAX_CONTROLLERS][8];
static PADDeadZones dead_zones[PAD_MAX_CONTROLLERS];
static PADDeadZones port_dead_zones[PAD_MAX_CONTROLLERS];
static bool has_port_dead_zones[PAD_MAX_CONTROLLERS];
static void require(bool condition, const char* message);

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
s16 PADGetMappedAxisState(u32 port, u32 axis)
{
    return PADGetNativeAxisState(port, (s32)axis);
}

void PADSetPortDeadZones(u32 port, const PADDeadZones* calibration)
{
    require(port < PAD_MAX_CONTROLLERS, "configure a valid physical port");
    has_port_dead_zones[port] = calibration != NULL;
    if (calibration) port_dead_zones[port] = *calibration;
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

static void default_equivalence(void)
{
    NativeProfile profile;
    nativeProfile_Default(&profile, "Default equivalence");
    memset(native_buttons, 0, sizeof(native_buttons));
    memset(native_axes, 0, sizeof(native_axes));
    for (unsigned port = 0; port < PAD_MAX_CONTROLLERS; ++port)
        require(nativeProfileInput_Assign(port, &profile), "assign default profiles on four ports");
    for (unsigned mask = 0; mask <= UINT16_MAX; ++mask) {
        PADStatus pads[4] = { 0 }, expected[4];
        for (unsigned port = 0; port < 4; ++port) {
            pads[port] = (PADStatus){
                .button = (uint16_t)(mask ^ (port * 0x1111)),
                .stickX = (int8_t)(mask + port), .stickY = (int8_t)(mask >> 8),
                .substickX = (int8_t)~mask, .substickY = (int8_t)(mask >> 4),
                .triggerLeft = (uint8_t)mask, .triggerRight = (uint8_t)(mask >> 8),
                .analogA = (uint8_t)(mask >> 3), .analogB = (uint8_t)(mask >> 5),
                .extButton = (mask * 37u) & UINT32_C(0x1fffffff),
                .err = PAD_ERR_NONE,
            };
        }
        memcpy(expected, pads, sizeof(pads));
        nativeProfileInput_Apply(pads);
        require(!memcmp(pads, expected, sizeof(pads)),
                "default profiles preserve canonical buttons, axes, triggers and other extensions");
    }
    nativeProfileInput_ClearAll();
}

static void combined_shield(void)
{
    static const int16_t values[] = { 0, 16384, 32767 };
    for (unsigned defaults = 0; defaults < 2; ++defaults)
    for (unsigned emulate = 0; emulate < 2; ++emulate) {
        NativeProfile profile;
        nativeProfile_Default(&profile, "Combined shield");
        profile.actions[NativeProfileAction_Shield].count = defaults ? 2 : 1;
        profile.actions[NativeProfileAction_Shield].bindings[0] = button(7);
        if (defaults) profile.actions[NativeProfileAction_Shield].bindings[1] =
            (NativeProfileBinding){ .native_input = -1, .type = NativeProfileBinding_Default };
        profile.axes[NativeProfileAxis_LeftTrigger] =
            (NativeProfileAxisBinding){ .native_axis = 4, .direction = 1, .assigned = true };
        profile.axes[NativeProfileAxis_RightTrigger] =
            (NativeProfileAxisBinding){ .native_axis = 5, .direction = 1, .assigned = true };
        profile.calibration.emulate_digital_triggers = emulate != 0;
        require(nativeProfileInput_Assign(3, &profile), "assign mixed analog/digital shielding");
        for (unsigned left = 0; left < 3; ++left)
        for (unsigned right = 0; right < 3; ++right)
        for (unsigned pressed = 0; pressed < 2; ++pressed) {
            native_axes[3][4] = values[left];native_axes[3][5] = values[right];
            native_buttons[3][7] = pressed != 0;
            PADStatus pads[4] = { 0 };
            pads[3].err = PAD_ERR_NONE;
            nativeProfileInput_Apply(pads);
            require(pads[3].triggerLeft == (pressed ? 255 : values[left] / 128),
                    "a custom digital Shield wins over its mapped analog trigger");
            require(pads[3].triggerRight == values[right] / 128,
                    "the other analog shield retains its current strength");
            require(((pads[3].button & PAD_TRIGGER_L) != 0) ==
                        (pressed || (emulate && values[left] >= profile.calibration.left_trigger_activation)),
                    "digital shielding merges with analog click emulation");
            require(((pads[3].button & PAD_TRIGGER_R) != 0) ==
                        (emulate && values[right] >= profile.calibration.right_trigger_activation),
                    "the right trigger retains independent click emulation");
        }
        nativeProfileInput_ClearAll();
    }
}

int main(void)
{
    NativeControllerCalibration capture;
    NativeProfile measured;
    nativeProfile_Default(&measured, "Measured controller");
    nativeControllerCalibration_Reset(&capture);
    const int16_t center[6] = { 400, -300, 200, 100, 500, 300 };
    int16_t raw[6]; memcpy(raw, center, sizeof(raw));
    for (unsigned i = 0; i < 60; ++i) nativeControllerCalibration_Sample(&capture, raw, true);
    require(!nativeControllerCalibration_Finish(&capture, &measured.calibration), "reject an unmoved controller");
    for (unsigned axis = 0; axis < 6; ++axis) {
        raw[axis] = center[axis] + 24000;
        nativeControllerCalibration_Sample(&capture, raw, false);
        if (axis < 4) { raw[axis] = center[axis] - 20000; nativeControllerCalibration_Sample(&capture, raw, false); }
        raw[axis] = center[axis];
    }
    require(nativeControllerCalibration_Finish(&capture, &measured.calibration), "accept a complete stable capture");
    require(nativeProfile_Validate(&measured), "measured ranges form a valid saved profile");
    for (unsigned axis = 0; axis < 6; ++axis) {
        require(nativeControllerCalibration_Normalize(&measured.calibration, axis, center[axis]) == 0, "measured rest is exactly neutral");
        require(nativeControllerCalibration_Normalize(&measured.calibration, axis, center[axis] + 12000) == 16383, "half measured travel remains half travel");
        require(nativeControllerCalibration_Normalize(&measured.calibration, axis, center[axis] + 25000) == 32767, "overtravel clamps");
    }
    require(nativeProfileInput_Assign(0, &measured), "apply measured calibration to default axes");
    for (unsigned axis = 0; axis < 6; ++axis) native_axes[0][axis] = center[axis];
    PADStatus calibrated[4] = { 0 };
    nativeProfileInput_Apply(calibrated);
    require(!calibrated[0].stickX && !calibrated[0].stickY && !calibrated[0].triggerLeft, "default axes use measured rest before conversion");
    native_axes[0][0] += 12000; native_axes[0][1] -= 10000;
    native_axes[0][4] += 24000;
    nativeProfileInput_Apply(calibrated);
    require(calibrated[0].stickX == 39 && calibrated[0].stickY == -39 && calibrated[0].triggerLeft == 140,
            "measured axes map to Melee stick and trigger scales");
    measured.axes[NativeProfileAxis_MoveX] = (NativeProfileAxisBinding){ 7, -1, true };
    require(nativeProfileInput_Assign(0, &measured), "apply a calibrated remapped axis");
    native_axes[0][7] = -(center[0] + 24000);
    nativeProfileInput_Apply(calibrated);
    require(calibrated[0].stickX == 80, "remapped and default axes share measured normalization");
    nativeProfileInput_ClearAll(); memset(native_axes, 0, sizeof(native_axes));
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
                has_port_dead_zones[0] && port_dead_zones[0].useDeadzones &&
                port_dead_zones[0].stickDeadZone == 8000 &&
                port_dead_zones[0].leftTriggerActivationZone == 31150 &&
                dead_zones[0].stickDeadZone == 1234,
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
                !has_port_dead_zones[0] &&
                !dead_zones[0].useDeadzones &&
                dead_zones[0].stickDeadZone == 1234 &&
                dead_zones[0].rightTriggerActivationZone == 4567 &&
                !nativeProfileInput_Assign(PAD_MAX_CONTROLLERS, &profile),
            "clearing restores canonical samples and controller calibration");
    nativeProfileInput_ClearAll();
    default_equivalence();
    combined_shield();
    puts("Native profile actions combine physical controls before rollback");
    return 0;
}
