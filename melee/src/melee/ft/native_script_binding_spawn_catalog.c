#include "native_script_binding_spawn.h"

const NativeScriptBindingDescriptor* nativeScriptBindingSpawn_Descriptor(uint32_t semantic)
{
    static const NativeScriptBindingDescriptor position_data = {
        .name="create_at_position", .parent=NativeScriptBinding_Articles,
        .flags=NativeScriptFlag_Readable|NativeScriptFlag_Callable|
            NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,
        .kind=NativeScriptBinding_Intrinsic, .contexts=NativeScriptBindings_Fighter,
        .result_type=NativeScriptType_I32, .argument_count=8,
        .arguments={{.type=NativeScriptType_I32},
            {.type=NativeScriptType_F32},{.type=NativeScriptType_F32},{.type=NativeScriptType_F32},
            {.type=NativeScriptType_F32},{.type=NativeScriptType_Bool},
            {.type=NativeScriptType_Invalid,.numeric_any=true},{.type=NativeScriptType_F32}}
    };
    return (semantic&NATIVE_SCRIPT_SPAWN_DOMAIN_MASK)==NATIVE_SCRIPT_SPAWN_DOMAIN &&
        (semantic&NATIVE_SCRIPT_SPAWN_OWNER_MASK) &&
        ((semantic&NATIVE_SCRIPT_SPAWN_OPERATION_MASK)>>20)==NativeScriptSpawn_AtPositionData ?
        &position_data : NULL;
}
