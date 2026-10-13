#ifndef NATIVE_FIGHTER_SCRIPT_TYPES_H
#define NATIVE_FIGHTER_SCRIPT_TYPES_H

#include "native_fighter_script.h"

typedef enum ScriptExpressionType {
    ScriptType_Invalid,
    ScriptType_Bool,
    ScriptType_Int,
    ScriptType_Float,
} ScriptExpressionType;

// Compiler context resolves a single property/call/array reference. The atom
// includes its arguments and suffix, but never surrounding binary operators.
typedef ScriptExpressionType (*ScriptTypeResolver)(const void*, const char*);
ScriptExpressionType nativeScriptTypes_Expression(const char*, ScriptTypeResolver,
                                                  const void*);
ScriptExpressionType nativeScriptTypes_Builtin(const char*);
ScriptExpressionType nativeScriptTypes_State(ScriptStateType);
ScriptExpressionType nativeScriptTypes_CType(const char*);
const char* nativeScriptTypes_Name(ScriptExpressionType);
bool nativeScriptTypes_Assignable(ScriptExpressionType, ScriptExpressionType);
// Returns the declared type and advances past an optional int/bool/float.
ScriptExpressionType nativeScriptTypes_Declaration(const char**);

#endif
