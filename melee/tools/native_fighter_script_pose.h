#ifndef NATIVE_FIGHTER_SCRIPT_POSE_H
#define NATIVE_FIGHTER_SCRIPT_POSE_H

#include "native_fighter_script_code.h"

enum { SCRIPT_POSE_GRAPHS = 16, SCRIPT_POSE_PATH_BYTES = 64 };
typedef struct ScriptPoseGraph {
    char name[32];
    char path[SCRIPT_POSE_PATH_BYTES];
    uint32_t hash;
    uint16_t slot;
} ScriptPoseGraph;
typedef struct ScriptPoseProgram {
    ScriptPoseGraph graphs[SCRIPT_POSE_GRAPHS];
    uint8_t order[SCRIPT_POSE_GRAPHS];
    unsigned count;
} ScriptPoseProgram;

// Build-time borrowed catalog, never a runtime pointer exposed to scripts.
bool nativeScriptPose_Parse(char*, size_t, ScriptPoseProgram*, NativeFighterScriptError*);
void nativeScriptPose_Emit(const ScriptPoseProgram*, FILE*);
void nativeScriptPose_EmitDefinition(const ScriptPoseProgram*, FILE*);
bool nativeScriptPose_Action(const ScriptCodeBlock*, const char*, char*, size_t);
const char* nativeScriptPose_Constant(const char*);

#endif
