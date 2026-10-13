#ifndef NATIVE_FIGHTER_SCRIPT_TRAIL_H
#define NATIVE_FIGHTER_SCRIPT_TRAIL_H

#include "native_fighter_script_code.h"

const char* nativeScriptTrail_Field(const char* name);
bool nativeScriptTrail_Assignment(const ScriptCodeBlock* block,
                                  const char* source, char* output,
                                  size_t capacity);
bool nativeScriptTrail_ReadOnlyLine(const char* source, const char* generated);

#endif
