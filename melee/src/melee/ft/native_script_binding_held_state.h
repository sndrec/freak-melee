#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_HELD_STATE_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_HELD_STATE_H
#include "native_script_vm.h"

bool nativeScriptBindingHeldState_Write(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingHeldState_Invoke(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,uint32_t,NativeScriptValue*,NativeScriptDiagnostic*);
#endif
