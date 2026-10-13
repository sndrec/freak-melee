#include <melee/ft/native_script_binding_signal.h>
#include <stdio.h>
bool nativeScriptBindingSignal_Invoke(NativeScriptRuntimeContext* c,uint32_t s,
 const NativeScriptValue* a,uint32_t n,NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)a;(void)n;(void)v;if(d)snprintf(d->message,sizeof(d->message),"Article signals unavailable in this fixture");return false;}

#include <melee/ft/native_script_binding_item_links.h>
bool nativeScriptBindingItemLinks_Invoke(NativeScriptRuntimeContext* c,uint32_t s,
 const NativeScriptValue* a,uint32_t n,NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)a;(void)n;(void)v;if(d)snprintf(d->message,sizeof(d->message),"Item links unavailable in this fixture");return false;}
