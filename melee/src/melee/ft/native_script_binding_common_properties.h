#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_COMMON_PROPERTIES_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_COMMON_PROPERTIES_H

#include "native_script_vm.h"

bool nativeScriptBindingCommonProperties_Read(NativeScriptRuntimeContext*,uint32_t,
    NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingCommonProperties_Write(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,NativeScriptDiagnostic*);

#endif
