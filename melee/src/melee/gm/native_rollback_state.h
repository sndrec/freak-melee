#ifndef MELEE_GM_NATIVE_ROLLBACK_STATE_H
#define MELEE_GM_NATIVE_ROLLBACK_STATE_H

#include <stddef.h>

#ifdef MELEE_NATIVE

typedef struct NativeRollbackStateRegion {
    void* memory;
    size_t size;
} NativeRollbackStateRegion;

size_t nativeRollbackState_GetGlobalRegions(
    NativeRollbackStateRegion* regions, size_t capacity);

#endif

#endif
