#include <melee/ft/native_script_binding_item_collision.h>
#include <stdio.h>
bool nativeScriptBindingItemCollision_Invoke(NativeScriptRuntimeContext* c,
    uint32_t s,NativeScriptValue* a,uint32_t n,NativeScriptValue* v,
    NativeScriptDiagnostic* d)
{
    (void)c;(void)s;(void)a;(void)n;(void)v;
    if(d)snprintf(d->message,sizeof(d->message),
        "Native Item segment collision unavailable in this fixture");
    return false;
}
