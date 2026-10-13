#include <melee/ft/native_script_binding_item_resources.h>
// Engine-free compiler consoles resolve and prepare the real resource catalog,
// but cannot execute collections without scene backing and live Item actors.
bool nativeScriptBindingItemResources_Read(NativeScriptRuntimeContext* c,uint32_t s,
 const NativeScriptValue* a,uint32_t n,NativeScriptValue* out,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)a;(void)n;(void)out;(void)d;return false;}
bool nativeScriptBindingItemResources_Write(NativeScriptRuntimeContext* c,uint32_t s,
 const NativeScriptValue* a,uint32_t n,const NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)a;(void)n;(void)v;(void)d;return false;}
bool nativeScriptBindingItemResources_Invoke(NativeScriptRuntimeContext* c,uint32_t s,
 const NativeScriptValue* a,uint32_t n,NativeScriptValue* args,uint32_t count,
 NativeScriptValue* out,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)a;(void)n;(void)args;(void)count;(void)out;(void)d;return false;}
