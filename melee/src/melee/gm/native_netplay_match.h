#ifndef MELEE_GM_NATIVE_NETPLAY_MATCH_H
#define MELEE_GM_NATIVE_NETPLAY_MATCH_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stdint.h>

#include "native_confirmed_checkpoint.h"

bool nativeNetplayMatch_Prepare(bool sudden_death);
bool nativeNetplayMatch_Init(void);
struct MatchEnd;
void nativeNetplayMatch_Shutdown(bool continue_into_sudden_death,
                                 const struct MatchEnd* result);
bool nativeNetplayMatch_BeginFrame(uint64_t* frame);
NativeConfirmedCheckpointStep nativeNetplayMatch_EndFrame(uint64_t frame,
                                                      uint64_t* next_frame, bool ending);
void nativeNetplayMatch_Abort(const char* reason);
bool nativeNetplayMatch_IsResimulating(void);
bool nativeNetplayMatch_HasFailed(void);
bool nativeNetplayMatch_ConsumeLobbyReturn(void);
void nativeNetplayMatch_Inspect(void);
// Host observations copied on the simulation owner when feedback is captured.
typedef struct NativeNetplayObservation {
    bool active, failed, replaying, has_checksum;
    uint64_t frame, checkpoint, restores, replay_ticks, wait_ms, pace_delay_ns;
    uint64_t checksum_frame, checksum;
    unsigned corrections, input_stalls, compared_checksums, received_packets, rejected_packets;
    unsigned local_mask, input_delay_frames, max_correction_depth;
    struct {
        bool configured, has_sample, has_checksum;
        int32_t lead_milliframes;
        uint64_t sample_frame, echo_ns, min_echo_ns, checksum_frame, checksum;
    } peers[4];
} NativeNetplayObservation;
void nativeNetplayMatch_Observe(NativeNetplayObservation* observation);

#endif
#endif
