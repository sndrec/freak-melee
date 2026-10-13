#ifndef MELEE_IT_NATIVE_SCRIPT_ITEM_SNAPSHOT_H
#define MELEE_IT_NATIVE_SCRIPT_ITEM_SNAPSHOT_H
#include "native_script_item_actor.h"
#include <melee/gm/native_match_pool.h>

// Private same-process occupied-state records. Program/specs stay pinned;
// aggregate restore resurrects GObj/native Item owners before loading these.
typedef struct NativeScriptItemSnapshotHeader {
    uint64_t epoch;
    uint32_t count, total_bytes, initialized_count;
} NativeScriptItemSnapshotHeader;
typedef struct NativeScriptItemSnapshotRecord {
    NativeMatchPoolReference object;
    NativeScriptItemRelation creator,attachment_owner;
    uint64_t program_identity, layout_identity, parameter_identity, revision;
    const struct NativeItemDefinition* recipe;
    const Article* article;
    const struct NativeScriptItemProjectile* projectile;
    const struct NativeScriptItemAttachment* attachment;
    uint32_t slot, generation, author_kind, kind, recipe_index, owner;
    uint32_t selected_state, tick, completed_callbacks, state_bytes, faulted;
    uint32_t phases[NativeScriptItemPhase_Count];
    uint32_t callbacks[NativeScriptItemCallback_Count];
    float creation_value;
} NativeScriptItemSnapshotRecord;
#endif
