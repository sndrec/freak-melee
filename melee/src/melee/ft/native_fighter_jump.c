#include "native_fighter_jump.h"

#ifdef MELEE_NATIVE
#include "types.h"
#include "native_fighter_runtime.h"
#include <math.h>

float nativeFighterJump_MultiVerticalSpeed(Fighter* fighter, int index)
{
    if (fighter == NULL || !fighter->can_multijump ||
        fighter->x2D0 == NULL || fighter->x2D0 != fighter->dat_attrs)
        return 0.0f;
    const struct Fighter_x2D0_t* jumps = fighter->x2D0;
    const unsigned capacity = sizeof(jumps->x14) / sizeof(jumps->x14[0]);
    if (jumps->x28 < 1 || (unsigned) jumps->x28 > capacity ||
        (unsigned) index >= (unsigned) jumps->x28)
        return 0.0f;
    return jumps->x14[index];
}

bool nativeFighterJump_ConfigureMulti(Fighter* fighter,
                                     const NativeFighterMultiJumpStats* stats)
{
    if (fighter == NULL || stats == NULL || !fighter->can_multijump ||
        fighter->x2D0 == NULL || fighter->x2D0 != fighter->dat_attrs ||
        stats->turn_frames < 1 || stats->count < 1 || stats->count > 5 ||
        !isfinite(stats->reverse_threshold) ||
        !isfinite(stats->horizontal_impulse) ||
        !isfinite(stats->acceleration_multiplier) ||
        !isfinite(stats->speed_multiplier)) return false;
    for (unsigned i = 0; i < 5; ++i)
        if (!isfinite(stats->vertical_impulses[i])) return false;
    const NativeFighterDefinition* definition =
        nativeFighterRuntime_Definition(fighter->kind);
    if (definition == NULL) return false;
    int first = definition->motion_state_first;
    int end = first + definition->motion_state_count;
    if (stats->first_motion < first || stats->first_motion > end - stats->count ||
        (stats->helmet_first_motion != -1 &&
         (stats->helmet_first_motion < first ||
          stats->helmet_first_motion > end - stats->count))) return false;
    struct Fighter_x2D0_t* jumps = fighter->x2D0;
    jumps->x0 = stats->turn_frames;
    jumps->x4 = stats->reverse_threshold;
    jumps->x8 = stats->horizontal_impulse;
    jumps->xC = stats->acceleration_multiplier;
    jumps->x10 = stats->speed_multiplier;
    for (unsigned i = 0; i < 5; ++i)
        jumps->x14[i] = stats->vertical_impulses[i];
    jumps->x28 = stats->count;
    jumps->x2C = stats->first_motion;
    jumps->x30 = stats->helmet_first_motion;
    fighter->co_attrs.max_jumps = stats->count + 1;
    return true;
}
#endif
