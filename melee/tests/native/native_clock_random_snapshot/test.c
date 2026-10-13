#include <melee/lb/lb_0195.h>
#include <melee/lb/native_input_clock_snapshot.h>
#include <melee/gm/native_profile_input.h>
#include <melee/gm/native_replay.h>
#include <sysdolphin/baselib/native_controller_snapshot.h>
#include <sysdolphin/baselib/random.h>
#include <sysdolphin/baselib/native_random_snapshot.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum { Period = 1000000000 / 60, Steps = 24 };
static unsigned checks, reads, motors, callbacks, rejected_callbacks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static HSD_PadData queue[5];
static HSD_PadRumbleListData rumble[12];
static unsigned char pad_saved[8192], pad_end[8192], pad_repeat[8192];
static NativeInputClockSnapshot clock_saved, clock_end, clock_repeat;
static NativeRandomSnapshot rng_saved, rng_end, rng_repeat;
static size_t pad_bytes;
static u32 generated[Steps];
static bool observed_update[Steps][2];
static bool callback_guard;

static void capture_refusal(void)
{
    NativeInputClockSnapshot candidate;
    size_t written = 192;
    CHECK(!nativeInputClock_Save(&candidate, sizeof(candidate), &written));
    CHECK(written == 192);
    CHECK(!nativeInputClock_ValidateSnapshot(&clock_saved, sizeof(clock_saved)));
    CHECK(!nativeInputClock_Load(&clock_saved, sizeof(clock_saved)));
    ++rejected_callbacks;
}

BOOL PADInit(void) { return true; }
u32 PADRead(PADStatus* status)
{
    ++reads;
    memset(status, 0, 4 * sizeof(*status));
    for (unsigned i = 0; i < 4; ++i) {
        status[i].button = PAD_BUTTON_A;
        status[i].stickX = (s8)(15 + i);
    }
    if (callback_guard) capture_refusal();
    return 0;
}
int PADReset(unsigned long mask) { (void)mask; return true; }
void PADControlMotor(s32 port, u32 command) { (void)port; (void)command; ++motors; }
bool nativeReplay_IsPlayback(void) { return true; }
void nativeReplay_RecordPads(const PADStatus* status) { CHECK(status); }
void nativeProfileInput_Apply(PADStatus status[4]) { (void)status; CHECK(false); }
void HSD_DevComPoll(void) { CHECK(false); }
void HSD_AudioOutputPoll(void) { CHECK(false); }
void lb_800192A8(Event cb) { (void)cb; CHECK(false); }
void __assert(char* file, u32 line, char* expression)
{ fprintf(stderr, "%s:%u: %s\n", file, (unsigned)line, expression); abort(); }

static void initialize(void)
{
    ++callbacks;
    if (callback_guard) capture_refusal();
    HSD_PadInit(5, queue, 12, rumble);
}

static void save_clock(NativeInputClockSnapshot* state)
{
    size_t written = 0;
    CHECK(nativeInputClock_Save(state, sizeof(*state), &written));
    CHECK(written == sizeof(*state));
    CHECK(nativeInputClock_ValidateSnapshot(state, written));
}

static void save_rng(NativeRandomSnapshot* state)
{
    size_t written = 0;
    CHECK(nativeRandom_Save(state, sizeof(*state), &written));
    CHECK(written == sizeof(*state));
}

static void clock_reject(NativeInputClockSnapshot candidate)
{
    NativeInputClockSnapshot before, after;
    save_clock(&before);
    unsigned before_reads = reads, before_motors = motors;
    CHECK(!nativeInputClock_ValidateSnapshot(&candidate, sizeof(candidate)));
    CHECK(!nativeInputClock_Load(&candidate, sizeof(candidate)));
    save_clock(&after);
    CHECK(!memcmp(&before, &after, sizeof(before)));
    CHECK(reads == before_reads && motors == before_motors);
}

static void run_window(bool repeat)
{
    for (unsigned i = 0; i < Steps; ++i) {
        // Includes stalled wall deadlines, speed changes and skipped phase
        // updates (the GM pause gate), using the real input/controller code.
        if (i == 4) lbInput_SetGameSpeed(0.5f);
        if (i == 13) lbInput_SetGameSpeed(2.0f);
        u64 now = lbInput_VirtualClockNowNs();
        if (i == 9 || i == 10) now += 20 * Period;
        u8 count = lbInput_Poll(now);
        for (unsigned j = 0; j < count; ++j) {
            lb_800198E0();
            if (i % 7) lb_80019900();
        }
        u32 number = (u32)HSD_Rand();
        // Exercise the same shared generator through all public paths; these
        // extra calls stand for ordinary draw/particle consumers, not skips.
        number ^= (u32)(HSD_Randf() * 65536.0f) << 16;
        number ^= (u32)HSD_Randi(31);
        if (!repeat) {
            generated[i] = number;
            for (int k = 0; k < 2; ++k) observed_update[i][k] = lb_80019A30(k);
        } else {
            CHECK(generated[i] == number);
            for (int k = 0; k < 2; ++k) CHECK(observed_update[i][k] == lb_80019A30(k));
        }
    }
}

int main(void)
{
    save_clock(&clock_saved); // Legal pre-initialization zero state.
    CHECK(!clock_saved.running && clock_saved.game_period_ns == 0);
    callback_guard = true;
    lb_80019AAC(initialize);
    CHECK(callbacks == 1 && rejected_callbacks == 1);
    callback_guard = false;
    lbInput_SetGameSpeed(0.5f);
    CHECK(lbInput_Poll(lbInput_VirtualClockNowNs()) == 1);
    lb_800198E0(); lb_80019900();
    CHECK(!lb_80019A30(0) && lb_80019A30(1));
    save_clock(&clock_saved);
    *seed_ptr = 0x12345678;
    save_rng(&rng_saved);
    CHECK(nativeController_Save(pad_saved, sizeof(pad_saved), &pad_bytes));
    run_window(false);
    save_clock(&clock_end); save_rng(&rng_end);
    size_t end_bytes, repeat_bytes;
    CHECK(nativeController_Save(pad_end, sizeof(pad_end), &end_bytes));
    unsigned before_reads = reads, before_motors = motors, before_callbacks = callbacks;
    CHECK(nativeInputClock_Load(&clock_saved, sizeof(clock_saved)));
    CHECK(nativeRandom_Load(&rng_saved, sizeof(rng_saved)));
    CHECK(nativeController_Load(pad_saved, pad_bytes));
    CHECK(reads == before_reads && motors == before_motors && callbacks == before_callbacks);
    run_window(true);
    save_clock(&clock_repeat); save_rng(&rng_repeat);
    CHECK(nativeController_Save(pad_repeat, sizeof(pad_repeat), &repeat_bytes));
    CHECK(end_bytes == repeat_bytes && !memcmp(pad_end, pad_repeat, end_bytes));
    CHECK(!memcmp(&clock_end, &clock_repeat, sizeof(clock_end)));
    CHECK(!memcmp(&rng_end, &rng_repeat, sizeof(rng_end)));

    NativeInputClockSnapshot bad = clock_end;
    bad.sample_period_ns++; clock_reject(bad);
    bad = clock_end; bad.phase_ns[0] = bad.game_period_ns; clock_reject(bad);
    bad = clock_end; bad.phase_ns[1] = Period; clock_reject(bad);
    bad = clock_end; bad.update[0] = 2; clock_reject(bad);
    bad = clock_end; bad.running = 2; clock_reject(bad);
    bad = clock_end; bad.reserved[2] = 1; clock_reject(bad);
    bad = clock_end; bad.game_period_ns = 0; clock_reject(bad);
    size_t written = 777;
    CHECK(!nativeInputClock_Save(&bad, sizeof(bad)-1, &written) && written == 777);
    CHECK(!nativeInputClock_Save(&bad, sizeof(bad), (size_t*)&bad));
    CHECK(!nativeInputClock_Load(&clock_end, sizeof(clock_end)-1));
    CHECK(!nativeInputClock_Load(NULL, sizeof(clock_end)));

    NativeRandomSnapshot bad_rng = rng_end;
    bad_rng.reserved = 1;
    CHECK(!nativeRandom_Load(&bad_rng, sizeof(bad_rng)));
    CHECK(!nativeRandom_Load(&rng_end, sizeof(rng_end)-1));
    CHECK(!nativeRandom_Save(seed_ptr, sizeof(rng_end), &written));
    CHECK(!nativeRandom_Save(&seed_ptr, sizeof(rng_end), &written));
    CHECK(!nativeRandom_Save(&bad_rng, sizeof(bad_rng), (size_t*)&bad_rng));
    u32 foreign_seed = 31415, *original_seed = seed_ptr;
    seed_ptr = &foreign_seed;
    CHECK(!nativeRandom_Save(&bad_rng, sizeof(bad_rng), &written));
    CHECK(!nativeRandom_Load(&rng_end, sizeof(rng_end)));
    CHECK(foreign_seed == 31415);
    seed_ptr = original_seed;
    rng_saved = (NativeRandomSnapshot){0, 0};
    CHECK(nativeRandom_Load(&rng_saved, sizeof(rng_saved)));
    CHECK(HSD_Rand() == (2531011 >> 16)); // Seed zero is legal, not silently fixed.
    rng_saved = (NativeRandomSnapshot){UINT32_MAX, 0};
    CHECK(nativeRandom_Load(&rng_saved, sizeof(rng_saved)));
    CHECK(HSD_Rand() == ((UINT32_MAX * 214013u + 2531011u) >> 16));

    callback_guard = true;
    CHECK(lbInput_Poll(lbInput_VirtualClockNowNs() + 100 * Period) >= 1);
    CHECK(rejected_callbacks == 2);
    callback_guard = false;
    lb_80019A48(); save_clock(&clock_saved);
    before_reads = reads;
    (void)lbInput_Poll(lbInput_VirtualClockNowNs() + 100 * Period);
    CHECK(reads == before_reads);
    CHECK(nativeInputClock_Load(&clock_saved, sizeof(clock_saved)));
    save_clock(&clock_repeat);
    CHECK(!memcmp(&clock_saved, &clock_repeat, sizeof(clock_saved)));
    printf("{\"status\":\"PASS\",\"checks\":%u,\"clock_bytes\":%zu,\"rng_bytes\":%zu,\"restored_ticks\":%u,\"callback_refusals\":%u}\n",
        checks, sizeof(clock_saved), sizeof(rng_saved), Steps, rejected_callbacks);
    return 0;
}
