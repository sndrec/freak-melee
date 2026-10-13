#include <melee/ft/native_script_binding_visuals.h>
#include <stdio.h>
bool nativeScriptBindingVisuals_Read(NativeScriptRuntimeContext* c,uint32_t s,NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)v;if(d)snprintf(d->message,sizeof(d->message),"Owned Fighter visibility read unavailable in this fixture");return false;}
bool nativeScriptBindingVisuals_Invoke(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* a,uint32_t n,NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)a;(void)n;(void)v;if(d)snprintf(d->message,sizeof(d->message),"Owned Fighter visuals unavailable in this fixture");return false;}
