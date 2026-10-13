#include <melee/gm/native_rollback_pages.h>

#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

static void require(bool condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

static void count_dirty_page(uint32_t region, uint32_t page, void* memory,
                             void* user_data)
{
    (void) region;
    (void) page;
    (void) memory;
    ++*(uint32_t*) user_data;
}

int main(void)
{
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    size_t page_size = system_info.dwPageSize;
    uint8_t* first = VirtualAlloc(NULL, page_size * 2,
                                  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    uint8_t* second = VirtualAlloc(NULL, page_size * 2,
                                   MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    require(first != NULL && second != NULL, "allocate fixtures");
    memset(first, 0x11, page_size * 2);
    memset(second, 0x22, page_size * 2);
    NativeRollbackPagedRegion regions[] = {
        { first, page_size * 2 },
        { second, page_size * 2 },
    };
    uint8_t exact_first[7] = { 1, 2, 3, 4, 5, 6, 7 };
    uint32_t exact_second = 0x12345678;
    NativeRollbackExactRegion exact_regions[] = {
        { exact_first, sizeof(exact_first) },
        { &exact_second, sizeof(exact_second) },
    };

    require(nativeRollbackPages_Init(regions, 2, exact_regions, 2, 4, 3),
            "initialize rollback storage");
    require(nativeRollbackPages_PageSize() == page_size, "page size");

    require(nativeRollbackPages_BeginFrame(10), "begin frame 10");
    first[3] = 0x30;
    first[20] = 0x31;
    second[page_size + 7] = 0x32;
    exact_first[2] = 0x81;
    exact_second = 0x87654321;
    require(nativeRollbackPages_DirtyPageCount() == 2,
            "one copy per dirty page across regions");
    require(nativeRollbackPages_EndFrame(), "end frame 10");
    uint32_t visited = 0;
    require(nativeRollbackPages_VisitFrame(10, count_dirty_page, &visited),
            "visit completed frame");
    require(visited == 2, "visit each dirty page once");

    require(nativeRollbackPages_BeginFrame(11), "begin frame 11");
    first[3] = 0x40;
    second[9] = 0x41;
    exact_first[5] = 0x82;
    exact_second = 0xABCDEF01;
    require(nativeRollbackPages_EndFrame(), "end frame 11");
    require(nativeRollbackPages_ToFrame(10, 12), "rollback two frames");
    for (size_t i = 0; i < page_size * 2; ++i) {
        require(first[i] == 0x11, "restore first region");
        require(second[i] == 0x22, "restore second region");
    }
    require(exact_first[2] == 3 && exact_first[5] == 6,
            "restore exact byte region");
    require(exact_second == 0x12345678, "restore exact scalar region");

    require(nativeRollbackPages_BeginFrame(12), "begin frame 12");
    first[0] = 1;
    first[page_size] = 2;
    second[0] = 3;
    second[page_size] = 4;
    require(!nativeRollbackPages_EndFrame(), "dirty page overflow rejected");
    require(!nativeRollbackPages_ToFrame(12, 13),
            "overflowed frame cannot restore");

    nativeRollbackPages_Shutdown();

    memset(first, 0x11, page_size * 2);
    require(nativeRollbackPages_Init(regions, 2, NULL, 0, 2, 1),
            "initialize short rollback ring");
    for (uint64_t frame = 20; frame < 23; ++frame) {
        require(nativeRollbackPages_BeginFrame(frame), "begin ring frame");
        first[0] = (uint8_t) frame;
        require(nativeRollbackPages_EndFrame(), "end ring frame");
    }
    require(!nativeRollbackPages_ToFrame(20, 23),
            "overwritten frame cannot restore");

    nativeRollbackPages_Shutdown();
    require(VirtualFree(first, 0, MEM_RELEASE), "release first fixture");
    require(VirtualFree(second, 0, MEM_RELEASE), "release second fixture");
    return 0;
}
