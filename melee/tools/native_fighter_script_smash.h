#ifndef NATIVE_FIGHTER_SCRIPT_SMASH_H
#define NATIVE_FIGHTER_SCRIPT_SMASH_H

#include "native_fighter_script_code.h"

// Common charge storage remains owned by the engine's ordinary frame update.
const char* nativeScriptSmash_Property(const char* name);
ScriptExpressionType nativeScriptSmash_Type(const char* name);
bool nativeScriptSmash_Action(const ScriptCodeBlock*, const char* source,
                             char* output, size_t capacity);

#endif
