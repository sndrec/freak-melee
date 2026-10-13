#include "native_fighter_events.h"
#include "native_callback_lifetime.h"
#include "ftlib.h"
#include "types.h"
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>

static bool valid_callback(const NativeFighterCallbackExport* event)
{
    switch (event->result_type) {
    case NativeFighterReturn_Void: return event->callback != NULL;
    case NativeFighterReturn_Bool: return event->bool_callback != NULL;
    case NativeFighterReturn_Int: return event->int_callback != NULL;
    case NativeFighterReturn_Float: return event->float_callback != NULL;
    default: return false;
    }
}

bool nativeFighterEvents_Validate(const NativeFighterDefinition* definition)
{
    if (definition == NULL ||
        definition->callback_export_count > NATIVE_FIGHTER_CALLBACK_EXPORT_LIMIT ||
        ((definition->callback_export_count == 0) !=
         (definition->callback_exports == NULL))) return false;
    uint32_t previous = 0;
    for (unsigned i = 0; i < definition->callback_export_count; ++i) {
        const NativeFighterCallbackExport* event = &definition->callback_exports[i];
        if (!valid_callback(event) || event->name_hash <= previous) return false;
        previous = event->name_hash;
    }
    return true;
}

static const NativeFighterCallbackExport* find_export(uint32_t target,
                                                     uint32_t name_hash,
                                                     HSD_GObj** object)
{
    if (target == 0 || name_hash == 0 || HSD_GObjPLinkHead == NULL) return NULL;
    *object = ftLib_8008741C(target);
    if (*object == NULL || (*object)->user_data == NULL) return NULL;
    Fighter* fighter = (*object)->user_data;
    const NativeFighterDefinition* definition =
        nativeFighterRuntime_Definition(fighter->kind);
    if (definition == NULL || definition->callback_exports == NULL ||
        definition->callback_export_count > NATIVE_FIGHTER_CALLBACK_EXPORT_LIMIT)
        return NULL;
    unsigned low = 0, high = definition->callback_export_count;
    while (low < high) {
        unsigned middle = low + (high - low) / 2;
        const NativeFighterCallbackExport* event =
            &definition->callback_exports[middle];
        if (event->name_hash < name_hash) low = middle + 1;
        else if (event->name_hash > name_hash) high = middle;
        else {
            return valid_callback(event) ? event : NULL;
        }
    }
    return NULL;
}

bool nativeFighterEvent_Call(uint32_t target, uint32_t name_hash)
{
    HSD_GObj* object;
    const NativeFighterCallbackExport* event = find_export(target, name_hash, &object);
    if (!event || !nativeCallbackDispatch_Enter()) return false;
    switch (event->result_type) {
    case NativeFighterReturn_Void: event->callback(object); break;
    case NativeFighterReturn_Bool: (void)event->bool_callback(object); break;
    case NativeFighterReturn_Int: (void)event->int_callback(object); break;
    case NativeFighterReturn_Float: (void)event->float_callback(object); break;
    }
    nativeCallbackDispatch_Leave();
    return true;
}

bool nativeFighterEvent_CallBool(uint32_t target, uint32_t name_hash)
{
    HSD_GObj* object;
    const NativeFighterCallbackExport* event = find_export(target, name_hash, &object);
    if (!event || event->result_type != NativeFighterReturn_Bool ||
        !nativeCallbackDispatch_Enter()) return false;
    bool result = event->bool_callback(object);
    nativeCallbackDispatch_Leave();
    return result;
}

int32_t nativeFighterEvent_CallInt(uint32_t target, uint32_t name_hash)
{
    HSD_GObj* object;
    const NativeFighterCallbackExport* event = find_export(target, name_hash, &object);
    if (!event || event->result_type != NativeFighterReturn_Int ||
        !nativeCallbackDispatch_Enter()) return 0;
    int32_t result = event->int_callback(object);
    nativeCallbackDispatch_Leave();
    return result;
}

float nativeFighterEvent_CallFloat(uint32_t target, uint32_t name_hash)
{
    HSD_GObj* object;
    const NativeFighterCallbackExport* event = find_export(target, name_hash, &object);
    if (!event || event->result_type != NativeFighterReturn_Float ||
        !nativeCallbackDispatch_Enter()) return 0.0f;
    float result = event->float_callback(object);
    nativeCallbackDispatch_Leave();
    return result;
}
