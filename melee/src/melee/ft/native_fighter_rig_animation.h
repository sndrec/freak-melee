#ifndef MELEE_FT_NATIVE_FIGHTER_RIG_ANIMATION_H
#define MELEE_FT_NATIVE_FIGHTER_RIG_ANIMATION_H
#include "native_fighter_project.h"

typedef struct NativeFighterRigAnimation {
    uint8_t* fighter;
    size_t fighter_size;
    uint8_t* animation;
    size_t animation_size;
    unsigned owned_roots, foreign_roots, unreferenced_roots, descriptors;
} NativeFighterRigAnimation;

#ifdef __cplusplus
extern "C" {
#endif
// Prepare a matching fighter/animation pair for a structural rig transaction.
// Maps address physical parts, including reserved omission parts. Omission
// identities/order and animation family remain unchanged; each clip's mask
// determines its animation-node map. Foreign-family and independent unreferenced
// roots retain their tracks. Unreferenced aliases follow their referenced tree.
// Descriptor aliases and unresolved demos retain names.
// No project state changes. Output must be empty and owns two malloc buffers
// on success. The caller publishes it with the matching costume/facts/ID table.
bool nativeFighterRig_RemapAnimationBank(const NativeFighterProject* source,
    const NativeSourceAssetFacts* destination, const uint16_t* new_to_old,
    unsigned destination_parts, NativeFighterRigAnimation* output,
    char* error, size_t error_size);
#ifdef __cplusplus
}
#endif
#endif
