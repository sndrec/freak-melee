#ifndef NATIVE_FIGHTER_SCRIPT_MOTION_POLICY_H
#define NATIVE_FIGHTER_SCRIPT_MOTION_POLICY_H

#include "native_fighter_script.h"

typedef struct ScriptMotionPolicy {
    uint16_t motion;
    uint16_t common_motion;
} ScriptMotionPolicy;

typedef struct ScriptMotionPolicies {
    ScriptMotionPolicy bindings[1024];
    unsigned count;
} ScriptMotionPolicies;

// Consume explicit top-level declarations without changing source line numbers.
bool nativeScriptMotionPolicy_Parse(char* source, size_t size,
                                    ScriptMotionPolicies*,
                                    NativeFighterScriptError*);
void nativeScriptMotionPolicy_EmitChecks(const ScriptMotionPolicies*, FILE*);
void nativeScriptMotionPolicy_EmitBindings(const ScriptMotionPolicies*, FILE*,
                                           bool independent);

#endif
