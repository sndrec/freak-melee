#include <melee/ft/native_script_binding_article_reference.h>
#include <stdio.h>
bool nativeScriptBindingArticleReference_Read(const NativeScriptReceiverScope* s,NativeScriptRuntimeContext* c,
 uint32_t id,const NativeScriptValue* a,uint32_t n,NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)s;(void)c;(void)id;(void)a;(void)n;(void)v;if(d)snprintf(d->message,sizeof(d->message),"Article references unavailable in this fixture");return false;}
bool nativeScriptBindingArticleReference_Invoke(const NativeScriptReceiverScope* s,NativeScriptRuntimeContext* c,
 uint32_t id,const NativeScriptValue* a,uint32_t n,NativeScriptValue* inputs,uint32_t count,NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)s;(void)c;(void)id;(void)a;(void)n;(void)inputs;(void)count;(void)v;if(d)snprintf(d->message,sizeof(d->message),"Article references unavailable in this fixture");return false;}
