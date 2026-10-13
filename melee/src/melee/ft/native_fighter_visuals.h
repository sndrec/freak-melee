#ifndef MELEE_FT_NATIVE_FIGHTER_VISUALS_H
#define MELEE_FT_NATIVE_FIGHTER_VISUALS_H

#include <melee/ft/forward.h>

void nativeFighterVisuals_SetGroupMaterialFrame(Fighter*, int table, int group,
                                              int variant, float frame);
float nativeFighterVisuals_InitializeGroupMaterialAnimation(
    Fighter*, int table, int group, int variant, float prior_end_frame);
int nativeFighterVisuals_CurrentGroup(Fighter*, int group);
int nativeFighterVisuals_PreviousGroup(Fighter*, int group);

#endif
