#include "native_confirmed_checkpoint.h"
#include <string.h>

static bool overlap(const void* a, size_t an, const void* b, size_t bn)
{
    uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y || (x<y+bn && y<x+an);
}
size_t nativeConfirmedCheckpoint_Bytes(NativeMatchArena* const* arenas, unsigned count)
{
    if (!arenas || !count || count>NativeConfirmedCheckpoint_Arenas) return 0;
    size_t bytes=0;
    for (unsigned i=0; i<count; ++i) {
        const NativeMatchArena* arena=arenas[i];
        if (!arena || !arena->lifetime || arena->capacity>SIZE_MAX-arena->root_bytes)
            return 0;
        size_t span=arena->capacity+arena->root_bytes;
        if (span>SIZE_MAX-7 || ((span+7)&~(size_t)7)>SIZE_MAX-bytes) return 0;
        bytes+=(span+7)&~(size_t)7;
        for (unsigned j=0; j<i; ++j) if (arena==arenas[j]) return 0;
    }
    return bytes;
}
static bool current(const NativeConfirmedCheckpoint* checkpoint, uint64_t epoch)
{
    if (!checkpoint || !epoch || checkpoint->epoch!=epoch || checkpoint->failed ||
        !checkpoint->arena_count || checkpoint->arena_count>NativeConfirmedCheckpoint_Arenas)
        return false;
    for (unsigned i=0; i<checkpoint->arena_count; ++i) {
        const NativeMatchArenaFrame* span=&checkpoint->spans[i];
        if (!span->arena || span->lifetime!=span->arena->lifetime) return false;
    }
    return true;
}
static bool capture(NativeConfirmedCheckpoint* checkpoint, uint64_t boundary)
{
    checkpoint->valid=false;
    size_t bytes=0;
    for (unsigned i=0; i<checkpoint->arena_count; ++i) {
        if (!nativeMatchArena_Save(&checkpoint->spans[i])) {
            checkpoint->failed=true; return false;
        }
        bytes+=checkpoint->spans[i].total_bytes;
    }
    checkpoint->boundary=boundary;
    checkpoint->valid=true;
    ++checkpoint->captures;
    if (bytes>checkpoint->peak_bytes) checkpoint->peak_bytes=bytes;
    return true;
}
bool nativeConfirmedCheckpoint_Bind(NativeConfirmedCheckpoint* checkpoint,
    NativeMatchArena* const* arenas, unsigned count, void* backing, size_t bytes,
    uint64_t epoch)
{
    size_t needed=nativeConfirmedCheckpoint_Bytes(arenas,count);
    if (!checkpoint || !backing || !epoch || !needed || bytes<needed ||
        ((uintptr_t)backing&7) ||
        overlap(checkpoint,sizeof(*checkpoint),backing,bytes) ||
        overlap(checkpoint,sizeof(*checkpoint),arenas,count*sizeof(*arenas)) ||
        overlap(backing,bytes,arenas,count*sizeof(*arenas))) return false;
    // Validate every owner against all output before binding either arena.
    for (unsigned i=0; i<count; ++i) {
        NativeMatchArena* arena=arenas[i];
        if (overlap(checkpoint,sizeof(*checkpoint),arena,sizeof(*arena)) ||
            overlap(backing,bytes,arena,sizeof(*arena)) ||
            overlap(checkpoint,sizeof(*checkpoint),arena->memory,arena->capacity) ||
            overlap(backing,bytes,arena->memory,arena->capacity)) return false;
        for (unsigned j=0; j<arena->root_count; ++j) {
            NativeMatchArenaRoot* root=&arena->roots[j];
            if (overlap(checkpoint,sizeof(*checkpoint),root->address,root->bytes) ||
                overlap(backing,bytes,root->address,root->bytes)) return false;
        }
    }
    *checkpoint=(NativeConfirmedCheckpoint){.epoch=epoch,.arena_count=count,
        .backing_bytes=needed};
    size_t offset=0;
    for (unsigned i=0; i<count; ++i) {
        size_t span=(arenas[i]->capacity+arenas[i]->root_bytes+7)&~(size_t)7;
        if (!nativeMatchArena_BindFrame(arenas[i],&checkpoint->spans[i],
                (uint8_t*)backing+offset,span)) {
            checkpoint->failed=true; return false;
        }
        offset+=span;
    }
    return capture(checkpoint,0);
}
bool nativeConfirmedCheckpoint_CanAdvance(const NativeConfirmedCheckpoint* checkpoint,
    uint64_t epoch)
{
    return current(checkpoint,epoch) && checkpoint->valid &&
        checkpoint->cursor>=checkpoint->boundary &&
        checkpoint->cursor-checkpoint->boundary<NativeConfirmedCheckpoint_MaxPrediction;
}
NativeConfirmedCheckpointStep nativeConfirmedCheckpoint_Reconcile(
    NativeConfirmedCheckpoint* checkpoint, NativeRollbackInputHistory* inputs,
    uint64_t epoch, uint64_t* next)
{
    if (!next || !inputs || !current(checkpoint,epoch) || !checkpoint->valid)
        return NativeConfirmedCheckpoint_Failed;
    *next=checkpoint->cursor;
    if (checkpoint->replaying) return NativeConfirmedCheckpoint_Replaying;
    uint64_t end;
    if (!nativeRollbackInput_ConfirmedBoundary(inputs,checkpoint->boundary,
            checkpoint->cursor,0xF,&end) ||
        (inputs->has_correction && (inputs->correction_frame<checkpoint->boundary ||
                                   inputs->correction_frame>=checkpoint->cursor))) {
        checkpoint->failed=true; return NativeConfirmedCheckpoint_Failed;
    }
    bool correction=inputs->has_correction;
    if (!correction && end==checkpoint->boundary) return NativeConfirmedCheckpoint_Continue;
    if (!correction && end==checkpoint->cursor) {
        return capture(checkpoint,end) && nativeRollbackInput_ReleaseBefore(inputs,end) ? NativeConfirmedCheckpoint_Continue :
            NativeConfirmedCheckpoint_Failed;
    }
    // All leases were checked by current() before either owner is mutated.
    for (unsigned i=0; i<checkpoint->arena_count; ++i)
        if (!checkpoint->spans[i].valid) {
            checkpoint->failed=true; return NativeConfirmedCheckpoint_Failed;
        }
    for (unsigned i=0; i<checkpoint->arena_count; ++i)
        if (!nativeMatchArena_Load(&checkpoint->spans[i])) {
            checkpoint->failed=true; return NativeConfirmedCheckpoint_Failed;
        }
    ++checkpoint->restores;
    checkpoint->replay_end=checkpoint->cursor;
    checkpoint->confirm_end=end;
    checkpoint->cursor=checkpoint->boundary;
    checkpoint->replaying=true;
    inputs->has_correction=false;
    *next=checkpoint->cursor;
    return NativeConfirmedCheckpoint_Replaying;
}
NativeConfirmedCheckpointStep nativeConfirmedCheckpoint_EndFrame(
    NativeConfirmedCheckpoint* checkpoint, NativeRollbackInputHistory* inputs,
    uint64_t frame, uint64_t epoch, uint64_t* next)
{
    if (!next || !inputs || !current(checkpoint,epoch) || !checkpoint->valid ||
        frame!=checkpoint->cursor || frame==UINT64_MAX ||
        !nativeConfirmedCheckpoint_CanAdvance(checkpoint,epoch))
        return NativeConfirmedCheckpoint_Failed;
    checkpoint->cursor=frame+1;
    *next=checkpoint->cursor;
    if (!checkpoint->replaying) return NativeConfirmedCheckpoint_Continue;
    ++checkpoint->replay_ticks;
    if (checkpoint->cursor==checkpoint->confirm_end &&
        checkpoint->confirm_end>checkpoint->boundary &&
        (!capture(checkpoint,checkpoint->confirm_end) ||
         !nativeRollbackInput_ReleaseBefore(inputs,checkpoint->confirm_end))) return NativeConfirmedCheckpoint_Failed;
    if (checkpoint->cursor==checkpoint->replay_end) {
        checkpoint->replaying=false;
        return NativeConfirmedCheckpoint_CaughtUp;
    }
    return NativeConfirmedCheckpoint_Replaying;
}
