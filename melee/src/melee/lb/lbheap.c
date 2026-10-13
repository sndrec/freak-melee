#include "lbheap.static.h"
#include "lbmemory.h"
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/initialize.h>
#include <sysdolphin/baselib/memory.h>

struct lbHeap_HeapDesc {
    u32 idx;
    u32 type;
    u32 prev_idx;
    u32 size;
};

struct lbHeap_HeapDesc lbHeap_803BA380[5] = {
    { 2, LbHeapType_MainLow, 6, 0x800 },
    { 3, LbHeapType_MainLow, 2, 0x4F8800 },
    { 4, LbHeapType_MainHigh, 6, 0x64B400 },
    { 5, LbHeapType_AramLow, 6, 0x96C800 },
    { 6, LbHeapType_Hsd, 0, 0 },
};

static inline void lbHeap_ResetHeap(struct Heap* heap)
{
    heap->id = -1;
    heap->handle = (Handle*) -1;
    heap->start = 0;
    heap->size = 0;
    heap->type = LbHeapType_MainLow;
    heap->transient = 1;
    heap->status = LbHeapStatus_Destroy;
}

static inline void
lbHeap_DestroyIfCreated(struct Heap* heap)
{

    if (heap->status == LbHeapStatus_Create) {
        if (heap->type == LbHeapType_Hsd) {
            OSDestroyHeap(heap->id);
            heap->id = -1;
        } else {
            lbMemory_80014EEC(heap->handle);
            heap->handle = (Handle*) -1;
        }
        heap->status = LbHeapStatus_Destroy;
    }
}

static inline void
lbHeap_CreateIfDestroyed(struct Heap* heap)
{

    if (heap->status == LbHeapStatus_Destroy) {
        if (heap->type == LbHeapType_Hsd) {
            heap->id = OSCreateHeap((void*) heap->start,
                                    (void*) (heap->start + heap->size));
        } else {
            heap->handle = lbMemory_80014E24(
                (void*) heap->start, (void*) (heap->start + heap->size));
        }
        heap->status = LbHeapStatus_Create;
    }
}

void lbHeap_800158D0(int arg0, int arg1)
{
    lbHeap_80431FA0.heap_array[arg0].transient = arg1;
}

int lbHeap_800158E8(int arg0)
{
    return lbHeap_80431FA0.heap_array[arg0].transient;
}

void lbHeap_80015900(void)
{
    uintptr_t preserved_end;
    s32 bounds_i;
    struct Heap* bounds_heap;
    s32 create_i;
    uintptr_t arena_lo;
    uintptr_t aram_lo;
    uintptr_t aram_hi;
    uintptr_t arena_hi;
    struct Heap* main_heap;
    s32 destroy_i;
    struct Heap* aram_heap;

    /// @remarks 0 and 1 are reserved for HSD and ARAM
    for (destroy_i = 2; destroy_i < 6; ++destroy_i) {
        struct Heap* heap = &lbHeap_80431FA0.heap_array[destroy_i];
        if (heap->transient == 1) {
            lbHeap_DestroyIfCreated(heap);
        }
    }

    arena_lo = (uintptr_t) lbHeap_80431FA0.arena_lo;
    arena_hi = (uintptr_t) lbHeap_80431FA0.arena_hi;
    aram_lo = lbHeap_80431FA0.aram_lo;
    aram_hi = lbHeap_80431FA0.aram_hi;

    for (bounds_i = 2; bounds_i < 6; bounds_i++) {
        bounds_heap = &lbHeap_80431FA0.heap_array[bounds_i];
        if (lbHeap_80431FA0.heap_array[bounds_i].transient == 0) {
            switch (bounds_heap->type) {
            case LbHeapType_Hsd:
            case LbHeapType_Aram:
                break;
            case LbHeapType_MainLow:
                preserved_end = bounds_heap->start + bounds_heap->size;
                if (arena_lo < preserved_end) {
                    arena_lo = preserved_end;
                }
                break;

            case LbHeapType_MainHigh:
                if (arena_hi > bounds_heap->start) {
                    arena_hi = bounds_heap->start;
                }
                break;

            case LbHeapType_AramLow:
                preserved_end = bounds_heap->start + bounds_heap->size;
                if (aram_lo < preserved_end) {
                    aram_lo = preserved_end;
                }
                break;
            }
        }
    }

    main_heap = &lbHeap_80431FA0.heap_array[0];
    main_heap->id = HSD_CreateMainHeap((void*) arena_lo, (void*) arena_hi);
    main_heap->start = arena_lo;
    aram_heap = &lbHeap_80431FA0.heap_array[1];
    main_heap->size = arena_hi - arena_lo;
    main_heap->status = LbHeapStatus_Create;
    main_heap->type = LbHeapType_Hsd;

    lbMemory_800155A4();

    aram_heap->handle = lbMemory_800154D4((void*) aram_lo, (void*) aram_hi);
    aram_heap->start = aram_lo;
    aram_heap->size = aram_hi - aram_lo;
    aram_heap->status = LbHeapStatus_Create;
    aram_heap->type = 3;

    for (create_i = 2; create_i < 6; ++create_i) {
        struct Heap* heap = &lbHeap_80431FA0.heap_array[create_i];
        if (heap->transient == 0) {
            lbHeap_CreateIfDestroyed(heap);
        }
    }
}

LbHeapStatus lbHeap_80015BB8(int arg0)
{
    return lbHeap_80431FA0.heap_array[arg0].status;
}

void* lbHeap_80015BD0(int heap_id, size_t size)
{
    Handle* result;
    struct Heap* p = &lbHeap_80431FA0.heap_array[heap_id];

    if (p->status == LbHeapStatus_Create) {
        if (p->type == LbHeapType_Hsd) {
            int cur_heap = HSD_GetHeap();
            HSD_SetHeap(p->id);
            result = HSD_MemAlloc(size);
            HSD_SetHeap(cur_heap);
        } else {
            result = lbMemory_80014FC8(p->handle, size);
            if (p->type == LbHeapType_Aram) {
                result = result->x4_lo;
            }
        }
    } else {
        result = NULL;
    }
    return result;
}

void lbHeap_80015CA8(int arg0, void* arg1)
{
    struct Heap* p = &lbHeap_80431FA0.heap_array[arg0];

    HSD_ASSERT(0x143, p->status == LbHeapStatus_Create);
    if (p->type == LbHeapType_Hsd) {
        int cur_heap = HSD_GetHeap();
        HSD_SetHeap(p->id);
        HSD_Free(arg1);
        HSD_SetHeap(cur_heap);
    } else {
        lbMemFreeToHeap(p->handle, arg1);
    }
}

void lbHeap_Compact(u32 heap_id)
{
    HSD_ASSERT(0, heap_id < 6);
    if (heap_id > 1) {
        struct Heap* heap = &lbHeap_80431FA0.heap_array[heap_id];
        HSD_ASSERT(0, heap->status == LbHeapStatus_Create && heap->handle);
        lbMemory_Compact(heap->handle);
    }
}

char* lbHeap_803BA448[] = {
    "     Hsd", "    ARAM", "     Seq", "    Stay", "    AllM", "    AllA",
};

void lbHeap_80015DF8(void)
{
    size_t bytes;
    int i;

    OSReport("[lbHeap] -- Report --\n");
    for (i = 0; i < 6; i++) {
        struct Heap* heap = &lbHeap_80431FA0.heap_array[i];
        OSReport("%s :", lbHeap_803BA448[i]);
        if (heap->status == LbHeapStatus_Create) {
            ssize_t free_bytes = heap->type == LbHeapType_Hsd
                ? OSCheckHeap(heap->id)
                : lbMemory_80014F7C(heap->handle);
            if (free_bytes < 0 || (size_t) free_bytes > heap->size) {
                OSReport(" invalid heap accounting (%lld free bytes)",
                         (long long) free_bytes);
            } else {
                OSReport(" %5zu KB used + %5zu KB free (%zu bytes)",
                         (heap->size - (size_t) free_bytes) / 1024,
                         (size_t) free_bytes / 1024, (size_t) free_bytes);
            }
        } else {
            OSReport(" destroyed");
        }
        OSReport(" / %5zu KB total\n", (size_t) heap->size / 1024);
    }

    bytes = (uintptr_t) lbHeap_80431FA0.arena_hi -
            (uintptr_t) lbHeap_80431FA0.arena_lo;
    OSReport("MainRAM Total : %5zu KB (%zu bytes)\n", bytes / 1024, bytes);
    bytes = lbHeap_80431FA0.aram_hi - lbHeap_80431FA0.aram_lo;
    OSReport("   ARAM Total : %5zu KB (%zu bytes)\n", bytes / 1024, bytes);
}

void lbHeap_80015F3C(void)
{
    int curr_idx;
    int prev_idx;
    struct Heap* curr_heap;
    struct Heap* prev_heap;
    struct lbHeap_HeapDesc* desc;

    HSD_GetNextArena(&lbHeap_80431FA0.arena_lo, &lbHeap_80431FA0.arena_hi);
    lbMemory_800154BC(&lbHeap_80431FA0.aram_lo, &lbHeap_80431FA0.aram_hi);

    lbHeap_ResetHeap(&lbHeap_80431FA0.heap_array[0]);
    lbHeap_ResetHeap(&lbHeap_80431FA0.heap_array[1]);
    lbHeap_ResetHeap(&lbHeap_80431FA0.heap_array[2]);
    lbHeap_ResetHeap(&lbHeap_80431FA0.heap_array[3]);
    lbHeap_ResetHeap(&lbHeap_80431FA0.heap_array[4]);
    lbHeap_ResetHeap(&lbHeap_80431FA0.heap_array[5]);

    desc = lbHeap_803BA380;
    while ((curr_idx = desc->idx) != 6) {
        curr_heap = &lbHeap_80431FA0.heap_array[curr_idx];

        curr_heap->type = desc->type;
        curr_heap->size = desc->size;
        prev_idx = desc->prev_idx;
        if (prev_idx == 6) {
            switch (curr_heap->type) {
            case LbHeapType_Hsd:
            case LbHeapType_Aram:
                break;
            case LbHeapType_MainLow:
                curr_heap->start = (uintptr_t) lbHeap_80431FA0.arena_lo;
                break;
            case LbHeapType_MainHigh:
                curr_heap->start =
                    (uintptr_t) lbHeap_80431FA0.arena_hi - curr_heap->size;
                break;
            case LbHeapType_AramLow:
                curr_heap->start = lbHeap_80431FA0.aram_lo;
                break;
            }
        } else {
            prev_heap = &lbHeap_80431FA0.heap_array[prev_idx];
            switch (curr_heap->type) {
            case LbHeapType_Hsd:
            case LbHeapType_Aram:
                break;
            case LbHeapType_MainLow:
                curr_heap->start = prev_heap->start + prev_heap->size;
                break;
            case LbHeapType_MainHigh:
                curr_heap->start = prev_heap->start - curr_heap->size;
                break;
            case LbHeapType_AramLow:
                curr_heap->start = prev_heap->start + prev_heap->size;
                break;
            }
        }
        desc++;
    }
}

size_t lbHeap_GetActiveMainMemoryRegions(LbHeapMemoryRegion* regions,
                                         size_t capacity)
{
    size_t count = 0;
    for (size_t i = 0; i < ARRAY_SIZE(lbHeap_80431FA0.heap_array); ++i) {
        struct Heap* heap = &lbHeap_80431FA0.heap_array[i];
        bool main_memory = heap->type == LbHeapType_Hsd ||
                           heap->type == LbHeapType_MainLow ||
                           heap->type == LbHeapType_MainHigh;
        if (heap->status != LbHeapStatus_Create || !main_memory ||
            heap->size == 0)
        {
            continue;
        }
        if (count < capacity) {
            regions[count] = (LbHeapMemoryRegion) {
                .memory = (void*) heap->start,
                .size = heap->size,
                .allocator_heap = heap->type == LbHeapType_Hsd ? heap->id : -1,
            };
        }
        ++count;
    }
    return count;
}
