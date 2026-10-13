#ifndef NATIVE_FIGHTER_SCRIPT_KINETICS_H
#define NATIVE_FIGHTER_SCRIPT_KINETICS_H
#include "native_fighter_script_code.h"
bool nativeScriptKinetics_Expression(const ScriptCodeBlock*, const char*,
    const char**, char*, size_t);
bool nativeScriptKinetics_Action(const ScriptCodeBlock*, const char*, char*, size_t);
#endif
