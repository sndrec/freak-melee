#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_COLORS_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_COLORS_H
#include "native_script_vm.h"

bool nativeScriptBindingColors_Read(NativeScriptRuntimeContext*,uint32_t,
    NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingColors_Write(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingColors_Invoke(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,uint32_t,NativeScriptValue*,NativeScriptDiagnostic*);
#endif
