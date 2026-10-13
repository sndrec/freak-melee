#include <melee/ft/native_script_binding_common_properties.h>
#include <stdio.h>
static bool unavailable(NativeScriptDiagnostic* d)
{if(d)snprintf(d->message,sizeof(d->message),"Common Fighter properties unavailable in this fixture");return false;}
bool nativeScriptBindingCommonProperties_Read(NativeScriptRuntimeContext* c,uint32_t s,NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)v;return unavailable(d);}
bool nativeScriptBindingCommonProperties_Write(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)v;return unavailable(d);}
