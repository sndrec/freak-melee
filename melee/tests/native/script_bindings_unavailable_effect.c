#include <melee/ft/native_script_binding_effect.h>
// Older isolated fixtures have no source effect actors/assets. Their pure load
// resolver is actual code; effect execution remains explicitly unavailable.
bool nativeScriptBindingEffect_Read(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* a,uint32_t n,NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)a;(void)n;(void)v;(void)d;return false;}
bool nativeScriptBindingEffect_Write(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* a,uint32_t n,const NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)a;(void)n;(void)v;(void)d;return false;}
bool nativeScriptBindingEffect_Invoke(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* a,uint32_t n,NativeScriptValue* p,uint32_t count,NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)a;(void)n;(void)p;(void)count;(void)v;(void)d;return false;}
