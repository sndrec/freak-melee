#include "native_profile_input.h"
#include "native_controller_calibration.h"

#ifdef MELEE_NATIVE

#include <stdlib.h>
#include <string.h>

typedef struct NativeProfileInputSlot {
    NativeProfile profile;
    bool assigned;
} NativeProfileInputSlot;

static NativeProfileInputSlot slots[PAD_MAX_CONTROLLERS];

static bool binding_pressed(uint32_t port,
                            const NativeProfileBinding* binding)
{
    switch (binding->type) {
    case NativeProfileBinding_Default:
        return false;
    case NativeProfileBinding_Button:
        return PADGetNativeButtonState(port, binding->native_input);
    case NativeProfileBinding_AxisPositive:
        return PADGetNativeAxisState(port, binding->native_input) >= 16384;
    case NativeProfileBinding_AxisNegative:
        return PADGetNativeAxisState(port, binding->native_input) <= -16384;
    default:
        return false;
    }
}

static bool action_has_default(const NativeProfileActionBindings* bindings)
{
    for (uint32_t i = 0; i < bindings->count; ++i) {
        if (bindings->bindings[i].type == NativeProfileBinding_Default) {
            return true;
        }
    }
    return false;
}

static bool action_pressed(uint32_t port,
                           const NativeProfileActionBindings* bindings)
{
    for (uint32_t i = 0; i < bindings->count; ++i) {
        if (binding_pressed(port, &bindings->bindings[i])) {
            return true;
        }
    }
    return false;
}

static bool apply_action(uint32_t port, PADStatus* pad,
                         NativeProfileAction action, uint16_t button_mask)
{
    const NativeProfileActionBindings* bindings =
        &slots[port].profile.actions[action];
    if (action_has_default(bindings)) {
        if (!action_pressed(port, bindings)) {
            return false;
        }
    } else {
        pad->button &= (uint16_t) ~button_mask;
        if (action == NativeProfileAction_Shield) {
            pad->triggerLeft = 0;
            pad->triggerRight = 0;
        }
        if (!action_pressed(port, bindings)) {
            return false;
        }
    }
    if (action == NativeProfileAction_Jump) {
        pad->button |= PAD_BUTTON_X;
    } else if (action == NativeProfileAction_Shield) {
        pad->button |= PAD_TRIGGER_L;
        pad->triggerLeft = UINT8_MAX;
    } else {
        pad->button |= button_mask;
    }
    return true;
}

static int8_t scaled_stick_axis(int16_t raw, int8_t direction,
                                uint16_t dead_zone, bool use_dead_zone)
{
    int32_t value = (int32_t) raw * direction;
    if (use_dead_zone && abs(value) <= dead_zone) {
        return 0;
    }
    value /= 256;
    if (value < -127) {
        value = -127;
    } else if (value > 127) {
        value = 127;
    }
    return (int8_t) value;
}

static uint8_t scaled_trigger_axis(int16_t raw, int8_t direction)
{
    int32_t value = (int32_t) raw * direction;
    if (value < 0) {
        value = 0;
    } else if (value > 32767) {
        value = 32767;
    }
    return (uint8_t) (value / 128);
}

int16_t nativeProfileInput_RawAxis(unsigned port, const NativeProfile* profile, unsigned axis)
{
    if (port >= 4 || axis >= NativeProfileAxis_Count || !profile) return 0;
    const NativeProfileAxisBinding* binding = &profile->axes[axis];
    if (!binding->assigned) return PADGetMappedAxisState(port, axis);
    int value = PADGetNativeAxisState(port, binding->native_axis) * binding->direction;
    return (int16_t)(value > 32767 ? 32767 : value < -32767 ? -32767 : value);
}

static void apply_axis(uint32_t port, PADStatus* pad, NativeProfileAxis axis)
{
    const NativeProfile* profile = &slots[port].profile;
    const NativeProfileAxisBinding* binding = &profile->axes[axis];
    if (profile->calibration.measured) {
        const NativeProfileCalibration* c = &profile->calibration;
        int value = nativeControllerCalibration_Normalize(c, axis,
            nativeProfileInput_RawAxis(port, profile, axis));
        if (axis < 4) {
            unsigned zone = axis < 2 ? c->main_stick_dead_zone : c->c_stick_dead_zone;
            if (c->use_dead_zones && abs(value) <= (int)zone) value = 0;
            // Melee's clamp and scale use 80 units at full stick travel.
            int8_t converted = (int8_t)(value * 80 / 32767);
            switch (axis) {
            case NativeProfileAxis_MoveX: pad->stickX = converted; break;
            case NativeProfileAxis_MoveY: pad->stickY = converted; break;
            case NativeProfileAxis_CStickX: pad->substickX = converted; break;
            default: pad->substickY = converted; break;
            }
        } else {
            const unsigned mask = axis == 4 ? PAD_TRIGGER_L : PAD_TRIGGER_R;
            const unsigned activation = axis == 4 ? c->left_trigger_activation : c->right_trigger_activation;
            uint8_t converted = (uint8_t)(value * 140 / 32767);
            if (c->emulate_digital_triggers && (unsigned)value >= activation) pad->button |= mask;
            if (axis == 4) pad->triggerLeft = converted;
            else pad->triggerRight = converted;
        }
        return;
    }
    if (!binding->assigned) {
        return;
    }
    int16_t raw = PADGetNativeAxisState(port, binding->native_axis);
    switch (axis) {
    case NativeProfileAxis_MoveX:
        pad->stickX = scaled_stick_axis(
            raw, binding->direction,
            profile->calibration.main_stick_dead_zone,
            profile->calibration.use_dead_zones);
        break;
    case NativeProfileAxis_MoveY:
        pad->stickY = scaled_stick_axis(
            raw, binding->direction,
            profile->calibration.main_stick_dead_zone,
            profile->calibration.use_dead_zones);
        break;
    case NativeProfileAxis_CStickX:
        pad->substickX = scaled_stick_axis(
            raw, binding->direction,
            profile->calibration.c_stick_dead_zone,
            profile->calibration.use_dead_zones);
        break;
    case NativeProfileAxis_CStickY:
        pad->substickY = scaled_stick_axis(
            raw, binding->direction,
            profile->calibration.c_stick_dead_zone,
            profile->calibration.use_dead_zones);
        break;
    case NativeProfileAxis_LeftTrigger:
        pad->triggerLeft = scaled_trigger_axis(raw, binding->direction);
        pad->button &= (uint16_t) ~PAD_TRIGGER_L;
        if (profile->calibration.emulate_digital_triggers &&
            (int32_t) raw * binding->direction >=
                profile->calibration.left_trigger_activation)
        {
            pad->button |= PAD_TRIGGER_L;
        }
        break;
    case NativeProfileAxis_RightTrigger:
        pad->triggerRight = scaled_trigger_axis(raw, binding->direction);
        pad->button &= (uint16_t) ~PAD_TRIGGER_R;
        if (profile->calibration.emulate_digital_triggers &&
            (int32_t) raw * binding->direction >=
                profile->calibration.right_trigger_activation)
        {
            pad->button |= PAD_TRIGGER_R;
        }
        break;
    default:
        break;
    }
}

bool nativeProfileInput_Assign(uint32_t port, const NativeProfile* profile)
{
    if (port >= PAD_MAX_CONTROLLERS || !nativeProfile_Validate(profile)) {
        return false;
    }
    slots[port].profile = *profile;
    slots[port].assigned = true;
    const PADDeadZones dead_zones = {
        .emulateTriggers = profile->calibration.emulate_digital_triggers,
        .useDeadzones = profile->calibration.use_dead_zones,
        .stickDeadZone = profile->calibration.main_stick_dead_zone,
        .substickDeadZone = profile->calibration.c_stick_dead_zone,
        .leftTriggerActivationZone =
            profile->calibration.left_trigger_activation,
        .rightTriggerActivationZone =
            profile->calibration.right_trigger_activation,
    };
    PADSetPortDeadZones(port, &dead_zones);
    return true;
}

void nativeProfileInput_Clear(uint32_t port)
{
    if (port < PAD_MAX_CONTROLLERS) {
        PADSetPortDeadZones(port, NULL);
        memset(&slots[port], 0, sizeof(slots[port]));
    }
}

void nativeProfileInput_ClearAll(void)
{
    for (uint32_t port = 0; port < PAD_MAX_CONTROLLERS; ++port) {
        nativeProfileInput_Clear(port);
    }
}

const NativeProfile* nativeProfileInput_Assigned(uint32_t port)
{
    return port < PAD_MAX_CONTROLLERS && slots[port].assigned
               ? &slots[port].profile
               : NULL;
}

void nativeProfileInput_Apply(PADStatus pads[4])
{
    if (pads == NULL) {
        return;
    }
    for (uint32_t port = 0; port < PAD_MAX_CONTROLLERS; ++port) {
        if (!slots[port].assigned || pads[port].err != PAD_ERR_NONE) {
            continue;
        }
        apply_action(port, &pads[port], NativeProfileAction_Attack,
                     PAD_BUTTON_A);
        apply_action(port, &pads[port], NativeProfileAction_Special,
                     PAD_BUTTON_B);
        apply_action(port, &pads[port], NativeProfileAction_Jump,
                     PAD_BUTTON_X | PAD_BUTTON_Y);
        const bool digital_shield =
            apply_action(port, &pads[port], NativeProfileAction_Shield,
                         PAD_TRIGGER_L | PAD_TRIGGER_R);
        apply_action(port, &pads[port], NativeProfileAction_Grab,
                     PAD_TRIGGER_Z);
        const NativeProfileActionBindings* short_hop =
            &slots[port].profile.actions[NativeProfileAction_ShortHop];
        if (action_pressed(port, short_hop)) {
            pads[port].extButton |= NATIVE_PROFILE_EXT_SHORT_HOP;
        } else {
            pads[port].extButton &= ~NATIVE_PROFILE_EXT_SHORT_HOP;
        }
        if (slots[port].profile.tap_jump) {
            pads[port].extButton &= ~NATIVE_PROFILE_EXT_TAP_JUMP_DISABLED;
        } else {
            pads[port].extButton |= NATIVE_PROFILE_EXT_TAP_JUMP_DISABLED;
        }
        if (slots[port].profile.c_stick == NativeProfileCStick_Tilt) {
            pads[port].extButton |= NATIVE_PROFILE_EXT_CSTICK_TILT;
        } else {
            pads[port].extButton &= ~NATIVE_PROFILE_EXT_CSTICK_TILT;
        }
        for (uint32_t axis = 0; axis < NativeProfileAxis_Count; ++axis) {
            apply_axis(port, &pads[port], (NativeProfileAxis) axis);
        }
        // Explicit digital Shield requests full strength even when a mapped
        // analog trigger is released or partly pressed in the same sample.
        if (digital_shield) {
            pads[port].button |= PAD_TRIGGER_L;
            pads[port].triggerLeft = UINT8_MAX;
        }
    }
}

#endif
