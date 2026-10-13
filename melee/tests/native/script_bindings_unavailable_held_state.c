#include <melee/ft/native_script_binding_held_state.h>

// Engine-free compiler fixtures have no owned native motion/animation peers.
bool nativeScriptBindingHeldState_Write(NativeScriptRuntimeContext* context,
    uint32_t semantic,const NativeScriptValue* value,NativeScriptDiagnostic* diagnostic)
{
    (void)context;(void)semantic;(void)value;(void)diagnostic;return false;
}
bool nativeScriptBindingHeldState_Invoke(NativeScriptRuntimeContext* context,
    uint32_t semantic,const NativeScriptValue* arguments,uint32_t count,
    NativeScriptValue* output,NativeScriptDiagnostic* diagnostic)
{
    (void)context;(void)semantic;(void)arguments;(void)count;(void)output;(void)diagnostic;return false;
}
