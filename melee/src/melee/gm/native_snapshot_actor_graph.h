#ifndef MELEE_GM_NATIVE_SNAPSHOT_ACTOR_GRAPH_H
#define MELEE_GM_NATIVE_SNAPSHOT_ACTOR_GRAPH_H
#include "native_match_snapshot_pool.h"

// Every view must come from immutable, fully validated candidate components.
// This checks both directions of actor/root ownership without consulting live
// occupancy, running callbacks, allocating or writing simulation bytes.
typedef struct NativeSnapshotActorGraph {
    NativeMatchPoolSnapshotView objects,fighters,ground,items;
} NativeSnapshotActorGraph;
bool nativeSnapshotActorGraph_Validate(const NativeSnapshotActorGraph*);
#endif
