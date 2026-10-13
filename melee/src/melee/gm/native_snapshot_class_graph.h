#ifndef MELEE_GM_NATIVE_SNAPSHOT_CLASS_GRAPH_H
#define MELEE_GM_NATIVE_SNAPSHOT_CLASS_GRAPH_H
#include "native_match_snapshot.h"
// Own, validated component captures; checks ordinary-class membership/type
// agreement before diagnostic Loads. Not a complete HSD graph/liveness proof.
typedef struct NativeSnapshotClassGraphReport {uint32_t blocks,types;} NativeSnapshotClassGraphReport;
bool nativeSnapshotClassGraph_Validate(const NativeMatchSnapshotPart*,NativeSnapshotClassGraphReport*);
#endif
