#include <melee/ft/native_script_binding_model_graph.h>
#include <stdio.h>

// Isolated catalog and unrelated engine fixtures have no admitted Fighter rig.
bool nativeScriptBindingModelGraph_Invoke(NativeScriptRuntimeContext* context,
    uint32_t semantic, const NativeScriptValue* arguments, uint32_t count,
    NativeScriptValue* output, NativeScriptDiagnostic* diagnostic)
{
    (void)context; (void)semantic; (void)arguments; (void)count; (void)output;
    if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),
        "Model graph installation is unavailable in this isolated fixture");
    return false;
}
