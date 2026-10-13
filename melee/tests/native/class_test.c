#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/memory.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    HSD_Class parent;
    unsigned char payload[120];
} TestClass;

static void init_class(void);
static HSD_ClassInfo test_class = { .head = { .info_init = init_class } };
static int releases;
static int fail_init;

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

static int init_object(HSD_Class* object)
{
    TestClass* instance = (TestClass*) object;
    for (int i = 0; i < sizeof(instance->payload); ++i) {
        require(instance->payload[i] == 0, "New class payload was not cleared");
    }
    instance->payload[0] = 0xAB;
    return fail_init ? -1 : 0;
}

static void release_object(HSD_Class* object)
{
    ++releases;
}

static void init_class(void)
{
    hsdInitClassInfo(&test_class, &hsdClass, "sysdolphin_base_library", "native_test",
                     sizeof(test_class), sizeof(TestClass));
    test_class.init = init_object;
    test_class.release = release_object;
}

void test_native_class_allocator(void)
{
    TestClass* first = hsdNew(&test_class);
    require(first && first->parent.class_info == &test_class &&
                first->payload[0] == 0xAB && test_class.head.nb_exist == 1,
            "Native class construction or inherited allocation failed");
    require(hsdIsDescendantOf(&test_class, &hsdClass),
            "Native class ancestry was lost");

    // These sizes force two expansions of the allocator's pointer table.
    unsigned char* medium = hsdAllocMemPiece(1800);
    unsigned char* large = hsdAllocMemPiece(3600);
    require(medium && large, "Class allocator could not grow its size table");
    memset(medium, 0x18, 1800);
    memset(large, 0x36, 3600);
    require(first->payload[0] == 0xAB && medium[1799] == 0x18 &&
                large[3599] == 0x36,
            "Growing the size table damaged active allocations");
    hsdFreeMemPiece(medium, 1800);
    hsdFreeMemPiece(large, 3600);
    require(hsdAllocMemPiece(1800) == medium, "Freed class memory was not reused");
    hsdFreeMemPiece(medium, 1800);

    hsdDelete(first);
    require(releases == 1 && test_class.head.nb_exist == 0,
            "Class release/destroy did not update live-instance counts");
    fail_init = 1;
    require(hsdNew(&test_class) == NULL && test_class.head.nb_exist == 0,
            "Failed initialization did not return its class allocation");
    fail_init = 0;
    first = hsdNew(&test_class);
    require(first && test_class.head.nb_exist == 1 && test_class.head.nb_peak == 1,
            "Class allocation did not recover after failed initialization");
    hsdDelete(first);
    hsdForgetClassLibrary(NULL);
    require(!(hsdClass.head.flags & 1) && !(test_class.head.flags & 1) &&
                test_class.head.nb_exist == 0 && test_class.head.nb_peak == 0,
            "Scene reset retained initialized class metadata");
    require(OSCheckHeap(HSD_GetHeap()) >= 0, "Class allocations damaged Aurora's heap");
    puts("Native class construction, table growth, reuse and reset passed");
}
