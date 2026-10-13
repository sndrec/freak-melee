#include <melee/ft/native_script_binding_article_feedback.h>
#include <stdio.h>
bool nativeScriptBindingArticleFeedback_Invoke(NativeScriptRuntimeContext* c,uint32_t s,
 const NativeScriptValue* a,uint32_t n,NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)a;(void)n;(void)v;if(d)snprintf(d->message,sizeof(d->message),"Article feedback unavailable in this fixture");return false;}
