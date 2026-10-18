#pragma once

extern "C" {
#include <sysdolphin/baselib/dat.h>
}
#include <cstddef>
#include <cstdint>

struct NativeAssetDat;

// Joint offsets address the original archive, in old/new costume preorder.
// Newly added, empty joints are omitted from the new list. The caller owns the
// private candidate transaction; this function may append selector records.
bool nativeFighterModel_RemapMetalSelectors(
    const HSD_Dat& source, const char* fighter_root,
    const uint32_t* before, size_t before_count,
    const uint32_t* after, size_t after_count, NativeAssetDat& candidate,
    char* error, size_t capacity);
