#include "native_rollback_probe.h"

#ifdef MELEE_NATIVE

#include "native_replay.h"
#include "native_rollback_hash.h"
#include "native_rollback_pages.h"
#include "native_rollback_regions.h"
#include "native_rollback_session.h"
#include <dolphin/os/OSAlloc.h>
#include <melee/ft/types.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/audio_rollback.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/rumble.h>

#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

enum {
    ROLLBACK_PROBE_HISTORY_FRAMES = 16,
    ROLLBACK_PROBE_DIRTY_PAGE_CAPACITY = 2048,
    ROLLBACK_PROBE_MINIMUM_FRAME = 240,
    ROLLBACK_PROBE_MAXIMUM_FRAME = 600,
    ROLLBACK_PROBE_DEPTH = 8,
};

typedef struct NativeRollbackProbeBoundary {
    uint64_t frame;
    uint64_t replay_sample;
    bool valid;
} NativeRollbackProbeBoundary;

typedef struct NativeRollbackProbeAuthoredObservation {
    int32_t p1_module_marker;
    uint32_t mario_fireballs;
    uint32_t p1_owned_fireballs;
    uint32_t p2_owned_fireballs;
    uint32_t mario_capes;
    uint32_t p2_owned_capes;
    uint32_t custom_article_mismatches;
} NativeRollbackProbeAuthoredObservation;

static struct NativeRollbackProbe {
    NativeRollbackSession session;
    NativeRollbackProbeBoundary boundaries[ROLLBACK_PROBE_HISTORY_FRAMES];
    uint64_t endpoint_hash;
    uint64_t endpoint_normalized_hash;
    uint64_t target_hash;
    uint64_t endpoint_region_hashes[NATIVE_ROLLBACK_REGION_CAPACITY];
    uint64_t endpoint_exact_hashes[NATIVE_ROLLBACK_EXACT_REGION_CAPACITY];
    uint64_t corrected_frame_hashes[ROLLBACK_PROBE_HISTORY_FRAMES];
    NativeRollbackProbeAuthoredObservation
        first_corrected_observations[ROLLBACK_PROBE_HISTORY_FRAMES];
    NativeRollbackProbeAuthoredObservation
        second_corrected_observations[ROLLBACK_PROBE_HISTORY_FRAMES];
    NativeRollbackProbeAuthoredObservation last_original_observation;
    NativeRollbackProbeAuthoredObservation target_observation;
    uint8_t* endpoint_state;
    size_t endpoint_state_size;
    uint64_t endpoint_frame;
    uint64_t current_frame;
    uint64_t correction_target_frame;
    uint32_t requested_correction_frame;
    uint64_t correction_sample;
    PADStatus correction_input;
    uint32_t resimulation_pass;
    bool active;
    bool correction_mode;
    bool correction_pending;
    bool correction_submitted;
    bool reported_corrected_frame_divergence;
    bool original_observation_valid;
    bool succeeded;
    bool failed;
} probe;

static uintptr_t diverged_addresses[16];
static uint32_t diverged_address_count;

static NativeRollbackProbeAuthoredObservation observe_authored_state(void)
{
    NativeRollbackProbeAuthoredObservation result = { .p1_module_marker = -1 };
    if (HSD_GObjPLinkHead == NULL) return result;
    for (HSD_GObj* gobj = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];
         gobj != NULL; gobj = gobj->next)
    {
        const Fighter* fighter = gobj->user_data;
        if (fighter != NULL && fighter->player_id == 0 &&
            fighter->kind >= Ft_Kind_CustomFirst) {
            result.p1_module_marker = fighter->native_module_state[0];
            break;
        }
    }
    for (HSD_GObj* gobj = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];
         gobj != NULL; gobj = gobj->next)
    {
        const Item* item = gobj->user_data;
        if (item != NULL && item->kind == It_Kind_Mario_Cape) {
            ++result.mario_capes;
            const HSD_GObj* owner = item->owner;
            if (owner != NULL && owner->p_link == HSD_GOBJ_PLINK_FIGHTER &&
                owner->user_data != NULL) {
                const Fighter* fighter = owner->user_data;
                if (fighter->player_id == 1 &&
                    fighter->kind >= Ft_Kind_CustomFirst) {
                    ++result.p2_owned_capes;
                }
                if (fighter->kind >= Ft_Kind_CustomFirst &&
                    fighter->behavior_kind == Ft_Kind_Mario) {
                    Article* expected =
                        fighter->ft_data != NULL &&
                                fighter->ft_data->x48_items != NULL
                            ? fighter->ft_data->x48_items[2] : NULL;
                    if (expected == NULL ||
                        item->xC4_article_data != expected ||
                        fighter->u.mr.x223C_capeGObj != gobj)
                        ++result.custom_article_mismatches;
                }
            }
        }
        if (item != NULL && item->kind == It_Kind_Mario_Fire) {
            ++result.mario_fireballs;
            const HSD_GObj* owner = item->owner;
            if (owner != NULL && owner->p_link == HSD_GOBJ_PLINK_FIGHTER &&
                owner->user_data != NULL)
            {
                const Fighter* fighter = owner->user_data;
                if (fighter->player_id == 0 &&
                    fighter->kind >= Ft_Kind_CustomFirst) {
                    ++result.p1_owned_fireballs;
                } else if (fighter->player_id == 1 &&
                           fighter->kind >= Ft_Kind_CustomFirst) {
                    ++result.p2_owned_fireballs;
                }
                if (fighter->kind >= Ft_Kind_CustomFirst &&
                    fighter->behavior_kind == Ft_Kind_Mario)
                {
                    // A reflection can change owner without replacing the
                    // projectile's article; report this rather than failing
                    // the generic probe on a legitimate ownership transfer.
                    Article* expected =
                        fighter->ft_data != NULL &&
                                fighter->ft_data->x48_items != NULL
                            ? fighter->ft_data->x48_items[0]
                            : NULL;
                    if (expected == NULL ||
                        item->xC4_article_data != expected) {
                        ++result.custom_article_mismatches;
                    }
                }
            }
        }
    }
    return result;
}

static bool same_authored_observation(
    NativeRollbackProbeAuthoredObservation first,
    NativeRollbackProbeAuthoredObservation second)
{
    return first.p1_module_marker == second.p1_module_marker &&
           first.mario_fireballs == second.mario_fireballs &&
           first.p1_owned_fireballs == second.p1_owned_fireballs &&
           first.p2_owned_fireballs == second.p2_owned_fireballs &&
           first.mario_capes == second.mario_capes &&
           first.p2_owned_capes == second.p2_owned_capes &&
           first.custom_article_mismatches ==
               second.custom_article_mismatches;
}

static void report_diverged_allocation(void* memory, u32 size)
{
    uintptr_t start = (uintptr_t) memory;
    uintptr_t end = start + size;
    for (uint32_t i = 0; i < diverged_address_count; ++i) {
        uintptr_t address = diverged_addresses[i];
        if (address >= start && address < end) {
            OSReport("[rollback-probe]   %p belongs to allocation %p "
                     "(%u bytes, +0x%zx).\n",
                     (void*) address, memory, size,
                     (size_t) (address - start));
        }
    }
}

static uint64_t hash_bytes(const void* memory, size_t size)
{
    const uint8_t* bytes = memory;
    uint64_t hash = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static void save_endpoint_region_hashes(void)
{
    uint8_t* snapshot = probe.endpoint_state;
    for (uint32_t i = 0; i < probe.session.regions.count; ++i) {
        probe.endpoint_region_hashes[i] =
            hash_bytes(probe.session.regions.regions[i].memory,
                       probe.session.regions.regions[i].size);
        memcpy(snapshot, probe.session.regions.regions[i].memory,
               probe.session.regions.regions[i].size);
        snapshot += probe.session.regions.regions[i].size;
    }
    for (uint32_t i = 0; i < probe.session.regions.exact_count; ++i) {
        probe.endpoint_exact_hashes[i] =
            hash_bytes(probe.session.regions.exact_regions[i].memory,
                       probe.session.regions.exact_regions[i].size);
        memcpy(snapshot, probe.session.regions.exact_regions[i].memory,
               probe.session.regions.exact_regions[i].size);
        snapshot += probe.session.regions.exact_regions[i].size;
    }
}

static void report_normalized_region_hashes(void)
{
    for (uint32_t i = 0; i < probe.session.regions.count; ++i) {
        OSReport("[rollback-probe] Address-normalized paged region %u at %p: "
                 "%016llx "
                 "(%zu bytes).\n",
                 i, probe.session.regions.regions[i].memory,
                 (unsigned long long)
                     nativeRollbackHash_AddressNormalizedPagedRegion(
                         &probe.session.regions, i),
                 probe.session.regions.regions[i].size);
    }
    for (uint32_t i = 0; i < probe.session.regions.exact_count; ++i) {
        OSReport("[rollback-probe] Address-normalized exact region %u at %p: "
                 "%016llx "
                 "(%zu bytes).\n",
                 i, probe.session.regions.exact_regions[i].memory,
                 (unsigned long long)
                     nativeRollbackHash_AddressNormalizedExactRegion(
                         &probe.session.regions, i),
                 probe.session.regions.exact_regions[i].size);
    }
    for (uint32_t i = 0; i < probe.session.regions.identity_count; ++i) {
        OSReport("[rollback-probe] Asset identity %u at %p: %016llx "
                 "(%zu bytes).\n",
                 i, probe.session.regions.identity_regions[i].memory,
                 (unsigned long long) probe.session.regions.identity_regions[i]
                     .identity,
                 probe.session.regions.identity_regions[i].size);
    }
}

static void write_normalized_manifest(void)
{
    char path[MAX_PATH];
    DWORD path_size = GetEnvironmentVariableA(
        "FREAK_MELEE_ROLLBACK_HASH_MANIFEST", path, ARRAY_SIZE(path));
    if (path_size == 0 || path_size >= ARRAY_SIZE(path)) {
        return;
    }
    FILE* file = fopen(path, "wb");
    if (file == NULL) {
        OSReport("[rollback-probe] Could not write address-normalized "
                 "manifest %s.\n",
                 path);
        return;
    }
    char word_region_text[16];
    char word_offset_text[32];
    DWORD word_region_size = GetEnvironmentVariableA(
        "FREAK_MELEE_ROLLBACK_HASH_WORD_REGION", word_region_text,
        ARRAY_SIZE(word_region_text));
    DWORD word_offset_size = GetEnvironmentVariableA(
        "FREAK_MELEE_ROLLBACK_HASH_WORD_OFFSET", word_offset_text,
        ARRAY_SIZE(word_offset_text));
    bool write_words = word_region_size != 0 &&
                       word_region_size < ARRAY_SIZE(word_region_text) &&
                       word_offset_size != 0 &&
                       word_offset_size < ARRAY_SIZE(word_offset_text);
    uint32_t word_region =
        write_words ? (uint32_t) strtoul(word_region_text, NULL, 0) : 0;
    size_t word_offset =
        write_words ? (size_t) strtoull(word_offset_text, NULL, 0) : 0;
    size_t page_size = nativeRollbackPages_PageSize();
    for (uint32_t region = 0; region < probe.session.regions.count; ++region) {
        const uint8_t* memory = probe.session.regions.regions[region].memory;
        size_t region_size = probe.session.regions.regions[region].size;
        for (size_t offset = 0; offset < region_size; offset += page_size) {
            size_t size = region_size - offset;
            if (size > page_size) {
                size = page_size;
            }
            uint64_t hash = nativeRollbackHash_AddressNormalizedRange(
                &probe.session.regions, memory + offset, size);
            fprintf(file, "P %u %zx %zx %016llx\n", region, offset, size,
                    (unsigned long long) hash);
            if (write_words && region == word_region && offset == word_offset)
            {
                for (size_t word = 0; word < size; word += sizeof(uintptr_t)) {
                    uintptr_t raw = 0;
                    memcpy(&raw, memory + offset + word, sizeof(raw));
                    uint64_t word_hash =
                        nativeRollbackHash_AddressNormalizedRange(
                            &probe.session.regions, memory + offset + word,
                            sizeof(raw));
                    fprintf(file, "W %u %zx %016llx %016llx\n", region,
                            offset + word, (unsigned long long) word_hash,
                            (unsigned long long) raw);
                }
            }
        }
    }
    for (uint32_t region = 0; region < probe.session.regions.exact_count;
         ++region)
    {
        const uint8_t* memory =
            probe.session.regions.exact_regions[region].memory;
        size_t region_size = probe.session.regions.exact_regions[region].size;
        for (size_t offset = 0; offset < region_size;
             offset += sizeof(uintptr_t))
        {
            size_t size = region_size - offset;
            if (size > sizeof(uintptr_t)) {
                size = sizeof(uintptr_t);
            }
            uint64_t hash = nativeRollbackHash_AddressNormalizedRange(
                &probe.session.regions, memory + offset, size);
            fprintf(file, "E %u %zx %zx %016llx\n", region, offset, size,
                    (unsigned long long) hash);
        }
    }
    fclose(file);
    OSReport("[rollback-probe] Wrote address-normalized manifest %s.\n", path);
}

static void report_diverged_regions(void)
{
    diverged_address_count = 0;
    const uint8_t* snapshot = probe.endpoint_state;
    size_t page_size = nativeRollbackPages_PageSize();
    for (uint32_t i = 0; i < probe.session.regions.count; ++i) {
        uint64_t replayed = hash_bytes(probe.session.regions.regions[i].memory,
                                       probe.session.regions.regions[i].size);
        if (replayed != probe.endpoint_region_hashes[i]) {
            OSReport("[rollback-probe] Paged region %u (%p, %zu bytes) "
                     "differs: %016llx != %016llx.\n",
                     i, probe.session.regions.regions[i].memory,
                     probe.session.regions.regions[i].size,
                     (unsigned long long) probe.endpoint_region_hashes[i],
                     (unsigned long long) replayed);
            const uint8_t* current = probe.session.regions.regions[i].memory;
            size_t differing_bytes = 0;
            size_t differing_pages = 0;
            size_t previous_page = SIZE_MAX;
            uint32_t reported = 0;
            for (size_t at = 0; at < probe.session.regions.regions[i].size;
                 ++at)
            {
                if (snapshot[at] == current[at]) {
                    continue;
                }
                ++differing_bytes;
                size_t page = at / page_size;
                if (page != previous_page) {
                    ++differing_pages;
                    previous_page = page;
                }
                if (reported < 16) {
                    OSReport("[rollback-probe]   %p + 0x%zx: %02x != %02x\n",
                             current, at, snapshot[at], current[at]);
                    ++reported;
                    if (diverged_address_count <
                        ARRAY_SIZE(diverged_addresses))
                    {
                        diverged_addresses[diverged_address_count++] =
                            (uintptr_t) current + at;
                    }
                }
            }
            OSReport("[rollback-probe]   %zu bytes differ across %zu pages.\n",
                     differing_bytes, differing_pages);
        }
        snapshot += probe.session.regions.regions[i].size;
    }
    for (uint32_t i = 0; i < probe.session.regions.exact_count; ++i) {
        uint64_t replayed =
            hash_bytes(probe.session.regions.exact_regions[i].memory,
                       probe.session.regions.exact_regions[i].size);
        if (replayed != probe.endpoint_exact_hashes[i]) {
            OSReport("[rollback-probe] Exact region %u (%p, %zu bytes) "
                     "differs: %016llx != %016llx.\n",
                     i, probe.session.regions.exact_regions[i].memory,
                     probe.session.regions.exact_regions[i].size,
                     (unsigned long long) probe.endpoint_exact_hashes[i],
                     (unsigned long long) replayed);
        }
        snapshot += probe.session.regions.exact_regions[i].size;
    }
    OSVisitAllocated(report_diverged_allocation);
}

static void fail(const char* reason)
{
    if (reason == NULL) {
        reason = "rollback session failed without a reason";
    }
    if (!probe.failed) {
        OSReport("[rollback-probe] FAILED: %s\n", reason);
    }
    probe.failed = true;
}

static void filter_inputs(PADStatus pads[4])
{
    if (!probe.active || probe.failed) {
        return;
    }
    uint64_t sample_count = nativeReplay_SampleCount();
    if (sample_count == 0) {
        fail("input provider did not advance its sample index");
        return;
    }
    uint64_t sample = sample_count - 1;
    uint64_t frame = probe.current_frame;
    PADStatus resolved[4];
    bool available;
    if (nativeRollbackSession_IsResimulating(&probe.session)) {
        available = nativeRollbackSession_ResolveInputs(&probe.session, frame,
                                                        pads, resolved);
    } else if (!probe.correction_mode) {
        available = nativeRollbackSession_ResolveInputs(&probe.session, frame,
                                                        pads, resolved);
    } else if (probe.current_frame == probe.requested_correction_frame &&
               !probe.correction_pending)
    {
        available = nativeRollbackSession_ResolveInputs(&probe.session, frame,
                                                        pads, resolved);
        if (available) {
            probe.correction_target_frame = probe.current_frame;
            probe.correction_sample = sample;
            probe.correction_input = resolved[0];
            probe.correction_input.button ^= PAD_BUTTON_A;
            probe.correction_pending = true;
            nativeReplay_SetTimelineValidation(false);
        }
    } else {
        available = true;
        if (probe.correction_pending && !probe.correction_submitted &&
            probe.current_frame ==
                probe.correction_target_frame + ROLLBACK_PROBE_DEPTH - 1)
        {
            available = nativeRollbackSession_SubmitRemote(
                &probe.session, probe.correction_target_frame, 0,
                &probe.correction_input);
            probe.correction_submitted = available;
        }
        if (available) {
            available = nativeRollbackSession_SubmitRemote(&probe.session,
                                                           frame, 0, &pads[0]);
        }
        if (available) {
            available = nativeRollbackSession_ResolveInputs(
                &probe.session, frame, pads, resolved);
        }
    }
    if (!available) {
        fail("input history could not provide the current sample");
        return;
    }
    memcpy(pads, resolved, sizeof(resolved));
}

bool nativeRollbackProbe_Init(bool correction_mode, uint32_t correction_frame)
{
    probe = (struct NativeRollbackProbe){ 0 };
    probe.correction_mode = correction_mode;
    probe.requested_correction_frame = correction_frame;
    if (!nativeRollbackSession_Init(&probe.session,
                                    correction_mode ? 0xE : 0xF,
                                    ROLLBACK_PROBE_DIRTY_PAGE_CAPACITY))
    {
        fail(nativeRollbackSession_FailureReason(&probe.session));
        return false;
    }
    probe.endpoint_state_size = probe.session.regions.tracked_bytes;
    probe.endpoint_state =
        VirtualAlloc(NULL, probe.endpoint_state_size, MEM_RESERVE | MEM_COMMIT,
                     PAGE_READWRITE);
    if (probe.endpoint_state == NULL) {
        nativeRollbackSession_Shutdown(&probe.session);
        fail("could not allocate endpoint comparison storage");
        return false;
    }
    probe.active = true;
    HSD_PadSetRawFilter(filter_inputs);
    if (correction_mode) {
        OSReport(
            "[rollback-probe] Tracking %zu bytes; will inject a late "
            "P1 input at frame %u and reproduce its corrected endpoint.\n",
            probe.session.regions.tracked_bytes, correction_frame);
    } else {
        OSReport("[rollback-probe] Tracking %zu bytes; will restore %u frames "
                 "after frame %u when the window includes a sound start.\n",
                 probe.session.regions.tracked_bytes, ROLLBACK_PROBE_DEPTH,
                 ROLLBACK_PROBE_MINIMUM_FRAME);
    }
    return true;
}

void nativeRollbackProbe_Shutdown(void)
{
    HSD_PadSetRawFilter(NULL);
    nativeReplay_SetTimelineValidation(true);
    nativeRollbackSession_Shutdown(&probe.session);
    if (probe.endpoint_state != NULL) {
        VirtualFree(probe.endpoint_state, 0, MEM_RELEASE);
        probe.endpoint_state = NULL;
    }
    if (probe.active && !probe.succeeded && !probe.failed) {
        fail("match ended before the restore-and-replay check completed");
    }
    probe.active = false;
}

bool nativeRollbackProbe_BeginFrame(uint64_t frame)
{
    if (!probe.active || probe.failed) {
        return false;
    }
    probe.current_frame = frame;
    NativeRollbackProbeBoundary* boundary =
        &probe.boundaries[frame % ROLLBACK_PROBE_HISTORY_FRAMES];
    *boundary = (NativeRollbackProbeBoundary){
        .frame = frame,
        .replay_sample = nativeReplay_SampleCount(),
        .valid = true,
    };
    if (!nativeRollbackSession_BeginFrame(&probe.session, frame)) {
        fail(nativeRollbackSession_FailureReason(&probe.session));
        return false;
    }
    return true;
}

static bool restore_timeline(uint64_t target_frame, uint64_t current_frame,
                             uint64_t replay_sample)
{
    uint64_t next_frame;
    if (!nativeRollbackSession_Restore(&probe.session, target_frame,
                                       current_frame, &next_frame))
    {
        fail(nativeRollbackSession_FailureReason(&probe.session));
        return false;
    }
    if (!nativeReplay_RewindToSample(replay_sample)) {
        fail("could not rewind recorded controller input");
        return false;
    }
    return next_frame == target_frame;
}

static NativeRollbackProbeStep
end_correction_probe(NativeRollbackSessionStep session_step,
                     uint64_t current_frame, uint64_t* next_frame)
{
    if (session_step == NativeRollbackSession_Failed) {
        fail(nativeRollbackSession_FailureReason(&probe.session));
        return NativeRollbackProbe_Failed;
    }
    if (session_step == NativeRollbackSession_Restored) {
        NativeRollbackProbeBoundary* target =
            &probe.boundaries[probe.correction_target_frame %
                              ROLLBACK_PROBE_HISTORY_FRAMES];
        if (!probe.correction_pending || !probe.correction_submitted ||
            probe.resimulation_pass != 0 ||
            *next_frame != probe.correction_target_frame || !target->valid ||
            target->frame != probe.correction_target_frame ||
            target->replay_sample != probe.correction_sample)
        {
            fail("input history reported an unexpected correction");
            return NativeRollbackProbe_Failed;
        }
        if (!nativeReplay_RewindToSample(probe.correction_sample)) {
            fail("could not rewind recorded controller input");
            return NativeRollbackProbe_Failed;
        }
        probe.endpoint_frame = current_frame;
        probe.target_hash = nativeRollbackRegions_Hash(&probe.session.regions);
        probe.target_observation = observe_authored_state();
        probe.resimulation_pass = 1;
        OSReport("[rollback-probe] P1 sample %llu arrived late; restored "
                 "frame %llu from frame %llu.\n",
                 (unsigned long long) probe.correction_sample,
                 (unsigned long long) probe.correction_target_frame,
                 (unsigned long long) current_frame);
        return NativeRollbackProbe_Restored;
    }
    if (probe.resimulation_pass == 0) {
        if (current_frame >= ROLLBACK_PROBE_MAXIMUM_FRAME) {
            fail("late input did not request a correction");
            return NativeRollbackProbe_Failed;
        }
        return NativeRollbackProbe_Continue;
    }

    uint64_t corrected_index =
        current_frame - probe.correction_target_frame - 1;
    if (corrected_index < ARRAY_SIZE(probe.corrected_frame_hashes)) {
        uint64_t frame_hash =
            nativeRollbackRegions_Hash(&probe.session.regions);
        if (probe.resimulation_pass == 1) {
            probe.corrected_frame_hashes[corrected_index] = frame_hash;
        } else if (!probe.reported_corrected_frame_divergence &&
                   probe.corrected_frame_hashes[corrected_index] != frame_hash)
        {
            probe.reported_corrected_frame_divergence = true;
            OSReport("[rollback-probe] Corrected state first differs after "
                     "frame %llu: first %016llx, second %016llx.\n",
                     (unsigned long long) (current_frame - 1),
                     (unsigned long long)
                         probe.corrected_frame_hashes[corrected_index],
                     (unsigned long long) frame_hash);
        }
    }
    if (session_step != NativeRollbackSession_CaughtUp) {
        return NativeRollbackProbe_Continue;
    }
    if (current_frame != probe.endpoint_frame) {
        fail("resimulation caught up at an unexpected frame");
        return NativeRollbackProbe_Failed;
    }
    if (probe.resimulation_pass == 1) {
        probe.endpoint_hash =
            nativeRollbackRegions_Hash(&probe.session.regions);
        probe.endpoint_normalized_hash =
            nativeRollbackHash_AddressNormalized(&probe.session.regions);
        save_endpoint_region_hashes();
        if (!restore_timeline(probe.correction_target_frame, current_frame,
                              probe.correction_sample))
        {
            return NativeRollbackProbe_Failed;
        }
        uint64_t restored_hash =
            nativeRollbackRegions_Hash(&probe.session.regions);
        if (restored_hash != probe.target_hash) {
            OSReport("[rollback-probe] Target hashes differ after restore: "
                     "first %016llx, second %016llx.\n",
                     (unsigned long long) probe.target_hash,
                     (unsigned long long) restored_hash);
            fail("repeated restore produced a different target state");
            return NativeRollbackProbe_Failed;
        }
        probe.resimulation_pass = 2;
        *next_frame = probe.correction_target_frame;
        OSReport("[rollback-probe] Corrected endpoint %016llx captured; "
                 "restored it for a second full-draw replay.\n",
                 (unsigned long long) probe.endpoint_hash);
        return NativeRollbackProbe_Restored;
    }

    uint64_t replayed_hash =
        nativeRollbackRegions_Hash(&probe.session.regions);
    uint64_t replayed_normalized_hash =
        nativeRollbackHash_AddressNormalized(&probe.session.regions);
    if (replayed_hash != probe.endpoint_hash) {
        OSReport("[rollback-probe] Corrected endpoint hashes differ: first "
                 "%016llx, second %016llx.\n",
                 (unsigned long long) probe.endpoint_hash,
                 (unsigned long long) replayed_hash);
        report_diverged_regions();
        fail("corrected timeline diverged on repeated resimulation");
        return NativeRollbackProbe_Failed;
    }
    if (replayed_normalized_hash != probe.endpoint_normalized_hash) {
        fail(
            "normalized corrected endpoint digest changed within one process");
        return NativeRollbackProbe_Failed;
    }
    NativeRollbackProbeAuthoredObservation previous =
        probe.target_observation;
    uint32_t fireball_increases = 0;
    uint32_t fireball_decreases = 0;
    uint32_t cape_increases = 0;
    uint32_t cape_decreases = 0;
    uint32_t marker_changes = 0;
    for (uint32_t index = 0; index < ROLLBACK_PROBE_DEPTH; ++index) {
        NativeRollbackProbeAuthoredObservation first =
            probe.first_corrected_observations[index];
        NativeRollbackProbeAuthoredObservation second =
            probe.second_corrected_observations[index];
        if (!same_authored_observation(first, second)) {
            fail("corrected authored state or article inventory differs");
            return NativeRollbackProbe_Failed;
        }
        if (first.mario_fireballs > previous.mario_fireballs) {
            fireball_increases += first.mario_fireballs -
                                  previous.mario_fireballs;
        } else {
            fireball_decreases += previous.mario_fireballs -
                                  first.mario_fireballs;
        }
        if (first.mario_capes > previous.mario_capes) {
            cape_increases += first.mario_capes - previous.mario_capes;
        } else {
            cape_decreases += previous.mario_capes - first.mario_capes;
        }
        marker_changes += first.p1_module_marker !=
                          previous.p1_module_marker;
        previous = first;
    }
    probe.succeeded = true;
    HSD_AudioRollbackStats audio = HSD_AudioRollbackGetStats();
    OSReport("[rollback-probe] PASS: repeated corrected timeline reproduced "
             "the complete tracked state (%016llx).\n",
             (unsigned long long) replayed_hash);
    OSReport("[rollback-probe] Address-normalized endpoint: %016llx.\n",
             (unsigned long long) replayed_normalized_hash);
    OSReport("[rollback-probe] Authored P1 marker %d -> %d (%u changes); "
             "Mario fireballs %u -> %u (+%u/-%u across corrected frames); "
             "P1-owned %u -> %u, P2-owned %u -> %u; "
             "Mario capes %u -> %u (+%u/-%u), P2-owned %u -> %u; "
             "custom article mismatches %u -> %u.\n",
             probe.target_observation.p1_module_marker,
             previous.p1_module_marker, marker_changes,
             probe.target_observation.mario_fireballs,
             previous.mario_fireballs, fireball_increases,
             fireball_decreases, probe.target_observation.p1_owned_fireballs,
             previous.p1_owned_fireballs,
             probe.target_observation.p2_owned_fireballs,
             previous.p2_owned_fireballs,
             probe.target_observation.mario_capes,
             previous.mario_capes, cape_increases, cape_decreases,
             probe.target_observation.p2_owned_capes,
             previous.p2_owned_capes,
             probe.target_observation.custom_article_mismatches,
             previous.custom_article_mismatches);
    report_normalized_region_hashes();
    write_normalized_manifest();
    OSReport(
        "[rollback-probe] Audio starts: %u reused, %u new, %u canceled.\n",
        audio.reused_starts, audio.new_starts, audio.canceled_starts);
    return NativeRollbackProbe_Succeeded;
}

NativeRollbackProbeStep nativeRollbackProbe_EndFrame(uint64_t frame,
                                                     uint64_t* next_frame)
{
    uint64_t current_frame = frame + 1;
    if (probe.correction_mode) {
        NativeRollbackProbeAuthoredObservation observation =
            observe_authored_state();
        if (probe.resimulation_pass == 0) {
            if (!probe.original_observation_valid ||
                !same_authored_observation(
                    observation, probe.last_original_observation))
            {
                OSReport("[rollback-probe] Original frame %llu: "
                         "P1 module marker %d, Mario fireballs %u "
                         "(%u owned by P1, %u by P2); "
                         "Mario capes %u (%u owned by P2); "
                         "custom article mismatches %u.\n",
                         (unsigned long long) frame,
                         observation.p1_module_marker,
                         observation.mario_fireballs,
                         observation.p1_owned_fireballs,
                         observation.p2_owned_fireballs,
                         observation.mario_capes,
                         observation.p2_owned_capes,
                         observation.custom_article_mismatches);
                probe.last_original_observation = observation;
                probe.original_observation_valid = true;
            }
        } else if (frame >= probe.correction_target_frame &&
                   frame - probe.correction_target_frame <
                       ROLLBACK_PROBE_DEPTH)
        {
            uint32_t index =
                (uint32_t) (frame - probe.correction_target_frame);
            if (probe.resimulation_pass == 1) {
                probe.first_corrected_observations[index] = observation;
            } else {
                probe.second_corrected_observations[index] = observation;
            }
        }
    }
    NativeRollbackSessionStep session_step =
        nativeRollbackSession_EndFrame(&probe.session, frame, next_frame);
    if (probe.correction_mode) {
        return end_correction_probe(session_step, current_frame, next_frame);
    }
    if (session_step == NativeRollbackSession_Failed) {
        fail(nativeRollbackSession_FailureReason(&probe.session));
        return NativeRollbackProbe_Failed;
    }
    if (session_step == NativeRollbackSession_Restored) {
        fail("all-local rollback probe received an input correction");
        return NativeRollbackProbe_Failed;
    }
    bool recent_audio_start = false;
    if (session_step == NativeRollbackSession_Continue &&
        !nativeRollbackSession_IsResimulating(&probe.session) &&
        current_frame >= ROLLBACK_PROBE_MINIMUM_FRAME)
    {
        for (uint64_t at = current_frame - ROLLBACK_PROBE_DEPTH;
             at < current_frame; ++at)
        {
            recent_audio_start |= HSD_AudioRollbackFrameHasStarts(at);
        }
    }
    if (session_step == NativeRollbackSession_Continue &&
        !nativeRollbackSession_IsResimulating(&probe.session) &&
        ((current_frame >= ROLLBACK_PROBE_MINIMUM_FRAME &&
          recent_audio_start) ||
         current_frame == ROLLBACK_PROBE_MAXIMUM_FRAME))
    {
        uint64_t target_frame = current_frame - ROLLBACK_PROBE_DEPTH;
        NativeRollbackProbeBoundary* target =
            &probe.boundaries[target_frame % ROLLBACK_PROBE_HISTORY_FRAMES];
        if (!target->valid || target->frame != target_frame) {
            fail("target input boundary fell out of history");
            return NativeRollbackProbe_Failed;
        }
        probe.endpoint_frame = current_frame;
        probe.endpoint_hash =
            nativeRollbackRegions_Hash(&probe.session.regions);
        probe.endpoint_normalized_hash =
            nativeRollbackHash_AddressNormalized(&probe.session.regions);
        save_endpoint_region_hashes();
        if (!restore_timeline(target_frame, current_frame,
                              target->replay_sample))
        {
            return NativeRollbackProbe_Failed;
        }
        *next_frame = target_frame;
        OSReport("[rollback-probe] Restored frame %llu from frame %llu; "
                 "replaying without presentation.\n",
                 (unsigned long long) target_frame,
                 (unsigned long long) current_frame);
        return NativeRollbackProbe_Restored;
    }

    if (session_step == NativeRollbackSession_CaughtUp) {
        if (current_frame != probe.endpoint_frame) {
            fail("resimulation caught up at an unexpected frame");
            return NativeRollbackProbe_Failed;
        }
        uint64_t replayed_hash =
            nativeRollbackRegions_Hash(&probe.session.regions);
        uint64_t replayed_normalized_hash =
            nativeRollbackHash_AddressNormalized(&probe.session.regions);
        if (replayed_hash != probe.endpoint_hash) {
            OSReport("[rollback-probe] Endpoint hashes differ: original "
                     "%016llx, replayed %016llx.\n",
                     (unsigned long long) probe.endpoint_hash,
                     (unsigned long long) replayed_hash);
            report_diverged_regions();
            fail("tracked state diverged after resimulation");
            return NativeRollbackProbe_Failed;
        }
        if (replayed_normalized_hash != probe.endpoint_normalized_hash) {
            fail("normalized endpoint digest changed within one process");
            return NativeRollbackProbe_Failed;
        }
        probe.succeeded = true;
        HSD_AudioRollbackStats audio = HSD_AudioRollbackGetStats();
        OSReport("[rollback-probe] PASS: %u restored frames reproduced "
                 "the complete tracked state (%016llx).\n",
                 ROLLBACK_PROBE_DEPTH, (unsigned long long) replayed_hash);
        OSReport("[rollback-probe] Address-normalized endpoint: %016llx.\n",
                 (unsigned long long) replayed_normalized_hash);
        report_normalized_region_hashes();
        write_normalized_manifest();
        OSReport("[rollback-probe] Audio starts: %u reused, %u new, %u "
                 "canceled.\n",
                 audio.reused_starts, audio.new_starts, audio.canceled_starts);
        return NativeRollbackProbe_Succeeded;
    }
    return NativeRollbackProbe_Continue;
}

void nativeRollbackProbe_Abort(const char* reason)
{
    nativeRollbackSession_Abort(&probe.session, reason);
    fail(reason);
}

bool nativeRollbackProbe_IsResimulating(void)
{
    return nativeRollbackSession_IsResimulating(&probe.session);
}

bool nativeRollbackProbe_HasFailed(void)
{
    return probe.failed || nativeRollbackSession_HasFailed(&probe.session);
}

#endif
