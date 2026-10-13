#ifndef MELEE_GM_NATIVE_SNAPSHOT_ENDPOINT_H
#define MELEE_GM_NATIVE_SNAPSHOT_ENDPOINT_H
#include "native_rollback_regions.h"
#include "native_match_pool.h"
#include <stdio.h>
typedef struct NativeSnapshotEndpoint {
    NativeRollbackRegionSet regions;
    uint8_t* data;
    size_t bytes;
    bool saved;
} NativeSnapshotEndpoint;
// Offline experiment safeguard, not the replacement rollback representation.
// Caller reserves independent host storage before gameplay. No allocation,
// drawing, callback or simulator runs while saving/restoring these regions.
bool nativeSnapshotEndpoint_Save(NativeSnapshotEndpoint*,void*,size_t);
void nativeSnapshotEndpoint_Restore(const NativeSnapshotEndpoint*);
bool nativeSnapshotEndpoint_Matches(const NativeSnapshotEndpoint*);
// Bounded raw mismatch report for coverage research. Differences can include
// inactive payload/padding/catalog tails, so they are not automatically desyncs.
enum { NATIVE_SNAPSHOT_ENDPOINT_POOLS = 16 };
typedef struct NativeSnapshotEndpointPool {
    const char* name;
    const NativeMatchPool* pool;
} NativeSnapshotEndpointPool;
// Classify differing bytes against restored live ownership at this idle boundary.
// Pools must be disjoint, valid bindings; no allocation/free may occur during the
// report. Keep every raw difference: classification never changes acceptance or
// treats unclassified bytes as harmless. SIZE_MAX means registration failed.
size_t nativeSnapshotEndpoint_ReportDifferences(const NativeSnapshotEndpoint*,
    const NativeSnapshotEndpointPool*,size_t pool_count,FILE*);

#endif
