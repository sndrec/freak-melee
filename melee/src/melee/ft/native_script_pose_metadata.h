#ifndef MELEE_FT_NATIVE_SCRIPT_POSE_METADATA_H
#define MELEE_FT_NATIVE_SCRIPT_POSE_METADATA_H
#include "native_script_expression.h"
#include "native_script_source.h"

struct NativeScriptCompileBank;
struct NativeSourcePoseGraph;
enum { NATIVE_SCRIPT_POSE_SYMBOLS=16 };
typedef struct NativeScriptPoseSymbol {
    NativeScriptSourceSpan name,source;
    uint32_t row; // Exact immutable owned-facts index; never a native address.
} NativeScriptPoseSymbol;
typedef struct NativeScriptPoseResolverContext {
    const struct NativeScriptCompileBank* bank;
    uint32_t owner,context_mask;
} NativeScriptPoseResolverContext;

bool nativeScriptPoseMetadata_Owns(const NativeScriptSourceTree*,uint32_t node);
// Load-only: bounded parsing/exact owned tuple validation, no allocation.
bool nativeScriptPoseMetadata_Prepare(struct NativeScriptCompileBank*,
    const struct NativeSourcePoseGraph*,uint32_t count,uint16_t mask,NativeScriptDiagnostic*);
// Revalidation may admit only a node in this exact bank's prepared table.
bool nativeScriptPoseMetadata_Validate(const struct NativeScriptCompileBank*,
    const NativeScriptSourceTree*,uint32_t node,NativeScriptDiagnostic*);
bool nativeScriptPoseMetadata_CheckSymbols(const struct NativeScriptCompileBank*,NativeScriptDiagnostic*);
// 1 resolved, 0 unrelated, -1 matched invalid owner/context. Literal bits=row.
int nativeScriptBindingPose_TryResolve(void*,const NativeScriptIRArena*,
    const NativeScriptIRNode*,NativeScriptIRResolution*,NativeScriptDiagnostic*);
#endif
