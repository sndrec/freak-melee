#pragma once
#include <string>
#include <vector>

#include "native_fighter_animation_editor.h"

namespace NativeAnimationEdits {
    enum {
        Time = 1,
        Value = 2,
        Tangent = 4
    };
    struct Change {
        NativeFighterAnimationKeyRef ref;
        int frame;
        float value, tangent;
        unsigned fields;
    };
    // Authoring operation on a staged project or a gesture with a saved
    // baseline. Validates all requested fields before editing, snaps to the
    // source encoding, retimes once, and preserves source key indices. The
    // owner publishes the whole edit or restores its baseline on failure.
    bool apply(NativeFighterProject&, NativeFighterAnimationClip,
               const std::vector<Change>& requested,
               std::vector<Change>& applied, std::string& error);
}
