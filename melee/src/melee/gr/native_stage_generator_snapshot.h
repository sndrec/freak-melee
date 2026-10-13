#ifndef MELEE_GR_NATIVE_STAGE_GENERATOR_SNAPSHOT_H
#define MELEE_GR_NATIVE_STAGE_GENERATOR_SNAPSHOT_H
#include "stage_generator_state.h"
#include <melee/gm/native_match_pool.h>
#include <stdint.h>

typedef struct NativeStageGeneratorSnapshot {
    uint64_t epoch;
    StageGeneratorState state;
    NativeMatchPoolReference driver;
    NativeMatchPoolReference objects[STAGE_GENERATOR_ENTRIES],items[STAGE_GENERATOR_ENTRIES];
} NativeStageGeneratorSnapshot;
void nativeStageGenerator_BeginScene(void);
void nativeStageGenerator_EndScene(void);
bool nativeStageGenerator_Save(void*,size_t,size_t*);
bool nativeStageGenerator_Validate(const void*,size_t);
bool nativeStageGenerator_Load(const void*,size_t);
#endif
