#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_GROUND_CONTROLS_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_GROUND_CONTROLS_H
#include "native_script_bindings.h"
bool nativeScriptBindingGroundControls_Read(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,uint32_t,NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingGroundControls_Invoke(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,uint32_t,NativeScriptValue*,NativeScriptDiagnostic*);
#endif
