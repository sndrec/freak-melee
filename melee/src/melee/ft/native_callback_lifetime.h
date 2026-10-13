#ifndef MELEE_FT_NATIVE_CALLBACK_LIFETIME_H
#define MELEE_FT_NATIVE_CALLBACK_LIFETIME_H

#include <sysdolphin/baselib/forward.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum NativeCallbackSourceClass {
    NativeCallbackSource_Fighter,
    NativeCallbackSource_Item,
    NativeCallbackSource_ModelEffect,
} NativeCallbackSourceClass;

enum { NATIVE_CALLBACK_DISPATCH_LIMIT = 64 };

// Check live registry membership before reading a retained object address.
// A recycled address with a different full creation number is a new entity.
bool nativeCallbackSource_Alive(HSD_GObj*, uint32_t generation,
                                uint32_t source_class);
bool nativeCallbackSource_Snapshot(HSD_GObj*, uint32_t source_class,
                                   uint32_t* generation);

// Balanced synchronous named-event dispatch. This is transient call-stack
// bookkeeping, not saved gameplay state; no callback survives the call.
bool nativeCallbackDispatch_Enter(void);
void nativeCallbackDispatch_Leave(void);

#endif
