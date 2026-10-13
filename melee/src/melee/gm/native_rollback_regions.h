#ifndef MELEE_GM_NATIVE_ROLLBACK_REGIONS_H
#define MELEE_GM_NATIVE_ROLLBACK_REGIONS_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "native_asset_identity.h"
#include "native_rollback_pages.h"

enum {
    NATIVE_ROLLBACK_HEAP_REGION_CAPACITY = 6,
    NATIVE_ROLLBACK_REGION_CAPACITY = NATIVE_ROLLBACK_HEAP_REGION_CAPACITY + 5,
    NATIVE_ROLLBACK_EXACT_REGION_CAPACITY = 10,
};

typedef struct NativeRollbackRegionSet {
    NativeRollbackPagedRegion regions[NATIVE_ROLLBACK_REGION_CAPACITY];
    NativeRollbackExactRegion
        exact_regions[NATIVE_ROLLBACK_EXACT_REGION_CAPACITY];
    NativeAssetIdentityRegion
        identity_regions[NATIVE_ASSET_IDENTITY_REGION_CAPACITY];
    NativeRollbackPagedRegion globals;
    uint32_t count;
    uint32_t exact_count;
    uint32_t identity_count;
    size_t tracked_bytes;
} NativeRollbackRegionSet;

bool nativeRollbackRegions_Build(NativeRollbackRegionSet* result);
uint64_t nativeRollbackRegions_Hash(const NativeRollbackRegionSet* regions);

#endif
#endif
