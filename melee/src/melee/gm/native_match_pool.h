#ifndef MELEE_GM_NATIVE_MATCH_POOL_H
#define MELEE_GM_NATIVE_MATCH_POOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Ceiling, not a preallocated array: each owner reserves its measured demand
// at the scene transition. Full resource collections can require more than
// 16K class pieces even with fewer than 1.5K live graphs.
enum { NATIVE_MATCH_POOL_MAX_SLOTS = 65536 };

typedef struct NativeMatchPoolSlot {
    uint32_t generation;
    bool occupied;
} NativeMatchPoolSlot;

typedef struct NativeMatchPool {
    uint8_t* payload;
    NativeMatchPoolSlot* slots;
    uint32_t* free_order;
    size_t object_size, stride;
    uint64_t lifetime;
    uint32_t capacity, used, free_count, peak, initialized_count;
} NativeMatchPool;

typedef struct NativeMatchPoolReference {
    const NativeMatchPool* pool;
    uint64_t lifetime;
    uint32_t slot, generation;
} NativeMatchPoolReference;

// Bind caller-reserved storage at an ownership transition. lifetime is a new
// nonzero scene/owner epoch, never reused while a snapshot/reference can survive.
// Physical storage may remain reserved across scenes; logical state cannot.
// Invalid binding leaves the pool and all supplied storage unchanged.
size_t nativeMatchPool_Stride(size_t object_size, size_t alignment);
bool nativeMatchPool_Bind(NativeMatchPool*, void* payload, size_t payload_bytes,
                         NativeMatchPoolSlot*, uint32_t* free_order,
                         uint32_t capacity, size_t object_size,
                         size_t alignment, uint64_t lifetime);
bool nativeMatchPool_Validate(const NativeMatchPool*);
void* nativeMatchPool_Allocate(NativeMatchPool*, NativeMatchPoolReference*);
bool nativeMatchPool_Free(NativeMatchPool*, NativeMatchPoolReference);
void* nativeMatchPool_Resolve(NativeMatchPoolReference);
bool nativeMatchPool_Reference(const NativeMatchPool*, const void*,
                               NativeMatchPoolReference*);

#endif
