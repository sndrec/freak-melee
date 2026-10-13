#ifndef MELEE_FT_NATIVE_FIGHTER_STATE_H
#define MELEE_FT_NATIVE_FIGHTER_STATE_H

#include "native_fighter_runtime.h"

// Called once while loading a module, never during fighter simulation.
bool nativeFighterState_Validate(const NativeFighterStateField*, unsigned count,
                                unsigned declared_bytes, unsigned capacity);

#endif
