#include <melee/ft/native_script_binding_normal_attacks.h>
#include <stdio.h>

bool nativeScriptBindingNormalAttacks_Invoke(NativeScriptRuntimeContext* context,
    uint32_t semantic,const NativeScriptValue* arguments,uint32_t count,
    NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    (void)context;(void)semantic;(void)arguments;(void)count;(void)result;
    if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),
        "Normal attack engine operations are unavailable in this fixture.");
    return false;
}
