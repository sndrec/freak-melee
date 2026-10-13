#ifndef MELEE_FT_NATIVE_FIGHTER_JUMP_H
#define MELEE_FT_NATIVE_FIGHTER_JUMP_H

#include <melee/ft/forward.h>
#include <stdbool.h>

typedef struct NativeFighterMultiJumpStats {
    int turn_frames;
    float reverse_threshold;
    float horizontal_impulse;
    float acceleration_multiplier;
    float speed_multiplier;
    float vertical_impulses[5];
    int count;
    int first_motion;
    int helmet_first_motion;
} NativeFighterMultiJumpStats;

#ifdef MELEE_NATIVE
// Read an impulse from this fighter's owned, common-scaled multijump resource.
// Absent/unbound tables, invalid declared counts and out-of-range rows return 0.
float nativeFighterJump_MultiVerticalSpeed(Fighter*, int index);
// Atomically replace owned tuning before common attribute scaling. Total
// maximum jumps becomes count+1; the current used-jump count is preserved.
bool nativeFighterJump_ConfigureMulti(Fighter*, const NativeFighterMultiJumpStats*);
#endif

#endif
