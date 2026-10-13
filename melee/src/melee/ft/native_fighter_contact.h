#ifndef MELEE_FT_NATIVE_FIGHTER_CONTACT_H
#define MELEE_FT_NATIVE_FIGHTER_CONTACT_H
#include <melee/ft/forward.h>
#include "native_script_vm.h"

// Query the current body-contact recipient, honoring its authored policy.
// Failure/retirement preserves the caller's output. Both contact actors and
// their relation remain pinned through the recipient's true or false decision.
NativeScriptVMStatus nativeFighterContact_ReboundAllowed(Fighter_GObj* source,
                                                       bool* allowed);
#endif
