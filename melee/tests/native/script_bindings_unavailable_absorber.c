#include <melee/ft/native_script_binding_absorber.h>
// Engine-free fixtures reject this unavailable native family.
bool nativeScriptBindingAbsorber_Read(NativeScriptRuntimeContext* c,uint32_t id,NativeScriptValue* out,NativeScriptDiagnostic* d)
{(void)c;(void)id;(void)out;(void)d;return false;}
bool nativeScriptBindingAbsorber_Write(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* in,NativeScriptDiagnostic* d)
{(void)c;(void)id;(void)in;(void)d;return false;}
bool nativeScriptBindingAbsorber_Invoke(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* in,uint32_t n,NativeScriptValue* out,NativeScriptDiagnostic* d)
{(void)c;(void)id;(void)in;(void)n;(void)out;(void)d;return false;}
