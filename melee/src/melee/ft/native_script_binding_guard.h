#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_GUARD_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_GUARD_H
#include "native_script_bindings.h"

bool nativeScriptBindingGuard_Read(NativeScriptRuntimeContext*,uint32_t,
    NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingGuard_Write(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingGuard_Invoke(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,uint32_t,NativeScriptValue*,NativeScriptDiagnostic*);
#endif
