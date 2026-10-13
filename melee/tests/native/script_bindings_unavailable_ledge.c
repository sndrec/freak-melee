#include <melee/ft/native_script_binding_ledge.h>
// Pure frontend and unrelated native-family fixtures have no ledge/jump engine.
bool nativeScriptBindingLedge_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* arguments,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{(void)context;(void)semantic;(void)arguments;(void)count;(void)result;(void)diagnostic;return false;}
