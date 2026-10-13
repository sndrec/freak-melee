#include <melee/ft/native_script_binding_item_lifecycle.h>
// Isolated consumers without the actual Item scheduler reject this family.
bool nativeScriptBindingItemLifecycle_Invoke(NativeScriptRuntimeContext* c,uint32_t id,
 const NativeScriptValue* a,uint32_t n,NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)id;(void)a;(void)n;(void)v;(void)d;return false;}
