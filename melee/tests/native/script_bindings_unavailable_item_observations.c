#include <stdio.h>
#include <melee/ft/native_script_binding_item_observations.h>
bool nativeScriptBindingItemObservations_Read(NativeScriptRuntimeContext* c,uint32_t s,
    NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)v;(void)d;return false;}
bool nativeScriptBindingItemObservations_Invoke(NativeScriptRuntimeContext* c,uint32_t s,
    NativeScriptValue* a,uint32_t n,NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)a;(void)n;(void)v;(void)d;return false;}

bool nativeScriptBindingItemObservations_Write(NativeScriptRuntimeContext* c,uint32_t s,
 const NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)v;if(d)snprintf(d->message,sizeof(d->message),"Item observations unavailable in this fixture");return false;}
