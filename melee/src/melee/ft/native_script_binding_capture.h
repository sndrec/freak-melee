#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_CAPTURE_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_CAPTURE_H
#include "native_script_bindings.h"

bool nativeScriptBindingCapture_Invoke(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,uint32_t,NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingCapture_Read(NativeScriptRuntimeContext*,uint32_t,
    NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingCapture_Write(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,NativeScriptDiagnostic*);

#endif
