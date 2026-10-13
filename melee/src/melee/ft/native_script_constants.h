#ifndef MELEE_FT_NATIVE_SCRIPT_CONSTANTS_H
#define MELEE_FT_NATIVE_SCRIPT_CONSTANTS_H

#include "native_script_ir.h"

typedef enum NativeScriptConstantFamily {
    NativeScriptConstant_Joint, NativeScriptConstant_CommonMotion,
    NativeScriptConstant_TransitionFlag, NativeScriptConstant_Item,
    NativeScriptConstant_ItemCategory, NativeScriptConstant_HitStatus,
    NativeScriptConstant_CollisionMask, NativeScriptConstant_Axis,
    NativeScriptConstant_DebugLevel, NativeScriptConstant_AnimationFamily,
    NativeScriptConstant_Callback,
    NativeScriptConstant_CPUCommand,
    NativeScriptConstant_ShieldTransition,
    NativeScriptConstant_StageChannel, NativeScriptConstant_Smash,
} NativeScriptConstantFamily;

typedef struct NativeScriptConstant {
    const char* name;
    const char* source_evidence;
    uint64_t bits;
    NativeScriptIRType type;
    NativeScriptConstantFamily family;
} NativeScriptConstant;

#define NATIVE_SCRIPT_CONSTANT_NAMESPACE UINT32_C(0x3d000000)

// Immutable load-time catalog. Unknown names leave output unchanged. Enum values
// come from current native declarations, never guessed fighter motion indices.
uint32_t nativeScriptConstants_Count(void);
bool nativeScriptConstants_At(uint32_t index, NativeScriptConstant* output);
bool nativeScriptConstants_Lookup(const char* identifier, uint32_t length,
                                 NativeScriptConstant* output);

// 1: scalar constant, 2: namespace, 0: unrelated path, -1: invalid constant path.
// Scalar output is a literal recipe for the frontend, not a runtime binding ID.
// String/Text values are never interpreted as identifiers by this API.
int nativeScriptConstants_LookupNode(const NativeScriptIRArena*,
    const NativeScriptIRNode*, NativeScriptConstant* scalar,
    NativeScriptIRResolution* name_space, NativeScriptDiagnostic*);
// 1: resolved literal/namespace, 0: unrelated, -1: invalid constant path.
int nativeScriptConstants_TryResolve(void*, const NativeScriptIRArena*,
    const NativeScriptIRNode*, NativeScriptIRResolution*, NativeScriptDiagnostic*);

#endif
