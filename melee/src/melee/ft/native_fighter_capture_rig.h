#ifndef MELEE_FT_NATIVE_FIGHTER_CAPTURE_RIG_H
#define MELEE_FT_NATIVE_FIGHTER_CAPTURE_RIG_H
#include "native_fighter_lifetime.h"

// Resolve a semantic joint only through the retained actor's own admitted rig.
// Neither a different fighter's parts table nor an omitted/missing joint is
// an attachment target. The output stays unchanged on failure.
bool nativeFighterCapture_Joint(const NativeFighterLifetime*,Fighter_Part,HSD_JObj**);
#endif
