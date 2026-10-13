#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_FORMS_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_FORMS_H
#include "native_script_bindings.h"
bool nativeScriptBindingForms_Invoke(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,uint32_t,NativeScriptValue*,NativeScriptDiagnostic*);
#endif
