#ifndef HSD_NATIVE_ID_SNAPSHOT_H
#define HSD_NATIVE_ID_SNAPSHOT_H

#include "id.h"
#include <stdint.h>

// Private memory component; entry bytes and pointed objects restore through
// ObjAlloc/reservation/model owners before any lookup or callback resumes.
// Caller buffers must remain outside all companion backing. Current live ID
// entry backing overlap is rejected; graph references are not dereferenced.
typedef struct NativeIdSnapshot {
    uint64_t epoch;
    HSD_IDTable roots;
} NativeIdSnapshot;

bool nativeID_Save(void*, size_t capacity, size_t* written);
bool nativeID_ValidateSnapshot(const void*, size_t bytes);
bool nativeID_Load(const void*, size_t bytes);

#endif
