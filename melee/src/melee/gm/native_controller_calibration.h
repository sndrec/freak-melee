#pragma once
#include "native_profile.h"

typedef struct NativeControllerCalibration {
    int16_t low[6], high[6], rest_low[6], rest_high[6];
    unsigned samples;
    float stick_gate[2][32];
    int16_t trigger_maximum[2], trigger_previous[2];
    bool trigger_click_supported[2], trigger_captured[2], trigger_sampled[2], trigger_pressed[2];
} NativeControllerCalibration;

#ifdef __cplusplus
extern "C" {
#endif
void nativeControllerCalibration_Reset(NativeControllerCalibration* capture);
// trigger_click: -1 unavailable, 0 released, 1 pressed; NULL outside trigger capture.
void nativeControllerCalibration_Sample(NativeControllerCalibration* capture,
                                         const int16_t raw[6], bool resting,
                                         const int8_t trigger_click[2]);
bool nativeControllerCalibration_Finish(const NativeControllerCalibration* capture,
                                         NativeProfileCalibration* result);
int16_t nativeControllerCalibration_Normalize(const NativeProfileCalibration* calibration,
                                               unsigned axis, int raw);
// Raw logical input before dead zones and saved center/range calibration.
int16_t nativeProfileInput_RawAxis(unsigned port, const NativeProfile* profile, unsigned axis);
#ifdef __cplusplus
}
#endif
