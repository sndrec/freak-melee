#pragma once
#include <stdbool.h>
#include <dolphin/pad.h>
#ifdef __cplusplus
extern "C" {
#endif
void nativeFeedbackInput_SetOpen(bool open);
void nativeFeedbackInput_SetResimulating(bool resimulating);
void nativeFeedbackInput_Filter(PADStatus pads[4]);
#ifdef __cplusplus
}
#endif
