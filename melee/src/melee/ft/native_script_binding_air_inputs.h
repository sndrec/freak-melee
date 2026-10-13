#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_AIR_INPUTS_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_AIR_INPUTS_H
#include "native_script_vm.h"

bool nativeScriptBindingAirInputs_Invoke(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,uint32_t,NativeScriptValue*,NativeScriptDiagnostic*);
#endif
