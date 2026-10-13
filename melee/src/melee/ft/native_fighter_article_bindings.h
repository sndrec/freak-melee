#ifndef NATIVE_FIGHTER_ARTICLE_BINDINGS_H
#define NATIVE_FIGHTER_ARTICLE_BINDINGS_H

#include <melee/ft/forward.h>
#include <melee/it/forward.h>
#include <stdbool.h>

// Source-archive layout used by stock import and by owned article assets.
// Behavior and tuning never use these table indices as script state.
typedef struct NativeStockArticleSlot {
    FighterKind source_kind;
    ItemKind item_kind;
    unsigned slot;
    // Native converted extended-attribute byte offset; -1 selects item_kind.
    int kind_attribute_offset;
    const char* name;
} NativeStockArticleSlot;

extern const NativeStockArticleSlot nativeStockArticleSlots[];
extern const unsigned nativeStockArticleSlotCount;
#endif
