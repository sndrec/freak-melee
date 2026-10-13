#include <melee/ft/native_script_binding_actions.h>
// Catalog-only and unrelated fixtures have no native effect/reflector framework.
bool nativeScriptBindingFeedback_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* arguments,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{(void)context;(void)semantic;(void)arguments;(void)count;(void)result;(void)diagnostic;return false;}
