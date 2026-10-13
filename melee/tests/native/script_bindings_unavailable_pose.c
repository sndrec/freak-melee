#include <melee/ft/native_script_binding_pose.h>
#include <stdio.h>

// Compiler-only console fixtures have no native pose/rig runtime. A reached
// operation must fail explicitly; it cannot stand in for engine acceptance.
bool nativeScriptBindingPose_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* arguments,uint32_t count,
    NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    (void)context;(void)semantic;(void)arguments;(void)count;(void)result;
    if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),"Native pose execution is unavailable in this fixture");
    return false;
}
