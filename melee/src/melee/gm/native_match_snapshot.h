#ifndef MELEE_GM_NATIVE_MATCH_SNAPSHOT_H
#define MELEE_GM_NATIVE_MATCH_SNAPSHOT_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { NativeMatchSnapshot_Count=39 };
typedef struct NativeMatchSnapshotPart {
    const void* data;
    size_t bytes;
    const char* name;
    bool validated;
} NativeMatchSnapshotPart;
typedef struct NativeMatchSnapshotMeasurement {
    size_t expected;
    uint64_t query_ns,save_ns,validate_ns;
    bool size_known,saved;
} NativeMatchSnapshotMeasurement;
typedef struct NativeMatchSnapshotLoadReport {
    uint64_t owner_ns[NativeMatchSnapshot_Count],preflight_ns,total_ns;
    uint32_t failed_owner,fighters,ground,items,class_blocks,class_types;
    bool actors_valid,classes_valid;
} NativeMatchSnapshotLoadReport;

// Private own-capture API, with caller-reserved independent HOST buffers and
// resident scene/package definitions. No allocation, logging, callbacks or ticks.
// Packing is eight-byte aligned; backing and all metadata outputs must be disjoint.
// A serializer refusal yields an invalid part and false; backing can be partial.
// Metadata must not alias any engine owner. Measurements are optional; null avoids
// timing/query instrumentation, not owner validation. This is not a disk format.
bool nativeMatchSnapshot_Capture(NativeMatchSnapshotPart parts[NativeMatchSnapshot_Count],
    void* backing,size_t capacity,size_t* written,NativeMatchSnapshotMeasurement* optional);
// Immutable own captures only, individually validated at capture time. Checks
// actor/class agreement, then invokes the direct owner Loads in dependency order.
// Complete candidate-wide prevalidation remains open: late failure can leave a
// partial world. Experimental callers must abort/recover; production is not wired.
bool nativeMatchSnapshot_Load(const NativeMatchSnapshotPart*,NativeMatchSnapshotLoadReport* optional);
#endif
