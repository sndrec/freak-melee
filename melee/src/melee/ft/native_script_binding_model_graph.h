#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_MODEL_GRAPH_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_MODEL_GRAPH_H
#include "native_script_vm.h"

// Source-owned Fighter reserved-part installation. Native graph construction
// uses the scene heap; callers choose an explicit setup transition.
bool nativeScriptBindingModelGraph_Invoke(NativeScriptRuntimeContext*,uint32_t semantic,
    const NativeScriptValue* arguments,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic*);
#endif
