#include "native_match_os_heap.h"
#include <sysdolphin/baselib/memory.h>
#include <dolphin/os/OSHeapSnapshot.h>
#include <stdlib.h>
#include <string.h>
// Construction scratch and lease identity are host state, excluded from copied
// gameplay. Captured roots are the actual heap descriptor/Cell headers.
#pragma clang section data = "" bss = ""
static OSHeapHandle admitted_heap=-1;
static struct {
    OSHeapSnapshotHeader header;
    OSHeapSnapshotCell cells[OS_HEAP_SNAPSHOT_MAX_CELLS];
} construction;

bool nativeMatchOsHeap_AdmitHistory(NativeMatchArena* arena)
{
    OSHeapHandle heap=HSD_GetHeap();
    OSAllocStateRegion descriptor;
    u32 written;
    if (!arena || !arena->lifetime || arena->sealed || admitted_heap!=-1 ||
        !OSGetHeapStateRegion(heap,&descriptor) ||
        !OSSaveHeapSnapshot(heap,&construction,sizeof(construction),&written) ||
        construction.header.cells>NATIVE_MATCH_ARENA_ROOTS-2 ||
        construction.header.cells+2>NATIVE_MATCH_ARENA_ROOTS-arena->root_count)
        return false;
    if (!nativeMatchArena_AddRoot(arena,descriptor.memory,descriptor.size) ||
        !nativeMatchArena_AddRoot(arena,(void*)&__OSCurrHeap,sizeof(__OSCurrHeap))) return false;
    for (u32 i=0;i<construction.header.cells;++i)
        if (!nativeMatchArena_AddRoot(arena,construction.cells[i].address,
                sizeof(construction.cells[i].state))) return false;
    // The complete source allocator validation happens above and at locking,
    // only during construction. Save/load traverse fixed spans afterwards.
    if (!OSLockHeapForHistory(heap)) return false;
    admitted_heap=heap;
    return true;
}

void nativeMatchOsHeap_Retire(void)
{
    if (admitted_heap==-1) return;
    if (!OSUnlockHeapForHistory(admitted_heap)) abort();
    admitted_heap=-1;
}
