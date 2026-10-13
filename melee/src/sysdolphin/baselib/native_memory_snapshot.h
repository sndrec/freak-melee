#ifndef HSD_NATIVE_MEMORY_SNAPSHOT_H
#define HSD_NATIVE_MEMORY_SNAPSHOT_H

#include "memory.h"
#include <stdint.h>

// Private native state, not a file format. OS heap metadata/payload owners must
// restore before any allocations resume. Heap geometry changes retire history.
typedef struct NativeMemorySnapshot {
    uint64_t epoch;
    int32_t heap;
    uint32_t reserved;
} NativeMemorySnapshot;

bool nativeMemory_Save(void*, size_t capacity, size_t* written);
bool nativeMemory_ValidateSnapshot(const void*, size_t bytes);
bool nativeMemory_Load(const void*, size_t bytes);

#endif
