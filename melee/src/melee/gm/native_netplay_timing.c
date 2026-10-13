#include "native_netplay_timing.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL_timer.h>
#include <sysdolphin/baselib/debug.h>

#ifdef MELEE_NATIVE
#pragma clang section data = "" bss = ""
#endif

enum { TimingRecords = 1024, WarmupFrame = 120 };
typedef struct NetplayTimingRecord {
    uint64_t frame, elapsed;
    uint64_t phases[NativeNetplayTiming_Phases];
    uint64_t captures, restores;
    size_t peak_bytes;
    unsigned replay_ticks;
} NetplayTimingRecord;

bool nativeNetplayTimingEnabled;
static struct {
    NetplayTimingRecord records[TimingRecords], current;
    uint64_t start[NativeNetplayTiming_Phases], cycle_start;
    unsigned count, overflow;
    bool started;
} timing;

void nativeNetplayTiming_Init(void)
{
    const char* enabled = getenv("FREAK_MELEE_PROFILE_NETPLAY");
    nativeNetplayTimingEnabled = enabled && !strcmp(enabled, "1");
    memset(&timing, 0, sizeof(timing));
}

void nativeNetplayTiming_Begin(NativeNetplayTimingPhase phase)
{
    timing.start[phase] = SDL_GetTicksNS();
}

void nativeNetplayTiming_End(NativeNetplayTimingPhase phase)
{
    timing.current.phases[phase] += SDL_GetTicksNS() - timing.start[phase];
}

void nativeNetplayTiming_Checkpoints(uint64_t captures, uint64_t restores,
                                   size_t bytes)
{
    timing.current.captures += captures;
    timing.current.restores += restores;
    if (bytes > timing.current.peak_bytes) timing.current.peak_bytes = bytes;
}

void nativeNetplayTiming_Submitted(uint64_t frame, bool hidden)
{
    if (hidden) {
        ++timing.current.replay_ticks;
        return;
    }
    uint64_t now = SDL_GetTicksNS();
    if (timing.started && frame >= WarmupFrame) {
        timing.current.frame = frame;
        timing.current.elapsed = now - timing.cycle_start;
        if (timing.count < TimingRecords) timing.records[timing.count++] = timing.current;
        else ++timing.overflow;
    }
    timing.current = (NetplayTimingRecord) { 0 };
    timing.cycle_start = now;
    timing.started = true;
}

void nativeNetplayTiming_Report(void)
{
    if (!nativeNetplayTimingEnabled) return;
    OSReport("[netplay-timing] records=%u overflow=%u warmup_frame=%u "
             "scope=visible-submission-to-visible-submission pending_cycle_excluded=1\n",
             timing.count, timing.overflow, WarmupFrame);
    for (unsigned i = 0; i < timing.count; ++i) {
        const NetplayTimingRecord* record = &timing.records[i];
        const uint64_t* phase = record->phases;
        uint64_t wait = phase[NativeNetplayTiming_InputWait] + phase[NativeNetplayTiming_NetworkWait];
        OSReport("[netplay-timing] frame=%llu replay_ticks=%u visible_ticks=1 "
                 "captures=%llu restores=%llu peak_bytes=%zu total_us=%.3f "
                 "explicit_wait_us=%.3f input_us=%.3f simulation_us=%.3f drawing_us=%.3f "
                 "render_begin_us=%.3f hsd_draw_us=%.3f render_end_us=%.3f "
                 "netplay_us=%.3f checkpoint_us=%.3f instrumentation=phase-clocks\n",
                 (unsigned long long)record->frame, record->replay_ticks,
                 (unsigned long long)record->captures, (unsigned long long)record->restores,
                 record->peak_bytes, record->elapsed / 1000.0, wait / 1000.0,
                 phase[NativeNetplayTiming_Input] / 1000.0,
                 phase[NativeNetplayTiming_Simulation] / 1000.0,
                 phase[NativeNetplayTiming_Drawing] / 1000.0,
                 phase[NativeNetplayTiming_RenderBegin] / 1000.0,
                 phase[NativeNetplayTiming_HsdDraw] / 1000.0,
                 phase[NativeNetplayTiming_RenderEnd] / 1000.0,
                 phase[NativeNetplayTiming_Netplay] / 1000.0,
                 phase[NativeNetplayTiming_Checkpoint] / 1000.0);
    }
    nativeNetplayTimingEnabled = false;
}
