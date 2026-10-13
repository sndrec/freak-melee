#ifndef NATIVE_FIGHTER_SCRIPT_STORE_H
#define NATIVE_FIGHTER_SCRIPT_STORE_H

#include "native_fighter_script_state.h"

// Effectful external stores evaluate index/value once, then validate the
// originally selected creator generation and resolve its destination afresh.
bool nativeScriptStore_External(const ScriptStateField*, bool creator,
    const char* index, const char* value, char* output, size_t capacity);

#endif
