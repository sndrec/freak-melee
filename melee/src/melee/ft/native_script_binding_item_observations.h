#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_ITEM_OBSERVATIONS_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_ITEM_OBSERVATIONS_H
#include "native_script_vm.h"
bool nativeScriptBindingItemObservations_Read(NativeScriptRuntimeContext*,uint32_t,
    NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingItemObservations_Invoke(NativeScriptRuntimeContext*,uint32_t,
    NativeScriptValue*,uint32_t,NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingItemObservations_Write(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,NativeScriptDiagnostic*);
#endif
