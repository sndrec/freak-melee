#ifndef NATIVE_FIGHTER_SCRIPT_COLLISION_H
#define NATIVE_FIGHTER_SCRIPT_COLLISION_H
#include "native_fighter_script_code.h"
bool nativeScriptCollision_Expression(const ScriptCodeBlock*, const char*,
                                      const char**, char*, size_t);
bool nativeScriptCollision_Action(const ScriptCodeBlock*, const char*, char*, size_t);
#endif
