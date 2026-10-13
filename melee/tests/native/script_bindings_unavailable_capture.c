#include <melee/ft/native_script_binding_capture.h>
#include <stdio.h>

bool nativeScriptBindingCapture_Invoke(NativeScriptRuntimeContext* context,
    uint32_t semantic, const NativeScriptValue* arguments, uint32_t count,
    NativeScriptValue* output, NativeScriptDiagnostic* diagnostic)
{
    (void)context; (void)semantic; (void)arguments; (void)count; (void)output;
    if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),
        "Native capture operations unavailable in this fixture");
    return false;
}

bool nativeScriptBindingCapture_Read(NativeScriptRuntimeContext* context,uint32_t semantic,
    NativeScriptValue* output,NativeScriptDiagnostic* diagnostic)
{
    (void)context;(void)semantic;(void)output;
    if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),"Native capture observations unavailable in this fixture");
    return false;
}
bool nativeScriptBindingCapture_Write(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* value,NativeScriptDiagnostic* diagnostic)
{
    (void)context;(void)semantic;(void)value;
    if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),"Native capture assignments unavailable in this fixture");
    return false;
}
