#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_FIGHTER_ITEMS_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_FIGHTER_ITEMS_H
#include "native_script_vm.h"
bool nativeScriptBindingFighterItems_Read(NativeScriptRuntimeContext*,uint32_t,
    NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingFighterItems_Write(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingFighterItems_Invoke(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,uint32_t,NativeScriptValue*,NativeScriptDiagnostic*);
#endif
