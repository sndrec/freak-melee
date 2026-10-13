#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_HITLAG_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_HITLAG_H

#include "native_script_vm.h"

bool nativeScriptBindingHitlag_Read(NativeScriptRuntimeContext*, uint32_t,
    NativeScriptValue*, NativeScriptDiagnostic*);
bool nativeScriptBindingHitlag_Invoke(NativeScriptRuntimeContext*, uint32_t,
    const NativeScriptValue*, uint32_t, NativeScriptValue*, NativeScriptDiagnostic*);

#endif
