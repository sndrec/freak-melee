#include "native_fighter_script_store.h"
#include <stdio.h>

bool nativeScriptStore_External(const ScriptStateField* field, bool creator,
    const char* index, const char* value, char* output, size_t capacity)
{
    char store[512], capture_index[1200] = "";
    if (field->array) {
        if (!index || !nativeScriptState_ArrayWrite(field,
            "write_fighter->native_module_state + script_state_base", "write_index",
            "write_value", store, sizeof(store))) return false;
        int size = snprintf(capture_index, sizeof(capture_index), "__auto_type write_index = %s; ", index);
        if (size < 0 || (size_t)size >= sizeof(capture_index)) return false;
    } else if (field->type == ScriptState_Bool) {
        snprintf(store, sizeof(store), "write_fighter->native_module_state[script_state_base + %u] = (uint8_t)write_value;", field->offset);
    } else {
        snprintf(store, sizeof(store), "script_state_set_%s(write_fighter, script_state_base + %u, write_value);",
            field->type == ScriptState_Int ? "int" : "float", field->offset);
    }
    int size = snprintf(output, capacity,
        "{ HSD_GObj* write_target = engine->item_creator(gobj); "
        "uint32_t write_generation = write_target != NULL ? (uint32_t)((Fighter*)write_target->user_data)->x8_spawnNum : 0; "
        "%s"
        "%s__auto_type write_value = %s; "
        "HSD_GObj* write_current = engine->item_creator(gobj); "
        "if (write_current != NULL && write_current == write_target%s && "
        "engine->callback_source_alive(write_current, write_generation, NativeCallbackSource_Fighter)) "
        "{ Fighter* write_fighter = write_current->user_data; %s } %s}",
        creator && !field->array ? "if (write_target != NULL) { " : "",
        capture_index, value, creator ? "" : " && item->owner == write_current", store,
        creator && !field->array ? "} " : "");
    return size >= 0 && (size_t)size < capacity;
}
