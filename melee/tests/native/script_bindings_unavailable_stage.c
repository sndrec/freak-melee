#include <melee/ft/native_script_binding_stage.h>

// Engine-free consoles have no live stage collision data. Explicitly reject;
// do not fabricate a clear path or successful collision query.
bool nativeScriptBindingStage_Invoke(NativeScriptRuntimeContext* context,
    uint32_t semantic, NativeScriptValue* arguments, uint32_t count,
    NativeScriptValue* output, NativeScriptDiagnostic* diagnostic)
{
    (void)context; (void)semantic; (void)arguments; (void)count;
    (void)output; (void)diagnostic;
    return false;
}
