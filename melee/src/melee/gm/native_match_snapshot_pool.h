#ifndef MELEE_GM_NATIVE_MATCH_SNAPSHOT_POOL_H
#define MELEE_GM_NATIVE_MATCH_SNAPSHOT_POOL_H

#include "native_match_pool.h"

// Private in-memory snapshot only. Code, address layout and pool binding remain
// resident for its lifetime; this is not a disk/network format or script API.
typedef struct NativeMatchPoolSnapshotHeader {
    const NativeMatchPool* pool;
    const void* payload;
    uint64_t lifetime;
    size_t object_size, stride;
    uint32_t capacity, used, free_count, peak, initialized_count;
} NativeMatchPoolSnapshotHeader;

// Header, capacity generations, ordered free slots, then occupied records:
// uint32 slot + object_size payload bytes. Occupied records are ascending slots.
size_t nativeMatchSnapshotPool_Bytes(const NativeMatchPool*);
// Successful save guarantees a valid own candidate. Invalid live metadata may
// leave partial scratch bytes, but never writes outside capacity, changes live
// pool state or publishes the caller's written count.
bool nativeMatchSnapshotPool_Save(const NativeMatchPool*, void* snapshot,
                                  size_t capacity, size_t* written);
// Read-only, validated candidate view. No allocation or copy of live payloads.
// Keep snapshot bytes immutable and resident at an idle ownership boundary.
// A null records pointer marks failure, including a retired pool binding.
typedef struct NativeMatchPoolSnapshotView {
    NativeMatchPoolSnapshotHeader header;
    const uint8_t* generations;
    const uint8_t* records;
} NativeMatchPoolSnapshotView;
NativeMatchPoolSnapshotView nativeMatchSnapshotPool_View(const NativeMatchPool*,
                                                        const void*,size_t bytes);
// Find the saved payload by slot, not the currently occupied live payload.
// Ordered records permit binary search without a per-slot allocation/index.
const void* nativeMatchSnapshotPool_Find(NativeMatchPoolSnapshotView,uint32_t slot);
// Resolve a saved generation to its stable physical address. This does not
// dereference current object bytes or assert live-timeline occupancy.
void* nativeMatchSnapshotPool_Resolve(NativeMatchPoolSnapshotView,NativeMatchPoolReference);
// Validates the entire candidate before changing any payload/metadata.
bool nativeMatchSnapshotPool_Load(NativeMatchPool*, const void* snapshot,
                                  size_t bytes);
#endif
