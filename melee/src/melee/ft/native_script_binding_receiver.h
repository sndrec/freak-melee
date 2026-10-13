#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_RECEIVER_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_RECEIVER_H

#include "native_script_receiver.h"
#include "native_script_bindings.h"

#include "native_script_receiver_semantics.h"

typedef struct NativeScriptBindingReceiverResolver {
    const NativeScriptProgramIndex* program;
    uint32_t owner,source_class;
} NativeScriptBindingReceiverResolver;

// 1 resolved, 0 unrelated, -1 invalid receiver path. Only explicitly typed
// creator/owner/current_owner/holder roots become captured Fighter receivers.
int nativeScriptBindingReceiver_TryResolve(void*,const NativeScriptIRArena*,
    const NativeScriptIRNode*,NativeScriptIRResolution*,NativeScriptDiagnostic*);
bool nativeScriptBindingReceiver_DescribeCall(void*,uint32_t semantic,
    NativeScriptCallLayout*,NativeScriptDiagnostic*);
bool nativeScriptBindingReceiver_Read(const NativeScriptReceiverScope*,NativeScriptRuntimeContext*,
    uint32_t semantic,const NativeScriptValue* selectors,uint32_t count,
    NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingReceiver_Write(const NativeScriptReceiverScope*,NativeScriptRuntimeContext*,
    uint32_t semantic,const NativeScriptValue* selectors,uint32_t count,
    const NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingReceiver_Invoke(const NativeScriptReceiverScope*,NativeScriptRuntimeContext*,
    uint32_t semantic,const NativeScriptValue* selectors,uint32_t selector_count,
    NativeScriptValue* arguments,uint32_t count,NativeScriptValue*,NativeScriptDiagnostic*);
// No native field read/write: validate a captured assignment target, including
// exact owned declaration/schema, index bounds, class and writable access.
bool nativeScriptBindingReceiver_ValidateTarget(const NativeScriptReceiverScope*,NativeScriptRuntimeContext*,
    uint32_t semantic,const NativeScriptValue* selectors,uint32_t count,NativeScriptDiagnostic*);

#endif
