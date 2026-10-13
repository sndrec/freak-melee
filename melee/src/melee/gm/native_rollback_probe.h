#ifndef MELEE_GM_NATIVE_ROLLBACK_PROBE_H
#define MELEE_GM_NATIVE_ROLLBACK_PROBE_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stdint.h>

typedef enum NativeRollbackProbeStep {
    NativeRollbackProbe_Continue,
    NativeRollbackProbe_Restored,
    NativeRollbackProbe_Succeeded,
    NativeRollbackProbe_Failed,
} NativeRollbackProbeStep;

bool nativeRollbackProbe_Init(bool correction_mode, uint32_t correction_frame);
void nativeRollbackProbe_Shutdown(void);
bool nativeRollbackProbe_BeginFrame(uint64_t frame);
NativeRollbackProbeStep nativeRollbackProbe_EndFrame(uint64_t frame,
                                                     uint64_t* next_frame);
void nativeRollbackProbe_Abort(const char* reason);
bool nativeRollbackProbe_IsResimulating(void);
bool nativeRollbackProbe_HasFailed(void);

#endif
#endif
