#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int events[16], event_count, mode, removed;
static void require(bool ok, const char* text)
{
    if (!ok) { fprintf(stderr, "%s\n", text); exit(1); }
}
static void release_data(void* data)
{
    require(data == &removed, "GObj user-data pointer lost native precision");
    ++removed;
}
static void tick(HSD_GObj* object)
{
    require(event_count < 16, "Scheduler invoked callbacks repeatedly");
    require(HSD_GObj_CurrentInvokedProcGObj == object &&
            HSD_GObj_CurrentInvokedProc->gobj == object, "Incorrect scheduler callback context");
    events[event_count++] = object->classifier;
    if (mode == 1 && object->classifier == 20) HSD_GObjFree(object);
    if (mode == 2 && object->classifier == 10)
        HSD_GObjProc_RemoveProc(HSD_GObj_CurrentInvokedProc);
}
static void run(const int* expected, int count)
{
    event_count = 0;
    HSD_GObj_80390CFC();
    require(event_count == count && !memcmp(events, expected, count * sizeof(int)),
            "Scheduler callback order or deferred removal is incorrect");
    require(!HSD_GObj_CurrentInvokedProcGObj && !HSD_GObj_CurrentInvokedProc &&
            !HSD_GObj_DelayedProcInfo.flags, "Scheduler left an active callback after its frame");
}
void test_native_gobj(void)
{
    HSD_GObjLibInitDataType config;
    u64 paused = 0;
    HSD_GObj_803912E0(&config);
    config.unk_2 = &paused;
    HSD_GObj_80391304(&config);
    HSD_GObj* a = GObj_Create(10, 1, 2);
    HSD_GObj* b = GObj_Create(20, 1, 1);
    HSD_GObj* c = GObj_Create(30, 0, 0);
    HSD_GObj* d = GObj_Create(40, 63, 0);
    require(a && b && c && d, "Cannot create native GObjs");
    GObj_InitUserData(b, 0, release_data, &removed);
    HSD_GObj_SetupProc(a, tick, 0); HSD_GObj_SetupProc(b, tick, 0);
    HSD_GObj_SetupProc(c, tick, 1); HSD_GObj_SetupProc(d, tick, 0);
    const int all[] = { 20, 10, 40, 30 }, masked[] = { 20, 10, 30 };
    const int no_b[] = { 10, 40, 30 }, no_a_b[] = { 40, 30 };
    long free_bytes = OSCheckHeap(HSD_GetHeap());
    run(all, 4);
    paused = 1ULL << 63; run(masked, 3); paused = 0;
    mode = 1; run(all, 4);
    require(removed == 1, "Deferred object removal did not release user data exactly once");
    mode = 2; run(no_b, 3);
    mode = 0; run(no_a_b, 2);
    require(OSCheckHeap(HSD_GetHeap()) == free_bytes, "Scheduler allocated memory during callbacks");
    HSD_GObjFree(a); HSD_GObjFree(c); HSD_GObjFree(d);
    for (unsigned i = 0; i <= config.p_link_max; ++i)
        require(!HSD_GObjPLinkHead[i] && !plinklow_gobjs[i], "Removed GObj remains linked");
    for (unsigned i = 0; i <= config.gproc_pri_max; ++i)
        require(!HSD_GObj_GObjProcHead[i], "Removed callback remains queued");
    HSD_Free(HSD_GObjPLinkHead); HSD_Free(plinklow_gobjs);
    HSD_Free(HSD_GObjGXLinkHead); HSD_Free(HSD_GObj_804D7820);
    HSD_Free(HSD_GObj_GObjProcHead); HSD_Free(HSD_GObj_ProcList); HSD_Free(HSD_GObj_804D7810);
    HSD_GObjPLinkHead = plinklow_gobjs = NULL;
    HSD_GObjGXLinkHead = HSD_GObj_804D7820 = NULL;
    HSD_GObj_GObjProcHead = HSD_GObj_ProcList = NULL;
    HSD_GObj_804D7810 = NULL;
    HSD_GObjLibInitData.unk_2 = NULL;
    puts("Original GObj scheduler order, high pause mask and deferred removals passed");
}
