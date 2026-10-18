#ifndef MELEE_NATIVE_FIGHTER_COSTUME_COMPACT_H
#define MELEE_NATIVE_FIGHTER_COSTUME_COMPACT_H
#include "native_fighter_project.h"
#include <vector>

struct NativeCostumeRange { uint32_t source, destination, size; };
struct NativeCostumeCompaction {
    size_t before = 0, after = 0, live = 0, pinned = 0;
    unsigned groups = 0, relocations = 0;
    bool supported = false;
    char reason[160] = {};
    std::vector<NativeCostumeRange> ranges;
};
// Authoring transaction. Bone/mesh/material/texture/sequence identities stay at
// their existing offsets; other reachable storage is packed into the free gaps.
// Unknown roots or opaque references keep the complete input unchanged.
// Invalid known graphs fail without publishing a partial result.
bool nativeFighterCostume_Compact(const NativeFighterProject*, std::vector<uint8_t>*,
                                  NativeCostumeCompaction*, char* error, size_t error_size);
#endif
