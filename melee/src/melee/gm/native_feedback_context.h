#pragma once
#include <aurora/aurora.h>
#include <stdbool.h>
#include <stdint.h>
#include <yyjson.h>
#include "native_feedback_history.h"

#ifdef __cplusplus
extern "C" {
#endif
// Host-only observation state. Values are updated at the input boundary and
// copied into an immutable report before annotation changes input or UI state.
void nativeFeedbackContext_RecordWarning(AuroraLogLevel, const char* module, const char* message_template, unsigned length);
bool nativeFeedbackContext_Online(void);
typedef struct NativeFeedbackInterface {
    const char* workspace;
    const char* mode;
    const char* focused_control;
    int hub_page;
    bool menu_transition;
    yyjson_val* agent_report;
} NativeFeedbackInterface;
// The caller owns the serialized capture and releases it with free().
char* nativeFeedbackContext_Collect(const char* id, const char* captured_at,
                                 const AuroraPresentedCapture* image,
                                 const NativeFeedbackInterface* interface_state, size_t* length);
#ifdef __cplusplus
}
#endif
