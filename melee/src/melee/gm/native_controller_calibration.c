#include "native_controller_calibration.h"
#include <string.h>
#include <stdlib.h>

static int64_t turn(NativeCalibrationPoint a, NativeCalibrationPoint b, NativeCalibrationPoint c)
{
    return (int64_t)(b.x-a.x)*(c.y-a.y) - (int64_t)(b.y-a.y)*(c.x-a.x);
}

static void sample_stick(NativeControllerCalibration* capture, unsigned stick, const int16_t raw[6])
{
    // Keep the convex boundary of real samples. Angular buckets rotate samples
    // away from the input dot and can create artificial dents in the gate.
    NativeCalibrationPoint points[NATIVE_CALIBRATION_GATE_POINTS+1], hull[2*(NATIVE_CALIBRATION_GATE_POINTS+1)];
    unsigned count = capture->stick_gate_count[stick], at = count;
    NativeCalibrationPoint sample = {raw[stick*2], raw[stick*2+1]};
    memcpy(points, capture->stick_gate[stick], count*sizeof(*points));
    points[count++] = sample;
    for (unsigned i = 1; i < count; ++i) {
        NativeCalibrationPoint p = points[i]; unsigned j = i;
        while (j && (points[j-1].x > p.x || (points[j-1].x == p.x && points[j-1].y > p.y))) {
            points[j] = points[j-1]; --j;
        }
        points[j] = p;
    }
    unsigned unique = 0;
    for (unsigned i = 0; i < count; ++i)
        if (!unique || points[i].x != points[unique-1].x || points[i].y != points[unique-1].y)
            points[unique++] = points[i];
    count = unique;
    if (count <= 2) { memcpy(capture->stick_gate[stick], points, count*sizeof(*points)); capture->stick_gate_count[stick] = count; return; }
    unsigned n = 0;
    for (unsigned i = 0; i < count; ++i) {
        while (n >= 2 && turn(hull[n-2], hull[n-1], points[i]) <= 0) --n;
        hull[n++] = points[i];
    }
    unsigned lower = n+1;
    for (int i = (int)count-2; i >= 0; --i) {
        while (n >= lower && turn(hull[n-2], hull[n-1], points[i]) <= 0) --n;
        hull[n++] = points[i];
    }
    --n;
    if (n > NATIVE_CALIBRATION_GATE_POINTS) {
        // Bound capture storage by removing the least significant corner.
        int64_t smallest = INT64_MAX;
        for (unsigned i = 0; i < n; ++i) {
            int64_t area = turn(hull[(i+n-1)%n], hull[i], hull[(i+1)%n]);
            if (area < smallest) { smallest = area; at = i; }
        }
        memmove(hull+at, hull+at+1, (--n-at)*sizeof(*hull));
    }
    memcpy(capture->stick_gate[stick], hull, n*sizeof(*hull));
    capture->stick_gate_count[stick] = n;
}

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
    for (unsigned axis = resting ? 0 : trigger_click ? 4 : 0;
         axis < (resting || trigger_click ? 6 : 4); ++axis) {
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
            !capture->trigger_captured[i] && !capture->trigger_adjusted[i] &&
            capture->trigger_previous[i] - capture->rest_high[4 + i] >= 8192) {
            capture->trigger_maximum[i] = capture->trigger_previous[i];
            capture->trigger_captured[i] = true;
        }
        capture->trigger_previous[i] = raw[4 + i];
        capture->trigger_sampled[i] = true;
        capture->trigger_pressed[i] = pressed;
    }
    if (resting) ++capture->samples;
    else if (capture->samples && !trigger_click)
        for (unsigned stick = 0; stick < 2; ++stick) sample_stick(capture, stick, raw);
}

void nativeControllerCalibration_TriggerRange(const NativeControllerCalibration* capture,
                                              unsigned trigger, int16_t* start, int16_t* end)
{
    int zero = capture->trigger_adjusted[trigger] ? capture->trigger_minimum[trigger] :
        (int)capture->rest_high[4+trigger] + 256;
    *start = (int16_t)(zero > 24575 ? 24575 : zero < 0 ? 0 : zero);
    *end = capture->trigger_adjusted[trigger] || capture->trigger_captured[trigger] ?
        capture->trigger_maximum[trigger] : capture->high[4+trigger];
    if (!capture->trigger_adjusted[trigger] && !capture->trigger_captured[trigger] &&
        *end-*start < 8192) *end = 32767;
}
bool nativeControllerCalibration_SetTriggerRange(NativeControllerCalibration* capture,
                                                 unsigned trigger, int start, int end)
{
    if (trigger >= 2 || start < 0 || end > 32767 || end-start < 8192) return false;
    capture->trigger_minimum[trigger] = (int16_t)start;
    capture->trigger_maximum[trigger] = (int16_t)end;
    capture->trigger_adjusted[trigger] = true;
    return true;
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
        if (axis >= 4) {
            unsigned trigger = axis-4;
            if (!capture->trigger_adjusted[trigger] && capture->high[axis]-high < 8192) return false;
            if (capture->trigger_click_supported[trigger] && !capture->trigger_captured[trigger] &&
                !capture->trigger_adjusted[trigger]) return false;
            nativeControllerCalibration_TriggerRange(capture, trigger, &c.center[axis], &c.maximum[axis]);
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
