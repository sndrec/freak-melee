#pragma once
#include "native_profile.h"

enum { NATIVE_CALIBRATION_GATE_POINTS = 64 };
typedef struct NativeCalibrationPoint { int16_t x, y; } NativeCalibrationPoint;
typedef struct NativeControllerCalibration {
    int16_t low[6], high[6], rest_low[6], rest_high[6];
    unsigned samples;
    NativeCalibrationPoint stick_gate[2][NATIVE_CALIBRATION_GATE_POINTS];
    unsigned stick_gate_count[2];
    int16_t trigger_minimum[2], trigger_maximum[2], trigger_previous[2];
    bool trigger_adjusted[2];
    bool trigger_click_supported[2], trigger_captured[2], trigger_sampled[2], trigger_pressed[2];
} NativeControllerCalibration;

#ifdef __cplusplus
extern "C" {
#endif
void nativeControllerCalibration_Reset(NativeControllerCalibration* capture);
// Rest samples all axes. Otherwise NULL captures sticks; a click array captures triggers.
// trigger_click: -1 unavailable, 0 released, 1 pressed.
void nativeControllerCalibration_Sample(NativeControllerCalibration* capture,
                                         const int16_t raw[6], bool resting,
                                         const int8_t trigger_click[2]);
void nativeControllerCalibration_TriggerRange(const NativeControllerCalibration* capture,
                                              unsigned trigger, int16_t* start, int16_t* end);
bool nativeControllerCalibration_SetTriggerRange(NativeControllerCalibration* capture,
                                                 unsigned trigger, int start, int end);
bool nativeControllerCalibration_Finish(const NativeControllerCalibration* capture,
                                         NativeProfileCalibration* result);
int16_t nativeControllerCalibration_Normalize(const NativeProfileCalibration* calibration,
                                               unsigned axis, int raw);
// Raw logical input before dead zones and saved center/range calibration.
int16_t nativeProfileInput_RawAxis(unsigned port, const NativeProfile* profile, unsigned axis);
void nativeProfileInput_RawAxes(unsigned port, const NativeProfile* profile, int16_t values[6]);
#ifdef __cplusplus
}
#endif
