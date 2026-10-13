#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_PRESENTATION_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_PRESENTATION_H

#include "native_script_vm.h"

// The single native binding catalog resolves names, overloads, formal types,
// contexts and output modes before entering these operations. Values contain
// semantic Fighter_Part numbers or checked native joint indices, never pointers.
bool nativeScriptBindingPresentation_Read(NativeScriptRuntimeContext*,uint32_t,
    NativeScriptValue*,NativeScriptDiagnostic*);

// World-position writes Float arguments 1..3 into this temporary argument array
// only after its native matrix operation leaves the source alive. The catalog
// and VM publish these outputs to authored lvalues after their own source guard.
bool nativeScriptBindingPresentation_Invoke(NativeScriptRuntimeContext*,uint32_t,
    NativeScriptValue* arguments,uint32_t count,NativeScriptValue*,NativeScriptDiagnostic*);

// Standard transient fighter accessory: owned immutable recipes plus the
// ordinary HSD animation/transform primitives, with retained rig/source checks.
bool nativeScriptBindingPresentation_AccessoryInvoke(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,uint32_t,NativeScriptValue*,NativeScriptDiagnostic*);

#endif
