#include <melee/ft/native_script_binding_hitbox_create.h>
#include <stdio.h>
bool nativeScriptBindingHitboxCreate_Invoke(NativeScriptRuntimeContext* context,
    const NativeScriptValue* args,uint32_t count,NativeScriptValue* output,NativeScriptDiagnostic* d)
{
    (void)context;(void)args;(void)count;(void)output;
    if(d)snprintf(d->message,sizeof(d->message),"Hitbox creation is not selected by this fixture");
    return false;
}
