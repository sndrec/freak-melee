#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_FAMILY_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_FAMILY_H

#include "native_script_bindings.h"

bool nativeScriptBindingFamily_Invoke(NativeScriptRuntimeContext*, uint32_t,
    const NativeScriptValue*, uint32_t, NativeScriptValue*,
    NativeScriptDiagnostic*);

#endif
