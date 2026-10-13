#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_POSE_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_POSE_H

#include "native_script_vm.h"

// Executes one already-typed operation on the selected source fighter's owned
// assets. All graph/clip pointers are local to the synchronous native call.
bool nativeScriptBindingPose_Invoke(NativeScriptRuntimeContext*, uint32_t semantic,
    const NativeScriptValue* arguments, uint32_t count,
    NativeScriptValue* result, NativeScriptDiagnostic*);

#endif
