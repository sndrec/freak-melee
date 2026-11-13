#include "native_feedback_input.h"
#include "native_profile_input.h"
#include <string.h>

// Host UI state stays outside rollback snapshots. It only changes newly
// sampled physical input, before the replay recorder and netplay mapping.
static bool open, resimulating, closing, releasing;
static u16 held[4];
static u32 extended[4];
static bool left[4], right[4], analog_a[4], analog_b[4];

void nativeFeedbackInput_SetOpen(bool value)
{
    if (open && !value) closing = true;
    open = value;
}
void nativeFeedbackInput_SetResimulating(bool value) { resimulating = value; }
void nativeFeedbackInput_Filter(PADStatus pads[4])
{
    if (resimulating || (!open && !closing && !releasing)) return;
    bool any = false;
    for (unsigned port = 0; port < 4; ++port) {
        PADStatus* pad = &pads[port];
        if (open || closing) {
            held[port] = pad->button;
            extended[port] = pad->extButton & NATIVE_PROFILE_EXT_SHORT_HOP;
            left[port] = pad->triggerLeft != 0;
            right[port] = pad->triggerRight != 0;
            analog_a[port] = pad->analogA != 0;
            analog_b[port] = pad->analogB != 0;
            const s8 error = pad->err;
            const u32 preferences = pad->extButton & (NATIVE_PROFILE_EXT_TAP_JUMP_DISABLED |
                NATIVE_PROFILE_EXT_CSTICK_TILT | NATIVE_PROFILE_EXT_ACTION_BUFFER_MASK);
            memset(pad, 0, sizeof(*pad));
            pad->err = error; pad->extButton = preferences;
        } else {
            held[port] &= pad->button;
            extended[port] &= pad->extButton;
            left[port] = left[port] && pad->triggerLeft != 0;
            right[port] = right[port] && pad->triggerRight != 0;
            analog_a[port] = analog_a[port] && pad->analogA != 0;
            analog_b[port] = analog_b[port] && pad->analogB != 0;
            pad->button &= ~held[port]; pad->extButton &= ~extended[port];
            if (left[port]) pad->triggerLeft = 0;
            if (right[port]) pad->triggerRight = 0;
            if (analog_a[port]) pad->analogA = 0;
            if (analog_b[port]) pad->analogB = 0;
        }
        any |= held[port] != 0 || extended[port] != 0 || left[port] || right[port] || analog_a[port] || analog_b[port];
    }
    closing = false; releasing = any;
}
