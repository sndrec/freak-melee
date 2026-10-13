#include <melee/ft/native_script_binding_family.h>

// Isolated consoles without a published fighter family cannot resolve members
// or mutate its sleep policy. These operations must fail rather than fabricate
// a successful query or pair.
bool nativeScriptBindingFamily_Invoke(NativeScriptRuntimeContext* context,
    uint32_t semantic, const NativeScriptValue* arguments, uint32_t count,
    NativeScriptValue* output, NativeScriptDiagnostic* diagnostic)
{
    (void)context; (void)semantic; (void)arguments; (void)count;
    (void)output; (void)diagnostic;
    return false;
}
