#ifndef MELEE_FT_NATIVE_SCRIPT_MODEL_METADATA_H
#define MELEE_FT_NATIVE_SCRIPT_MODEL_METADATA_H
#include "native_script_source.h"
#include "native_fighter_runtime.h"

typedef struct NativeScriptModelMetadata {
    NativeScriptSourceSpan name; // Borrows the immutable source text.
    bool animation;
    union {
        uint32_t root_offset;
        NativeFighterArticleAnimationComponent components[3]; // Joint/material/shape.
    };
} NativeScriptModelMetadata;

// Root: Name OFFSET. Animation: Name TRIPLET_OFFSET, or Name followed by
// joint/material/shape OFFSET[INDEX] or None pairs in any order. Missing channels
// are omitted. Every pointer offset is aligned; only successful parses publish.
bool nativeScriptModelMetadata_Parse(const NativeScriptSourceTree*,
    NativeScriptSourceSpan value, bool animation, NativeScriptModelMetadata*);
#endif
