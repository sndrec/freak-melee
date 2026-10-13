#include <sysdolphin/baselib/native_class_snapshot.h>

static unsigned class_checks, class_rejections;
static size_t class_bytes;
static unsigned char class_saved[131072], class_bad[131072];
static unsigned char class_before[131072], class_after[131072];
static unsigned char class_heap_before[sizeof(heap)], class_heap_control[32768];
static HSD_ClassInfo test_class, later_class;
static const uint8_t* class_candidate_index[NATIVE_CLASS_SNAPSHOT_BLOCKS];
static const uint8_t* class_other_index[NATIVE_CLASS_SNAPSHOT_BLOCKS];

static void class_init_info(void)
{
    hsdInitClassInfo(&test_class,&hsdClass,"fixture","model",sizeof(test_class),96);
}
static void later_init_info(void)
{
    hsdInitClassInfo(&later_class,&hsdClass,"fixture","late",sizeof(later_class),96);
}

static void reject_class(size_t bytes)
{
    size_t before,after;
    CHECK(nativeClass_Save(class_before,sizeof(class_before),&before));
    memcpy(class_heap_before,heap,sizeof(heap));
    CHECK(!nativeClass_ValidateSnapshot(class_bad,bytes));
    CHECK(!nativeClass_Load(class_bad,bytes));
    CHECK(!memcmp(class_heap_before,heap,sizeof(heap)));
    CHECK(nativeClass_Save(class_after,sizeof(class_after),&after));
    CHECK(before==after&&!memcmp(class_before,class_after,before));
    ++class_rejections;
}

static void test_class_storage(void)
{
    unsigned first=checks;
    // Startup has not retired any heap yet: epoch zero is a valid span.
    test_class.head.info_init=class_init_info;
    HSD_Class* startup=hsdNew(&test_class);CHECK(startup);
    NativeClassReference startup_ref=nativeClass_Reference(startup,96);
    CHECK(startup_ref.epoch==0 && startup_ref.bytes==96);
    size_t startup_bytes;
    CHECK(nativeClass_Save(class_saved,sizeof(class_saved),&startup_bytes));
    NativeClassSnapshotView startup_view=nativeClass_ViewSnapshot(class_saved,startup_bytes,class_candidate_index,NATIVE_CLASS_SNAPSHOT_BLOCKS);
    CHECK(startup_view.types && nativeClass_ResolveSnapshot(startup_view,startup_ref)==startup);
    CHECK(nativeClass_SnapshotObjectType(startup_view,startup_ref,&test_class));
    CHECK(nativeClass_Resolve(startup_ref)==startup);
    hsdDelete(startup);
    _hsdClassAmnesia(&hsdClass); // Retire preceding raw-piece fixture records.
    test_class.head.info_init=class_init_info;
    HSD_Class* a=hsdNew(&test_class);HSD_Class* b=hsdNew(&test_class);
    CHECK(a&&b&&test_class.head.nb_exist==2);
    memset((unsigned char*)a+sizeof(*a),0x39,96-sizeof(*a));
    memset((unsigned char*)b+sizeof(*b),0x4a,96-sizeof(*b));
    hsdDelete(b);CHECK(test_class.head.nb_exist==1);
    // A large request exercises pointer-table growth and old-table recycling.
    void* large=hsdAllocMemPiece(2048);CHECK(large);memset(large,0x5b,2048);
    hsdFreeMemPiece(large,2048);
    class_bytes=nativeClass_SnapshotBytes();CHECK(class_bytes);
    size_t written=999;
    CHECK(!nativeClass_Save(a,96,&written));
    CHECK(!nativeClass_Save(class_saved,class_bytes-1,&written)&&written==999);
    CHECK(!nativeClass_Save(class_saved,sizeof(class_saved),(size_t*)a));
    CHECK(!nativeClass_Save(class_saved,sizeof(class_saved),(size_t*)class_saved));
    CHECK(nativeClass_Save(class_saved,sizeof(class_saved),&written)&&written==class_bytes);
    CHECK(nativeClass_ValidateSnapshot(class_saved,written));
    // Backing references prove geometry independently of type-tree validity.
    // A malformed live counter must still prevent publishing a class snapshot.
    u32 peak_before = test_class.head.nb_peak;
    test_class.head.nb_peak = 0;
    CHECK(test_class.head.nb_exist > 0);
    size_t unpublished = SIZE_MAX;
    CHECK(!nativeClass_Save(class_bad,sizeof(class_bad),&unpublished));
    CHECK(unpublished == SIZE_MAX);
    CHECK(!nativeClass_TypeKnown(&test_class,96));
    NativeClassReference geometry = nativeClass_Reference(a,96);
    CHECK(geometry.bytes == 96 && nativeClass_Resolve(geometry) == a);
    test_class.head.nb_peak = peak_before;
    CHECK(nativeClass_TypeKnown(&test_class,96));
    NativeClassReference reference, unchanged;
    CHECK((reference=nativeClass_Reference(a,96)).bytes);
    CHECK(nativeClass_Resolve(reference)==a);
    CHECK((unchanged=nativeClass_Reference((u8*)a+8,16)).bytes);
    CHECK(nativeClass_Resolve(unchanged)==(u8*)a+8);
    unchanged=reference;
    CHECK(!(unchanged=nativeClass_Reference(NULL,96)).bytes);
    CHECK(!(unchanged=nativeClass_Reference((void*)1,96)).bytes);
    CHECK(!(unchanged=nativeClass_Reference(a,0)).bytes);
    CHECK(!nativeClass_Reference(a,SIZE_MAX).bytes);
    CHECK(!nativeClass_Reference((void*)(UINTPTR_MAX-4),8).bytes);
    NativeClassReference zero={0};CHECK(!memcmp(&zero,&unchanged,sizeof(zero)));
    unchanged=reference;++unchanged.epoch;CHECK(!nativeClass_Resolve(unchanged));
    unchanged=reference;unchanged.block=UINT32_MAX;CHECK(!nativeClass_Resolve(unchanged));
    unchanged=reference;unchanged.reserved=1;CHECK(!nativeClass_Resolve(unchanged));
    unchanged=reference;unchanged.offset=SIZE_MAX;CHECK(!nativeClass_Resolve(unchanged));
    unchanged=reference;unchanged.bytes=SIZE_MAX;CHECK(!nativeClass_Resolve(unchanged));
    unchanged=reference;unchanged.bytes=0;CHECK(!nativeClass_Resolve(unchanged));
    NativeClassSnapshotHeader header;memcpy(&header,class_saved,sizeof(header));
    CHECK(header.entries==64&&header.types==2&&header.blocks>=6);
    CHECK(!nativeClass_Save(header.memory_list,class_bytes,&written));
    u32 heap_written;
    CHECK(OSSaveHeapSnapshot(HSD_GetHeap(),class_heap_control,sizeof(class_heap_control),&heap_written));
    long heap_budget=OSCheckHeap(HSD_GetHeap());
    HSD_Class* next=hsdNew(&test_class);CHECK(next==b);
    void* split=hsdAllocMemPiece(1024);CHECK(split==large);
    // This allocation grows the OS heap and size-class table after the save.
    void* added=hsdAllocMemPiece(8192);CHECK(added);
    memset(a,0x6c,96);memset(next,0x7d,96);memset(split,0x8e,1024);
    // The original candidate owns only its original block prefix. Current raw
    // bytes and later allocations must not supply a saved type or membership.
    NativeClassSnapshotView view=nativeClass_ViewSnapshot(class_saved,class_bytes,class_candidate_index,NATIVE_CLASS_SNAPSHOT_BLOCKS);
    CHECK(view.types && view.header.blocks==header.blocks);
    const void* saved_a=nativeClass_ReadSnapshot(view,reference);CHECK(saved_a);
    HSD_Class saved_header;memcpy(&saved_header,saved_a,sizeof(saved_header));
    CHECK(saved_header.class_info==&test_class && ((const uint8_t*)saved_a)[95]==0x39);
    CHECK(nativeClass_ResolveSnapshot(view,reference)==a);
    CHECK(nativeClass_SnapshotObjectType(view,reference,&test_class));
    CHECK(nativeClass_SnapshotTypeKnown(view,&test_class,96));
    CHECK(!nativeClass_SnapshotTypeKnown(view,(void*)1,96));
    CHECK(!nativeClass_SnapshotTypeKnown(view,&test_class,97));
    CHECK(!nativeClass_SnapshotObjectType(view,reference,(void*)1));
    NativeClassReference newer=nativeClass_Reference(added,8192);CHECK(newer.bytes && nativeClass_Resolve(newer)==added);
    CHECK(!nativeClass_ReadSnapshot(view,newer) && !nativeClass_ResolveSnapshot(view,newer));
    NativeClassReference wrong=reference;wrong.offset=SIZE_MAX;CHECK(!nativeClass_ReadSnapshot(view,wrong));
    wrong=reference;wrong.bytes=SIZE_MAX;CHECK(!nativeClass_ReadSnapshot(view,wrong));
    wrong=reference;++wrong.epoch;CHECK(!nativeClass_ReadSnapshot(view,wrong));
    CHECK(!nativeClass_ViewSnapshot(class_saved,class_bytes,class_other_index,1).types);
    CHECK(!nativeClass_ViewSnapshot(class_saved,class_bytes,(const uint8_t**)a,NATIVE_CLASS_SNAPSHOT_BLOCKS).types);
    CHECK(!nativeClass_ViewSnapshot(class_saved,class_bytes,(const uint8_t**)class_saved,NATIVE_CLASS_SNAPSHOT_BLOCKS).types);
    size_t current_bytes;CHECK(nativeClass_Save(class_after,sizeof(class_after),&current_bytes));
    NativeClassSnapshotView current=nativeClass_ViewSnapshot(class_after,current_bytes,class_other_index,NATIVE_CLASS_SNAPSHOT_BLOCKS);
    CHECK(current.types && nativeClass_ReadSnapshot(current,newer));
    CHECK(!nativeClass_SnapshotObjectType(current,reference,&test_class));
    CHECK(nativeClass_SnapshotObjectType(view,reference,&test_class)); // Independent indexes coexist.

    CHECK(test_class.head.nb_exist==2);

    memcpy(class_bad,class_saved,class_bytes);reject_class(class_bytes-1);
    memcpy(class_bad,class_saved,class_bytes);class_bad[class_bytes]=0;reject_class(class_bytes+1);
    NativeClassSnapshotHeader h=header;
    ++h.epoch;memcpy(class_bad,class_saved,class_bytes);memcpy(class_bad,&h,sizeof(h));reject_class(class_bytes);
    h=header;++h.blocks;memcpy(class_bad,class_saved,class_bytes);memcpy(class_bad,&h,sizeof(h));reject_class(class_bytes);
    h=header;++h.types;memcpy(class_bad,class_saved,class_bytes);memcpy(class_bad,&h,sizeof(h));reject_class(class_bytes);
    h=header;h.arena_used=64;memcpy(class_bad,class_saved,class_bytes);memcpy(class_bad,&h,sizeof(h));reject_class(class_bytes);
    h=header;h.arena_used=SIZE_MAX;memcpy(class_bad,class_saved,class_bytes);memcpy(class_bad,&h,sizeof(h));reject_class(class_bytes);
    h=header;h.memory_list=(HSD_MemoryEntry**)a;memcpy(class_bad,class_saved,class_bytes);memcpy(class_bad,&h,sizeof(h));reject_class(class_bytes);
    h=header;++h.entries;memcpy(class_bad,class_saved,class_bytes);memcpy(class_bad,&h,sizeof(h));reject_class(class_bytes);
    size_t type_at=sizeof(header)+sizeof(NativeClassSnapshotType);
    NativeClassSnapshotType type;memcpy(&type,class_saved+type_at,sizeof(type));
    type.state.nb_peak=0;type.state.nb_exist=1;
    memcpy(class_bad,class_saved,class_bytes);memcpy(class_bad+type_at,&type,sizeof(type));reject_class(class_bytes);
    memcpy(&type,class_saved+type_at,sizeof(type));type.info=&later_class;
    memcpy(class_bad,class_saved,class_bytes);memcpy(class_bad+type_at,&type,sizeof(type));reject_class(class_bytes);
    size_t last=sizeof(header)+header.types*sizeof(type);
    NativeClassSnapshotBlock block;
    for(unsigned i=0;i+1<header.blocks;++i){memcpy(&block,class_saved+last,sizeof(block));last+=sizeof(block)+block.bytes;}
    memcpy(&block,class_saved+last,sizeof(block));++block.bytes;
    memcpy(class_bad,class_saved,class_bytes);memcpy(class_bad+last,&block,sizeof(block));reject_class(class_bytes);
    memcpy(&block,class_saved+last,sizeof(block));block.address=a;
    memcpy(class_bad,class_saved,class_bytes);memcpy(class_bad+last,&block,sizeof(block));reject_class(class_bytes);

    CHECK(nativeClass_ValidateSnapshot(class_saved,class_bytes));
    // Actual OS component restores only Cell headers/descriptors. Actual class
    // owner restores every owned payload, free link and counter.
    CHECK(OSLoadHeapSnapshot(HSD_GetHeap(),class_heap_control,heap_written));
    memset(a,0x91,96);memset(b,0xa2,96);memset(large,0xb3,2048);
    CHECK(nativeClass_Load(class_saved,class_bytes));
    CHECK(a->class_info==&test_class&&((unsigned char*)a)[95]==0x39);
    CHECK(((unsigned char*)b)[95]==0x4a&&((unsigned char*)large)[2047]==0x5b);
    CHECK(test_class.head.nb_exist==1&&test_class.head.nb_peak==2);
    CHECK(OSCheckHeap(HSD_GetHeap())==heap_budget);
    CHECK(nativeClass_Save(class_bad,sizeof(class_bad),&written));
    CHECK(written==class_bytes&&!memcmp(class_saved,class_bad,written));
    CHECK(hsdNew(&test_class)==next);
    CHECK(hsdAllocMemPiece(1024)==split);
    CHECK(hsdAllocMemPiece(8192)==added);

    // Late class initialization is a frozen-definition boundary, not a silent
    // partial restore. Aggregate preparation must initialize required classes.
    later_class.head.info_init=later_init_info;CHECK(hsdNew(&later_class));
    memcpy(class_bad,class_saved,class_bytes);reject_class(class_bytes);
    CHECK(nativeClass_Save(class_saved,sizeof(class_saved),&written));
    hsdForgetClassLibrary(NULL);
    CHECK(!nativeClass_Resolve(reference));
    CHECK(nativeClass_SnapshotBytes()==sizeof(header));
    CHECK(!nativeClass_Load(class_saved,written));
    CHECK(nativeClass_Save(class_bad,sizeof(class_bad),&written));
    CHECK(nativeClass_Load(class_bad,written));
    CHECK(hsdNew(&test_class));
    CHECK(!nativeClass_Load(class_saved,class_bytes));
    class_checks=checks-first;
}
