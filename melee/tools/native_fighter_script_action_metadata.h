#ifndef NATIVE_FIGHTER_SCRIPT_ACTION_METADATA_H
#define NATIVE_FIGHTER_SCRIPT_ACTION_METADATA_H

#include "native_fighter_script_code.h"

// source starts at '('; after points past the closing ')'.
// All four arguments must have integer type. The canonical word is a value,
// not the native byte layout of Struct2070.
bool nativeScriptActionMetadata_Expression(const ScriptCodeBlock* block,
                                           const char* source, char* output,
                                           size_t capacity, const char** after);

#endif
