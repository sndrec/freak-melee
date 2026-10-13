#ifndef MELEE_FT_NATIVE_FIGHTER_MOVE_COPY_H
#define MELEE_FT_NATIVE_FIGHTER_MOVE_COPY_H

#ifdef MELEE_NATIVE

#include "native_fighter_project.h"

// Copies one compatible Mario-shaped normal asset action and its source command
// graph, animation clip and animation flags between independent drafts. The
// action must have the same archive geometry and no shared commands or clip.
// Owned facts/rig geometry must match. Source FMS/state are preserved and never
// transferred; scripts may remain present. No donor behavior lookup is used.
bool nativeFighterMove_CopyNormalAction(const NativeFighterProject* source,
                                        NativeFighterProject* destination,
                                        unsigned action);

typedef enum NativeFighterMoveGroup {
    NativeFighterMoveGroup_Jab,
    NativeFighterMoveGroup_ForwardTilt,
    NativeFighterMoveGroup_ForwardSmash,
    NativeFighterMoveGroup_NeutralAir,
    NativeFighterMoveGroup_ForwardAir,
    NativeFighterMoveGroup_BackAir,
    NativeFighterMoveGroup_UpAir,
    NativeFighterMoveGroup_DownAir,
} NativeFighterMoveGroup;

// Copy every stock action variant in one logical normal move. Either all
// source command graphs/clips replace the destination's in-memory copies or
// none do. The caller saves the two changed draft archives afterward.
bool nativeFighterMove_CopyNormalGroup(const NativeFighterProject* source,
                                       NativeFighterProject* destination,
                                       NativeFighterMoveGroup group);

#endif
#endif
