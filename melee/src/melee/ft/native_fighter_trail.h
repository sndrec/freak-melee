#ifndef MELEE_FT_NATIVE_FIGHTER_TRAIL_H
#define MELEE_FT_NATIVE_FIGHTER_TRAIL_H

#include <stdbool.h>
#include <stdint.h>

// Authored ribbons use the existing three-sample afterimage renderer. Values
// are supplied without allocating or retaining pointers into package data.
typedef struct NativeFighterTrail {
    int32_t bone; // Costume joint index, not a stock fighter part number.
    int32_t axis; // Joint matrix column: 0=X, 1=Y, 2=Z.
    float inner_distance;
    float outer_distance;
    float inner_fade;
    float outer_fade;
    int32_t start_alpha;
    int32_t end_alpha;
    int32_t inner_red, inner_green, inner_blue;
    int32_t outer_red, outer_green, outer_blue;
} NativeFighterTrail;

// Invalid descriptors disable the ribbon for that sample. In particular an
// unset bone cannot select a joint through an uninitialized stock attribute.
bool nativeFighterTrail_Valid(const NativeFighterTrail* trail,
                              unsigned joint_count);

#endif
