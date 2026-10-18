#include <melee/lb/lb_0195.h>
#include <sysdolphin/baselib/controller.h>
#include <stdio.h>
#include <stdlib.h>

enum { PERIOD = 1000000000 / 60 };
static unsigned samples, queued, card_polls, snapshot_polls;
static unsigned game_updates, copy_updates;

static void require(bool condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

// Hardware and card boundaries are replaced here; the production scheduler
// and its channel gating are compiled unchanged into this test.
void HSD_PadRenewRawStatus(bool check) { ++samples; ++queued; }
u8 HSD_PadGetRawQueueCount(void) { return queued; }
void HSD_PadRenewMasterStatus(void) { require(queued > 0, "empty queue"); --queued; }
void HSD_PadRenewGameStatus(void) { ++game_updates; }
void HSD_PadRenewCopyStatus(void) { ++copy_updates; }
void HSD_DevComPoll(void) {}
void HSD_AudioOutputPoll(void) {}
void lb_8001C600(void) { ++card_polls; }
void lbSnap_8001D2BC(void) { ++snapshot_polls; }
void lb_8001B6F8(void) {}
void lb_8001CC84(void) {}
void lb_800192A8(void (*callback)(void)) {}
void __assert(char* file, u32 line, char* expression)
{
    fprintf(stderr, "%s:%u: %s\n", file, line, expression);
    abort();
}

static void initialize(void) {}
static void reset(void)
{
    samples = queued = card_polls = snapshot_polls = 0;
    game_updates = copy_updates = 0;
    lb_80019AAC(initialize);
}

static void step(u64 now)
{
    require(lbInput_Poll(now) == 1, "expected one fresh sample");
    lb_800198E0();
    lb_80019900();
}

int main(void)
{
    reset();
    require(lbInput_VirtualClockNowNs() == PERIOD,
            "first virtual input tick");
    require(lbInput_VirtualClockNowNs() == 2 * PERIOD,
            "virtual input clock advances without wall time");

    reset();
    for (unsigned i = 0; i < 600; ++i) {
        step((u64) i * PERIOD);
        require(lb_80019A30(0) && lb_80019A30(1), "normal-speed channel cadence");
        require(lbInput_Poll((u64) i * PERIOD + PERIOD - 1) == 0,
                "sample appeared before its deadline");
    }
    require(samples == 600 && game_updates == 600 && copy_updates == 600,
            "normal-speed update count");
    require(card_polls == 0 && snapshot_polls == 0,
            "native card services remained active");

    reset();
    step(0);
    step(PERIOD + 1000);
    require(lbInput_Poll(2 * PERIOD - 1) == 0, "early sample after jitter");
    step(2 * PERIOD);

    reset();
    const u64 replay_origin = 10000000000ULL;
    step(replay_origin);
    for (unsigned i = 1; i <= 15; ++i) {
        u64 scheduled = lbInput_NextSampleTimeNs();
        require(scheduled == replay_origin + (u64)i * PERIOD,
                "replay must advance the saved schedule, without wall-clock anchoring");
        require(samples == i, "querying the replay deadline must not sample input");
        step(scheduled);
        require(lb_80019A30(0) && lb_80019A30(1), "replayed channel cadence");
    }
    u64 live_deadline = lbInput_NextSampleTimeNs();
    require(lbInput_Poll(live_deadline - 1) == 0,
            "returning to live input must retain its next sampling deadline");
    step(live_deadline);
    require(samples == 17, "fifteen replayed samples and the following live sample");

    reset();
    step(replay_origin);
    for (unsigned i = 0; i < 6000; ++i) step(lbInput_NextSampleTimeNs());
    require(lbInput_Poll(replay_origin + PERIOD) == 0,
            "unpaced playback should leave its schedule ahead of real time");
    lbInput_RebaseClock(replay_origin + PERIOD);
    step(replay_origin + PERIOD);
    require(lb_80019A30(0) && lb_80019A30(1),
            "the next scene must accept live input immediately after catch-up");

    reset();
    lbInput_SetGameSpeed(0.5f);
    step(0);
    require(!lb_80019A30(0), "initial half-speed phase");
    lbInput_RebaseClock(replay_origin);
    step(replay_origin);
    require(lb_80019A30(0) && lb_80019A30(1),
            "rebasing host time must preserve simulation phase and speed");

    reset();
    lbInput_SetGameSpeed(0.5f);
    for (unsigned i = 0; i < 120; ++i) {
        step((u64) i * PERIOD);
        require(lb_80019A30(0) == (i % 2 == 1), "half-speed phase");
        require(lb_80019A30(1), "half-speed base channel");
    }
    require(game_updates == 60 && copy_updates == 60, "half-speed update count");

    reset();
    lbInput_SetGameSpeed(2.0f);
    for (unsigned i = 0; i < 120; ++i) {
        step((u64) i * (PERIOD / 2));
        require(lb_80019A30(0), "double-speed game channel");
        require(lb_80019A30(1) == (i % 2 == 1), "double-speed base phase");
    }

    reset();
    step(0);
    step(10000000000ULL);
    require(lbInput_Poll(10000000000ULL) == 0 && samples == 2,
            "stall generated fabricated historical samples");
    step(10000000000ULL + PERIOD);
    lbInput_SetGameSpeed(2.0f);
    step(10000000000ULL + PERIOD + 1);
    lb_80019A48();
    require(lbInput_Poll(20000000000ULL) == 0, "stopped clock sampled input");
    puts("Native input timing checks passed");
    return 0;
}
