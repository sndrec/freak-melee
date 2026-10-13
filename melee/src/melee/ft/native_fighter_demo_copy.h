#ifndef MELEE_FT_NATIVE_FIGHTER_DEMO_COPY_H
#define MELEE_FT_NATIVE_FIGHTER_DEMO_COPY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct NativeFighterDemoCopyInput {
    const void* fighter_dat;
    size_t fighter_size;
    const char* fighter_root;
    uint32_t demo_count;
    const void* animation_dat;
    size_t animation_size;
} NativeFighterDemoCopyInput;

typedef struct NativeFighterDemoCopySource {
    const void* archive;
    size_t archive_size;
    const char* root_name;
    uint32_t first;
    uint32_t count;
} NativeFighterDemoCopySource;

typedef struct NativeFighterDemoCopyOutput {
    void* fighter_dat;
    size_t fighter_size;
    void* animation_dat;
    size_t animation_size;
    uint32_t copied_clips;
} NativeFighterDemoCopyOutput;

// Construction only. Explicit archive roots supply disjoint descriptor ranges.
// Every populated demo is copied by its original offset/size, including clips
// whose names also occur in the action bank. Only demo offset/size words change.
// Inputs stay immutable; an empty output is published only on complete success.
bool nativeFighterDemoCopy_Materialize(
    const NativeFighterDemoCopyInput* input,
    const NativeFighterDemoCopySource* sources, unsigned source_count,
    NativeFighterDemoCopyOutput* output);
void nativeFighterDemoCopy_Free(NativeFighterDemoCopyOutput* output);

#endif
