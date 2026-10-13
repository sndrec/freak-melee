#include <melee/ft/native_script_binding_attachments.h>
#include <stdio.h>

bool nativeScriptBindingAttachments_Invoke(NativeScriptRuntimeContext* context,
    uint32_t semantic, const NativeScriptValue* arguments, uint32_t count,
    NativeScriptValue* output, NativeScriptDiagnostic* diagnostic)
{
    (void)context;(void)semantic;(void)arguments;(void)count;(void)output;
    if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),
        "Costume attachments are unavailable in this isolated fixture.");
    return false;
}
