#ifndef NATIVE_FIGHTER_SCRIPT_TERRAIN_H
#define NATIVE_FIGHTER_SCRIPT_TERRAIN_H

#include "native_fighter_script_code.h"

ScriptExpressionType nativeScriptTerrain_Type(const ScriptCodeBlock*, const char*);
bool nativeScriptTerrain_Expression(const ScriptCodeBlock*, const char* name,
                                   const char** source, char* output, size_t capacity);
bool nativeScriptTerrain_Action(const ScriptCodeBlock*, const char*, char*, size_t);

#endif
