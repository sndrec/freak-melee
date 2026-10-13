#include "native_fighter_relations.h"
#include "native_callback_lifetime.h"
#include "ftlib.h"
#include "types.h"
#include <melee/mp/mpcoll.h>
#include <sysdolphin/baselib/gobj.h>

bool nativeFighterRelations_Link(Fighter_GObj* source, uint32_t target)
{
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(source, NativeCallbackSource_Fighter, &generation)) return false;
    Fighter_GObj* endpoint = target != 0 ? ftLib_8008741C(target) : NULL;
    if (target != 0 && (endpoint == NULL || endpoint == source)) return false;
    Fighter* fighter = source->user_data;
    fighter->x1A5C = endpoint;
    fighter->native_hitlag_link_generation = target;
    return true;
}

uint32_t nativeFighterRelations_LinkedId(Fighter_GObj* source)
{
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(source, NativeCallbackSource_Fighter, &generation)) return 0;
    const Fighter* fighter = source->user_data;
    return nativeCallbackSource_Alive(fighter->x1A5C, fighter->native_hitlag_link_generation,
                                      NativeCallbackSource_Fighter)
        ? fighter->native_hitlag_link_generation : 0;
}

bool nativeFighterRelations_CopyCollision(Fighter_GObj* destination, uint32_t source)
{
    uint32_t generation;
    if (source == 0 ||
        !nativeCallbackSource_Snapshot(destination, NativeCallbackSource_Fighter, &generation)) return false;
    Fighter_GObj* object = ftLib_8008741C(source);
    if (object == NULL) return false;
    Fighter* from = object->user_data;
    Fighter* to = destination->user_data;
    mpCopyCollData(&from->coll_data, &to->coll_data, 2);
    return true;
}
