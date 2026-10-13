#ifndef NATIVE_FIGHTER_SCRIPT_RELATIONS_H
#define NATIVE_FIGHTER_SCRIPT_RELATIONS_H
#include "native_fighter_script_code.h"
const char* nativeScriptRelations_Property(const char*);
bool nativeScriptRelations_Expression(const ScriptCodeBlock*, const char*, const char**, char*, size_t);
bool nativeScriptRelations_Action(const ScriptCodeBlock*, const char*, char*, size_t);
#endif
