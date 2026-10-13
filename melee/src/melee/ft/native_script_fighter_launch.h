#ifndef MELEE_FT_NATIVE_SCRIPT_FIGHTER_LAUNCH_H
#define MELEE_FT_NATIVE_SCRIPT_FIGHTER_LAUNCH_H
#include <stdbool.h>

// Direct offline diagnostic/authoring entry. Loads and validates source before
// any match actor can reference it. Does not manufacture a fighter package or
// claim full Workshop/template activation.
bool nativeScriptFighterLaunch_Load(const char* kind,const char* path);

#endif
