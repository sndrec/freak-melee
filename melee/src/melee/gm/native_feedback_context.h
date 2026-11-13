#pragma once
#include <aurora/aurora.h>
#include <stdbool.h>
#include <stdint.h>
#include "native_feedback_history.h"

#ifdef __cplusplus
extern "C" {
#endif
// Host-only observation state. Values are updated at the input boundary and
// copied into an immutable report before annotation changes input or UI state.
void nativeFeedbackContext_Log(AuroraLogLevel, const char* module, const char* message, unsigned length);
bool nativeFeedbackContext_Online(void);
bool nativeFeedbackContext_Write(const char* path, const char* id, const char* captured_at,
                                 const AuroraPresentedCapture* image,
                                 const char* workspace, const char* focused_control);
#ifdef __cplusplus
}
#endif
