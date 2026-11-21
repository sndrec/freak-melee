#ifndef MELEE_GM_NATIVE_NETPLAY_TIMING_H
#define MELEE_GM_NATIVE_NETPLAY_TIMING_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum NativeNetplayTimingPhase {
    NativeNetplayTiming_Input,
    NativeNetplayTiming_Simulation,
    NativeNetplayTiming_Drawing,
    NativeNetplayTiming_RenderBegin,
    NativeNetplayTiming_HsdDraw,
    NativeNetplayTiming_RenderEnd,
    NativeNetplayTiming_Netplay,
    NativeNetplayTiming_Checkpoint,
    NativeNetplayTiming_InputWait,
    NativeNetplayTiming_NetworkWait,
    NativeNetplayTiming_Send,
    NativeNetplayTiming_Receive,
    NativeNetplayTiming_RoomTransfers,
    NativeNetplayTiming_Phases,
} NativeNetplayTimingPhase;

// Optional host instrumentation; never restored and never allocates per tick.
extern bool nativeNetplayTimingEnabled;
void nativeNetplayTiming_Init(void);
void nativeNetplayTiming_Begin(NativeNetplayTimingPhase);
void nativeNetplayTiming_End(NativeNetplayTimingPhase);
void nativeNetplayTiming_Checkpoints(uint64_t captures, uint64_t restores,
                                   size_t bytes);
void nativeNetplayTiming_Sample(uint64_t late_ns, int32_t lead_milliframes);
// Cycle boundaries are successive visible-frame submissions. Thus checkpoint
// work following the prior submission belongs to the next complete cycle.
void nativeNetplayTiming_Submitted(uint64_t frame, bool hidden);
void nativeNetplayTiming_Report(void);
#endif
