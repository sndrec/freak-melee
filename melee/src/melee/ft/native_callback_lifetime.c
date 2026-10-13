#include "native_callback_lifetime.h"
#include "types.h"
#include <melee/it/types.h>
#include <melee/ef/native_model_effect.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>

static unsigned dispatch_depth;

bool nativeCallbackSource_Snapshot(HSD_GObj* source, uint32_t source_class,
                                   uint32_t* generation)
{
    if (generation == NULL) return false;
    *generation = 0;
    if (source_class == NativeCallbackSource_ModelEffect)
        return nativeModelEffect_Snapshot(source, generation);
    if (source == NULL || HSD_GObjPLinkHead == NULL ||
        source_class > NativeCallbackSource_Item) return false;
    unsigned link = source_class == NativeCallbackSource_Fighter
        ? HSD_GOBJ_PLINK_FIGHTER : HSD_GOBJ_PLINK_ITEM;
    for (HSD_GObj* live = HSD_GObjPLinkHead[link]; live; live = live->next) {
        if (live != source) continue;
        if (live->user_data == NULL) return false;
        *generation = source_class == NativeCallbackSource_Fighter
            ? (uint32_t) ((Fighter*) live->user_data)->x8_spawnNum
            : ((Item*) live->user_data)->x1C;
        return true;
    }
    return false;
}

bool nativeCallbackSource_Alive(HSD_GObj* source, uint32_t generation,
                                uint32_t source_class)
{
    if (source_class == NativeCallbackSource_ModelEffect)
        return nativeModelEffect_Resolve(source, generation) != NULL;
    if (source == NULL || HSD_GObjPLinkHead == NULL ||
        source_class > NativeCallbackSource_Item) return false;
    unsigned link = source_class == NativeCallbackSource_Fighter
        ? HSD_GOBJ_PLINK_FIGHTER : HSD_GOBJ_PLINK_ITEM;
    for (HSD_GObj* live = HSD_GObjPLinkHead[link]; live; live = live->next) {
        if (live != source) continue;
        if (live->user_data == NULL) return false;
        uint32_t current = source_class == NativeCallbackSource_Fighter
            ? (uint32_t) ((Fighter*) live->user_data)->x8_spawnNum
            : ((Item*) live->user_data)->x1C;
        return current == generation;
    }
    return false;
}

bool nativeCallbackDispatch_Enter(void)
{
    if (dispatch_depth == NATIVE_CALLBACK_DISPATCH_LIMIT) return false;
    ++dispatch_depth;
    return true;
}

void nativeCallbackDispatch_Leave(void)
{
    if (dispatch_depth != 0) --dispatch_depth;
}
