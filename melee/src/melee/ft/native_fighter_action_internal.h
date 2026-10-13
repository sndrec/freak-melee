#ifndef MELEE_FT_NATIVE_FIGHTER_ACTION_INTERNAL_H
#define MELEE_FT_NATIVE_FIGHTER_ACTION_INTERNAL_H
#include "native_fighter_action_editor.h"
#include <sysdolphin/baselib/dat.h>
#ifdef MELEE_NATIVE
bool nativeFighterAction_OpenTable(const NativeFighterProject*, HSD_Dat*,
                                   u32*);
#endif
#endif
