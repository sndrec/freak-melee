#include <melee/lb/lbmemory.h>
#include <melee/lb/lbarq.h>
#include <melee/lb/lbheap.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

static u8 aram[0x10000];
static uintptr_t arena_base, main_lo, main_hi;
void HSD_GetNextArena(void** lo, void** hi)
{
    *lo = (void*) arena_base;
    *hi = (void*) (arena_base + 0x8000000);
}
int HSD_CreateMainHeap(void* lo, void* hi)
{
    main_lo = (uintptr_t) lo;
    main_hi = (uintptr_t) hi;
    return 1;
}
int OSCreateHeap(void* lo, void* hi) { abort(); }
void OSDestroyHeap(int heap) { abort(); }
long OSCheckHeap(int heap) { abort(); }
void OSFreeToHeap(int heap, void* ptr) { abort(); }
void* OSAllocFromHeap(int heap, u32 size) { abort(); }
void* ARGetStorageAddress(void) { return aram; }

// Only the console ARAM address-range discovery is substituted. All handle
// initialization, allocation, free and accounting below use production code.
u32 ARAlloc(u32 length) { return 0x4000; }
u32 ARFree(u32* length) { *length = 32; return 0x4000; }
u32 ARGetSize(void) { return 0x1000000; }
void OSReport(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}
void __assert(char* file, u32 line, char* expression)
{
    fprintf(stderr, "%s:%lu: %s\n", file, line, expression);
    abort();
}
static void require(bool condition, const char* message)
{
    if (!condition) { fputs(message, stderr); exit(1); }
}

int main(void)
{
    lbMemory_8001564C();
    // Synthetic ranges exercise address bookkeeping without allocating 8 GB.
    const uintptr_t base = 0x100004000ULL;
    const size_t capacity = 0x200000000ULL;
    Handle* heaps[5];
    for (unsigned i = 0; i < 5; ++i) {
        heaps[i] = lbMemory_80014E24((void*) base, (void*) (base + capacity));
        for (unsigned j = 0; j < i; ++j) {
            require(heaps[i] != heaps[j], "duplicate heap handle\n");
        }
    }
    Handle* heap = heaps[0];
    require(lbMemory_80014F7C(heap) == capacity, "wide free-space count\n");
    Handle* first = lbMemory_80014FC8(heap, 33);
    Handle* middle = lbMemory_80014FC8(heap, 95);
    Handle* last = lbMemory_80014FC8(heap, 129);
    require((uintptr_t) first->x4_lo == base && (uintptr_t) middle->x4_lo == base + 64 &&
            (uintptr_t) last->x4_lo == base + 160, "native addresses/alignment\n");
    require(lbMemory_80014F7C(heap) == capacity - 320, "allocated accounting\n");
    lbMemFreeToHeap(heap, middle->x4_lo);
    Handle* reused = lbMemory_80014FC8(heap, 65);
    require((uintptr_t) reused->x4_lo == base + 64, "best-fit gap reuse\n");
    for (unsigned i = 0; i < 5; ++i) lbMemory_80014EEC(heaps[i]);

    heap = lbMemory_80014E24((void*) base, (void*) (base + capacity));
    // Exercise the entire fixed block pool and its recycling.
    void* addresses[131];
    for (unsigned i = 0; i < 131; ++i) {
        addresses[i] = lbMemory_80014FC8(heap, 32)->x4_lo;
        require((uintptr_t) addresses[i] == base + i * 32, "block pool chain\n");
    }
    for (unsigned i = 131; i > 0; --i) lbMemFreeToHeap(heap, addresses[i - 1]);
    require(lbMemory_80014F7C(heap) == capacity, "recycled free space\n");
    Handle* large = lbMemory_80014FC8(heap, 0x100000021ULL);
    require((uintptr_t) large->x8_hi == 0x100000040ULL, "wide allocation size\n");
    require(lbMemory_80014F7C(heap) == capacity - 0x100000040ULL, "wide remainder\n");
    lbMemory_80014EEC(heap);

    // Copying a larger block into a smaller preceding hole overlaps its
    // source; both native RAM and ARAM compaction must retain every byte.
    u8* ram = _aligned_malloc(4096, 32);
    require(ram != NULL && (uintptr_t) ram > 0xFFFFFFFFULL, "native RAM address\n");
    for (unsigned i = 0; i < 4096; ++i) aram[0x4000 + i] = (u8) (i * 13);
    memset(ram, 0xAA, 4096);
    lbArq_Read(0x4000, ram, 1027);
    require(memcmp(ram, aram + 0x4000, 1027) == 0, "native animation ARAM read\n");
    require(ram[1027] == 0xAA, "ARAM read copied alignment padding\n");
    for (unsigned space = 0; space < 2; ++space) {
        uintptr_t start = space ? 0x4000 : (uintptr_t) ram;
        u8* storage = space ? aram + start : ram;
        heap = lbMemory_80014E24((void*) start, (void*) (start + 4096));
        Handle* gap = lbMemory_80014FC8(heap, 32);
        Handle* payload = lbMemory_80014FC8(heap, 1024);
        Handle* tail = lbMemory_80014FC8(heap, 64);
        for (unsigned i = 0; i < 1024; ++i) storage[32 + i] = (u8) (i * 17);
        memset(storage + 1056, 0xAD, 64);
        lbMemFreeToHeap(heap, gap->x4_lo);
        lbMemory_Compact(heap);
        require((uintptr_t) payload->x4_lo == start &&
                    (uintptr_t) tail->x4_lo == start + 1024, "compacted handles\n");
        for (unsigned i = 0; i < 1024; ++i) {
            require(storage[i] == (u8) (i * 17), "overlapping payload copy\n");
        }
        for (unsigned i = 1024; i < 1088; ++i) require(storage[i] == 0xAD, "tail copy\n");
        require(lbMemory_80014F7C(heap) == 4096 - 1088, "compaction accounting\n");
        lbMemory_Compact(heap);
        require((uintptr_t) payload->x4_lo == start, "already compact heap\n");
        lbMemory_80014EEC(heap);
    }
    _aligned_free(ram);
    // Scene transitions must exclude preserved low/high heaps at both signs
    // of the low 32 address bits. No memory at these synthetic addresses is
    // dereferenced; production lbHeap/lbMemory compute the partition bounds.
    const uintptr_t bases[] = { 0x120004000ULL, 0x180004000ULL };
    for (unsigned i = 0; i < 2; ++i) {
        arena_base = bases[i];
        lbHeap_80015F3C();
        lbHeap_800158D0(2, 0);
        lbHeap_800158D0(3, 0);
        lbHeap_800158D0(4, 0);
        lbHeap_80015900();
        require(main_lo == arena_base + 0x4F9000 &&
                    main_hi == arena_base + 0x8000000 - 0x64B400,
                "scene main heap overlaps preserved native heaps\n");
        lbHeap_80015900();
        require(main_lo == arena_base + 0x4F9000,
                "repeated scene transition lost native address\n");
        for (int heap_id = 2; heap_id < 5; ++heap_id)
            lbHeap_800158D0(heap_id, 1);
        lbHeap_80015900();
        require(main_lo == arena_base && main_hi == arena_base + 0x8000000,
                "transient scene heaps were not released\n");
    }
    puts("Native handle allocator checks passed");
    return 0;
}
