#ifndef NATIVE_FIGHTER_SCRIPT_SIGNAL_H
#define NATIVE_FIGHTER_SCRIPT_SIGNAL_H
#include "native_fighter_script_code.h"
// Stable signal identity, independent of module addresses and mutable state.
uint32_t nativeScriptSignal_Hash(const char* name);
bool nativeScriptSignal_Emit(const ScriptCodeBlock*, const char* source,
                            char* output, size_t capacity);
#endif
