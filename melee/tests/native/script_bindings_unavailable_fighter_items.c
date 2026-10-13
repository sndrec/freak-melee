#include <melee/ft/native_script_binding_fighter_items.h>
#include <stdio.h>
bool nativeScriptBindingFighterItems_Read(NativeScriptRuntimeContext* context,
    uint32_t semantic,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    (void)context;(void)semantic;(void)result;
    if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),
        "Fighter item queries are unavailable in this fixture.");
    return false;
}
bool nativeScriptBindingFighterItems_Invoke(NativeScriptRuntimeContext* context,
    uint32_t semantic,const NativeScriptValue* arguments,uint32_t count,
    NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    (void)context;(void)semantic;(void)arguments;(void)count;(void)result;
    if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),
        "Fighter item controls are unavailable in this fixture.");
    return false;
}

bool nativeScriptBindingFighterItems_Write(NativeScriptRuntimeContext* context,
    uint32_t semantic,const NativeScriptValue* value,NativeScriptDiagnostic* diagnostic)
{
    (void)context;(void)semantic;(void)value;
    if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),
        "Fighter item properties are unavailable in this fixture.");
    return false;
}
