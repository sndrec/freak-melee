#include "native_match_pool.h"
#include <limits.h>
#include <string.h>

static bool overlaps(const void* first, size_t first_size,
                     const void* second, size_t second_size)
{
    uintptr_t a = (uintptr_t)first, b = (uintptr_t)second;
    if (first_size > UINTPTR_MAX - a || second_size > UINTPTR_MAX - b) return true;
    return a < b + second_size && b < a + first_size;
}

size_t nativeMatchPool_Stride(size_t size, size_t alignment)
{
    if (size == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0 ||
        size > SIZE_MAX - (alignment - 1)) return 0;
    return (size + alignment - 1) & ~(alignment - 1);
}

bool nativeMatchPool_Bind(NativeMatchPool* pool, void* payload, size_t bytes,
                         NativeMatchPoolSlot* slots, uint32_t* free_order,
                         uint32_t capacity, size_t size, size_t alignment,
                         uint64_t lifetime)
{
    size_t stride = nativeMatchPool_Stride(size, alignment);
    if (pool == NULL || payload == NULL || slots == NULL || free_order == NULL ||
        lifetime == 0 || capacity == 0 || capacity > NATIVE_MATCH_POOL_MAX_SLOTS ||
        stride == 0 || stride > SIZE_MAX / capacity || bytes < stride * capacity ||
        ((uintptr_t)payload & (alignment - 1)) != 0 ||
        (uintptr_t)slots % _Alignof(NativeMatchPoolSlot) != 0 ||
        (uintptr_t)free_order % _Alignof(uint32_t) != 0) return false;
    const void* spans[] = { pool, payload, slots, free_order };
    size_t lengths[] = { sizeof(*pool), stride * capacity,
                         sizeof(*slots) * capacity, sizeof(*free_order) * capacity };
    for (unsigned i = 0; i < 4; ++i) {
        if (lengths[i] > UINTPTR_MAX - (uintptr_t)spans[i]) return false;
        for (unsigned j = i + 1; j < 4; ++j)
            if (overlaps(spans[i], lengths[i], spans[j], lengths[j])) return false;
    }
    for (uint32_t i = 0; i < capacity; ++i) {
        slots[i] = (NativeMatchPoolSlot){ 0, false };
        // Pop from the end; initial allocation is slot0, then slot1, ...
        free_order[i] = capacity - i - 1;
    }
    *pool = (NativeMatchPool){ payload, slots, free_order, size, stride, lifetime,
                              capacity, 0, capacity, 0, 0 };
    return true;
}

bool nativeMatchPool_Validate(const NativeMatchPool* pool)
{
    if (pool == NULL || pool->payload == NULL || pool->slots == NULL ||
        pool->free_order == NULL || pool->lifetime == 0 || pool->object_size == 0 ||
        pool->stride < pool->object_size || pool->capacity == 0 ||
        pool->capacity > NATIVE_MATCH_POOL_MAX_SLOTS || pool->used > pool->capacity ||
        pool->free_count != pool->capacity - pool->used || pool->peak < pool->used ||
        pool->peak > pool->capacity || pool->initialized_count > pool->capacity ||
        pool->initialized_count < pool->peak) return false;
    uint8_t seen[NATIVE_MATCH_POOL_MAX_SLOTS / 8] = { 0 };
    uint32_t count = 0;
    for (uint32_t i = 0; i < pool->capacity; ++i) {
        if (pool->slots[i].occupied) {
            if (pool->slots[i].generation == 0 || i >= pool->initialized_count) return false;
            seen[i / 8] |= (uint8_t)(1u << (i % 8));
            ++count;
        }
    }
    if (count != pool->used) return false;
    for (uint32_t i = 0; i < pool->free_count; ++i) {
        uint32_t slot = pool->free_order[i];
        if (slot >= pool->capacity || (seen[slot / 8] & (1u << (slot % 8)))) return false;
        seen[slot / 8] |= (uint8_t)(1u << (slot % 8));
    }
    return true;
}

void* nativeMatchPool_Allocate(NativeMatchPool* pool, NativeMatchPoolReference* reference)
{
    if (pool == NULL || reference == NULL || pool->free_count == 0 ||
        pool->free_count > pool->capacity) return NULL;
    uint32_t slot = pool->free_order[pool->free_count - 1];
    if (slot >= pool->capacity || pool->slots[slot].occupied ||
        pool->slots[slot].generation == UINT32_MAX) return NULL;
    --pool->free_count;
    ++pool->used;
    if (pool->used > pool->peak) pool->peak = pool->used;
    pool->slots[slot].occupied = true;
    ++pool->slots[slot].generation;
    if (slot >= pool->initialized_count) pool->initialized_count = slot + 1;
    *reference = (NativeMatchPoolReference){ pool, pool->lifetime, slot,
                                            pool->slots[slot].generation };
    void* payload = pool->payload + pool->stride * slot;
    // Occupied-only snapshots omit free payload. Every allocation initializes
    // all bytes before a consumer can read them, including reused slots.
    memset(payload, 0, pool->object_size);
    return payload;
}

void* nativeMatchPool_Resolve(NativeMatchPoolReference reference)
{
    const NativeMatchPool* pool = reference.pool;
    if (pool == NULL || reference.lifetime == 0 || reference.lifetime != pool->lifetime ||
        reference.slot >= pool->capacity || reference.generation == 0 ||
        !pool->slots[reference.slot].occupied ||
        pool->slots[reference.slot].generation != reference.generation) return NULL;
    return pool->payload + pool->stride * reference.slot;
}

bool nativeMatchPool_Free(NativeMatchPool* pool, NativeMatchPoolReference reference)
{
    if (reference.pool != pool || nativeMatchPool_Resolve(reference) == NULL ||
        pool->used == 0 || pool->free_count >= pool->capacity) return false;
    pool->slots[reference.slot].occupied = false;
    pool->free_order[pool->free_count++] = reference.slot;
    --pool->used;
    return true;
}

bool nativeMatchPool_Reference(const NativeMatchPool* pool, const void* pointer,
                               NativeMatchPoolReference* reference)
{
    if (pool == NULL || pointer == NULL || reference == NULL || pool->stride == 0 ||
        (uintptr_t)pointer < (uintptr_t)pool->payload) return false;
    uintptr_t offset = (uintptr_t)pointer - (uintptr_t)pool->payload;
    size_t slot = offset / pool->stride;
    if (offset % pool->stride != 0 || slot >= pool->capacity ||
        !pool->slots[slot].occupied) return false;
    *reference = (NativeMatchPoolReference){ pool, pool->lifetime, (uint32_t)slot,
                                            pool->slots[slot].generation };
    return true;
}
