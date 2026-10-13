#ifndef HSD_NATIVE_CALLBACK_PROFILE_H
#define HSD_NATIVE_CALLBACK_PROFILE_H
#include <stdbool.h>
#include <stdint.h>
typedef enum NativeCallbackProfileKind {
    NativeCallbackProfile_Process,
    NativeCallbackProfile_Display,
    NativeCallbackProfile_Camera,
} NativeCallbackProfileKind;
typedef struct NativeCallbackProfileStamp {
    uint64_t started;
    unsigned bucket, lane;
} NativeCallbackProfileStamp;
// Caller-thread diagnostics; no game state, callback replacement or allocation.
extern bool nativeCallbackProfileEnabled;
void nativeCallbackProfile_Init(void);
void nativeCallbackProfile_SetFrame(uint64_t frame, bool hidden);
NativeCallbackProfileStamp nativeCallbackProfile_Begin(uintptr_t callback,
                                                       NativeCallbackProfileKind);
void nativeCallbackProfile_End(NativeCallbackProfileStamp);
void nativeCallbackProfile_Report(void);
#endif
