#include "native_replay.h"

#ifdef MELEE_NATIVE

#include "gmmain.h"
#include "native_match_fixture.h"

#include <dolphin/pad.h>
#include <sysdolphin/baselib/random.h>

#include <stdio.h>
#include <string.h>

typedef struct NativeReplayHeader {
    char magic[8];
    uint64_t match_signature;
    uint32_t initial_seed;
    uint32_t seed_ready;
} NativeReplayHeader;

typedef struct NativeReplaySample {
    uint64_t state_digest;
    PADStatus pads[4];
    uint32_t random_seed;
    uint32_t reserved;
} NativeReplaySample;

_Static_assert(sizeof(PADStatus) == 16, "Replay expects native PADStatus");
_Static_assert(sizeof(NativeReplayHeader) == 24, "Replay header layout changed");
_Static_assert(sizeof(NativeReplaySample) == 80, "Replay sample layout changed");

static const char replay_magic[8] = { 'F', 'M', 'R', 'E', 'P', 'L', 'A', 'Y' };

static struct {
    FILE* file;
    FILE* alternate_file;
    NativeReplayHeader header;
    uint64_t sample_count;
    bool playback;
    bool inputs_only;
    bool validate_timeline;
    bool failed;
    bool alternate;
} replay;

static void nativeReplay_ReadPads(PADStatus* pads)
{
    if (replay.failed) {
        return;
    }
    NativeReplaySample sample;
    FILE* input = replay.alternate ? replay.alternate_file : replay.file;
    size_t bytes = fread(&sample, 1, sizeof(sample), input);
    if (bytes == 0 && feof(input)) {
        fprintf(stderr, "[replay] End after %llu controller samples.\n",
                (unsigned long long) replay.sample_count);
        gmMain_ExitRequested = true;
        for (int i = 0; i < 4; ++i) {
            memset(&pads[i], 0, sizeof(pads[i]));
            pads[i].err = PAD_ERR_NO_CONTROLLER;
        }
        return;
    }
    if (bytes != sizeof(sample)) {
        fprintf(stderr, "[replay] Truncated or unreadable sample %llu.\n",
                (unsigned long long) replay.sample_count);
        replay.failed = true;
        gmMain_ExitRequested = true;
        return;
    }
    if (replay.validate_timeline && sample.random_seed != *seed_ptr) {
        fprintf(stderr,
                "[replay] RNG divergence before sample %llu: recorded %08x, got %08x.\n",
                (unsigned long long) replay.sample_count,
                sample.random_seed, (unsigned) *seed_ptr);
        replay.failed = true;
        gmMain_ExitRequested = true;
        return;
    }
    uint64_t current_digest = replay.validate_timeline
                                  ? nativeReplay_StateDigest(
                                        replay.sample_count)
                                  : sample.state_digest;
    if (replay.inputs_only) {
        if (!nativeMatchFixture_Apply(replay.sample_count)) {
            replay.failed = true;
            gmMain_ExitRequested = true;
            return;
        }
        // Keep action/damage diagnostics tied to the new simulation, not
        // the state digest stored by the source recording.
        if (nativeReplay_StateObserversEnabled()) {
            nativeReplay_StateDigest(replay.sample_count);
        }
    }
    if (replay.validate_timeline && sample.state_digest != current_digest) {
        fprintf(stderr,
                "[replay] Fighter-state divergence before sample %llu: recorded %016llx, got %016llx.\n",
                (unsigned long long) replay.sample_count,
                sample.state_digest, (unsigned long long) current_digest);
        replay.failed = true;
        gmMain_ExitRequested = true;
        return;
    }
    memcpy(pads, sample.pads, sizeof(sample.pads));
    ++replay.sample_count;
}

bool nativeReplay_IsPlayback(void)
{
    return replay.file != NULL && replay.playback;
}

void nativeReplay_RecordPads(const PADStatus pads[4])
{
    if (replay.file == NULL || replay.playback || replay.failed) {
        return;
    }
    NativeReplaySample sample = { 0 };
    sample.state_digest = nativeReplay_StateDigest(replay.sample_count);
    memcpy(sample.pads, pads, sizeof(sample.pads));
    sample.random_seed = *seed_ptr;
    if (fwrite(&sample, 1, sizeof(sample), replay.file) != sizeof(sample) ||
        fflush(replay.file) != 0) {
        fprintf(stderr, "[replay] Could not write sample %llu.\n",
                (unsigned long long) replay.sample_count);
        replay.failed = true;
        gmMain_ExitRequested = true;
        return;
    }
    ++replay.sample_count;
}

bool nativeReplay_Open(const char* path, NativeReplayMode mode,
                       uint64_t match_signature)
{
    memset(&replay, 0, sizeof(replay));
    if (path == NULL) {
        return true;
    }
    replay.playback = mode != NativeReplay_Record;
    replay.inputs_only = mode == NativeReplay_InputsOnly;
    replay.validate_timeline = mode == NativeReplay_Verify;
    replay.file = fopen(path, replay.playback ? "rb" : "wb+");
    if (replay.file == NULL) {
        fprintf(stderr, "[replay] Could not open %s.\n", path);
        return false;
    }
    if (replay.playback) {
        if (fread(&replay.header, 1, sizeof(replay.header), replay.file) !=
                sizeof(replay.header) ||
            memcmp(replay.header.magic, replay_magic, sizeof(replay_magic)) != 0 ||
            (!replay.inputs_only &&
             replay.header.match_signature != match_signature) ||
            replay.header.seed_ready != 1) {
            fprintf(stderr,
                    "[replay] Invalid recording or different match settings: "
                    "%s (recorded %016llx, requested %016llx).\n",
                    path, (unsigned long long) replay.header.match_signature,
                    (unsigned long long) match_signature);
            fclose(replay.file);
            replay.file = NULL;
            return false;
        }
    } else {
        memcpy(replay.header.magic, replay_magic, sizeof(replay_magic));
        replay.header.match_signature = match_signature;
        if (fwrite(&replay.header, 1, sizeof(replay.header), replay.file) !=
                sizeof(replay.header) ||
            fflush(replay.file) != 0) {
            fprintf(stderr, "[replay] Could not write recording header: %s.\n", path);
            fclose(replay.file);
            replay.file = NULL;
            return false;
        }
    }
    if (replay.playback) PADSetReadCallback(nativeReplay_ReadPads);
    fprintf(stderr, "[replay] %s %s.\n",
            replay.inputs_only ? "Comparing inputs from" :
            replay.playback ? "Playing" : "Recording", path);
    return true;
}

uint32_t nativeReplay_Seed(uint32_t proposed_seed)
{
    if (replay.file == NULL || replay.failed) {
        return proposed_seed;
    }
    if (replay.playback) {
        return replay.header.initial_seed;
    }
    replay.header.initial_seed = proposed_seed;
    replay.header.seed_ready = 1;
    long at = ftell(replay.file);
    if (at < 0 || fseek(replay.file, 0, SEEK_SET) != 0 ||
        fwrite(&replay.header, 1, sizeof(replay.header), replay.file) !=
            sizeof(replay.header) ||
        fflush(replay.file) != 0 || fseek(replay.file, at, SEEK_SET) != 0) {
        fprintf(stderr, "[replay] Could not save initial seed.\n");
        replay.failed = true;
        gmMain_ExitRequested = true;
    }
    return proposed_seed;
}

uint64_t nativeReplay_SampleCount(void)
{
    return replay.sample_count;
}

bool nativeReplay_RewindToSample(uint64_t sample)
{
    if (replay.file == NULL || !replay.playback || replay.failed ||
        sample > replay.sample_count ||
        sample > (UINT64_MAX - sizeof(NativeReplayHeader)) /
                     sizeof(NativeReplaySample))
    {
        return false;
    }
    uint64_t offset = sizeof(NativeReplayHeader) +
                      sample * sizeof(NativeReplaySample);
    FILE* input = replay.alternate ? replay.alternate_file : replay.file;
    if (offset > INT64_MAX || _fseeki64(input, (int64_t) offset, SEEK_SET)) {
        return false;
    }
    clearerr(input);
    replay.sample_count = sample;
    return true;
}

bool nativeReplay_PrepareAlternateInputs(const char* path)
{
    if (!path || !*path || !replay.file || !replay.inputs_only ||
        replay.failed || replay.sample_count || replay.alternate_file)
        return false;
    FILE* input = fopen(path, "rb");
    NativeReplayHeader header;
    if (!input) return false;
    if (fread(&header, 1, sizeof(header), input) != sizeof(header) ||
        memcmp(&header, &replay.header, sizeof(header))) {
        fclose(input);
        return false;
    }
    replay.alternate_file = input;
    return true;
}

bool nativeReplay_UseAlternateInputs(bool alternate)
{
    FILE* input = alternate ? replay.alternate_file : replay.file;
    if (!input || !replay.inputs_only || replay.failed ||
        replay.sample_count > (INT64_MAX - sizeof(NativeReplayHeader)) /
                                  sizeof(NativeReplaySample))
        return false;
    int64_t offset = sizeof(NativeReplayHeader) +
                     replay.sample_count * sizeof(NativeReplaySample);
    if (_fseeki64(input, offset, SEEK_SET)) return false;
    clearerr(input);
    replay.alternate = alternate;
    return true;
}

void nativeReplay_SetTimelineValidation(bool enabled)
{
    // Input-comparison replays intentionally run with different fighter
    // packages, so their recorded digests are never a validation oracle.
    replay.validate_timeline = enabled && !replay.inputs_only;
}

bool nativeReplay_Close(void)
{
    PADSetReadCallback(NULL);
    if (replay.alternate_file) {
        if (fclose(replay.alternate_file)) replay.failed = true;
        replay.alternate_file = NULL;
    }
    if (replay.file != NULL) {
        if (fclose(replay.file) != 0) {
            replay.failed = true;
        }
        replay.file = NULL;
    }
    return !replay.failed;
}

#endif
