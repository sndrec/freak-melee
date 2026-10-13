#include "native_rollback_benchmark.h"

#ifdef MELEE_NATIVE
#include "native_replay.h"
#include "native_rollback_hash.h"
#include "native_rollback_pages.h"
#include "native_rollback_session.h"
#include <melee/ft/types.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_fighter_snapshot.h>
#include <melee/pl/player.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/rumble.h>
#include <sysdolphin/baselib/native_animation_pool.h>
#include "native_match_snapshot_pool.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { Depth = 15, Windows = 64, Warmup = 600, History = 16,
       DirtyPages = 2048 };
enum { ScriptSnapshotCapacity=sizeof(NativeScriptFighterSnapshotHeader)+
    NATIVE_SCRIPT_FIGHTER_ACTORS*(sizeof(NativeScriptFighterSnapshotRecord)+NATIVE_SCRIPT_FIGHTER_STATE_BYTES) };
enum { AnimationSnapshotCapacity=64+2*sizeof(NativeMatchPoolSnapshotHeader)+
    NATIVE_AOBJ_CAPACITY*(8+sizeof(HSD_AObj))+NATIVE_FOBJ_CAPACITY*(8+sizeof(HSD_FObj)) };

typedef struct BenchmarkBoundary {
    uint64_t frame, sample;
    bool valid;
} BenchmarkBoundary;

typedef struct BenchmarkWindow {
    uint64_t endpoint;
    uint64_t semantic_hash;
    double restore_ms, replay_ms, forward_ms;
    double replay_simulation_ms, replay_draw_ms;
    double forward_simulation_ms, forward_draw_ms;
    double validation_ms, reference_hash_ms;
    double replay_snapshot_begin_ms, replay_session_end_ms, replay_input_ms;
    double forward_snapshot_begin_ms, forward_session_end_ms, forward_input_ms;
    uint32_t replay_dirty_pages, forward_dirty_pages;
    unsigned replay_input_polls, replay_input_waits;
    unsigned forward_input_polls, forward_input_waits;
} BenchmarkWindow;

// This unit is MELEE_ROLLBACK_HOST_STATE: benchmark control/timing must not be
// rewound with game state. Fixed records avoid allocating during measurement.
static struct {
    NativeRollbackSession session;
    BenchmarkBoundary boundary[History];
    BenchmarkWindow records[Windows];
    HSD_GObj* actors[4][2];
    uint32_t generations[4][2];
    uint64_t endpoint_hash, normalized_hash, endpoint_frame;
    uint64_t tick_begin, phase_begin, replay_begin;
    uint64_t current_frame;
    double tick_simulation_ms, tick_draw_ms;
    double tick_snapshot_begin_ms, tick_input_ms;
    unsigned tick_input_polls, tick_input_waits;
    unsigned completed, replay_ticks;
    LARGE_INTEGER frequency;
    bool active, failed, succeeded, awaiting_forward;
} benchmark;

// Optional owner migration check, outside measured rollback ticks. These are
// host diagnostic reservations, not mutable game state or a second backend.
static struct {
    uint8_t history[History][ScriptSnapshotCapacity],endpoint[ScriptSnapshotCapacity],check[ScriptSnapshotCapacity];
    size_t sizes[History];
    uint64_t frames[History];
    uint32_t verified,peak_bytes;
    double save_ms,load_ms;
    bool enabled;
} script_snapshots;
static struct {
    uint8_t history[History][AnimationSnapshotCapacity],endpoint[AnimationSnapshotCapacity],check[AnimationSnapshotCapacity];
    size_t sizes[History];
    uint64_t frames[History];
    uint32_t verified,peak_bytes;
    double save_ms,load_ms;
    bool enabled;
} animation_snapshots;

static uint64_t now(void)
{
    LARGE_INTEGER value;
    QueryPerformanceCounter(&value);
    return (uint64_t)value.QuadPart;
}

static double milliseconds(uint64_t start, uint64_t end)
{
    return (double)(end - start) * 1000.0 /
        (double)benchmark.frequency.QuadPart;
}

// Optional exports exist only in the isolated interpreter-evaluation modules.
// Observe them outside the measured work; normal packages expose none of these.
static bool evaluation_stats(bool reset)
{
    HMODULE modules[512];
    DWORD bytes = 0;
    if (!K32EnumProcessModules(GetCurrentProcess(), modules, sizeof(modules), &bytes) ||
        bytes > sizeof(modules)) {
        OSReport("[interpreter-evaluation] Module census unavailable.\n");
        return false;
    }
    unsigned found = 0;
    bool valid = true;
    for (unsigned i = 0; i < bytes / sizeof(modules[0]); ++i) {
        typedef void (__cdecl *Read)(uint64_t[4]);
        typedef void (__cdecl *Reset)(void);
        Read read = (Read)GetProcAddress(modules[i], "FreakMelee_EvalStats");
        if (read == NULL) continue;
        ++found;
        if (reset) {
            Reset clear = (Reset)GetProcAddress(modules[i], "FreakMelee_EvalResetStats");
            if (clear != NULL) clear();
            continue;
        }
        char path[MAX_PATH];
        DWORD length = GetModuleFileNameA(modules[i], path, sizeof(path));
        uint64_t stats[4] = { 0 };
        read(stats);
        if (stats[3] != 0) valid = false;
        OSReport("[interpreter-evaluation] module=%.*s entries=%llu native_calls=%llu "
                 "interpreted_calls=%llu errors=%llu\n", (int)length, path,
            (unsigned long long)stats[0], (unsigned long long)stats[1],
            (unsigned long long)stats[2], (unsigned long long)stats[3]);
        typedef unsigned (__cdecl *Count)(void);
        typedef const char* (__cdecl *Name)(unsigned);
        typedef uint64_t (__cdecl *Entries)(unsigned);
        Count count = (Count)GetProcAddress(modules[i], "FreakMelee_EvalFunctionCount");
        Name name = (Name)GetProcAddress(modules[i], "FreakMelee_EvalFunctionName");
        Entries entries = (Entries)GetProcAddress(modules[i], "FreakMelee_EvalFunctionEntries");
        if (!count || !name || !entries) continue;
        unsigned total = count(), covered = 0;
        if (total > 1024) { valid = false; continue; }
        for (unsigned function = 0; function < total; ++function) {
            uint64_t calls = entries(function);
            if (!calls) continue;
            ++covered;
            OSReport("[interpreter-evaluation] function=%s calls=%llu\n",
                name(function), (unsigned long long)calls);
        }
        OSReport("[interpreter-evaluation] covered=%u authored_functions=%u\n", covered, total);
    }
    if (!reset) OSReport("[interpreter-evaluation] modules=%u\n", found);
    return valid;
}

void nativeRollbackBenchmark_Abort(const char* reason)
{
    if (!benchmark.failed)
        OSReport("[rollback-benchmark] FAILED/INVALID: %s (frame %llu, window %u).\n",
            reason ? reason : "unknown error",
            (unsigned long long)benchmark.current_frame, benchmark.completed);
    benchmark.failed = true;
    nativeRollbackSession_Abort(&benchmark.session, reason);
}

static bool validate_actors(bool capture)
{
    unsigned count = 0;
    if (HSD_GObjPLinkHead == NULL) return false;
    for (HSD_GObj* object = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];
         object != NULL; object = object->next)
    {
        const Fighter* fighter = object->user_data;
        if (fighter == NULL || fighter->player_id >= 4 || fighter->x221F_b3)
            return false;
        unsigned port = fighter->player_id;
        unsigned member = fighter->x221F_b4 ? 1 : 0;
        if (Player_GetPlayerSlotType(port) != Gm_PKind_Cpu ||
            Player_GetEntityAtIndex(port, member) != object ||
            Player_80032610(port, member != 0) != fighter->kind)
            return false;
        if (capture) {
            benchmark.actors[port][member] = object;
            benchmark.generations[port][member] = (uint32_t)fighter->x8_spawnNum;
        } else if (benchmark.actors[port][member] != object ||
                   benchmark.generations[port][member] != (uint32_t)fighter->x8_spawnNum)
            return false;
        ++count;
    }
    if (count != 8) return false;
    for (unsigned port = 0; port < 4; ++port) {
        if (benchmark.actors[port][0] == NULL || benchmark.actors[port][1] == NULL ||
            benchmark.actors[port][0] == benchmark.actors[port][1]) return false;
    }
    return true;
}

static void filter_inputs(PADStatus pads[4])
{
    PADStatus resolved[4];
    if (!benchmark.active || benchmark.failed) return;
    if (!nativeRollbackSession_ResolveInputs(&benchmark.session,
            benchmark.current_frame, pads, resolved)) {
        nativeRollbackBenchmark_Abort("input history could not provide a tick");
        return;
    }
    memcpy(pads, resolved, sizeof(resolved));
}

bool nativeRollbackBenchmark_Init(void)
{
    memset(&benchmark, 0, sizeof(benchmark));
    memset(&script_snapshots,0,sizeof(script_snapshots));
    memset(&animation_snapshots,0,sizeof(animation_snapshots));
    const char* script_check=getenv("FREAK_MELEE_SCRIPT_SNAPSHOT_CHECK");
    script_snapshots.enabled=script_check && script_check[0]=='1';
    const char* animation_check=getenv("FREAK_MELEE_ANIMATION_SNAPSHOT_CHECK");
    animation_snapshots.enabled=animation_check && animation_check[0]=='1';
    if (!QueryPerformanceFrequency(&benchmark.frequency) ||
        benchmark.frequency.QuadPart <= 0 ||
        !nativeRollbackSession_Init(&benchmark.session, 0xF, DirtyPages)) {
        nativeRollbackBenchmark_Abort("clock or real rollback session initialization failed");
        return false;
    }
    benchmark.active = true;
    HSD_PadSetRawFilter(filter_inputs);
    OSReport("[rollback-benchmark] %u windows, %u replay ticks plus one advancing tick; "
             "warmup %u, deadline 16.666667ms; tracking %zu bytes.\n",
             Windows, Depth, Warmup, benchmark.session.regions.tracked_bytes);
    return true;
}

bool nativeRollbackBenchmark_BeginFrame(uint64_t frame)
{
    if (!benchmark.active || benchmark.failed) return false;
    benchmark.current_frame = frame;
    if ((benchmark.awaiting_forward || nativeRollbackBenchmark_IsResimulating()) &&
        !validate_actors(false)) {
        nativeRollbackBenchmark_Abort("eight live paired CPU actors lost or replaced");
        return false;
    }
    benchmark.boundary[frame % History] = (BenchmarkBoundary){
        frame, nativeReplay_SampleCount(), true };
    if(script_snapshots.enabled) {
        unsigned slot=(unsigned)(frame%History);uint64_t start=now();
        if(!nativeScriptFighter_Save(script_snapshots.history[slot],ScriptSnapshotCapacity,&script_snapshots.sizes[slot])) {
            nativeRollbackBenchmark_Abort("could not capture declared script owner state");return false;
        }
        script_snapshots.frames[slot]=frame;
        script_snapshots.save_ms+=milliseconds(start,now());
        if(script_snapshots.sizes[slot]>script_snapshots.peak_bytes)script_snapshots.peak_bytes=(uint32_t)script_snapshots.sizes[slot];
    }
    if(animation_snapshots.enabled) {
        unsigned slot=(unsigned)(frame%History);uint64_t start=now();
        if(!nativeAnimationPool_Save(animation_snapshots.history[slot],AnimationSnapshotCapacity,&animation_snapshots.sizes[slot])) {
            nativeRollbackBenchmark_Abort("could not capture animation owner state");return false;
        }
        animation_snapshots.frames[slot]=frame;animation_snapshots.save_ms+=milliseconds(start,now());
        if(animation_snapshots.sizes[slot]>animation_snapshots.peak_bytes)animation_snapshots.peak_bytes=(uint32_t)animation_snapshots.sizes[slot];
    }
    benchmark.tick_simulation_ms = benchmark.tick_draw_ms = 0;
    benchmark.tick_input_ms = 0;
    benchmark.tick_input_polls = benchmark.tick_input_waits = 0;
    benchmark.tick_begin = now();
    if (!nativeRollbackSession_BeginFrame(&benchmark.session, frame)) {
        nativeRollbackBenchmark_Abort(nativeRollbackSession_FailureReason(&benchmark.session));
        return false;
    }
    benchmark.tick_snapshot_begin_ms = milliseconds(benchmark.tick_begin, now());
    return true;
}

void nativeRollbackBenchmark_BeginInput(void) { benchmark.phase_begin = now(); }
void nativeRollbackBenchmark_InputPoll(bool waited)
{
    ++benchmark.tick_input_polls;
    benchmark.tick_input_waits += waited;
}
void nativeRollbackBenchmark_EndInput(void)
{
    benchmark.tick_input_ms += milliseconds(benchmark.phase_begin, now());
}

void nativeRollbackBenchmark_BeginSimulation(void) { benchmark.phase_begin = now(); }
void nativeRollbackBenchmark_EndSimulation(void)
{
    benchmark.tick_simulation_ms += milliseconds(benchmark.phase_begin, now());
    if ((benchmark.awaiting_forward || nativeRollbackBenchmark_IsResimulating()) &&
        !validate_actors(false))
        nativeRollbackBenchmark_Abort("eight live paired CPU actors lost during simulation");
}
void nativeRollbackBenchmark_BeginRender(void) { benchmark.phase_begin = now(); }
void nativeRollbackBenchmark_EndRender(void)
{
    benchmark.tick_draw_ms += milliseconds(benchmark.phase_begin, now());
}

static bool restore_window(uint64_t endpoint, uint64_t* next_frame)
{
    if (benchmark.completed == 0 && !evaluation_stats(true)) {
        nativeRollbackBenchmark_Abort("evaluation module statistics unavailable");
        return false;
    }
    BenchmarkWindow* record = &benchmark.records[benchmark.completed];
    uint64_t target_frame = endpoint - Depth;
    BenchmarkBoundary* target = &benchmark.boundary[target_frame % History];
    memset(benchmark.actors, 0, sizeof(benchmark.actors));
    if (!validate_actors(true) || !target->valid || target->frame != target_frame) {
        nativeRollbackBenchmark_Abort("window requires eight live paired CPU actors and 15 stored ticks");
        return false;
    }
    uint64_t start = now();
    benchmark.endpoint_hash = nativeRollbackRegions_Hash(&benchmark.session.regions);
    benchmark.normalized_hash = nativeRollbackHash_AddressNormalized(&benchmark.session.regions);
    record->semantic_hash = nativeReplay_StateDigest(endpoint);
    record->reference_hash_ms = milliseconds(start, now());
    record->endpoint = benchmark.endpoint_frame = endpoint;
    benchmark.replay_ticks = 0;
    size_t script_endpoint_bytes=0;
    if(script_snapshots.enabled && !nativeScriptFighter_Save(script_snapshots.endpoint,ScriptSnapshotCapacity,&script_endpoint_bytes)) {
        nativeRollbackBenchmark_Abort("could not capture endpoint script owner state");return false;
    }
    size_t animation_endpoint_bytes=0;
    if(animation_snapshots.enabled && !nativeAnimationPool_Save(animation_snapshots.endpoint,AnimationSnapshotCapacity,&animation_endpoint_bytes)) {
        nativeRollbackBenchmark_Abort("could not capture endpoint animation owner state");return false;
    }
    start = now();
    if (!nativeRollbackSession_Restore(&benchmark.session, target_frame, endpoint, next_frame) ||
        !nativeReplay_RewindToSample(target->sample)) {
        nativeRollbackBenchmark_Abort("snapshot restore or controller rewind failed");
        return false;
    }
    record->restore_ms = milliseconds(start, now());
    if(script_snapshots.enabled) {
        unsigned slot=(unsigned)(target_frame%History);size_t check_bytes=0;
        if(script_snapshots.frames[slot]!=target_frame ||
            !nativeScriptFighter_Save(script_snapshots.check,ScriptSnapshotCapacity,&check_bytes) ||
            check_bytes!=script_snapshots.sizes[slot] ||
            memcmp(script_snapshots.check,script_snapshots.history[slot],check_bytes)!=0) {
            nativeRollbackBenchmark_Abort("packed script state disagrees with broad restore");return false;
        }
        // Deliberately put the later owner's variables/clocks/bindings back,
        // then rewind only the packed owner. Other graph owners stay restored.
        start=now();
        if(!nativeScriptFighter_Load(script_snapshots.endpoint,script_endpoint_bytes) ||
            !nativeScriptFighter_Load(script_snapshots.history[slot],script_snapshots.sizes[slot])) {
            nativeRollbackBenchmark_Abort("packed script owner restore failed");return false;
        }
        script_snapshots.load_ms+=milliseconds(start,now());
        if(!nativeScriptFighter_Save(script_snapshots.check,ScriptSnapshotCapacity,&check_bytes) ||
            check_bytes!=script_snapshots.sizes[slot] ||
            memcmp(script_snapshots.check,script_snapshots.history[slot],check_bytes)!=0) {
            nativeRollbackBenchmark_Abort("packed script owner roundtrip changed declared state");return false;
        }
        ++script_snapshots.verified;
    }
    if(animation_snapshots.enabled) {
        unsigned slot=(unsigned)(target_frame%History);size_t check_bytes=0;
        if(animation_snapshots.frames[slot]!=target_frame ||
            !nativeAnimationPool_Save(animation_snapshots.check,AnimationSnapshotCapacity,&check_bytes) ||
            check_bytes!=animation_snapshots.sizes[slot] ||
            memcmp(animation_snapshots.check,animation_snapshots.history[slot],check_bytes)!=0) {
            nativeRollbackBenchmark_Abort("packed animation state disagrees with broad restore");return false;
        }
        start=now();
        if(!nativeAnimationPool_Load(animation_snapshots.endpoint,animation_endpoint_bytes) ||
            !nativeAnimationPool_Load(animation_snapshots.history[slot],animation_snapshots.sizes[slot])) {
            nativeRollbackBenchmark_Abort("packed animation owner restore failed");return false;
        }
        animation_snapshots.load_ms+=milliseconds(start,now());
        if(!nativeAnimationPool_Save(animation_snapshots.check,AnimationSnapshotCapacity,&check_bytes) ||
            check_bytes!=animation_snapshots.sizes[slot] ||
            memcmp(animation_snapshots.check,animation_snapshots.history[slot],check_bytes)!=0) {
            nativeRollbackBenchmark_Abort("packed animation owner roundtrip changed state");return false;
        }
        ++animation_snapshots.verified;
    }
    benchmark.replay_begin = now();
    return true;
}

static int compare_double(const void* a, const void* b)
{
    double x = *(const double*)a, y = *(const double*)b;
    return (x > y) - (x < y);
}

static void report(void)
{
    if(script_snapshots.enabled)OSReport("[script-snapshot] verified=%u peak_bytes=%u capture_total_ms=%.6f double_load_total_ms=%.6f scope=script_owner_only\n",
        script_snapshots.verified,script_snapshots.peak_bytes,script_snapshots.save_ms,script_snapshots.load_ms);
    if(animation_snapshots.enabled)OSReport("[animation-snapshot] verified=%u peak_bytes=%u capture_total_ms=%.6f double_load_total_ms=%.6f scope=animation_owners_only\n",
        animation_snapshots.verified,animation_snapshots.peak_bytes,animation_snapshots.save_ms,animation_snapshots.load_ms);
    double totals[Windows], sum = 0, restore = 0, replay = 0, forward = 0;
    double validation = 0, references = 0;
    unsigned missed = 0;
    for (unsigned i = 0; i < Windows; ++i) {
        const BenchmarkWindow* r = &benchmark.records[i];
        totals[i] = r->restore_ms + r->replay_ms + r->forward_ms;
        sum += totals[i]; restore += r->restore_ms; replay += r->replay_ms;
        forward += r->forward_ms; validation += r->validation_ms;
        references += r->reference_hash_ms;
        missed += totals[i] > 1000.0 / 60.0;
        OSReport("[rollback-benchmark] window=%u endpoint=%llu actors=8 replay_ticks=%u "
                 "restore_ms=%.6f replay_ms=%.6f forward_ms=%.6f total_ms=%.6f "
                 "replay_simulation_ms=%.6f replay_draw_ms=%.6f "
                 "forward_simulation_ms=%.6f forward_draw_ms=%.6f "
                 "replay_snapshot_begin_ms=%.6f replay_session_end_ms=%.6f replay_input_ms=%.6f "
                 "forward_snapshot_begin_ms=%.6f forward_session_end_ms=%.6f forward_input_ms=%.6f "
                 "replay_dirty_pages=%u forward_dirty_pages=%u "
                 "replay_input_polls=%u replay_input_waits=%u forward_input_polls=%u forward_input_waits=%u "
                 "validation_ms=%.6f reference_hash_ms=%.6f\n",
            i, (unsigned long long)r->endpoint, Depth, r->restore_ms, r->replay_ms,
            r->forward_ms, totals[i], r->replay_simulation_ms, r->replay_draw_ms,
            r->forward_simulation_ms, r->forward_draw_ms,
            r->replay_snapshot_begin_ms, r->replay_session_end_ms, r->replay_input_ms,
            r->forward_snapshot_begin_ms, r->forward_session_end_ms, r->forward_input_ms,
            r->replay_dirty_pages, r->forward_dirty_pages,
            r->replay_input_polls, r->replay_input_waits, r->forward_input_polls, r->forward_input_waits,
            r->validation_ms,
            r->reference_hash_ms);
        OSReport("[interpreter-evaluation] endpoint=%llu semantic=%016llx\n",
            (unsigned long long)r->endpoint, (unsigned long long)r->semantic_hash);
    }
    qsort(totals, Windows, sizeof(totals[0]), compare_double);
    OSReport("[rollback-benchmark] PASS windows=%u verified=%u actors=8 depth=%u "
             "mean_ms=%.6f p50_ms=%.6f p95_ms=%.6f p99_ms=%.6f max_ms=%.6f "
             "deadline_misses=%u restore_mean_ms=%.6f replay_mean_ms=%.6f "
             "forward_mean_ms=%.6f validation_mean_ms=%.6f reference_hash_mean_ms=%.6f\n",
        Windows, benchmark.completed, Depth, sum/Windows, totals[31], totals[60],
        totals[63], totals[63], missed, restore/Windows, replay/Windows,
        forward/Windows, validation/Windows, references/Windows);
}

bool nativeRollbackBenchmark_EndFrame(uint64_t frame, uint64_t* next_frame)
{
    *next_frame = frame + 1;
    bool replaying = nativeRollbackBenchmark_IsResimulating();
    uint32_t dirty_pages = nativeRollbackPages_DirtyPageCount();
    uint64_t session_end_begin = now();
    NativeRollbackSessionStep step =
        nativeRollbackSession_EndFrame(&benchmark.session, frame, next_frame);
    uint64_t end = now();
    double session_end_ms = milliseconds(session_end_begin, end);
    if (benchmark.failed || step == NativeRollbackSession_Failed ||
        step == NativeRollbackSession_Restored) {
        nativeRollbackBenchmark_Abort("snapshot/audio/input session failed during benchmark tick");
        return false;
    }
    BenchmarkWindow* record = &benchmark.records[benchmark.completed];
    if (replaying || benchmark.awaiting_forward) {
        if (!validate_actors(false)) {
            nativeRollbackBenchmark_Abort("eight live paired CPU actors lost during drawing");
            return false;
        }
    }
    if (replaying) {
        ++benchmark.replay_ticks;
        record->replay_simulation_ms += benchmark.tick_simulation_ms;
        record->replay_draw_ms += benchmark.tick_draw_ms;
        record->replay_snapshot_begin_ms += benchmark.tick_snapshot_begin_ms;
        record->replay_session_end_ms += session_end_ms;
        record->replay_input_ms += benchmark.tick_input_ms;
        record->replay_dirty_pages += dirty_pages;
        record->replay_input_polls += benchmark.tick_input_polls;
        record->replay_input_waits += benchmark.tick_input_waits;
        if (step == NativeRollbackSession_CaughtUp) {
            record->replay_ms = milliseconds(benchmark.replay_begin, end);
            uint64_t start = now();
            uint64_t hash = nativeRollbackRegions_Hash(&benchmark.session.regions);
            uint64_t normalized = nativeRollbackHash_AddressNormalized(&benchmark.session.regions);
            uint64_t semantic = nativeReplay_StateDigest(*next_frame);
            record->validation_ms = milliseconds(start, now());
            if (*next_frame != benchmark.endpoint_frame || benchmark.replay_ticks != Depth ||
                hash != benchmark.endpoint_hash || normalized != benchmark.normalized_hash ||
                semantic != record->semantic_hash) {
                nativeRollbackBenchmark_Abort("15-tick replay did not reproduce complete endpoint state");
                return false;
            }
            benchmark.awaiting_forward = true;
        }
        return true;
    }
    if (benchmark.awaiting_forward) {
        record->forward_ms = milliseconds(benchmark.tick_begin, end);
        record->forward_simulation_ms = benchmark.tick_simulation_ms;
        record->forward_draw_ms = benchmark.tick_draw_ms;
        record->forward_snapshot_begin_ms = benchmark.tick_snapshot_begin_ms;
        record->forward_session_end_ms = session_end_ms;
        record->forward_input_ms = benchmark.tick_input_ms;
        record->forward_dirty_pages = dirty_pages;
        record->forward_input_polls = benchmark.tick_input_polls;
        record->forward_input_waits = benchmark.tick_input_waits;
        benchmark.awaiting_forward = false;
        if (++benchmark.completed == Windows) {
            if (!evaluation_stats(false)) {
                nativeRollbackBenchmark_Abort("interpreter reported an execution error");
                return false;
            }
            benchmark.succeeded = true;
            report();
            return false;
        }
    }
    if (*next_frame >= Warmup) return restore_window(*next_frame, next_frame);
    return true;
}

void nativeRollbackBenchmark_Shutdown(void)
{
    HSD_PadSetRawFilter(NULL);
    nativeRollbackSession_Shutdown(&benchmark.session);
    if (benchmark.active && !benchmark.failed && !benchmark.succeeded)
        nativeRollbackBenchmark_Abort("match ended before 64 verified windows completed");
    benchmark.active = false;
}
bool nativeRollbackBenchmark_IsResimulating(void)
{
    return nativeRollbackSession_IsResimulating(&benchmark.session);
}
bool nativeRollbackBenchmark_HasFailed(void) { return benchmark.failed; }
#endif
