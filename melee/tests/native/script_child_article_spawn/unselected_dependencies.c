#include <melee/ft/native_script_binding_item_links.h>
#include <stdio.h>
bool nativeScriptBindingItemLinks_Invoke(NativeScriptRuntimeContext* c,uint32_t op,
    const NativeScriptValue* args,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* d)
{
    (void)c;(void)op;(void)args;(void)count;(void)result;
    if(d)snprintf(d->message,sizeof(d->message),"Link reads/mutations are not selected by the constructor fixture");
    return false;
}
