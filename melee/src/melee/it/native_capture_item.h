#ifndef MELEE_NATIVE_CAPTURE_ITEM_H
#define MELEE_NATIVE_CAPTURE_ITEM_H

#include <melee/it/forward.h>
#include <melee/ft/forward.h>
#include <stdbool.h>
#include <melee/ft/native_fighter_lifetime.h>
#include "native_item_lifetime.h"

typedef struct NativeCaptureItemPair {
    NativeFighterLifetime holder;
    NativeItemLifetime item;
} NativeCaptureItemPair;
// A missing target is a valid retained pair with item.object == NULL.
bool nativeCaptureItem_Retain(Fighter_GObj*,NativeCaptureItemPair*);
bool nativeCaptureItem_Current(const NativeCaptureItemPair*,bool attached);

// An arbitrary existing item becomes inert while attached to a captor.
// The caller resolves the holder's semantic bone to its current raw index.
bool nativeCaptureItem_Attach(Item_GObj*, Fighter_GObj*, unsigned bone);
bool nativeCaptureItem_Release(Item_GObj*, unsigned bone, bool clear_destroy_type);

#endif
