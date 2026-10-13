#ifndef NATIVE_FIGHTER_SCRIPT_SURFACE_POLICY_H
#define NATIVE_FIGHTER_SCRIPT_SURFACE_POLICY_H
#include "native_fighter_script.h"

typedef struct ScriptSurfacePolicy {
    bool declared;
    bool ignore;
} ScriptSurfacePolicy;
bool nativeScriptSurfacePolicy_Parse(char*, size_t, ScriptSurfacePolicy*,
                                     NativeFighterScriptError*);
void nativeScriptSurfacePolicy_EmitDefinition(const ScriptSurfacePolicy*, FILE*);
#endif
