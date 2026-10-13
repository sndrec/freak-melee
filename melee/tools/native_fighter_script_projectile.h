#ifndef NATIVE_FIGHTER_SCRIPT_PROJECTILE_H
#define NATIVE_FIGHTER_SCRIPT_PROJECTILE_H

#include "native_fighter_script_code.h"

bool nativeScriptProjectile_Emit(const ScriptCodeBlock* block, const char* source,
                                 char* output, size_t capacity);
bool nativeScriptProjectile_CreateExpression(const ScriptCodeBlock*, const char*, bool,
                                             char*, size_t, const char**);

#endif
