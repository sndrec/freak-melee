#include "native_asset_identity.h"

#ifdef MELEE_NATIVE

#include <string.h>

static struct NativeAssetIdentityRegistry {
    NativeAssetIdentityRegion regions[NATIVE_ASSET_IDENTITY_REGION_CAPACITY];
    size_t count;
} registry;

uint64_t nativeAssetIdentity_Hash(const void* memory, size_t size)
{
    if (memory == NULL && size != 0) {
        return 0;
    }
    const uint8_t* bytes = memory;
    uint64_t hash = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

uint64_t nativeAssetIdentity_Derive(uint64_t content_hash, uint64_t role)
{
    uint64_t hash = content_hash;
    for (uint32_t byte = 0; byte < 8; ++byte) {
        hash ^= (uint8_t) (role >> (byte * 8));
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

bool nativeAssetIdentity_Register(const void* memory, size_t size,
                                  uint64_t identity)
{
    if (memory == NULL || size == 0 || identity == 0) {
        return false;
    }
    uintptr_t start = (uintptr_t) memory;
    if (start > UINTPTR_MAX - size) {
        return false;
    }
    uintptr_t end = start + size;
    for (size_t i = 0; i < registry.count; ++i) {
        uintptr_t existing_start = (uintptr_t) registry.regions[i].memory;
        uintptr_t existing_end = existing_start + registry.regions[i].size;
        if (start == existing_start && size == registry.regions[i].size &&
            identity == registry.regions[i].identity)
        {
            return true;
        }
        if (start < existing_end && existing_start < end) {
            return false;
        }
    }
    if (registry.count == NATIVE_ASSET_IDENTITY_REGION_CAPACITY) {
        return false;
    }
    registry.regions[registry.count++] = (NativeAssetIdentityRegion){
        .memory = memory,
        .size = size,
        .identity = identity,
    };
    return true;
}

size_t nativeAssetIdentity_GetRegions(NativeAssetIdentityRegion* regions,
                                      size_t capacity)
{
    if (regions != NULL && capacity != 0) {
        size_t copy_count =
            registry.count < capacity ? registry.count : capacity;
        memcpy(regions, registry.regions, copy_count * sizeof(*regions));
    }
    return registry.count;
}

#endif
