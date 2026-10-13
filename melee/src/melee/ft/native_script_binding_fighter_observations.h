#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_FIGHTER_OBSERVATIONS_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_FIGHTER_OBSERVATIONS_H
#include "native_script_vm.h"

bool nativeScriptBindingFighterObservations_Read(NativeScriptRuntimeContext*,uint32_t,
    NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingFighterObservations_Write(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingFighterObservations_Invoke(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,uint32_t,NativeScriptValue*,NativeScriptDiagnostic*);
#endif
