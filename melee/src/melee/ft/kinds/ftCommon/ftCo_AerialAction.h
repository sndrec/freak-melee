#ifndef MELEE_FT_CO_AERIAL_ACTION_H
#define MELEE_FT_CO_AERIAL_ACTION_H
#include <melee/ft/forward.h>
#include <stdbool.h>

// Ordered optional aerial actions at the shared Fall/Jump interrupt boundaries.
bool ftCo_AerialAction_CheckManual(Fighter_GObj*);
bool ftCo_AerialAction_CheckAutomatic(Fighter_GObj*);
#endif
