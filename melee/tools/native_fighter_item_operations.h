#ifndef NATIVE_FIGHTER_ITEM_OPERATIONS_H
#define NATIVE_FIGHTER_ITEM_OPERATIONS_H
#include "native_fighter_item_script.h"
#include "native_fighter_script_functions.h"

bool nativeItemOperations_Action(const char*, const ArticleDeclaration*,
                                 const NativeFighterParameters*, char*, size_t);
const ScriptFunction* nativeItemOperations_FighterFunction(const char*, const ScriptFunctionProgram*, char [9][192]);
const char* nativeItemOperations_Property(const char* token, bool writable);
bool nativeItemOperations_DynamicJoint(const char*, char [192], unsigned*, const char**);
#endif
