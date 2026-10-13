#include <melee/gm/native_match_os_heap.h>
static unsigned os_checks,os_rejections;
static u32 os_bytes;
static unsigned char os_space[8*1024*1024] ATTRIBUTE_ALIGN(64);
static unsigned char os_saved[32768],os_bad[32768],os_before[32768],os_after[32768];
static unsigned char os_memory_before[sizeof(os_space)];
static unsigned char os_limit_saved[sizeof(OSHeapSnapshotHeader)+OS_HEAP_SNAPSHOT_MAX_CELLS*sizeof(OSHeapSnapshotCell)];
static OSHeapHandle os_handle,os_other;
typedef struct FixtureCell { void* prev;void* next;int32_t size;void* owner; } FixtureCell;
typedef struct FixtureHeap { s32 size;FixtureCell* free_list;FixtureCell* allocated; } FixtureHeap;
_Static_assert(sizeof(FixtureCell)==32,"actual native Cell layout");

static void reject_os(u32 bytes)
{
    u32 before,after;OSAllocStateRegion region;unsigned char control[128];
    CHECK(OSGetHeapStateRegion(os_handle,&region)&&region.size<=sizeof(control));
    memcpy(control,region.memory,region.size);memcpy(os_memory_before,os_space,sizeof(os_space));
    OSHeapHandle current=__OSCurrHeap;
    CHECK(OSSaveHeapSnapshot(os_handle,os_before,sizeof(os_before),&before));
    CHECK(!OSValidateHeapSnapshot(os_handle,os_bad,bytes));
    CHECK(!OSLoadHeapSnapshot(os_handle,os_bad,bytes));
    CHECK(!memcmp(os_memory_before,os_space,sizeof(os_space))&&!memcmp(control,region.memory,region.size));
    CHECK(current==__OSCurrHeap);
    CHECK(OSSaveHeapSnapshot(os_handle,os_after,sizeof(os_after),&after));
    CHECK(before==after&&!memcmp(os_before,os_after,before));++os_rejections;
}
static void test_os_headers(void)
{
    unsigned first=checks;
    void* start=OSInitAlloc(os_space,os_space+sizeof(os_space),2);CHECK(start);
    os_handle=OSCreateHeap(start,os_space+sizeof(os_space)/2);
    os_other=OSCreateHeap(os_space+sizeof(os_space)/2,os_space+sizeof(os_space));
    CHECK(os_handle>=0&&os_other>=0&&os_handle!=os_other);
    OSSetCurrentHeap(os_handle);
    void* a=OSAllocFromHeap(os_handle,96);void* b=OSAllocFromHeap(os_handle,96);
    void* c=OSAllocFromHeap(os_handle,96);void* foreign=OSAllocFromHeap(os_other,96);CHECK(a&&b&&c&&foreign);
    OSFreeToHeap(os_handle,b);memset(a,0x37,96);memset(c,0x48,96);
    long budget=OSCheckHeap(os_handle);CHECK(budget>0);
    CHECK(OSHeapSnapshotBytes(os_handle)>0);
    CHECK(OSPrepareHeapSnapshotOwnership());
    // A forged current list can look structurally valid while claiming another
    // heap's Cells. The writer must reject it before publishing output.
    OSAllocStateRegion descriptor;CHECK(OSGetHeapStateRegion(os_handle,&descriptor));
    FixtureHeap original_heap,forged_heap;CHECK(descriptor.size==sizeof(original_heap));
    memcpy(&original_heap,descriptor.memory,sizeof(original_heap));forged_heap=original_heap;
    FixtureCell* stolen=(FixtureCell*)((char*)foreign-sizeof(FixtureCell));
    FixtureCell original_cell=*stolen;
    forged_heap.free_list=NULL;forged_heap.allocated=stolen;
    *stolen=(FixtureCell){.size=original_heap.size,.owner=descriptor.memory};
    memcpy(descriptor.memory,&forged_heap,sizeof(forged_heap));
    u32 forged_written=999;memset(os_bad,0x5A,sizeof(os_bad));
    CHECK(!OSSaveHeapSnapshot(os_handle,os_bad,sizeof(os_bad),&forged_written));
    CHECK(forged_written==999 && os_bad[0]==0x5A);
    memcpy(descriptor.memory,&original_heap,sizeof(original_heap));*stolen=original_cell;
    CHECK(OSCheckHeap(os_handle)==budget && OSCheckHeap(os_other)>0);
    u32 written=999;
    CHECK(!OSSaveHeapSnapshot(os_handle,a,sizeof(os_saved),&written));
    CHECK(!OSSaveHeapSnapshot(os_handle,foreign,sizeof(os_saved),&written));
    CHECK(!OSSaveHeapSnapshot(os_handle,os_saved,sizeof(os_saved),(u32*)a));
    CHECK(!OSSaveHeapSnapshot(os_handle,os_saved,sizeof(os_saved),(u32*)os_saved));
    CHECK(OSSaveHeapSnapshot(os_handle,os_saved,sizeof(os_saved),&os_bytes));
    CHECK(!OSSaveHeapSnapshot(os_handle,os_bad,os_bytes-1,&written)&&written==999);
    OSHeapSnapshotHeader header;memcpy(&header,os_saved,sizeof(header));CHECK(header.cells==4);

    static NativeMatchArena copy_arena;
    static NativeMatchArenaFrame copy_frame;
    static unsigned char copy_arena_bytes[64] ATTRIBUTE_ALIGN(64),copy_bytes[65536];
    CHECK(nativeMatchArena_Begin(&copy_arena,copy_arena_bytes,sizeof(copy_arena_bytes),123));
    HSD_SetHeap(os_handle);
    CHECK(nativeMatchOsHeap_AdmitHistory(&copy_arena));
    CHECK(nativeMatchArena_BindFrame(&copy_arena,&copy_frame,copy_bytes,sizeof(copy_bytes)));
    CHECK(nativeMatchArena_Save(&copy_frame));
    CHECK(!OSLockHeapForHistory(os_other));
    for(unsigned repeat=0;repeat<2;++repeat) {
        // No SDK/allocator method runs while its real control bytes are corrupt.
        for(unsigned i=0;i<copy_arena.root_count;++i)
            memset(copy_arena.roots[i].address,0xA5,copy_frame.root_bytes[i]);
        CHECK(nativeMatchArena_Load(&copy_frame));
        u32 copied;
        CHECK(OSSaveHeapSnapshot(os_handle,os_before,sizeof(os_before),&copied));
        CHECK(copied==os_bytes && !memcmp(os_saved,os_before,copied));
    }
    copy_arena.lifetime=0;nativeMatchOsHeap_Retire();
    CHECK(!OSUnlockHeapForHistory(os_handle));
    CHECK(OSLockHeapForHistory(os_handle) && OSUnlockHeapForHistory(os_handle));

    void* reused=OSAllocFromHeap(os_handle,96);CHECK(reused==b);
    void* grown=OSAllocFromHeap(os_handle,160);CHECK(grown);
    OSFreeToHeap(os_handle,a);OSFreeToHeap(os_handle,c); // Coalesce/reorder current topology.
    OSSetCurrentHeap(os_other);memset(reused,0x59,96);
    memcpy(os_bad,os_saved,os_bytes);reject_os(os_bytes-1);
    memcpy(os_bad,os_saved,os_bytes);os_bad[os_bytes]=0;reject_os(os_bytes+1);
    OSHeapSnapshotHeader h=header;++h.epoch;
    memcpy(os_bad,os_saved,os_bytes);memcpy(os_bad,&h,sizeof(h));reject_os(os_bytes);
    h=header;h.current_heap=7;
    memcpy(os_bad,os_saved,os_bytes);memcpy(os_bad,&h,sizeof(h));reject_os(os_bytes);
    h=header;h.heap=os_other;
    memcpy(os_bad,os_saved,os_bytes);memcpy(os_bad,&h,sizeof(h));reject_os(os_bytes);
    h=header;h.allocated_head=(char*)header.allocated_head+32;
    memcpy(os_bad,os_saved,os_bytes);memcpy(os_bad,&h,sizeof(h));reject_os(os_bytes);
    size_t last=sizeof(header)+(header.cells-1)*sizeof(OSHeapSnapshotCell);
    OSHeapSnapshotCell record;FixtureCell state;
    memcpy(&record,os_saved+last,sizeof(record));record.address=(unsigned char*)foreign-32;
    memcpy(os_bad,os_saved,os_bytes);memcpy(os_bad+last,&record,sizeof(record));reject_os(os_bytes);
    memcpy(&record,os_saved+last,sizeof(record));memcpy(&state,record.state,sizeof(state));state.size-=32;
    memcpy(record.state,&state,sizeof(state));memcpy(os_bad,os_saved,os_bytes);memcpy(os_bad+last,&record,sizeof(record));reject_os(os_bytes);
    memcpy(&record,os_saved+last,sizeof(record));memcpy(&state,record.state,sizeof(state));state.next=record.address;
    memcpy(record.state,&state,sizeof(state));memcpy(os_bad,os_saved,os_bytes);memcpy(os_bad+last,&record,sizeof(record));reject_os(os_bytes);
    memcpy(&record,os_saved+last,sizeof(record));memcpy(&state,record.state,sizeof(state));state.owner=header.descriptor;
    memcpy(record.state,&state,sizeof(state));memcpy(os_bad,os_saved,os_bytes);memcpy(os_bad+last,&record,sizeof(record));reject_os(os_bytes);
    CHECK(OSLoadHeapSnapshot(os_handle,os_saved,os_bytes));
    CHECK(__OSCurrHeap==os_handle&&OSCheckHeap(os_handle)==budget);
    CHECK(((unsigned char*)reused)[95]==0x59); // Payload deliberately NOT restored.
    CHECK(OSReferentSize(a)==96&&OSReferentSize(c)==96);
    CHECK(OSSaveHeapSnapshot(os_handle,os_after,sizeof(os_after),&written));
    CHECK(written==os_bytes&&!memcmp(os_after,os_saved,written));
    CHECK(OSAllocFromHeap(os_handle,96)==reused);
    CHECK(OSAllocFromHeap(os_handle,160)==grown);
    OSDestroyHeap(os_other);CHECK(!OSLoadHeapSnapshot(os_handle,os_saved,os_bytes));
    OSDestroyHeap(os_handle);
    os_handle=OSCreateHeap(start,os_space+sizeof(os_space)/2);CHECK(os_handle>=0);
    CHECK(!OSLoadHeapSnapshot(os_handle,os_saved,os_bytes));
    // Freeze geometry after Add and Fixed carve, including two disjoint ranges.
    OSAddToHeap(os_handle,os_space+sizeof(os_space)/2,os_space+sizeof(os_space));
    CHECK(OSSaveHeapSnapshot(os_handle,os_saved,sizeof(os_saved),&written));
    CHECK(OSAllocFixed(os_space+4096,os_space+8192));
    CHECK(!OSLoadHeapSnapshot(os_handle,os_saved,written));
    CHECK(OSSaveHeapSnapshot(os_handle,os_saved,sizeof(os_saved),&written));
    CHECK(OSLoadHeapSnapshot(os_handle,os_saved,written));
    CHECK(OSCheckHeap(os_handle)>0);
    uint64_t epoch = OSHeapSnapshotEpoch();CHECK(epoch);
    CHECK(OSPrepareHeapSnapshotOwnership());
    CHECK(OSHeapSnapshotOwnsRange(os_handle,os_space+1024,128));
    CHECK(!OSHeapSnapshotOwnsRange(os_handle,os_space+4096,128));
    CHECK(!OSHeapSnapshotOwnsRange(os_handle,os_space+8190,16));
    CHECK(!OSHeapSnapshotOwnsRange(os_handle,os_space+sizeof(os_space)-32,64));
    CHECK(!OSHeapSnapshotOwnsRange(os_handle,os_saved,16));
    CHECK(!OSHeapSnapshotOwnsRange(os_handle,(void*)(UINTPTR_MAX-4),16));
    CHECK(!OSHeapSnapshotBufferIndependent(os_space+1024,16));
    CHECK(OSHeapSnapshotBufferIndependent(os_saved,sizeof(os_saved)));
    void* payload=OSAllocFromHeap(os_handle,400);CHECK(payload);
    CHECK(OSHeapSnapshotOwnsRange(os_handle,payload,400));
    CHECK(OSHeapSnapshotOwnsAllocation(os_handle,payload,400));
    CHECK(!OSHeapSnapshotOwnsAllocation(os_handle,payload,800));
    CHECK(!OSHeapSnapshotOwnsAllocation(os_handle,(char*)payload+1,399));
    OSFreeToHeap(os_handle,payload);
    CHECK(OSHeapSnapshotOwnsRange(os_handle,payload,400));
    CHECK(!OSHeapSnapshotOwnsAllocation(os_handle,payload,400));
    CHECK(OSHeapSnapshotEpoch()==epoch);
    // Candidate validation uses the immutable admitted heap geometry, not the
    // current allocation chain. It can restore a valid older snapshot even
    // when the diverged live chain is corrupt; saved links are still checked.
    payload=OSAllocFromHeap(os_handle,400);CHECK(payload);
    FixtureCell* corrupt=(FixtureCell*)((char*)payload-sizeof(FixtureCell));
    corrupt->next=corrupt;
    CHECK(OSValidateHeapSnapshot(os_handle,os_saved,written));
    CHECK(OSLoadHeapSnapshot(os_handle,os_saved,written));
    CHECK(OSCheckHeap(os_handle)>0);
    // Capacity guard is real: allocate more minimum cells than fixed scratch.
    start=OSInitAlloc(os_space,os_space+sizeof(os_space),1);CHECK(start);
    os_handle=OSCreateHeap(start,os_space+sizeof(os_space));CHECK(os_handle>=0);
    for(unsigned i=0;i<OS_HEAP_SNAPSHOT_MAX_CELLS-1;++i)CHECK(OSAllocFromHeap(os_handle,32));
    CHECK(OSSaveHeapSnapshot(os_handle,os_limit_saved,sizeof(os_limit_saved),&written));
    OSHeapSnapshotHeader limit_header;memcpy(&limit_header,os_limit_saved,sizeof(limit_header));
    CHECK(limit_header.cells==OS_HEAP_SNAPSHOT_MAX_CELLS);
    CHECK(OSValidateHeapSnapshot(os_handle,os_limit_saved,written));
    CHECK(OSLoadHeapSnapshot(os_handle,os_limit_saved,written));
    CHECK(OSAllocFromHeap(os_handle,32));
    CHECK(!OSHeapSnapshotBytes(os_handle));
    CHECK(!OSSaveHeapSnapshot(os_handle,os_saved,sizeof(os_saved),&written));
    // Shrink a maximum-size chain without retiring its geometry. A pointer
    // indexed by the previous candidate must not resolve in the new candidate.
    // The extra newest allocation is before this saved head in the live list.
    OSAllocStateRegion limit_region;CHECK(OSGetHeapStateRegion(os_handle,&limit_region));
    FixtureHeap limit_heap;
    CHECK(limit_region.size==sizeof(limit_heap));memcpy(&limit_heap,limit_region.memory,sizeof(limit_heap));
    FixtureCell* current=limit_heap.allocated;
    while(current->next) {
        FixtureCell* next=current->next;
        OSFreeToHeap(os_handle,(char*)current+sizeof(*current));current=next;
    }
    CHECK(OSSaveHeapSnapshot(os_handle,os_saved,sizeof(os_saved),&os_bytes));
    OSHeapSnapshotHeader small;memcpy(&small,os_saved,sizeof(small));CHECK(small.cells==2);
    CHECK(OSValidateHeapSnapshot(os_handle,os_saved,os_bytes));
    small.allocated_head=limit_header.allocated_head;
    memcpy(os_bad,os_saved,os_bytes);memcpy(os_bad,&small,sizeof(small));reject_os(os_bytes);
    OSDestroyHeap(os_handle);CHECK(!OSHeapSnapshotBytes(os_handle));
    os_checks=checks-first;
}
