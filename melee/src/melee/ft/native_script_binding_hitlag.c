#include "native_script_binding_hitlag.h"
#include "native_script_bindings.h"
#include "native_fighter_lifetime.h"
#include "fighter.h"
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>

static bool error(NativeScriptDiagnostic* diagnostic, const char* message)
{
    if (diagnostic) snprintf(diagnostic->message, sizeof(diagnostic->message), "%s", message);
    return false;
}

static bool capture(NativeScriptRuntimeContext* context, bool owned, NativeFighterLifetime* source,
                    NativeScriptDiagnostic* diagnostic)
{
    NativeMatchPoolReference reference;
    if (!context || context->source_class != NativeCallbackSource_Fighter ||
        !context->scene_epoch || !nativeMatchPool_Reference(&nativeGObjPool, context->source, &reference) ||
        reference.lifetime != context->scene_epoch ||
        ((HSD_GObj*)context->source)->classifier != HSD_GOBJ_CLASS_FIGHTER ||
        !((HSD_GObj*)context->source)->user_data || !nativeScriptBindings_SourceAlive(context) ||
        !nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data, source) ||
        source->generation != context->generation ||
        (owned && (!source->source_owned ||
         nativeScriptBindings_SourceFighterLayout(context, diagnostic) != &source->program->bank->motion_layout)))
        return error(diagnostic, owned ? "Hitlag operation requires its live owned Fighter/program/storage" :
                     "Hitlag observation requires its live physical Fighter/program/storage");
    return true;
}

static bool current(NativeScriptRuntimeContext* context, bool owned, const NativeFighterLifetime* source)
{
    return context->source == source->object && context->source_class == NativeCallbackSource_Fighter &&
        source->reference.lifetime == context->scene_epoch &&
        source->generation == context->generation && nativeFighterLifetime_Current(source) &&
        nativeScriptBindings_SourceAlive(context) &&
        (!owned || (source->source_owned &&
         nativeScriptBindings_SourceFighterLayout(context, NULL) == &source->program->bank->motion_layout));
}

static bool handle(const NativeScriptValue* value, uint32_t* output,
                   NativeScriptDiagnostic* diagnostic)
{
    uint64_t bits;
    switch (value->type) {
    case NativeScriptType_Bool:
        if (value->as.u64 > 1) return error(diagnostic, "Hitlag handle Bool is malformed");
        bits = value->as.u64;
        break;
    case NativeScriptType_I32:
        if (value->as.i64 < INT32_MIN || value->as.i64 > INT32_MAX)
            return error(diagnostic, "Hitlag handle exceeds its declared Int width");
        bits = (uint32_t)value->as.i64;
        break;
    case NativeScriptType_I64:
        if (value->as.i64 < INT32_MIN || value->as.i64 > UINT32_MAX)
            return error(diagnostic, "Hitlag handle exceeds native 32-bit width");
        bits = (uint32_t)value->as.i64;
        break;
    case NativeScriptType_U32: case NativeScriptType_U64:
        bits = value->as.u64;
        break;
    default:
        return error(diagnostic, "Hitlag handle must be integral");
    }
    if (bits > UINT32_MAX) return error(diagnostic, "Hitlag handle exceeds native 32-bit width");
    *output = (uint32_t)bits;
    return true;
}

static bool peer_capture(Fighter_GObj* object, uint32_t generation, uint64_t epoch,
                         NativeFighterLifetime* peer)
{
    NativeMatchPoolReference reference;
    // A retained relation may point to retired storage. Admit physical identity
    // before reading userdata, then retain the actual peer program if attached.
    return object && nativeMatchPool_Reference(&nativeGObjPool, object, &reference) &&
        reference.lifetime == epoch && object->classifier == HSD_GOBJ_CLASS_FIGHTER &&
        object->user_data && nativeCallbackSource_Alive(object, generation, NativeCallbackSource_Fighter) &&
        nativeFighterLifetime_Capture(object->user_data, peer) && peer->generation == generation &&
        peer->reference.lifetime == epoch;
}

static bool select(uint32_t generation, uint64_t epoch, NativeFighterLifetime* peer,
                   NativeScriptDiagnostic* diagnostic)
{
    if (!generation || !HSD_GObjPLinkHead) return true;
    uint32_t visited = 0;
    for (Fighter_GObj* object = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER]; object; object = object->next) {
        NativeMatchPoolReference reference;
        if (visited++ >= nativeGObjPool.capacity ||
            !nativeMatchPool_Reference(&nativeGObjPool, object, &reference) || reference.lifetime != epoch)
            return error(diagnostic, "Hitlag Fighter registry is stale or cyclic");
        uint32_t actual;
        if (object->classifier != HSD_GOBJ_CLASS_FIGHTER ||
            !nativeCallbackSource_Snapshot(object, NativeCallbackSource_Fighter, &actual) || actual != generation)
            continue;
        if (!peer_capture(object, generation, epoch, peer))
            return error(diagnostic, "Hitlag peer has changed physical/program/storage identity");
        return true;
    }
    return true;
}

bool nativeScriptBindingHitlag_Read(NativeScriptRuntimeContext* context, uint32_t semantic,
    NativeScriptValue* output, NativeScriptDiagnostic* diagnostic)
{
    if (!output || (semantic != NativeScriptBinding_HitlagActive && semantic != NativeScriptBinding_HitlagLinkedId))
        return error(diagnostic, "Unknown hitlag observation");
    NativeFighterLifetime source;
    bool active = semantic == NativeScriptBinding_HitlagActive;
    if (!capture(context, !active, &source, diagnostic)) return false;
    NativeScriptValue result = {.type = active ? NativeScriptType_Bool : NativeScriptType_I32};
    NativeFighterLifetime peer = {0};
    Fighter_GObj* relation = NULL;
    uint32_t generation = 0;
    if (active) result.as.u64 = source.data->x2219_b5;
    else {
        relation = source.data->x1A5C;
        generation = source.data->native_hitlag_link_generation;
        if (peer_capture(relation, generation, context->scene_epoch, &peer))
            result.as.i64 = (int32_t)generation;
    }
    // Missing/stale peers are the native zero default, not a pointer cleanup.
    if (!current(context, !active, &source) ||
        (semantic == NativeScriptBinding_HitlagLinkedId &&
         (source.data->x1A5C != relation || source.data->native_hitlag_link_generation != generation ||
          (peer.object && !nativeFighterLifetime_Current(&peer)))))
        return error(diagnostic, "Hitlag relation/source changed before observation publication");
    *output = result;
    return true;
}

bool nativeScriptBindingHitlag_Invoke(NativeScriptRuntimeContext* context, uint32_t semantic,
    const NativeScriptValue* arguments, uint32_t count, NativeScriptValue* output, NativeScriptDiagnostic* diagnostic)
{
    bool link = semantic == NativeScriptBinding_HitlagLink;
    if (!output || (!link && semantic != NativeScriptBinding_HitlagUnlink && semantic != NativeScriptBinding_HitlagRequestRelease) ||
        count != (link ? 1u : 0u) || (link && !arguments))
        return error(diagnostic, "Hitlag operation has an invalid argument shape");
    uint32_t generation = 0;
    if (link && !handle(arguments, &generation, diagnostic)) return false;
    NativeFighterLifetime source, peer = {0};
    if (!capture(context, true, &source, diagnostic)) return false;
    NativeScriptValue result = {.type = link ? NativeScriptType_Bool : NativeScriptType_Void};
    if (link && generation && !select(generation, context->scene_epoch, &peer, diagnostic)) return false;
    if (!current(context, true, &source) || (peer.object && !nativeFighterLifetime_Current(&peer)))
        return error(diagnostic, "Hitlag source or selected peer changed before mutation");
    if (semantic == NativeScriptBinding_HitlagRequestRelease) {
        Fighter_UnkSetFlag_8006CFBC(source.object);
    } else if (!link || !generation || (peer.object && peer.object != source.object)) {
        // One-sided native relation assignment. Authored cleanup owns any
        // reciprocal unlink/release; linking never dispatches hitlag callbacks.
        source.data->x1A5C = peer.object;
        source.data->native_hitlag_link_generation = generation;
        result.as.u64 = link;
    }
    if (!current(context, true, &source) || (peer.object && !nativeFighterLifetime_Current(&peer)))
        return error(diagnostic, "Hitlag source or peer changed before operation publication");
    *output = result;
    return true;
}
