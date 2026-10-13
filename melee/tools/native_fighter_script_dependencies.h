#ifndef NATIVE_FIGHTER_SCRIPT_DEPENDENCIES_H
#define NATIVE_FIGHTER_SCRIPT_DEPENDENCIES_H

#include <stdbool.h>
#include <stdint.h>

enum { SCRIPT_FUNCTIONS = 128, SCRIPT_FUNCTION_WORDS = SCRIPT_FUNCTIONS / 64 };

// Authoring-time dependency sets. Generated fighter code has no dependency
// tables or allocation; these sets drive recursion, context and lifetime checks.
typedef struct ScriptFunctionSet {
    uint64_t words[SCRIPT_FUNCTION_WORDS];
} ScriptFunctionSet;

static inline void nativeScriptDependencies_Set(ScriptFunctionSet* set, unsigned index)
{
    set->words[index / 64] |= UINT64_C(1) << (index % 64);
}

static inline bool nativeScriptDependencies_Test(ScriptFunctionSet set, unsigned index)
{
    return (set.words[index / 64] & (UINT64_C(1) << (index % 64))) != 0;
}

static inline void nativeScriptDependencies_Union(ScriptFunctionSet* set, ScriptFunctionSet other)
{
    for (unsigned i = 0; i < SCRIPT_FUNCTION_WORDS; ++i) set->words[i] |= other.words[i];
}

static inline bool nativeScriptDependencies_Any(ScriptFunctionSet set)
{
    for (unsigned i = 0; i < SCRIPT_FUNCTION_WORDS; ++i) if (set.words[i]) return true;
    return false;
}

static inline bool nativeScriptDependencies_Equal(ScriptFunctionSet a, ScriptFunctionSet b)
{
    for (unsigned i = 0; i < SCRIPT_FUNCTION_WORDS; ++i)
        if (a.words[i] != b.words[i]) return false;
    return true;
}

#endif
