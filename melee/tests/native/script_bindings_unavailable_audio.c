#include <melee/ft/native_script_binding_audio.h>
#include <stdio.h>

bool nativeScriptBindingAudio_Invoke(NativeScriptRuntimeContext* context,
    uint32_t semantic, const NativeScriptValue* arguments, uint32_t count,
    NativeScriptValue* output, NativeScriptDiagnostic* diagnostic)
{
    (void)context; (void)semantic; (void)arguments; (void)count; (void)output;
    if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),
        "Audio engine is unavailable in this isolated fixture");
    return false;
}
