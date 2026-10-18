#pragma once
#include "native_fighter_project.h"
#include <array>
#include <vector>

// Authoring history for the matching fighter/animation/costume/facts set.
// Scripts, parameters and names have independent editing owners.
struct NativeFighterCoreAssetState {
    std::array<std::vector<uint8_t>,4> bytes;
    NativeSourceAssetFacts facts = {};
    uint64_t rig_revision = 0;
};
bool nativeFighterCoreAssets_Capture(const NativeFighterProject&,
                                    NativeFighterCoreAssetState&);
bool nativeFighterCoreAssets_Matches(const NativeFighterProject&,
                                    const NativeFighterCoreAssetState&);
// Guard the full expected state, allocate/admit the replacement privately, then
// swap all four buffers. Failure leaves the draft untouched. The serial is not
// rewound, so a new edit after Undo cannot reuse a discarded history token.
bool nativeFighterCoreAssets_Restore(NativeFighterProject&,
    const NativeFighterCoreAssetState& expected,
    const NativeFighterCoreAssetState& replacement,char* error,size_t capacity);
