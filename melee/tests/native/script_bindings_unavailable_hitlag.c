#include <melee/ft/native_script_binding_hitlag.h>

// Catalog-only consoles have no retained Fighter/peer scene. These leaves
// reject execution; they do not provide a successful native relation policy.
bool nativeScriptBindingHitlag_Read(NativeScriptRuntimeContext* context, uint32_t semantic,
    NativeScriptValue* output, NativeScriptDiagnostic* diagnostic)
{
    (void)context; (void)semantic; (void)output; (void)diagnostic;
    return false;
}

bool nativeScriptBindingHitlag_Invoke(NativeScriptRuntimeContext* context, uint32_t semantic,
    const NativeScriptValue* arguments, uint32_t count, NativeScriptValue* output,
    NativeScriptDiagnostic* diagnostic)
{
    (void)context; (void)semantic; (void)arguments; (void)count; (void)output; (void)diagnostic;
    return false;
}
