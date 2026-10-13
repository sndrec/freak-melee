#ifndef MELEE_GM_NATIVE_ROLLBACK_HASH_H
#define MELEE_GM_NATIVE_ROLLBACK_HASH_H

#ifdef MELEE_NATIVE

#include <stdint.h>

#include "native_rollback_regions.h"

// Replaces recognized native addresses with stable region identities before
// hashing. This removes ordinary ASLR differences, but it is not yet a portable
// gameplay checksum: opaque packed values, padding, and draw-owned cache state
// can still differ between processes.
uint64_t
nativeRollbackHash_AddressNormalized(const NativeRollbackRegionSet* regions);
uint64_t nativeRollbackHash_AddressNormalizedPagedRegion(
    const NativeRollbackRegionSet* regions, uint32_t region);
uint64_t nativeRollbackHash_AddressNormalizedExactRegion(
    const NativeRollbackRegionSet* regions, uint32_t region);
uint64_t nativeRollbackHash_AddressNormalizedRange(
    const NativeRollbackRegionSet* regions, const void* memory, size_t size);

#endif
#endif
