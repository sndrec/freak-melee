#include <melee/ft/native_script_binding_contact.h>

// Engine-free fixtures cannot perform a recipient-owned rebound decision.
bool nativeScriptBindingContact_Invoke(NativeScriptRuntimeContext* context,
    uint32_t semantic,const NativeScriptValue* arguments,uint32_t count,
    NativeScriptValue* output,NativeScriptDiagnostic* diagnostic)
{
    (void)context;(void)semantic;(void)arguments;(void)count;(void)output;(void)diagnostic;return false;
}
