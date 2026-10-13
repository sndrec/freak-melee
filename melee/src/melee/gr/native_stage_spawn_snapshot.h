#ifndef MELEE_GR_NATIVE_STAGE_SPAWN_SNAPSHOT_H
#define MELEE_GR_NATIVE_STAGE_SPAWN_SNAPSHOT_H
#include "stage_spawn_state.h"
#include <stddef.h>
#include <stdint.h>

typedef struct NativeStageSpawnSnapshotHeader {
    uint64_t epoch;
    uint32_t count, reserved;
} NativeStageSpawnSnapshotHeader;
void nativeStageSpawn_BeginScene(void);
void nativeStageSpawn_EndScene(void);
size_t nativeStageSpawn_SnapshotBytes(void);
bool nativeStageSpawn_Save(void* data, size_t capacity, size_t* written);
bool nativeStageSpawn_Validate(const void* data, size_t bytes);
bool nativeStageSpawn_Load(const void* data, size_t bytes);
#endif
