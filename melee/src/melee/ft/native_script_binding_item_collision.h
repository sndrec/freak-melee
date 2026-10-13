#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_ITEM_COLLISION_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_ITEM_COLLISION_H
#include "native_script_vm.h"

// Mutable value arguments carry staged state outputs. The compiler admits only
// distinct scalar item.state destinations; the shared VM publishes after guard.
bool nativeScriptBindingItemCollision_Invoke(NativeScriptRuntimeContext*,
    uint32_t, NativeScriptValue*, uint32_t, NativeScriptValue*,
    NativeScriptDiagnostic*);
#endif
