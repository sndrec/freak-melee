#ifndef NATIVE_FIGHTER_SCRIPT_JUMP_H
#define NATIVE_FIGHTER_SCRIPT_JUMP_H

#include "native_fighter_script_code.h"

const char* nativeScriptJump_Property(const char* name);
bool nativeScriptJump_Expression(const ScriptCodeBlock*, const char* name,
                                 const char** source, char* output,
                                 size_t capacity);
bool nativeScriptJump_Action(const ScriptCodeBlock*, const char* source,
                             char* output, size_t capacity);

#endif
