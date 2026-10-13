#include <sysdolphin/baselib/objalloc.h>
#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/id.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char storage[1024] ATTRIBUTE_ALIGN(64);
static HSD_ObjAllocData small_pool;
static HSD_ObjAllocData aligned_pool;

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

// This test exercises the production fixed-pool path. Fail if it tries to use
// the separate OS-heap path; no replacement allocator supplies test successes.
void* HSD_MemAlloc(ssize_t size)
{
    fprintf(stderr, "Unexpected OS allocation: %d\n", size);
    exit(1);
}

OSHeapHandle HSD_GetHeap(void)
{
    fputs("Unexpected OS heap lookup\n", stderr);
    exit(1);
}

long OSCheckHeap(int heap)
{
    fputs("Unexpected OS heap query\n", stderr);
    exit(1);
}

void __assert(char* file, u32 line, char* condition)
{
    fprintf(stderr, "%s:%lu: %s\n", file, line, condition);
    exit(1);
}

static void test_object_ids(void)
{
    HSD_IDTable separate_table = { 0 };
    int first_value = 1;
    int second_value = 2;
    int replacement = 3;
    s32 found;
    // Equal low halves catch truncation; the third key also collides in the
    // hash bucket, exercising chain removal independently of pointer width.
    uintptr_t first_id = (uintptr_t) 0x12345678;
    uintptr_t second_id = first_id + ((uintptr_t) 1 << 32);
    uintptr_t third_id = second_id + 101;
    _HSD_ObjAllocForgetMemory(NULL, NULL);
    HSD_ObjSetHeap(sizeof(storage), storage);
    HSD_IDSetup();
    HSD_IDInitAllocData();
    HSD_IDInsertToTable(NULL, first_id, &first_value);
    HSD_IDInsertToTable(NULL, second_id, &second_value);
    HSD_IDInsertToTable(NULL, third_id, NULL);
    require(HSD_IDGetData(first_id, &found) == &first_value && found == 1,
            "Low ID entry was overwritten by a high ID");
    require(HSD_IDGetData(second_id, &found) == &second_value && found == 1,
            "High ID entry cannot be resolved");
    require(HSD_IDGetData(third_id, &found) == NULL && found == 1,
            "Present NULL entry was confused with an absent ID");
    HSD_IDInsertToTable(NULL, second_id, &replacement);
    require(HSD_IDGetAllocData()->used == 3 &&
                HSD_IDGetData(second_id, NULL) == &replacement,
            "Replacing an ID allocated a duplicate entry");
    HSD_IDInsertToTable(&separate_table, second_id, &second_value);
    require(HSD_IDGetDataFromTable(&separate_table, second_id, NULL) ==
                &second_value && HSD_IDGetData(second_id, NULL) == &replacement,
            "Separate ID tables share entries");
    HSD_IDRemoveByIDFromTable(NULL, second_id);
    require(HSD_IDGetData(second_id, &found) == NULL && found == 0 &&
                HSD_IDGetData(third_id, &found) == NULL && found == 1,
            "Removing a chained ID damaged its neighbor");
    HSD_IDRemoveByIDFromTable(NULL, third_id);
    HSD_IDRemoveByIDFromTable(NULL, first_id);
    HSD_IDRemoveByIDFromTable(&separate_table, second_id);
    require(HSD_IDGetAllocData()->used == 0,
            "Removed ID entries were not returned to their pool");
    HSD_IDInsertToTable(NULL, second_id, &second_value);
    require(HSD_IDGetData(second_id, NULL) == &second_value,
            "ID entry reuse failed");
    _HSD_IDForgetMemory(NULL, NULL);
    require(HSD_IDGetData(second_id, &found) == NULL && found == 0,
            "Scene reset retained an old object ID");
}

int main(void)
{
    require((uintptr_t) storage > U32_MAX,
            "Test requires actual storage above the 32-bit address range");
    require(OSRoundUp32B(storage + 1) == (uintptr_t) (storage + 32) &&
                OSRoundDown32B(storage + 31) == (uintptr_t) storage,
            "Arena alignment truncated a native address");
    memset(storage, 0xA5, sizeof(storage));
    HSD_ObjSetHeap(512, storage + 1);
    HSD_ObjAllocInit(&small_pool, 4, 4);
    require(HSD_ObjAllocAddFree(&small_pool, 0) == 0,
            "Zero-slot reservation changed the pool");
    require(HSD_ObjAllocAddFree(&small_pool, U32_MAX) == 0,
            "Overflowing reservation was accepted");
    require(HSD_ObjAllocAddFree(&small_pool, 3) == 3,
            "Cannot reserve small payloads at a native address");

    void* first = HSD_ObjAlloc(&small_pool);
    void* second = HSD_ObjAlloc(&small_pool);
    void* third = HSD_ObjAlloc(&small_pool);
    require(first && second && third && first != second && second != third,
            "Small payload slots overlap or cannot be allocated");
    require((uintptr_t) first % sizeof(void*) == 0 &&
                (uintptr_t) second % sizeof(void*) == 0,
            "Free-list pointers are misaligned");
    memset(first, 0x11, 4);
    memset(second, 0x22, 4);
    memset(third, 0x33, 4);
    HSD_ObjFree(&small_pool, second);
    require(HSD_ObjAlloc(&small_pool) == second, "Freed slot was not reused");
    require(*(unsigned char*) first == 0x11 &&
                *(unsigned char*) third == 0x33,
            "Free-list writes damaged neighboring payloads");
    require(small_pool.used == 3 && small_pool.free == 0 && small_pool.peak == 3,
            "Pool usage accounting is incorrect");
    HSD_ObjAllocSetNumLimit(&small_pool, 3);
    HSD_ObjAllocEnableNumLimit(&small_pool);
    require(HSD_ObjAlloc(&small_pool) == NULL, "Object limit was ignored");

    HSD_ObjAllocInit(&aligned_pool, 33, 32);
    require(HSD_ObjAllocAddFree(&aligned_pool, 2) == 2,
            "Cannot reserve explicitly aligned slots");
    void* aligned_first = HSD_ObjAlloc(&aligned_pool);
    void* aligned_second = HSD_ObjAlloc(&aligned_pool);
    require((uintptr_t) aligned_first % 32 == 0 &&
                (uintptr_t) aligned_second % 32 == 0 &&
                (unsigned char*) aligned_second -
                        (unsigned char*) aligned_first == 64,
            "Explicit alignment or rounded payload stride was lost");
    require(storage[0] == 0xA5 && storage[513] == 0xA5,
            "Pool writes escaped the supplied heap");

    _HSD_ObjAllocForgetMemory(NULL, NULL);
    memset(storage, 0xA5, sizeof(storage));
    // Only two 32-byte objects fit after aligning this deliberately offset base.
    HSD_ObjSetHeap(95, storage + 1);
    HSD_ObjAllocInit(&aligned_pool, 32, 32);
    require(HSD_ObjAllocAddFree(&aligned_pool, 10) == 2,
            "Reservation did not clip to available whole slots");
    require(HSD_ObjAlloc(&aligned_pool) == storage + 32 &&
                HSD_ObjAlloc(&aligned_pool) == storage + 64,
            "Clipped pool addresses are incorrect");
    require(HSD_ObjAlloc(&aligned_pool) == NULL,
            "Exhausted fixed heap did not return NULL");
    require(storage[0] == 0xA5 && storage[96] == 0xA5,
            "Clipped reservation damaged the heap boundary");
    puts("Native fixed-pool allocation passed at a 64-bit address");
    test_object_ids();
    puts("Native object ID lookup, collision, replacement and removal passed");
    return 0;
}
