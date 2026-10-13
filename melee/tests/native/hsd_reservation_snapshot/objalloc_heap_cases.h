static unsigned heap_object_checks,heap_object_rejections;
static size_t heap_object_bytes;
static unsigned char objects_saved[32768],objects_bad[32768];
static unsigned char objects_before[32768],objects_after[32768];
static unsigned char objects_heap_before[sizeof(heap)],objects_heap_control[32768];

static void reject_heap_objects(size_t bytes)
{
    size_t before,after;
    CHECK(nativeObjAlloc_Save(objects_before,sizeof(objects_before),&before));
    memcpy(objects_heap_before,heap,sizeof(heap));
    CHECK(!nativeObjAlloc_ValidateSnapshot(objects_bad,bytes));
    CHECK(!nativeObjAlloc_Load(objects_bad,bytes));
    CHECK(!memcmp(objects_heap_before,heap,sizeof(heap)));
    CHECK(nativeObjAlloc_Save(objects_after,sizeof(objects_after),&after));
    CHECK(before==after&&!memcmp(objects_before,objects_after,before));
    ++heap_object_rejections;
}

static void test_heap_objects(void)
{
    unsigned first=checks;
    _HSD_ObjAllocForgetMemory(NULL,NULL);HSD_ObjSetHeap(0,NULL);
    HSD_ObjAllocInit(&satellite,24,16);HSD_ObjAllocInit(&other_descriptor,56,32);
    CHECK(HSD_ObjAllocAddFree(&satellite,2)==2&&HSD_ObjAllocAddFree(&other_descriptor,3)==3);
    void* a=HSD_ObjAlloc(&satellite);void* b=HSD_ObjAlloc(&other_descriptor);CHECK(a&&b);
    NativeObjAllocReference a_ref,b_ref,bad_ref,added_ref;
    CHECK((a_ref=nativeObjAlloc_Reference(&satellite,a,24)).epoch);
    CHECK((b_ref=nativeObjAlloc_Reference(&other_descriptor,b,56)).epoch);
    CHECK(nativeObjAlloc_Resolve(a_ref)==a && nativeObjAlloc_Resolve(b_ref)==b);
    bad_ref=a_ref;++bad_ref.epoch;CHECK(!nativeObjAlloc_Resolve(bad_ref));
    bad_ref=a_ref;bad_ref.block=UINT32_MAX;CHECK(!nativeObjAlloc_Resolve(bad_ref));
    bad_ref=a_ref;bad_ref.slot=UINT32_MAX;CHECK(!nativeObjAlloc_Resolve(bad_ref));
    bad_ref=a_ref;bad_ref.allocator=&other_descriptor;CHECK(!nativeObjAlloc_Resolve(bad_ref));
    bad_ref=a_ref;bad_ref.bytes=0;CHECK(!nativeObjAlloc_Resolve(bad_ref));
    bad_ref=a_ref;bad_ref.bytes=satellite.size+1;CHECK(!nativeObjAlloc_Resolve(bad_ref));
    CHECK(!(bad_ref=nativeObjAlloc_Reference(&satellite,(u8*)a+1,1)).epoch);
    CHECK(!(bad_ref=nativeObjAlloc_Reference((void*)1,a,24)).epoch);
    CHECK(!(bad_ref=nativeObjAlloc_Reference(&satellite,a,satellite.size+1)).epoch);
    CHECK(!nativeObjAlloc_Reference(&satellite,(void*)(UINTPTR_MAX-4),8).epoch);
    memset(a,0x37,satellite.size);memset(b,0x48,other_descriptor.size);
    NativeHsdDemand demand={&satellite,satellite.size,4};
    CHECK(nativeHsdReservation_Bind(&owner,backing,sizeof(backing),&demand,1,92));
    CHECK(nativeHsdReservation_Begin(&owner,&demand,1));
    CHECK(!(bad_ref=nativeObjAlloc_Reference(&satellite,a,24)).epoch);
    CHECK(!nativeObjAlloc_Resolve(a_ref));
    void* c=HSD_ObjAlloc(&satellite);CHECK(c);memset(c,0x59,satellite.size);
    CHECK(nativeHsdReservation_End(&owner));
    CHECK(!(bad_ref=nativeObjAlloc_Reference(&satellite,c,24)).epoch);
    size_t reserved_bytes;CHECK(nativeHsdReservation_Save(snapshot_data,sizeof(snapshot_data),&reserved_bytes));
    heap_object_bytes=nativeObjAlloc_SnapshotBytes();CHECK(heap_object_bytes);
    size_t written;CHECK(nativeObjAlloc_Save(objects_saved,sizeof(objects_saved),&written)&&written==heap_object_bytes);
    NativeObjAllocSnapshotHeader header;memcpy(&header,objects_saved,sizeof(header));
    CHECK(header.blocks==2&&header.count==2&&header.heap.top==0);
    CHECK(!nativeObjAlloc_Save(a,32768,&written));
    CHECK(!nativeObjAlloc_Save(objects_bad,sizeof(objects_bad),(size_t*)a));
    u32 heap_written;
    CHECK(OSSaveHeapSnapshot(HSD_GetHeap(),objects_heap_control,sizeof(objects_heap_control),&heap_written));
    long budget=OSCheckHeap(HSD_GetHeap());
    void* next=HSD_ObjAlloc(&other_descriptor);CHECK(next);
    // New chunks exercise OS growth and physical-directory prefix restore.
    CHECK(HSD_ObjAllocAddFree(&other_descriptor,4)==4);
    void* added=HSD_ObjAlloc(&other_descriptor);CHECK(added);
    CHECK((added_ref=nativeObjAlloc_Reference(&other_descriptor,added,56)).epoch);
    CHECK(nativeObjAlloc_Resolve(added_ref)==added);
    HSD_ObjFree(&satellite,a);HSD_ObjFree(&satellite,c);
    // A physical slot reference remains resolvable after free. It deliberately
    // does not claim allocation liveness or invent a per-object generation.
    CHECK(nativeObjAlloc_Resolve(a_ref)==a);
    memset(b,0x6a,other_descriptor.size);memset(next,0x7b,other_descriptor.size);

    memcpy(objects_bad,objects_saved,heap_object_bytes);reject_heap_objects(heap_object_bytes-1);
    memcpy(objects_bad,objects_saved,heap_object_bytes);objects_bad[heap_object_bytes]=0;reject_heap_objects(heap_object_bytes+1);
    NativeObjAllocSnapshotHeader h=header;++h.blocks;
    memcpy(objects_bad,objects_saved,heap_object_bytes);memcpy(objects_bad,&h,sizeof(h));reject_heap_objects(heap_object_bytes);
    h=header;h.arena_used=0;
    memcpy(objects_bad,objects_saved,heap_object_bytes);memcpy(objects_bad,&h,sizeof(h));reject_heap_objects(heap_object_bytes);
    size_t last=sizeof(header)+header.count*sizeof(NativeObjAllocSnapshotRecord);
    NativeObjAllocSnapshotBlock block;memcpy(&block,objects_saved+last,sizeof(block));
    last+=sizeof(block)+block.bytes;memcpy(&block,objects_saved+last,sizeof(block));
    ++block.bytes;memcpy(objects_bad,objects_saved,heap_object_bytes);memcpy(objects_bad+last,&block,sizeof(block));reject_heap_objects(heap_object_bytes);
    memcpy(&block,objects_saved+last,sizeof(block));block.address=a;
    memcpy(objects_bad,objects_saved,heap_object_bytes);memcpy(objects_bad+last,&block,sizeof(block));reject_heap_objects(heap_object_bytes);
    memcpy(&block,objects_saved+last,sizeof(block));block.allocator=&satellite;
    memcpy(objects_bad,objects_saved,heap_object_bytes);memcpy(objects_bad+last,&block,sizeof(block));reject_heap_objects(heap_object_bytes);

    CHECK(nativeObjAlloc_ValidateSnapshot(objects_saved,heap_object_bytes));
    CHECK(nativeHsdReservation_ValidateSnapshot(snapshot_data,reserved_bytes));
    // Actual OS metadata component restores headers only. Deliberately damage
    // payloads afterwards so only the object serializer can repair them.
    CHECK(OSLoadHeapSnapshot(HSD_GetHeap(),objects_heap_control,heap_written));
    memset(a,0x91,satellite.size);memset(b,0xa2,other_descriptor.size);memset(next,0xb3,other_descriptor.size);
    CHECK(nativeObjAlloc_Load(objects_saved,heap_object_bytes));
    CHECK(nativeObjAlloc_Resolve(a_ref)==a && nativeObjAlloc_Resolve(b_ref)==b);
    CHECK(!nativeObjAlloc_Resolve(added_ref));
    CHECK(nativeHsdReservation_Load(snapshot_data,reserved_bytes));
    CHECK(((unsigned char*)a)[satellite.size-1]==0x37&&((unsigned char*)b)[other_descriptor.size-1]==0x48);
    CHECK(((unsigned char*)c)[satellite.size-1]==0x59&&satellite.used==2&&other_descriptor.used==1);
    CHECK(OSCheckHeap(HSD_GetHeap())==budget);
    CHECK(nativeObjAlloc_Save(objects_bad,sizeof(objects_bad),&written));
    CHECK(written==heap_object_bytes&&!memcmp(objects_bad,objects_saved,written));
    CHECK(HSD_ObjAlloc(&other_descriptor)==next);
    CHECK(HSD_ObjAllocAddFree(&other_descriptor,4)==4);
    CHECK(HSD_ObjAlloc(&other_descriptor)==added);
    CHECK(nativeObjAlloc_Resolve(added_ref)==added);
    HSD_ObjFree(&satellite,c);CHECK(nativeHsdReservation_Release(&owner));
    _HSD_ObjAllocForgetMemory(NULL,NULL);
    CHECK(!nativeObjAlloc_Resolve(a_ref));
    CHECK(nativeObjAlloc_SnapshotBytes()==sizeof(header));
    CHECK(!nativeObjAlloc_Load(objects_saved,heap_object_bytes));
    heap_object_checks=checks-first;
}
