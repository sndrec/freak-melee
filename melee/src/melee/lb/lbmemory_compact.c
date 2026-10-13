#include "lbmemory.h"

#include <dolphin/ar.h>
#include <string.h>
#include <sysdolphin/baselib/debug.h>

void lbMemory_Compact(Handle* heap)
{
    uintptr_t cursor = (uintptr_t) heap->x4_lo;
    const uintptr_t end = (uintptr_t) heap->x8_hi;
    HSD_ASSERT(0, cursor <= end);
    u8* aram = NULL;
    if (end <= ARGetSize()) {
        aram = ARGetStorageAddress();
        HSD_ASSERT(0, aram != NULL);
    }
    for (Handle* block = heap->xC_prev; block; block = block->x0_next) {
        uintptr_t source = (uintptr_t) block->x4_lo;
        size_t size = (uintptr_t) block->x8_hi;
        HSD_ASSERT(0, source >= cursor && source <= end && size <= end - source);
        if (source != cursor) {
            void* destination = aram ? aram + cursor : (void*) cursor;
            const void* data = aram ? aram + source : (void*) source;
            memmove(destination, data, size);
            block->x4_lo = (void*) cursor;
        }
        cursor += size;
    }
}
