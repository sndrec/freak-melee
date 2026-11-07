#include "native_controller_calibration.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

void nativeControllerCalibration_Reset(NativeControllerCalibration* capture)
{
    memset(capture, 0, sizeof(*capture));
    for (unsigned axis = 0; axis < 6; ++axis) {
        capture->low[axis] = capture->rest_low[axis] = 32767;
        capture->high[axis] = capture->rest_high[axis] = -32767;
    }
}
void nativeControllerCalibration_Sample(NativeControllerCalibration* capture,
                                         const int16_t raw[6], bool resting,
                                         const int8_t trigger_click[2])
{
    for (unsigned axis = 0; axis < 6; ++axis) {
        if (raw[axis] < capture->low[axis]) capture->low[axis] = raw[axis];
        if (raw[axis] > capture->high[axis]) capture->high[axis] = raw[axis];
        if (resting) {
            if (raw[axis] < capture->rest_low[axis]) capture->rest_low[axis] = raw[axis];
            if (raw[axis] > capture->rest_high[axis]) capture->rest_high[axis] = raw[axis];
        }
    }
    // A real digital click is supplied separately from synthesized PAD buttons.
    // Capture only its rising edge; full click travel and release cannot widen it.
    for (unsigned i = 0; !resting && trigger_click && i < 2; ++i) {
        bool pressed = trigger_click[i] > 0;
        capture->trigger_click_supported[i] = trigger_click[i] >= 0;
        if (pressed && !capture->trigger_pressed[i] && capture->trigger_sampled[i] &&
            !capture->trigger_captured[i] &&
            capture->trigger_previous[i] - capture->rest_high[4 + i] >= 8192) {
            capture->trigger_maximum[i] = capture->trigger_previous[i];
            capture->trigger_captured[i] = true;
        }
        capture->trigger_previous[i] = raw[4 + i];
        capture->trigger_sampled[i] = true;
        capture->trigger_pressed[i] = pressed;
    }
    if (resting) ++capture->samples;
    else if (capture->samples) {
        for (unsigned stick = 0; stick < 2; ++stick) {
            unsigned axis = stick * 2;
            float x = raw[axis] - ((int)capture->rest_low[axis] + capture->rest_high[axis]) * .5f;
            float y = raw[axis + 1] - ((int)capture->rest_low[axis + 1] + capture->rest_high[axis + 1]) * .5f;
            unsigned bin = (unsigned)((atan2f(y, x) + 3.14159265f) * (32.f / 6.2831853f)) % 32;
            float distance = sqrtf(x*x + y*y);
            if (distance > capture->stick_gate[stick][bin]) capture->stick_gate[stick][bin] = distance;
        }
    }
}
bool nativeControllerCalibration_Finish(const NativeControllerCalibration* capture,
                                         NativeProfileCalibration* result)
{
    if (capture->samples < 30) return false;
    NativeProfileCalibration c = *result;
    c.measured = true;
    int main_noise = 0, c_noise = 0;
    for (unsigned axis = 0; axis < 6; ++axis) {
        int low = capture->rest_low[axis], high = capture->rest_high[axis];
        if (high - low > 3000) return false;
        c.center[axis] = (int16_t)((low + high) / 2);
        c.minimum[axis] = capture->low[axis];
        c.maximum[axis] = capture->high[axis];
        if (axis >= 4 && capture->trigger_click_supported[axis - 4]) {
            if (!capture->trigger_captured[axis - 4]) return false;
            c.maximum[axis] = capture->trigger_maximum[axis - 4];
        }
        int negative = c.center[axis] - c.minimum[axis];
        int positive = c.maximum[axis] - c.center[axis];
        // Reject incomplete sweeps rather than amplifying noise into full travel.
        if (positive < 8192 || (axis < 4 && negative < 8192)) return false;
        if (axis < 4) {
            int span = negative < positive ? negative : positive;
            int noise = ((high - low) / 2 + 256) * 32767 / span;
            int* maximum = axis < 2 ? &main_noise : &c_noise;
            if (noise > *maximum) *maximum = noise;
        }
    }
    c.main_stick_dead_zone = (uint16_t)(main_noise < 512 ? 512 : main_noise);
    c.c_stick_dead_zone = (uint16_t)(c_noise < 512 ? 512 : c_noise);
    c.left_trigger_activation = c.right_trigger_activation = 31150;
    c.use_dead_zones = true;
    *result = c;
    return true;
}
int16_t nativeControllerCalibration_Normalize(const NativeProfileCalibration* c,
                                               unsigned axis, int raw)
{
    if (!c->measured || axis >= 6) return (int16_t)raw;
    int delta = raw - c->center[axis];
    int span = delta < 0 && axis < 4 ? c->center[axis] - c->minimum[axis]
                                    : c->maximum[axis] - c->center[axis];
    if (span < 4096) return 0;
    int value = delta * 32767 / span;
    if (value > 32767) value = 32767;
    if (value < (axis < 4 ? -32767 : 0)) value = axis < 4 ? -32767 : 0;
    return (int16_t)value;
}
