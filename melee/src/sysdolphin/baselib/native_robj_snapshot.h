#ifndef HSD_NATIVE_ROBJ_SNAPSHOT_H
#define HSD_NATIVE_ROBJ_SNAPSHOT_H

#include "memory.h"
#include <stdint.h>

// Constraint-expression scratch is persistent mutable storage: unused tail
// values are not assumed regenerated. Restore OS Cell metadata before Load;
// resume constraints only after all model/allocator companions have restored.
// A physical heap range is provenance, not cross-owner allocation closure.
typedef struct NativeRObjSnapshotHeader {
    uint64_t epoch;
    float* arguments;
    int32_t heap;
    uint32_t count;
} NativeRObjSnapshotHeader;

size_t nativeRObj_SnapshotBytes(void);
bool nativeRObj_Save(void*, size_t capacity, size_t* written);
bool nativeRObj_ValidateSnapshot(const void*, size_t bytes);
bool nativeRObj_Load(const void*, size_t bytes);

#endif
