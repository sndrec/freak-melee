#ifndef MELEE_FT_NATIVE_FIGHTER_ANIMATION_COPY_H
#define MELEE_FT_NATIVE_FIGHTER_ANIMATION_COPY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NativeFighterAnimationCopyInput {
    const void* fighter_dat;
    size_t fighter_size;
    const char* fighter_root;
    uint32_t action_count;
    const void* animation_dat;
    size_t animation_size;
} NativeFighterAnimationCopyInput;

typedef struct NativeFighterAnimationCopyOutput {
    void* fighter_dat;
    size_t fighter_size;
    void* animation_dat;
    size_t animation_size;
    uint32_t filled_actions;
    uint32_t copied_blobs;
} NativeFighterAnimationCopyOutput;

// Authoring transition only. Both immutable inputs outlive this synchronous
// operation. Fill action holes by index; keep destination flags, commands,
// blend and demo records. Output is published only on complete success and
// owns two ordinary malloc allocations. Pass an empty output; a failure leaves
// it unchanged. No runtime donor metadata or native pointers are serialized.
bool nativeFighterAnimationCopy_Materialize(
    const NativeFighterAnimationCopyInput* destination,
    const NativeFighterAnimationCopyInput* source,
    NativeFighterAnimationCopyOutput* output);
void nativeFighterAnimationCopy_Free(NativeFighterAnimationCopyOutput* output);

#ifdef __cplusplus
}
#endif
#endif
