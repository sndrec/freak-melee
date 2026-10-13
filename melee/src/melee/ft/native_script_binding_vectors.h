#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_VECTORS_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_VECTORS_H
#include "native_script_vm.h"
// Native vector formulas, including zero/nonfinite results, are preserved.
// Inputs are copied F32 values; normalization and rotation publish three F32 outputs.
// No actor geometry/pointer is exposed to scripts.
bool nativeScriptBindingVectors_Invoke(NativeScriptRuntimeContext*,uint32_t,
    NativeScriptValue*,uint32_t,NativeScriptValue*,NativeScriptDiagnostic*);
#endif
