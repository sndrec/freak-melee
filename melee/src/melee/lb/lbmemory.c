#include "lbmemory.h"

#include <Runtime/platform.h>

#include <string.h>
#include <stdint.h>

#include <dolphin/ar.h>
#include <sysdolphin/baselib/debug.h>

struct Allocator {
    void* a_arenaLo;
    void* a_arenaHi;
    Handle x8_mem[0x83];
    Handle* free_mem;
    s32 x630_num_allocs;
    s32 x634_max_num_allocs;
    Handle x638_heap[6];
    Handle* free_heap;
    Handle* x69C;
};


struct Allocator lbMemory_804318B0;
#define _p(x) (lbMemory_804318B0.x)

#define PUSH_HANDLE(list, handle)                                             \
    do {                                                                      \
        handle->x0_next = *list;                                              \
        *list = handle;                                                       \
    } while (0)
#define POP_HANDLE(list, handle)                                              \
    do {                                                                      \
        handle = *list;                                                       \
        *list = handle->x0_next;                                              \
    } while (0)

static inline Handle* new_handle(void* arenaLo, void* arenaHi)
{
    Handle* h;
    HSD_ASSERT(0x7B, _p(free_heap));

    if (((uintptr_t) arenaLo < 0x80000000U) && ((uintptr_t) arenaHi < 0x80000000U)) {
        HSD_ASSERT(0x80, (uintptr_t)arenaLo >= (uintptr_t)_p(a_arenaLo) && (uintptr_t)arenaHi <= (uintptr_t)_p(a_arenaHi));
    }

    POP_HANDLE(&_p(free_heap), h);
    h->x0_next = NULL;
    h->x4_lo = arenaLo;
    h->x8_hi = arenaHi;
    h->xC_prev = NULL;
    return h;
}

Handle* lbMemory_80014E24(void* arenaLo, void* arenaHi)
{
    return new_handle(arenaLo, arenaHi);
}

void lbMemory_80014EEC(Handle* handle)
{
    Handle* iter;
    Handle* tmp_next;
    HSD_ASSERT(149, handle);
    for (iter = handle->xC_prev; iter != NULL;) {
        tmp_next = iter->x0_next;
        PUSH_HANDLE(&_p(free_mem), iter);
        iter = tmp_next;
        _p(x630_num_allocs) -= 1;
    }
    PUSH_HANDLE(&_p(free_heap), handle);
}

size_t lbMemory_80014F7C(Handle* heap)
{
    uintptr_t cursor = (uintptr_t) heap->x4_lo;
    size_t total = 0;
    for (Handle* block = heap->xC_prev; block; block = block->x0_next) {
        total += (uintptr_t) block->x4_lo - cursor;
        cursor = (uintptr_t) block->x4_lo + (uintptr_t) block->x8_hi;
    }
    return total + (uintptr_t) heap->x8_hi - cursor;
}

Handle* lbMemory_80014FC8(Handle* heap, size_t size)
{
    HSD_ASSERT(0, _p(free_mem));
    HSD_ASSERT(0, size > 0 && size <= SIZE_MAX - 31);
    size = (size + 31) & ~(size_t) 31;
    uintptr_t cursor = (uintptr_t) heap->x4_lo;
    uintptr_t chosen_address = 0;
    size_t least_leftover = SIZE_MAX;
    Handle** chosen_link = NULL;
    Handle** link = &heap->xC_prev;

    for (;;) {
        uintptr_t end = (uintptr_t) (*link ? (*link)->x4_lo : heap->x8_hi);
        HSD_ASSERT(0, end >= cursor);
        size_t available = end - cursor;
        if (available >= size && available - size <= least_leftover) {
            least_leftover = available - size;
            chosen_address = cursor;
            chosen_link = link;
        }
        if (!*link) break;
        cursor = (uintptr_t) (*link)->x4_lo + (uintptr_t) (*link)->x8_hi;
        link = &(*link)->x0_next;
    }
    HSD_ASSERT(0, chosen_link);
    Handle* result;
    POP_HANDLE(&_p(free_mem), result);
    result->x4_lo = (void*) chosen_address;
    result->x8_hi = (void*) size;
    result->x0_next = *chosen_link;
    *chosen_link = result;
    ++_p(x630_num_allocs);
    if (_p(x630_num_allocs) > _p(x634_max_num_allocs)) {
        _p(x634_max_num_allocs) = _p(x630_num_allocs);
    }
    return result;
}

void lbMemFreeToHeap(Handle* heap, void* address)
{
    for (Handle** link = &heap->xC_prev; *link; link = &(*link)->x0_next) {
        Handle* block = *link;
        if (block->x4_lo == address) {
            *link = block->x0_next;
            PUSH_HANDLE(&_p(free_mem), block);
            --_p(x630_num_allocs);
            return;
        }
    }
    OSReport("[LbMem] Error: lbMemFreeToHeap %p.\n", address);
    HSD_ASSERT(0, false);
}

void lbMemory_800154BC(uintptr_t* arenaLo, uintptr_t* arenaHi)
{
    *arenaLo = (uintptr_t) _p(a_arenaLo);
    *arenaHi = (uintptr_t) _p(a_arenaHi);
}

Handle* lbMemory_800154D4(void* arenaLo, void* arenaHi)
{
    _p(x69C) = new_handle(arenaLo, arenaHi);
    return _p(x69C);
}

void lbMemory_800155A4(void)
{
    Handle* handle = _p(x69C);
    Handle* iter;

    Handle** r5;

    HSD_ASSERT(149, handle);
    r5 = &_p(free_mem);
    for (iter = handle->xC_prev; iter != NULL;) {
        Handle* tmp_next = iter->x0_next;
        PUSH_HANDLE(r5, iter);
        iter = tmp_next;
        _p(x630_num_allocs) -= 1;
    }
    PUSH_HANDLE(&_p(free_heap), handle);
    _p(x69C) = NULL;
}

#ifdef MUST_MATCH
#pragma push
#pragma dont_inline on
#endif
void lbMemory_8001564C(void)
{
    u32 size[3];
    int i;

    _p(a_arenaLo) = (void*) ARAlloc(0x20);
    ARFree(&size[2]);
    _p(a_arenaHi) =
        (void*) ((ARGetSize() > 0x01000000U) ? 0x01000000U : ARGetSize());

    _p(free_mem) = &_p(x8_mem)[0];
    for (i = 0; i < 0x82; i++) {
        _p(x8_mem)[i].x0_next = &_p(x8_mem)[i + 1];
    }
    _p(x8_mem)[i].x0_next = NULL;

    _p(x634_max_num_allocs) = 0;
    _p(x630_num_allocs) = 0;
    _p(free_heap) = &_p(x638_heap)[0];
    for (i = 0; i < 5; ++i) {
        _p(x638_heap)[i].x0_next = &_p(x638_heap)[i + 1];
    }
    _p(x638_heap)[5].x0_next = NULL;
    _p(x69C) = NULL;
    {
        void* hi = _p(a_arenaHi);
        void* lo = _p(a_arenaLo);
        _p(x69C) = lbMemory_80014E24(lo, hi);
    }
}
#ifdef MUST_MATCH
#pragma pop
#endif
