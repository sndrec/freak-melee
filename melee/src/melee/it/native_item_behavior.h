#ifndef MELEE_IT_NATIVE_ITEM_BEHAVIOR_H
#define MELEE_IT_NATIVE_ITEM_BEHAVIOR_H

#include <melee/it/forward.h>
#include <stdint.h>

typedef enum NativeItemBehaviorSelection {
    NativeItemBehavior_Engine,
    NativeItemBehavior_Parent,
    NativeItemBehavior_Author,
} NativeItemBehaviorSelection;

// Ordinary common-item assets must be loaded before allocating the instance.
bool nativeItemBehavior_EngineAssetsAvailable(ItemKind kind);

// Called after the ordinary item table is selected, before model construction.
// Engine selection deliberately leaves both that table and its assets intact.
bool nativeItemBehavior_Select(Item_GObj* item, const SpawnItem* spawn,
                               NativeItemBehaviorSelection selection,
                               FighterKind author,uint32_t source_owner);

#endif
