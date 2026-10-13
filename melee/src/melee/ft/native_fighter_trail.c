#include "native_fighter_trail.h"

#include <math.h>
#include <stddef.h>

bool nativeFighterTrail_Valid(const NativeFighterTrail* trail,
                              unsigned joint_count)
{
    return trail != NULL && trail->bone >= 0 &&
        (unsigned) trail->bone < joint_count &&
        trail->axis >= 0 && trail->axis <= 2 &&
        isfinite(trail->inner_distance) && isfinite(trail->outer_distance) &&
        isfinite(trail->inner_fade) && isfinite(trail->outer_fade) &&
        (unsigned) trail->start_alpha <= 255 &&
        (unsigned) trail->end_alpha <= 255 &&
        (unsigned) trail->inner_red <= 255 &&
        (unsigned) trail->inner_green <= 255 &&
        (unsigned) trail->inner_blue <= 255 &&
        (unsigned) trail->outer_red <= 255 &&
        (unsigned) trail->outer_green <= 255 &&
        (unsigned) trail->outer_blue <= 255;
}
