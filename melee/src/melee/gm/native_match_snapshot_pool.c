#include "native_match_snapshot_pool.h"
#include <string.h>

static size_t bytes_for(uint32_t capacity, uint32_t used, size_t object_size)
{
    if (capacity == 0 || capacity > NATIVE_MATCH_POOL_MAX_SLOTS || used > capacity ||
        object_size == 0 || object_size > SIZE_MAX - sizeof(uint32_t)) return 0;
    size_t metadata = sizeof(NativeMatchPoolSnapshotHeader) +
        sizeof(uint32_t) * ((size_t)capacity + capacity - used);
    size_t entry = sizeof(uint32_t) + object_size;
    if (used != 0 && entry > (SIZE_MAX - metadata) / used) return 0;
    return metadata + entry * used;
}

static bool overlaps(const void* first, size_t first_size,
                     const void* second, size_t second_size)
{
    uintptr_t a = (uintptr_t)first, b = (uintptr_t)second;
    if (first_size > UINTPTR_MAX - a || second_size > UINTPTR_MAX - b) return true;
    return a < b + second_size && b < a + first_size;
}

static bool independent_buffer(const NativeMatchPool* pool, const void* buffer, size_t bytes)
{
    return buffer != NULL && pool != NULL && pool->payload != NULL &&
        pool->slots != NULL && pool->free_order != NULL && pool->lifetime != 0 &&
        pool->object_size != 0 && pool->stride >= pool->object_size && pool->capacity != 0 &&
        pool->capacity <= NATIVE_MATCH_POOL_MAX_SLOTS && pool->stride != 0 &&
        pool->stride <= SIZE_MAX / pool->capacity &&
        !overlaps(buffer, bytes, pool, sizeof(*pool)) &&
        !overlaps(buffer, bytes, pool->payload, pool->stride * pool->capacity) &&
        !overlaps(buffer, bytes, pool->slots, sizeof(*pool->slots) * pool->capacity) &&
        !overlaps(buffer, bytes, pool->free_order, sizeof(*pool->free_order) * pool->capacity);
}

static uint32_t read_u32(const uint8_t* data)
{
    uint32_t value;
    memcpy(&value, data, sizeof(value));
    return value;
}

size_t nativeMatchSnapshotPool_Bytes(const NativeMatchPool* pool)
{
    return nativeMatchPool_Validate(pool) ? bytes_for(pool->capacity, pool->used, pool->object_size) : 0;
}

bool nativeMatchSnapshotPool_Save(const NativeMatchPool* pool, void* snapshot,
                                  size_t capacity, size_t* written)
{
    if (pool == NULL) return false;
    size_t bytes = bytes_for(pool->capacity, pool->used, pool->object_size);
    if (bytes == 0 || written == NULL || capacity < bytes ||
        pool->free_count != pool->capacity - pool->used ||
        pool->peak < pool->used || pool->peak > pool->capacity ||
        pool->initialized_count < pool->peak || pool->initialized_count > pool->capacity ||
        !independent_buffer(pool, snapshot, capacity) ||
        overlaps(written, sizeof(*written), snapshot, capacity) ||
        !independent_buffer(pool, written, sizeof(*written))) return false;
    NativeMatchPoolSnapshotHeader header = { 0 };
    header.pool = pool; header.payload = pool->payload; header.lifetime = pool->lifetime;
    header.object_size = pool->object_size; header.stride = pool->stride;
    header.capacity = pool->capacity; header.used = pool->used;
    header.free_count = pool->free_count; header.peak = pool->peak;
    header.initialized_count = pool->initialized_count;
    uint8_t seen[NATIVE_MATCH_POOL_MAX_SLOTS / 8] = { 0 };
    uint8_t* generations = (uint8_t*)snapshot + sizeof(header);
    uint8_t* free_order = generations + pool->capacity * sizeof(uint32_t);
    uint8_t* cursor = free_order + pool->free_count * sizeof(uint32_t);
    uint32_t occupied = 0;
    // Validate and serialize generations/occupied records in one bounded pass.
    // Declared used may be corrupt: refuse before writing an extra record.
    for (uint32_t i = 0; i < pool->capacity; ++i) {
        memcpy(generations + i * sizeof(uint32_t), &pool->slots[i].generation, sizeof(uint32_t));
        if (!pool->slots[i].occupied) continue;
        if (pool->slots[i].generation == 0 || i >= pool->initialized_count || occupied == pool->used) return false;
        seen[i / 8] |= (uint8_t)(1u << (i % 8));
        memcpy(cursor, &i, sizeof(i)); cursor += sizeof(i);
        memcpy(cursor, pool->payload + i * pool->stride, pool->object_size);
        cursor += pool->object_size;
        ++occupied;
    }
    if (occupied != pool->used) return false;
    for (uint32_t i = 0; i < pool->free_count; ++i) {
        uint32_t slot = pool->free_order[i];
        if (slot >= pool->capacity || (seen[slot / 8] & (1u << (slot % 8)))) return false;
        seen[slot / 8] |= (uint8_t)(1u << (slot % 8));
        memcpy(free_order + i * sizeof(uint32_t), &slot, sizeof(slot));
    }
    memcpy(snapshot, &header, sizeof(header));
    *written = bytes;
    return true;
}

NativeMatchPoolSnapshotView nativeMatchSnapshotPool_View(const NativeMatchPool* pool, const void* snapshot,
                                      size_t bytes)
{
    NativeMatchPoolSnapshotView invalid={0};
    if (bytes < sizeof(NativeMatchPoolSnapshotHeader) ||
        !independent_buffer(pool, snapshot, bytes)) return invalid;
    NativeMatchPoolSnapshotHeader header;
    memcpy(&header, snapshot, sizeof(header));
    if (header.pool != pool || header.payload != pool->payload || header.lifetime == 0 ||
        header.lifetime != pool->lifetime || header.capacity != pool->capacity ||
        header.object_size != pool->object_size || header.stride != pool->stride ||
        header.used > header.capacity || header.free_count != header.capacity - header.used ||
        header.peak < header.used || header.peak > header.capacity ||
        header.initialized_count < header.peak || header.initialized_count > header.capacity ||
        bytes_for(header.capacity, header.used, header.object_size) != bytes) return invalid;
    uint8_t seen[NATIVE_MATCH_POOL_MAX_SLOTS / 8] = { 0 };
    const uint8_t* generations = (const uint8_t*)snapshot + sizeof(header);
    const uint8_t* cursor = generations + header.capacity * sizeof(uint32_t);
    for (uint32_t i = 0; i < header.free_count; ++i, cursor += sizeof(uint32_t)) {
        uint32_t slot = read_u32(cursor);
        if (slot >= header.capacity || (seen[slot / 8] & (1u << (slot % 8)))) return invalid;
        seen[slot / 8] |= (uint8_t)(1u << (slot % 8));
    }
    uint32_t previous = 0;
    for (uint32_t i = 0; i < header.used; ++i, cursor += sizeof(uint32_t) + header.object_size) {
        uint32_t slot = read_u32(cursor);
        if (slot >= header.initialized_count || (i != 0 && slot <= previous) ||
            (seen[slot / 8] & (1u << (slot % 8))) ||
            read_u32(generations + slot * sizeof(uint32_t)) == 0) return invalid;
        seen[slot / 8] |= (uint8_t)(1u << (slot % 8));
        previous = slot;
    }
    const uint8_t* records=generations+(header.capacity+header.free_count)*sizeof(uint32_t);
    return (NativeMatchPoolSnapshotView){header,generations,records};
}

const void* nativeMatchSnapshotPool_Find(NativeMatchPoolSnapshotView view,uint32_t slot)
{
    if(!view.records || !view.header.pool ||
       view.header.pool->lifetime!=view.header.lifetime || slot>=view.header.capacity)return NULL;
    uint32_t low=0,high=view.header.used;
    size_t stride=sizeof(uint32_t)+view.header.object_size;
    while(low<high) {
        uint32_t middle=low+(high-low)/2;
        uint32_t candidate=read_u32(view.records+(size_t)middle*stride);
        if(candidate<slot)low=middle+1;else high=middle;
    }
    if(low==view.header.used)return NULL;
    const uint8_t* record=view.records+(size_t)low*stride;
    return read_u32(record)==slot ? record+sizeof(uint32_t) : NULL;
}
void* nativeMatchSnapshotPool_Resolve(NativeMatchPoolSnapshotView view,NativeMatchPoolReference reference)
{
    if(!view.records || reference.pool!=view.header.pool ||
       reference.lifetime!=view.header.lifetime || !reference.generation ||
       reference.slot>=view.header.capacity ||
       read_u32(view.generations+(size_t)reference.slot*sizeof(uint32_t))!=reference.generation ||
       !nativeMatchSnapshotPool_Find(view,reference.slot))return NULL;
    return (uint8_t*)view.header.payload+(size_t)reference.slot*view.header.stride;
}

bool nativeMatchSnapshotPool_Load(NativeMatchPool* pool, const void* snapshot, size_t bytes)
{
    NativeMatchPoolSnapshotView view=nativeMatchSnapshotPool_View(pool,snapshot,bytes);
    if(!view.records)return false;
    NativeMatchPoolSnapshotHeader header=view.header;
    const uint8_t* cursor=view.generations;
    for (uint32_t i = 0; i < pool->capacity; ++i, cursor += sizeof(uint32_t))
        pool->slots[i] = (NativeMatchPoolSlot){ read_u32(cursor), false };
    memcpy(pool->free_order, cursor, header.free_count * sizeof(uint32_t));
    cursor += header.free_count * sizeof(uint32_t);
    for (uint32_t i = 0; i < header.used; ++i) {
        uint32_t slot = read_u32(cursor); cursor += sizeof(uint32_t);
        memcpy(pool->payload + slot * pool->stride, cursor, pool->object_size);
        cursor += pool->object_size;
        pool->slots[slot].occupied = true;
    }
    pool->used = header.used; pool->free_count = header.free_count; pool->peak = header.peak;
    pool->initialized_count = header.initialized_count;
    return true;
}
