#ifndef MELEE_FT_NATIVE_SCRIPT_FIGHTER_SNAPSHOT_H
#define MELEE_FT_NATIVE_SCRIPT_FIGHTER_SNAPSHOT_H
#include <melee/gm/native_match_pool.h>
#include "native_script_fighter.h"
#include <stdint.h>
// Private same-process records; no disk/network versioning or native asset copy.
typedef struct NativeScriptFighterSnapshotHeader {
    uint64_t epoch;
    uint32_t count,total_bytes,initialized_count;
} NativeScriptFighterSnapshotHeader;
typedef struct NativeScriptFighterSnapshotRecord {
    NativeMatchPoolReference object;
    uint64_t program_identity,layout_identity,parameter_identity;
    uint32_t slot,generation,kind,tick,completed_ticks,state_bytes,faulted;
    uint32_t accessory_cue;
    uint32_t motions[NativeScriptMotion_Count],callbacks[NativeScriptFighterCallback_Count];
} NativeScriptFighterSnapshotRecord;
#endif
