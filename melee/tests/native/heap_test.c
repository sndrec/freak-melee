#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/objalloc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char arena[1024 * 1024] ATTRIBUTE_ALIGN(64);
static HSD_ObjAllocData objects;
void test_native_class_allocator(void);
void test_native_animation(void);

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

static void panic_observer(const char* file, u32 line, const char* message)
{
    fprintf(stderr, "native panic observer: %s:%lu\n", file, line);
}

int main(int argc, char** argv)
{
    if (argc == 2 && strcmp(argv[1], "--panic") == 0) {
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
        HSD_SetPanicCallback(panic_observer);
        __assert("native_heap_test.c", 73, "intentional native panic test");
    }

    require((uintptr_t) arena > U32_MAX, "Arena must occupy a native high address");
    memset(arena, 0xA5, sizeof(arena));
    void* start = OSInitAlloc(arena, arena + sizeof(arena), 2);
    require(start != NULL, "Aurora heap metadata initialization failed");
    void* midpoint = arena + sizeof(arena) / 2;
    OSHeapHandle first = OSCreateHeap(start, midpoint);
    OSHeapHandle second = OSCreateHeap(midpoint, arena + sizeof(arena));
    require(first >= 0 && second >= 0 && first != second,
            "Aurora could not create separate heaps");
    HSD_SetHeap(first);
    OSSetCurrentHeap(first);
    long first_free = OSCheckHeap(first);
    long second_free = OSCheckHeap(second);
    require(first_free > 0 && second_free > 0, "New heaps failed validation");
    require(HSD_MemAlloc(0) == NULL && HSD_MemAlloc(-1) == NULL,
            "Nonpositive HSD allocation should return NULL");

    unsigned char* small = HSD_MemAlloc(13);
    unsigned char* large = HSD_MemAlloc(16385);
    require(small && large && (uintptr_t) small % 32 == 0 &&
                (uintptr_t) large % 32 == 0,
            "HSD allocations are missing or misaligned");
    memset(small, 0x13, 13);
    memset(large, 0x85, 16385);
    require(small[12] == 0x13 && large[16384] == 0x85 &&
                OSCheckHeap(first) < first_free,
            "Allocated payloads or heap accounting failed");
    HSD_Free(small);
    HSD_Free(large);
    require(OSCheckHeap(first) == first_free,
            "Freeing allocations did not coalesce the heap");

    // HSD's selected heap must win even if the OS default still names another.
    HSD_SetHeap(second);
    small = HSD_MemAlloc(17);
    require((uintptr_t) small >= (uintptr_t) midpoint &&
                OSCheckHeap(first) == first_free && OSCheckHeap(second) < second_free,
            "HSD allocation used the wrong selected heap");
    HSD_Free(small);
    require(OSCheckHeap(second) == second_free,
            "HSD free used the wrong selected heap");

    HSD_SetHeap(first);
    HSD_ObjSetHeap((u32) first_free, NULL);
    HSD_ObjAllocInit(&objects, 24, 8);
    require(HSD_ObjAllocAddFree(&objects, 4) == 4,
            "Object pool could not reserve from Aurora's heap");
    void* slots[4];
    for (int i = 0; i < 4; ++i) {
        slots[i] = HSD_ObjAlloc(&objects);
        require(slots[i] != NULL, "OS-backed pool allocation failed");
        memset(slots[i], i + 1, 24);
    }
    for (int i = 0; i < 4; ++i) {
        require(((unsigned char*) slots[i])[23] == i + 1,
                "OS-backed pool payloads overlap");
        HSD_ObjFree(&objects, slots[i]);
    }
    require(objects.used == 0 && objects.free == 4 && OSCheckHeap(first) >= 0,
            "OS-backed pool accounting failed");
    test_native_class_allocator();
    test_native_animation();
    _HSD_ObjAllocForgetMemory(NULL, NULL);
    OSDestroyHeap(first);
    first = OSCreateHeap(start, midpoint);
    HSD_SetHeap(first);
    require(OSCheckHeap(first) == first_free,
            "Recreating the scene heap did not recover its full capacity");
    OSDestroyHeap(first);
    OSDestroyHeap(second);
    HSD_SetHeap(-1);
    puts("HSD allocations and pools passed against Aurora's real heap implementation");
    return 0;
}
