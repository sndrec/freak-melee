#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_SIGNAL_RESOLVE_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_SIGNAL_RESOLVE_H
#include "native_script_binding_spawn.h"

#define NATIVE_SCRIPT_SIGNAL_DOMAIN UINT32_C(0x05000000)
#define NATIVE_SCRIPT_SIGNAL_DOMAIN_MASK UINT32_C(0xff000000)
#define NATIVE_SCRIPT_SIGNAL_OPERATION_MASK UINT32_C(0x00f00000)
#define NATIVE_SCRIPT_SIGNAL_FUNCTION_MASK UINT32_C(0x000fffff)
enum { NativeScriptSignal_Self=1,NativeScriptSignal_Target=2,NativeScriptSignal_Owned=3 };

static inline uint32_t nativeScriptBindingSignal_Base(uint32_t semantic)
{
    if((semantic&NATIVE_SCRIPT_SIGNAL_DOMAIN_MASK)!=NATIVE_SCRIPT_SIGNAL_DOMAIN)return semantic;
    switch((semantic&NATIVE_SCRIPT_SIGNAL_OPERATION_MASK)>>20) {
    case NativeScriptSignal_Self:return NativeScriptBinding_ItemSignal;
    case NativeScriptSignal_Target:return NativeScriptBinding_ArticlesSignal;
    case NativeScriptSignal_Owned:return NativeScriptBinding_ArticlesSignalOwned;
    default:return NativeScriptBinding_Invalid;
    }
}
// Uses the same exact indexed program/owner context as authored factories.
// Only literal source names are inspected at load; the semantic retains a
// callback code ID and operation. No text lookup is needed during execution.
// 1 resolved, 0 unrelated, -1 malformed/unknown/ambiguous signal call.
int nativeScriptBindingSignal_TryResolve(void*,const NativeScriptIRArena*,
    const NativeScriptIRNode*,NativeScriptIRResolution*,NativeScriptDiagnostic*);
#endif
