#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_TARGETS_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_TARGETS_H
#include "native_script_bindings.h"

// Position outputs publish together only for a captured live Fighter. Targets
// may belong to any fighter kind/program; the source caller remains owned.
// Controller color also admits a captured Object exported by Execution to an
// engine input token. It never selects a mutable owner again. Missing color is
// false for the predicate, zero for packed I32 RGBA; stale Object tokens fault.
bool nativeScriptBindingTargets_Invoke(NativeScriptRuntimeContext*,uint32_t,
    NativeScriptValue* arguments,uint32_t count,NativeScriptValue* result,
    NativeScriptDiagnostic*);
#endif
