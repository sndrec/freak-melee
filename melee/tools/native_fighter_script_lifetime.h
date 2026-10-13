#ifndef NATIVE_FIGHTER_SCRIPT_LIFETIME_H
#define NATIVE_FIGHTER_SCRIPT_LIFETIME_H

#include "native_fighter_script_code.h"

typedef enum ScriptCallbackClass {
    ScriptCallback_Fighter,
    ScriptCallback_Item,
    ScriptCallback_ModelEffect,
} ScriptCallbackClass;

typedef enum ScriptCallbackResult {
    ScriptCallback_Void,
    ScriptCallback_Bool,
    ScriptCallback_Int,
    ScriptCallback_Float,
} ScriptCallbackResult;

bool nativeScriptLifetime_DirectEffect(const char*);
bool nativeScriptLifetime_LineMayInvalidate(const char*, const ScriptFunctionProgram*,
                                            ScriptCallbackClass);
bool nativeScriptLifetime_BlockMayInvalidate(const ScriptCodeBlock*);
void nativeScriptLifetime_EmitEntry(FILE*, ScriptCallbackClass);
void nativeScriptLifetime_EmitLine(FILE*, const char*, const ScriptFunctionProgram*,
                                  ScriptCallbackClass, ScriptCallbackResult);
// Validates the bounded targets.call source surface. 0: absent, 1: valid whole
// call (optionally not), -1: nested/malformed. Emits the direct Host expression.
int nativeScriptLifetime_TargetCall(const char*, char arguments[2][192], bool* negate);
const char* nativeScriptLifetime_TargetHost(const char*);
ScriptExpressionType nativeScriptLifetime_TargetType(const char*);
bool nativeScriptLifetime_TargetSurface(const char*);

#endif
