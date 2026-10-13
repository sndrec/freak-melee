#include <melee/ft/native_script_binding_item_animation.h>
#include <stdio.h>

// Engine-free compiler fixtures must fail rather than fabricate model playback.
bool nativeScriptBindingItemAnimation_Invoke(NativeScriptRuntimeContext* context,
    uint32_t semantic, NativeScriptValue* args, uint32_t count,
    NativeScriptValue* output, NativeScriptDiagnostic* diagnostic)
{
    (void)context; (void)semantic; (void)args; (void)count; (void)output;
    if (diagnostic) snprintf(diagnostic->message, sizeof(diagnostic->message),
        "Item animation runtime is unavailable in this compiler fixture");
    return false;
}
