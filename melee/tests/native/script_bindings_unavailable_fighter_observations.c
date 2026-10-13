#include <melee/ft/native_script_binding_fighter_observations.h>

// Engine-free fixtures reject this family; they never simulate its outcomes.
bool nativeScriptBindingFighterObservations_Read(NativeScriptRuntimeContext* context,
    uint32_t semantic,NativeScriptValue* output,NativeScriptDiagnostic* diagnostic)
{
    (void)context;(void)semantic;(void)output;(void)diagnostic;return false;
}

bool nativeScriptBindingFighterObservations_Write(NativeScriptRuntimeContext* context,
    uint32_t semantic,const NativeScriptValue* input,NativeScriptDiagnostic* diagnostic)
{
    (void)context;(void)semantic;(void)input;(void)diagnostic;return false;
}

bool nativeScriptBindingFighterObservations_Invoke(NativeScriptRuntimeContext* context,
    uint32_t semantic,const NativeScriptValue* arguments,uint32_t count,
    NativeScriptValue* output,NativeScriptDiagnostic* diagnostic)
{
    (void)context;(void)semantic;(void)arguments;(void)count;(void)output;(void)diagnostic;return false;
}
