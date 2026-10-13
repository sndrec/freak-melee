#ifndef MELEE_GR_STAGE_SPAWN_STATE_H
#define MELEE_GR_STAGE_SPAWN_STATE_H
#include "types.h"

// Source configurations are retained until the stage is retired. Current
// stages use two configurations, with at most ten segments each. Keep room for
// authored stages and report exhaustion through the constructor's NULL result.
enum { STAGE_SPAWN_CONFIGS = 128, STAGE_SPAWN_SEGMENTS = 32 };
typedef struct StageSpawnRecord {
    grZakoGenerator_Config config;
    grZakoGenerator_Spawn segments[STAGE_SPAWN_SEGMENTS];
} StageSpawnRecord;
typedef struct StageSpawnState {
    unsigned count;
    StageSpawnRecord records[STAGE_SPAWN_CONFIGS];
} StageSpawnState;
extern StageSpawnState stage_spawn_state;
#endif
