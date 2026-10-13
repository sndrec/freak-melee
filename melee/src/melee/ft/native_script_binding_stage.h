#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_STAGE_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_STAGE_H
#include "native_script_bindings.h"
bool nativeScriptBindingStage_Invoke(NativeScriptRuntimeContext*, uint32_t,
    NativeScriptValue*, uint32_t, NativeScriptValue*, NativeScriptDiagnostic*);
#endif
