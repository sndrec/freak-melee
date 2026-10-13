#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_SPAWN_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_SPAWN_H

#include "native_script_bindings.h"
#include "native_script_program.h"

// The selected source declaration is immutable code data, never a text lookup
// during execution. Operation and owner fit in the native semantic ID domain.
#define NATIVE_SCRIPT_SPAWN_DOMAIN UINT32_C(0x02000000)
#define NATIVE_SCRIPT_SPAWN_DOMAIN_MASK UINT32_C(0xff000000)
#define NATIVE_SCRIPT_SPAWN_OPERATION_MASK UINT32_C(0x00f00000)
#define NATIVE_SCRIPT_SPAWN_OWNER_MASK UINT32_C(0x000fffff)
enum {
    NativeScriptSpawn_Projectile=1, NativeScriptSpawn_Attached,
    NativeScriptSpawn_AttachedPresentation,
    NativeScriptSpawn_AtPosition, NativeScriptSpawn_WithVelocity,
    NativeScriptSpawn_ItemAtPosition, NativeScriptSpawn_Child,
    NativeScriptSpawn_AtPositionData,
    NativeScriptSpawn_HeldAtCenter,
    NativeScriptSpawn_UnownedAtPosition,
    NativeScriptSpawn_LaunchAtPosition, NativeScriptSpawn_LaunchProjectile,
    NativeScriptSpawn_Linked,
};

typedef struct NativeScriptBindingSpawnResolverContext {
    const NativeScriptProgramIndex* program;
    uint32_t owner;
    NativeScriptBindingResolverContext bindings;
} NativeScriptBindingSpawnResolverContext;

static inline uint32_t nativeScriptBindingSpawn_Base(uint32_t semantic)
{
    if((semantic&NATIVE_SCRIPT_SPAWN_DOMAIN_MASK)!=NATIVE_SCRIPT_SPAWN_DOMAIN)return semantic;
    if(!(semantic&NATIVE_SCRIPT_SPAWN_OWNER_MASK))return NativeScriptBinding_Invalid;
    switch((semantic&NATIVE_SCRIPT_SPAWN_OPERATION_MASK)>>20) {
    case NativeScriptSpawn_Projectile:return NativeScriptBinding_SpawnAuthoredProjectile;
    case NativeScriptSpawn_LaunchAtPosition:return NativeScriptBinding_ArticlesLaunchAtPosition;
    case NativeScriptSpawn_LaunchProjectile:return NativeScriptBinding_ArticlesLaunchProjectile;
    case NativeScriptSpawn_Attached:return NativeScriptBinding_SpawnBoneAttached;
    case NativeScriptSpawn_AttachedPresentation:return NativeScriptBinding_SpawnAttachedPresentation;
    case NativeScriptSpawn_AtPosition:return NativeScriptBinding_ArticlesCreateAtPosition;
    case NativeScriptSpawn_AtPositionData:return NativeScriptBinding_ArticlesCreateAtPosition;
    case NativeScriptSpawn_WithVelocity:return NativeScriptBinding_ArticlesCreateWithVelocity;
    case NativeScriptSpawn_UnownedAtPosition:return NativeScriptBinding_ArticlesCreateUnownedAtPosition;
    case NativeScriptSpawn_ItemAtPosition:return NativeScriptBinding_ItemCreateAtPosition;
    case NativeScriptSpawn_Child:return NativeScriptBinding_ItemCreateChild;
    case NativeScriptSpawn_Linked:return NativeScriptBinding_ItemCreateLinked;
    case NativeScriptSpawn_HeldAtCenter:return NativeScriptBinding_ArticlesCreateHeldAtCenter;
    default:return NativeScriptBinding_Invalid;
    }
}
// 1 resolved, 0 unrelated, -1 invalid authored source factory call. The normal
// catalog resolves namespaces/callees; this resolver selects a declared owner.
int nativeScriptBindingSpawn_TryResolve(void*,const NativeScriptIRArena*,
    const NativeScriptIRNode*,NativeScriptIRResolution*,NativeScriptDiagnostic*);
// Exact extended positional form selected by the source resolver. NULL leaves
// ordinary catalog descriptors unchanged; no runtime optional-argument parsing.
const NativeScriptBindingDescriptor* nativeScriptBindingSpawn_Descriptor(uint32_t);
bool nativeScriptBindingSpawn_Read(NativeScriptRuntimeContext*,uint32_t,
    NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingSpawn_Invoke(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,uint32_t,NativeScriptValue*,NativeScriptDiagnostic*);

#endif
