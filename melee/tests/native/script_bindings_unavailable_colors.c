#include <melee/ft/native_script_binding_colors.h>
#include <stdio.h>
static bool unavailable(NativeScriptDiagnostic* diagnostic)
{
    if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),
        "Color and costume operations are unavailable in this fixture.");
    return false;
}
bool nativeScriptBindingColors_Read(NativeScriptRuntimeContext* context,uint32_t semantic,
    NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{(void)context;(void)semantic;(void)result;return unavailable(diagnostic);}
bool nativeScriptBindingColors_Write(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* value,NativeScriptDiagnostic* diagnostic)
{(void)context;(void)semantic;(void)value;return unavailable(diagnostic);}
bool nativeScriptBindingColors_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* arguments,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{(void)context;(void)semantic;(void)arguments;(void)count;(void)result;return unavailable(diagnostic);}
