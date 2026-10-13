#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_ITEM_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_ITEM_H
#include "native_script_vm.h"

// The common catalog checks Item source class, generation, type/access flags
// and formal inputs. This unit uses typed native fields and generic primitives;
// it never resolves another actor or installs authored native callbacks.
// Accessory tracking/clearing additionally checks the exact owned actor/view
// and stores only a prepared callback code ID in the saved Item actor slot.
bool nativeScriptBindingItem_Read(NativeScriptRuntimeContext*,uint32_t,
    NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingItem_Write(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingItem_Invoke(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,uint32_t,NativeScriptValue*,NativeScriptDiagnostic*);
#endif
