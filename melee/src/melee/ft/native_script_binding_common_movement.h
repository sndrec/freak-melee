#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_COMMON_MOVEMENT_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_COMMON_MOVEMENT_H

#include "native_script_vm.h"

bool nativeScriptBindingCommonMovement_Invoke(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,uint32_t,NativeScriptValue*,NativeScriptDiagnostic*);

#endif
