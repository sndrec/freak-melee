#ifndef NATIVE_FIGHTER_SCRIPT_VECTORS_H
#define NATIVE_FIGHTER_SCRIPT_VECTORS_H
#include <stddef.h>
#include "native_fighter_script_code.h"
// Pure compiler emission. Inputs are already translated ordinary expressions.
int nativeScriptVectors_CrossComponent(const char* name,
    const char values[6][128], char* output, size_t capacity);
bool nativeScriptVectors_Action(const ScriptCodeBlock*, const char* source,
                                char* output, size_t capacity);
#endif
