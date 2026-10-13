#include <sysdolphin/baselib/native_objalloc_snapshot.h>

static unsigned descriptor_checks,descriptor_rejections;
static size_t descriptor_bytes;
static HSD_ObjAllocData other_descriptor,limit_descriptors[NATIVE_OBJALLOC_SNAPSHOT_DESCRIPTORS+1];
static unsigned char ordinary_space[4096] ATTRIBUTE_ALIGN(64);
static unsigned char ordinary_before[sizeof(ordinary_space)];
static unsigned char descriptors_saved[32768],descriptors_bad[32768];
static unsigned char descriptor_current_before[32768],descriptor_current_after[32768];

static void descriptor_reject(size_t bytes)
{
    HSD_ObjAllocData a=satellite,b=other_descriptor;
    size_t before_bytes,after_bytes;
    CHECK(nativeObjAlloc_Save(descriptor_current_before,sizeof(descriptor_current_before),&before_bytes));
    memcpy(ordinary_before,ordinary_space,sizeof(ordinary_space));
    CHECK(!nativeObjAlloc_ValidateSnapshot(descriptors_bad,bytes));
    CHECK(!nativeObjAlloc_Load(descriptors_bad,bytes));
    CHECK(!memcmp(&a,&satellite,sizeof(a))&&!memcmp(&b,&other_descriptor,sizeof(b)));
    CHECK(!memcmp(ordinary_before,ordinary_space,sizeof(ordinary_space)));
    CHECK(nativeObjAlloc_Save(descriptor_current_after,sizeof(descriptor_current_after),&after_bytes));
    CHECK(before_bytes==after_bytes&&!memcmp(descriptor_current_before,descriptor_current_after,before_bytes));
    ++descriptor_rejections;
}

static void test_descriptors(void)
{
    unsigned start_check=checks;
    _HSD_ObjAllocForgetMemory(NULL,NULL);
    HSD_ObjSetHeap(sizeof(ordinary_space),ordinary_space);
    HSD_ObjAllocInit(&satellite,24,16);HSD_ObjAllocInit(&other_descriptor,56,32);
    CHECK(HSD_ObjAllocAddFree(&satellite,2)==2&&HSD_ObjAllocAddFree(&other_descriptor,3)==3);
    void* a=HSD_ObjAlloc(&satellite);void* b=HSD_ObjAlloc(&other_descriptor);CHECK(a&&b);
    memset(a,0x36,satellite.size);memset(b,0x47,other_descriptor.size);
    NativeHsdDemand demand={&satellite,satellite.size,4};
    CHECK(nativeHsdReservation_Bind(&owner,backing,sizeof(backing),&demand,1,81));
    CHECK(nativeHsdReservation_Begin(&owner,&demand,1));
    void* c=HSD_ObjAlloc(&satellite);CHECK(c);memset(c,0x58,satellite.size);
    CHECK(nativeObjAlloc_SnapshotBytes()==0);
    size_t written=999;
    CHECK(!nativeObjAlloc_Save(descriptors_saved,sizeof(descriptors_saved),&written)&&written==999);
    CHECK(nativeHsdReservation_End(&owner));
    NativeMatchPoolReference reference;CHECK(nativeMatchPool_Reference(&owner.bins[0].pool,c,&reference));
    descriptor_bytes=nativeObjAlloc_SnapshotBytes();CHECK(descriptor_bytes);
    CHECK(!nativeObjAlloc_Save(&satellite,sizeof(satellite),&written));
    CHECK(!nativeObjAlloc_Save(ordinary_space,sizeof(ordinary_space),&written));
    CHECK(!nativeObjAlloc_Save(descriptors_saved,sizeof(descriptors_saved),(size_t*)&satellite));
    CHECK(!nativeObjAlloc_Save(descriptors_saved,sizeof(descriptors_saved),(size_t*)descriptors_saved));
    CHECK(!nativeObjAlloc_Save(descriptors_saved,descriptor_bytes-1,&written)&&written==999);
    CHECK(nativeObjAlloc_Save(descriptors_saved,sizeof(descriptors_saved),&written)&&written==descriptor_bytes);
    size_t reserved_bytes;CHECK(nativeHsdReservation_Save(snapshot_data,sizeof(snapshot_data),&reserved_bytes));
    HSD_ObjAllocData expected_a=satellite,expected_b=other_descriptor;
    // Ordinary payload/free-link bytes now belong to this actual component.
    void* expected_next=HSD_ObjAlloc(&other_descriptor);CHECK(expected_next);
    CHECK(nativeObjAlloc_Load(descriptors_saved,written));
    HSD_ObjFree(&satellite,a);HSD_ObjFree(&satellite,c);
    CHECK(nativeHsdReservation_Begin(&owner,&demand,1));
    CHECK(HSD_ObjAlloc(&satellite)==c);CHECK(nativeHsdReservation_End(&owner));
    CHECK(nativeMatchPool_Resolve(reference)==NULL);
    CHECK(HSD_ObjAllocAddFree(&other_descriptor,2)==2); // Cursor and free count change.
    other_descriptor.num_limit=17;other_descriptor.num_limit_flag=1;
    other_descriptor.heap_limit_num=13;other_descriptor.heap_limit_size=112;
    satellite.peak+=3;
    memset(b,0x89,other_descriptor.size);memset(c,0x9a,satellite.size);

    memcpy(descriptors_bad,descriptors_saved,written);descriptor_reject(written-1);
    memcpy(descriptors_bad,descriptors_saved,written);descriptors_bad[written]=0;descriptor_reject(written+1);
    NativeObjAllocSnapshotHeader h;memcpy(&h,descriptors_saved,sizeof(h));
    ++h.epoch;memcpy(descriptors_bad,descriptors_saved,written);memcpy(descriptors_bad,&h,sizeof(h));descriptor_reject(written);
    memcpy(&h,descriptors_saved,sizeof(h));++h.count;
    memcpy(descriptors_bad,descriptors_saved,written);memcpy(descriptors_bad,&h,sizeof(h));descriptor_reject(written);
    memcpy(&h,descriptors_saved,sizeof(h));++h.heap.curr;
    memcpy(descriptors_bad,descriptors_saved,written);memcpy(descriptors_bad,&h,sizeof(h));descriptor_reject(written);
    NativeObjAllocSnapshotRecord r;
    size_t last=sizeof(h)+sizeof(r);memcpy(&r,descriptors_saved+last,sizeof(r));
    r.state.peak=0;memcpy(descriptors_bad,descriptors_saved,written);memcpy(descriptors_bad+last,&r,sizeof(r));descriptor_reject(written);
    memcpy(&r,descriptors_saved+last,sizeof(r));r.descriptor=&other_descriptor;
    memcpy(descriptors_bad,descriptors_saved,written);memcpy(descriptors_bad+last,&r,sizeof(r));descriptor_reject(written);
    memcpy(&r,descriptors_saved+last,sizeof(r));r.state.size*=2;
    memcpy(descriptors_bad,descriptors_saved,written);memcpy(descriptors_bad+last,&r,sizeof(r));descriptor_reject(written);
    memcpy(&r,descriptors_saved+last,sizeof(r));r.state.freehead=NULL;r.state.free=2;
    memcpy(descriptors_bad,descriptors_saved,written);memcpy(descriptors_bad+last,&r,sizeof(r));descriptor_reject(written);
    NativeHsdDemand one={&satellite,satellite.size,1};
    CHECK(nativeHsdReservation_Begin(&owner,&one,1));
    CHECK(!nativeObjAlloc_Load(descriptors_saved,written));CHECK(nativeHsdReservation_End(&owner));

    CHECK(nativeObjAlloc_ValidateSnapshot(descriptors_saved,written));
    CHECK(nativeHsdReservation_ValidateSnapshot(snapshot_data,reserved_bytes));
    CHECK(nativeHsdReservation_Load(snapshot_data,reserved_bytes));
    CHECK(nativeObjAlloc_Load(descriptors_saved,written));
    CHECK(!memcmp(&satellite,&expected_a,sizeof(satellite))&&!memcmp(&other_descriptor,&expected_b,sizeof(other_descriptor)));
    CHECK(nativeMatchPool_Resolve(reference)==c&&((unsigned char*)c)[satellite.size-1]==0x58);
    CHECK(((unsigned char*)a)[satellite.size-1]==0x36&&((unsigned char*)b)[other_descriptor.size-1]==0x47);
    size_t again;CHECK(nativeObjAlloc_Save(descriptors_bad,sizeof(descriptors_bad),&again));
    CHECK(again==written&&!memcmp(descriptors_saved,descriptors_bad,written));
    CHECK(HSD_ObjAlloc(&other_descriptor)==expected_next);
    HSD_ObjFree(&other_descriptor,expected_next);
    HSD_ObjFree(&satellite,a);HSD_ObjFree(&satellite,c);HSD_ObjFree(&other_descriptor,b);
    CHECK(nativeHsdReservation_Release(&owner));
    // Same-address/same-shape reinitialization invalidates old history too.
    HSD_ObjAllocInit(&satellite,24,16);
    memcpy(descriptors_bad,descriptors_saved,written);descriptor_reject(written);
    _HSD_ObjAllocForgetMemory(NULL,NULL);
    for(unsigned i=0;i<NATIVE_OBJALLOC_SNAPSHOT_DESCRIPTORS;++i)HSD_ObjAllocInit(&limit_descriptors[i],16,8);
    CHECK(nativeObjAlloc_SnapshotBytes()>0);
    CHECK(nativeObjAlloc_Save(descriptors_bad,sizeof(descriptors_bad),&again));
    CHECK(nativeObjAlloc_Load(descriptors_bad,again));
    HSD_ObjAllocInit(&limit_descriptors[NATIVE_OBJALLOC_SNAPSHOT_DESCRIPTORS],16,8);
    CHECK(!nativeObjAlloc_SnapshotBytes()&&!nativeObjAlloc_Save(descriptors_bad,sizeof(descriptors_bad),&again));
    _HSD_ObjAllocForgetMemory(NULL,NULL);
    CHECK(nativeObjAlloc_SnapshotBytes()==sizeof(h));
    CHECK(nativeObjAlloc_Save(descriptors_bad,sizeof(descriptors_bad),&again));
    CHECK(nativeObjAlloc_Load(descriptors_bad,again));
    descriptor_checks=checks-start_check;
}
