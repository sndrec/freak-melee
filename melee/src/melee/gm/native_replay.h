#ifndef MELEE_GM_NATIVE_REPLAY_H
#define MELEE_GM_NATIVE_REPLAY_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <melee/it/forward.h>
#include <melee/gm/forward.h>
#include <sysdolphin/baselib/forward.h>

struct PADStatus;
struct Fighter;

typedef enum NativeReplayMode {
    NativeReplay_Record,
    NativeReplay_Verify,
    NativeReplay_InputsOnly,
} NativeReplayMode;

// InputsOnly is a local comparison diagnostic: it reuses seed and pads while
// deliberately allowing a different package and divergent resulting state.
bool nativeReplay_Open(const char* path, NativeReplayMode mode,
                       uint64_t match_signature);
bool nativeReplay_IsPlayback(void);
void nativeReplay_RecordPads(const struct PADStatus* pads);
uint32_t nativeReplay_Seed(uint32_t proposed_seed);
uint64_t nativeReplay_SampleCount(void);
bool nativeReplay_RewindToSample(uint64_t sample);
// Reserve a second inputs-only stream before samples are consumed. Both streams
// must describe the same initial timeline. Switching preserves the sample index.
bool nativeReplay_PrepareAlternateInputs(const char* path);
bool nativeReplay_UseAlternateInputs(bool alternate);
void nativeReplay_SetTimelineValidation(bool enabled);
uint64_t nativeReplay_StateDigest(uint64_t sample_index);
bool nativeReplay_StateObserversEnabled(void);
void nativeReplay_SetStateTraceEnabled(bool enabled);
void nativeReplay_SetActionTraceEnabled(bool enabled);
enum { NativeReplay_TraceActors=GM_MAX_PLAYERS*2 };
typedef struct NativeReplayActionTraceCheckpoint {
    HSD_GObj* entities[NativeReplay_TraceActors];
    int32_t motions[NativeReplay_TraceActors];
    float damage[NativeReplay_TraceActors];
} NativeReplayActionTraceCheckpoint;
// Host-only transition observer history. Never part of game snapshot state.
void nativeReplay_CaptureActionTrace(NativeReplayActionTraceCheckpoint*);
void nativeReplay_RestoreActionTrace(const NativeReplayActionTraceCheckpoint*);
void nativeReplay_SetFighterTraceEnabled(bool enabled);
void nativeReplay_LogGeometry(uint64_t sample_index);
HSD_JObj* nativeReplay_AdditionalItemModel(const Item*, unsigned slot);
void nativeReplay_LogItemEvent(int kind, const char* event);
void nativeReplay_LogBodyContact(const struct Fighter* fighter);
void nativeReplay_WriteStateDigestSegments(FILE* output, uint64_t sample_index,
                                           bool fighter_only);
#ifdef NATIVE_NETPLAY_DIAGNOSTICS
bool nativeReplay_WriteCapsuleTrace(FILE* output, uint64_t sample_index,
                                   size_t* remaining);
#endif
bool nativeReplay_Close(void);

#endif
#endif
