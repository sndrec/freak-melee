#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_FIGHTER_HITBOXES_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_FIGHTER_HITBOXES_H
#include "native_script_vm.h"
bool nativeScriptBindingFighterHitboxes_Read(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue* selectors,uint32_t selector_count,
    NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingFighterHitboxes_Write(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue* selectors,uint32_t selector_count,
    const NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingFighterHitboxes_Invoke(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,uint32_t,NativeScriptValue*,NativeScriptDiagnostic*);
#endif
