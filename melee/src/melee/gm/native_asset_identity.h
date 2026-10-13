#ifndef MELEE_GM_NATIVE_ASSET_IDENTITY_H
#define MELEE_GM_NATIVE_ASSET_IDENTITY_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    NATIVE_ASSET_IDENTITY_REGION_CAPACITY = 64,
};

typedef struct NativeAssetIdentityRegion {
    const void* memory;
    size_t size;
    uint64_t identity;
} NativeAssetIdentityRegion;

uint64_t nativeAssetIdentity_Hash(const void* memory, size_t size);
uint64_t nativeAssetIdentity_Derive(uint64_t content_hash, uint64_t role);
bool nativeAssetIdentity_Register(const void* memory, size_t size,
                                  uint64_t identity);
size_t nativeAssetIdentity_GetRegions(NativeAssetIdentityRegion* regions,
                                      size_t capacity);

#endif
#endif
