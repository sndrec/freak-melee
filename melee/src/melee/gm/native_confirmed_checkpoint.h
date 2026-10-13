#ifndef MELEE_GM_NATIVE_CONFIRMED_CHECKPOINT_H
#define MELEE_GM_NATIVE_CONFIRMED_CHECKPOINT_H
#include "native_match_arena.h"
#include "native_rollback_input.h"

enum { NativeConfirmedCheckpoint_Arenas = 2,
       NativeConfirmedCheckpoint_MaxPrediction = 15 };
typedef enum NativeConfirmedCheckpointStep {
    NativeConfirmedCheckpoint_Continue,
    NativeConfirmedCheckpoint_Replaying,
    NativeConfirmedCheckpoint_CaughtUp,
    NativeConfirmedCheckpoint_Failed,
} NativeConfirmedCheckpointStep;

// Host control state, never restored with the live game. One saved boundary;
// inputs and audio intents have independent frame histories. No tick allocation.
typedef struct NativeConfirmedCheckpoint {
    NativeMatchArenaFrame spans[NativeConfirmedCheckpoint_Arenas];
    uint64_t epoch, boundary, cursor, confirm_end, replay_end;
    uint64_t captures, restores, replay_ticks;
    size_t backing_bytes, peak_bytes;
    unsigned arena_count;
    bool valid, failed, replaying;
} NativeConfirmedCheckpoint;

size_t nativeConfirmedCheckpoint_Bytes(NativeMatchArena* const*, unsigned count);
// Construction only, before input zero. Binds resident owners and captures zero.
bool nativeConfirmedCheckpoint_Bind(NativeConfirmedCheckpoint*,
    NativeMatchArena* const*, unsigned count, void* backing, size_t bytes,
    uint64_t epoch);
// Run after receiving inputs at a tick boundary, before preparing another tick.
// Returns the next input frame; Replaying means the live state was restored.
NativeConfirmedCheckpointStep nativeConfirmedCheckpoint_Reconcile(
    NativeConfirmedCheckpoint*, NativeRollbackInputHistory*, uint64_t epoch,
    uint64_t* next);
// Commit exactly one completed tick. At the reconstruction frontier save once;
// predicted replay ticks do not save. Call Reconcile again for fresh packets.
NativeConfirmedCheckpointStep nativeConfirmedCheckpoint_EndFrame(
    NativeConfirmedCheckpoint*, NativeRollbackInputHistory*, uint64_t frame,
    uint64_t epoch, uint64_t* next);
bool nativeConfirmedCheckpoint_CanAdvance(const NativeConfirmedCheckpoint*,
    uint64_t epoch);
#endif
