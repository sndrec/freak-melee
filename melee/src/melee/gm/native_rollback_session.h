#ifndef MELEE_GM_NATIVE_ROLLBACK_SESSION_H
#define MELEE_GM_NATIVE_ROLLBACK_SESSION_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stdint.h>

#include "native_rollback_input.h"
#include "native_rollback_regions.h"
#include <dolphin/pad.h>

enum {
    NATIVE_ROLLBACK_SESSION_HISTORY_FRAMES = NATIVE_ROLLBACK_INPUT_CAPACITY,
};

typedef enum NativeRollbackSessionStep {
    NativeRollbackSession_Continue,
    NativeRollbackSession_Restored,
    NativeRollbackSession_CaughtUp,
    NativeRollbackSession_Failed,
} NativeRollbackSessionStep;

typedef struct NativeRollbackSession {
    NativeRollbackRegionSet regions;
    NativeRollbackInputHistory inputs;
    uint64_t current_frame;
    uint64_t catch_up_frame;
    const char* failure_reason;
    uint8_t local_port_mask;
    bool active;
    bool frame_active;
    bool resimulating;
    bool rumble_output_suppressed;
    bool failed;
} NativeRollbackSession;

bool nativeRollbackSession_Init(NativeRollbackSession* session,
                                uint8_t local_port_mask,
                                uint32_t dirty_page_capacity);
void nativeRollbackSession_Shutdown(NativeRollbackSession* session);
bool nativeRollbackSession_BeginFrame(NativeRollbackSession* session,
                                      uint64_t frame);
bool nativeRollbackSession_ResolveInputs(NativeRollbackSession* session,
                                         uint64_t frame,
                                         const PADStatus local_pads[4],
                                         PADStatus result[4]);
bool nativeRollbackSession_SubmitRemote(NativeRollbackSession* session,
                                        uint64_t frame, uint32_t port,
                                        const PADStatus* input);
NativeRollbackSessionStep
nativeRollbackSession_EndFrame(NativeRollbackSession* session, uint64_t frame,
                               uint64_t* next_frame);
bool nativeRollbackSession_Restore(NativeRollbackSession* session,
                                   uint64_t target_frame,
                                   uint64_t current_frame,
                                   uint64_t* next_frame);
void nativeRollbackSession_Abort(NativeRollbackSession* session,
                                 const char* reason);
bool nativeRollbackSession_IsResimulating(
    const NativeRollbackSession* session);
bool nativeRollbackSession_HasFailed(const NativeRollbackSession* session);
const char*
nativeRollbackSession_FailureReason(const NativeRollbackSession* session);

#endif
#endif
