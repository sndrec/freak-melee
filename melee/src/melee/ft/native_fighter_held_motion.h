#ifndef MELEE_FT_NATIVE_FIGHTER_HELD_MOTION_H
#define MELEE_FT_NATIVE_FIGHTER_HELD_MOTION_H

#include <melee/ft/forward.h>
#include <stdint.h>

// Select before publishing a shared held-item transition. A negative result
// means the callback invalidated its source and the entry must stop.
int32_t nativeFighterHeldMotion_Select(Fighter_GObj*, int32_t common_motion,
                                      int32_t previous_motion);

#endif
