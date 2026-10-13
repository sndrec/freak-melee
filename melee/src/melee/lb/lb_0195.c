#include <melee/gm/native_match_control_history.h>
#include "lb_0195.h"
#include "native_input_clock_snapshot.h"

#include "lb_0192.h"
#include "lbaudio_ax.h"
#include "lbcardgame.h"
#include "lbcardnew.h"
#include "lbsnap.h"
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/devcom.h>
#include <sysdolphin/baselib/audio_output.h>
#include <sysdolphin/baselib/controller.h>

enum { INPUT_BASE_PERIOD_NS = 1000000000 / 60 };

static struct InputClock {
    u64 game_period_ns;
    u64 sample_period_ns;
    u64 next_sample_ns;
    u64 virtual_time_ns;
    u64 phase_ns[2];
    bool update[2];
    bool sampled;
    bool scheduled;
    bool running;
} input_clock;
// Actual callback activity, not rewindable history. Every accepted boundary
// observes zero; a callback cannot capture a half-produced controller sample.
static unsigned input_activity;

void lb_8001955C(void)
{
    HSD_DevComPoll();
    HSD_AudioOutputPoll();

#ifndef MELEE_NATIVE
    lb_8001B6F8();
    lb_8001CC84();
#endif
}

void lb_800195D0(void)
{
    HSD_DevComPoll();
    HSD_AudioOutputPoll();
    lb_800192A8(lb_8001955C);
#ifndef MELEE_NATIVE
    lb_8001CC84();
#endif
}

void lbInput_SetGameSpeed(float speed)
{
    HSD_ASSERT(0, isfinite(speed) && speed > 0.0f);
    double period = (double) INPUT_BASE_PERIOD_NS / speed;
    HSD_ASSERT(0, period >= 1.0 && period < 18446744073709551616.0);
    u64 game_period = (u64) period;
    if (input_clock.game_period_ns == game_period) {
        return;
    }
    input_clock.game_period_ns = game_period;
    if (input_clock.phase_ns[0] >= game_period) {
        input_clock.phase_ns[0] = 0;
    }
    u64 sample_period = game_period < INPUT_BASE_PERIOD_NS
                            ? game_period : INPUT_BASE_PERIOD_NS;
    if (sample_period != input_clock.sample_period_ns) {
        input_clock.sample_period_ns = sample_period;
        input_clock.scheduled = false;
    }
}

u8 lbInput_Poll(u64 now_ns)
{
    input_clock.sampled = false;
    if (!input_clock.running) {
        return HSD_PadGetRawQueueCount();
    }
    if (!input_clock.scheduled || now_ns >= input_clock.next_sample_ns) {
        if (!input_clock.scheduled) {
            input_clock.next_sample_ns = now_ns;
            input_clock.scheduled = true;
        }
        input_clock.next_sample_ns += input_clock.sample_period_ns;
        // A stall cannot be recovered by polling the same present-day input
        // repeatedly. Resume the cadence with one real sample, without debt.
        if (input_clock.next_sample_ns <= now_ns) {
            input_clock.next_sample_ns = now_ns + input_clock.sample_period_ns;
        }
        ++input_activity;
        HSD_PadRenewRawStatus(false);
        input_clock.sampled = true;
#ifndef MELEE_NATIVE
        lb_8001C600();
        lbSnap_8001D2BC();
#endif
        --input_activity;
    }
    return HSD_PadGetRawQueueCount();
}

bool lbInput_DidSample(void)
{
    return input_clock.sampled;
}

u64 lbInput_VirtualClockNowNs(void)
{
    input_clock.virtual_time_ns += INPUT_BASE_PERIOD_NS;
    return input_clock.virtual_time_ns;
}

u64 lbInput_NextSampleTimeNs(void)
{
    return input_clock.scheduled ? input_clock.next_sample_ns
                                 : input_clock.virtual_time_ns;
}

void lb_800198E0(void)
{
    HSD_PadRenewMasterStatus();
}

void lb_80019900(void)
{
    const u64 periods[2] = { input_clock.game_period_ns, INPUT_BASE_PERIOD_NS };
    for (int i = 0; i < 2; ++i) {
        input_clock.phase_ns[i] += input_clock.sample_period_ns;
        input_clock.update[i] = input_clock.phase_ns[i] >= periods[i];
        if (input_clock.update[i]) {
            input_clock.phase_ns[i] -= periods[i];
        }
    }
    if (lb_80019A30(0)) {
        HSD_PadRenewGameStatus();
        HSD_PadRenewCopyStatus();
    }
}

bool lb_80019A30(int index)
{
    return input_clock.update[index];
}

void lb_80019A48(void)
{
    input_clock.running = false;
}

void lb_80019AAC(Event initialize)
{
    ++input_activity;
    initialize();
    input_clock = (struct InputClock) { 0 };
    lbInput_SetGameSpeed(1.0f);
    input_clock.running = true;
    --input_activity;
}

static bool input_clock_buffer(const void* data, size_t bytes)
{
    uintptr_t at = (uintptr_t)data, root = (uintptr_t)&input_clock;
    uintptr_t activity = (uintptr_t)&input_activity;
    return data && bytes && bytes <= UINTPTR_MAX - at &&
        !(at < root + sizeof(input_clock) && root < at + bytes) &&
        !(at < activity + sizeof(input_activity) && activity < at + bytes);
}

bool nativeInputClock_Save(void* data, size_t capacity, size_t* written)
{
    size_t bytes = sizeof(NativeInputClockSnapshot);
    if (input_activity || capacity < bytes || !input_clock_buffer(data, bytes) ||
        !input_clock_buffer(written, sizeof(*written))) return false;
    uintptr_t a = (uintptr_t)data, b = (uintptr_t)written;
    if (a < b + sizeof(*written) && b < a + bytes) return false;
    NativeInputClockSnapshot state = {
        input_clock.game_period_ns, input_clock.sample_period_ns,
        input_clock.next_sample_ns, input_clock.virtual_time_ns,
        {input_clock.phase_ns[0], input_clock.phase_ns[1]},
        {input_clock.update[0], input_clock.update[1]}, input_clock.sampled,
        input_clock.scheduled, input_clock.running, {0}
    };
    // Validate before publishing; malformed live cadence must not look saved.
    if (!nativeInputClock_ValidateSnapshot(&state, sizeof(state))) return false;
    memcpy(data, &state, bytes);
    *written = bytes;
    return true;
}

bool nativeInputClock_ValidateSnapshot(const void* data, size_t bytes)
{
    if (input_activity || bytes != sizeof(NativeInputClockSnapshot) ||
        !input_clock_buffer(data, bytes)) return false;
    NativeInputClockSnapshot state;
    memcpy(&state, data, sizeof(state));
    if (state.update[0] > 1 || state.update[1] > 1 || state.sampled > 1 ||
        state.scheduled > 1 || state.running > 1 || state.reserved[0] ||
        state.reserved[1] || state.reserved[2]) return false;
    if (!state.game_period_ns) {
        // Zero-initialized pre-boot state is legal, but not a running clock.
        return !state.sample_period_ns && !state.next_sample_ns &&
            !state.virtual_time_ns && !state.phase_ns[0] && !state.phase_ns[1] &&
            !state.update[0] && !state.update[1] && !state.sampled &&
            !state.scheduled && !state.running;
    }
    u64 sample = state.game_period_ns < INPUT_BASE_PERIOD_NS ?
        state.game_period_ns : INPUT_BASE_PERIOD_NS;
    return state.sample_period_ns == sample &&
        state.phase_ns[0] < state.game_period_ns &&
        state.phase_ns[1] < INPUT_BASE_PERIOD_NS;
}

bool nativeInputClock_Load(const void* data, size_t bytes)
{
    if (!nativeInputClock_ValidateSnapshot(data, bytes)) return false;
    NativeInputClockSnapshot state;
    memcpy(&state, data, sizeof(state));
    input_clock = (struct InputClock) {
        state.game_period_ns, state.sample_period_ns, state.next_sample_ns,
        state.virtual_time_ns, {state.phase_ns[0], state.phase_ns[1]},
        {state.update[0], state.update[1]}, state.sampled, state.scheduled,
        state.running
    };
    return true;
}

#ifdef MELEE_NATIVE
bool nativeInputClock_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || !arena->lifetime)return false;
#define ROOT(v) do { if(!nativeMatchArena_AddRoot(arena,&(v),sizeof(v)))return false; } while(0)
    ROOT(input_clock);
#undef ROOT
    return true;
}
#endif
