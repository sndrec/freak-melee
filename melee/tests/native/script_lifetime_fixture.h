#ifndef SCRIPT_LIFETIME_FIXTURE_H
#define SCRIPT_LIFETIME_FIXTURE_H

#include <melee/ft/native_fighter_module.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdlib.h>

// Compiler fixtures register their actual stack/pool objects. This mock Host
// resolves only those objects, validates class and retained user_data, and then
// compares the complete engine generation field. It never accepts an unknown
// object or guesses its user_data type from the requested class alone.
static struct {
    HSD_GObj* object;
    void* data;
    uint32_t kind;
} script_lifetime_sources[8];
static unsigned script_lifetime_source_count;

static void script_lifetime_register(HSD_GObj* object, uint32_t kind)
{
    unsigned i = 0;
    while (i < script_lifetime_source_count && script_lifetime_sources[i].object != object) ++i;
    if (i == 8 || !object || !object->user_data) abort();
    if (i == script_lifetime_source_count) ++script_lifetime_source_count;
    script_lifetime_sources[i].object = object;
    script_lifetime_sources[i].data = object->user_data;
    script_lifetime_sources[i].kind = kind;
}

static bool script_lifetime_alive(HSD_GObj* object, uint32_t generation, uint32_t kind)
{
    for (unsigned i = 0; i < script_lifetime_source_count; ++i) {
        if (script_lifetime_sources[i].object != object || script_lifetime_sources[i].kind != kind ||
            object->user_data != script_lifetime_sources[i].data) continue;
        if (kind == NativeCallbackSource_Fighter)
            return (uint32_t)((Fighter*)object->user_data)->x8_spawnNum == generation;
        if (kind == NativeCallbackSource_Item)
            return (uint32_t)((Item*)object->user_data)->x1C == generation;
    }
    return false;
}

#endif
