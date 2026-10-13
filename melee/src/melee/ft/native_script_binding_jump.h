#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_JUMP_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_JUMP_H
#include "native_script_bindings.h"
bool nativeScriptBindingJump_Read(NativeScriptRuntimeContext*,uint32_t,
    NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingJump_Write(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingJump_Invoke(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,uint32_t,NativeScriptValue*,NativeScriptDiagnostic*);
#endif
