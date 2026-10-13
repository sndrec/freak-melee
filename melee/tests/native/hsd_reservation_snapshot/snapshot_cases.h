#include <sysdolphin/baselib/native_hsd_reservation_snapshot.h>
#include <melee/gm/native_match_snapshot_pool.h>

static unsigned snapshot_checks, snapshot_rejections;
static size_t snapshot_packed_bytes;
static unsigned char snapshot_data[8192], snapshot_bad[8192];
static unsigned char snapshot_backing_before[sizeof(backing)];
static unsigned char snapshot_second_before[sizeof(second_backing)];

static void snapshot_reject(size_t bytes)
{
    NativeHsdReservation a = owner, b = second;
    HSD_ObjAllocData counter = satellite;
    memcpy(snapshot_backing_before,backing,sizeof(backing));
    memcpy(snapshot_second_before,second_backing,sizeof(second_backing));
    bool claim_failed = nativeHsdReservation_Failed();
    CHECK(!nativeHsdReservation_ValidateSnapshot(snapshot_bad,bytes));
    CHECK(!nativeHsdReservation_Load(snapshot_bad,bytes));
    CHECK(!memcmp(&a,&owner,sizeof(a)) && !memcmp(&b,&second,sizeof(b)));
    CHECK(!memcmp(&counter,&satellite,sizeof(counter)));
    CHECK(!memcmp(backing,snapshot_backing_before,sizeof(backing)));
    CHECK(!memcmp(second_backing,snapshot_second_before,sizeof(second_backing)));
    CHECK(nativeHsdReservation_Failed() == claim_failed);
    ++snapshot_rejections;
}

static size_t snapshot_pool_at(size_t start)
{
    NativeHsdReservationSnapshotOwner r;
    memcpy(&r,snapshot_data+start,sizeof(r));
    start += sizeof(r);
    for (unsigned i = 0; i < r.bins; ++i) {
        NativeHsdReservationSnapshotBin b;
        memcpy(&b,snapshot_data+start,sizeof(b));
        start += sizeof(b)+b.bytes;
    }
    return start;
}

static void test_snapshots(void)
{
    unsigned first_check = checks;
    long heap_before = OSCheckHeap(HSD_GetHeap());
    NativeHsdDemand plan[] = {{NULL,48,4},{&satellite,satellite.size,4}};
    NativeHsdDemand other[] = {{NULL,128,3},{&satellite,satellite.size,3}};
    CHECK(nativeHsdReservation_Bind(&owner,backing,sizeof(backing),plan,2,71));
    CHECK(nativeHsdReservation_Bind(&second,second_backing,sizeof(second_backing),other,2,72));
    CHECK(nativeHsdReservation_Begin(&owner,plan,2));
    void* a = hsdAllocMemPiece(48);
    void* c = HSD_ObjAlloc(&satellite);
    CHECK(a && c);
    CHECK(nativeHsdReservation_SnapshotBytes() == 0);
    size_t written = 999;
    CHECK(!nativeHsdReservation_Save(snapshot_data,sizeof(snapshot_data),&written) && written == 999);
    CHECK(nativeHsdReservation_End(&owner));
    CHECK(nativeHsdReservation_Begin(&second,other,2));
    void* b = hsdAllocMemPiece(128);
    void* d = HSD_ObjAlloc(&satellite);
    CHECK(b && d && nativeHsdReservation_End(&second));
    memset(a,0x34,64); memset(b,0x56,128);
    memset(c,0x78,satellite.size); memset(d,0x9a,satellite.size);
    NativeMatchPoolReference a_ref, c_ref;
    CHECK(nativeMatchPool_Reference(&owner.bins[0].pool,a,&a_ref));
    CHECK(nativeMatchPool_Reference(&owner.bins[1].pool,c,&c_ref));
    // Leave an ended failed claim to verify that this registry-owned flag saves.
    NativeHsdDemand one = {NULL,64,1};
    CHECK(nativeHsdReservation_Begin(&owner,&one,1));
    CHECK(hsdAllocMemPiece(128) == NULL && !nativeHsdReservation_End(&owner));
    CHECK(nativeHsdReservation_Failed());
    snapshot_packed_bytes = nativeHsdReservation_SnapshotBytes();
    CHECK(snapshot_packed_bytes > 0 && snapshot_packed_bytes < sizeof(snapshot_data));
    CHECK(!nativeHsdReservation_Save(backing,sizeof(backing),&written));
    CHECK(!nativeHsdReservation_Save(snapshot_data,sizeof(snapshot_data),(size_t*)backing));
    CHECK(!nativeHsdReservation_Save(snapshot_data,sizeof(snapshot_data),(size_t*)snapshot_data));
    CHECK(!nativeHsdReservation_Save(snapshot_data,snapshot_packed_bytes-1,&written) && written == 999);
    CHECK(nativeHsdReservation_Save(snapshot_data,sizeof(snapshot_data),&written));
    CHECK(written == snapshot_packed_bytes);
    CHECK(nativeHsdReservation_ValidateSnapshot(snapshot_data,written));
    NativeHsdReservation saved_owner = owner, saved_second = second;
    HSD_ObjAllocData saved_counter = satellite;


    // Private copy-only history retains free order, future allocation identity,
    // source payload bytes and failed-claim state without reconstructing pools.
    static NativeMatchArena arena;
    static NativeMatchArenaFrame frame;
    static unsigned char arena_bytes[64] ATTRIBUTE_ALIGN(64);
    static unsigned char frame_bytes[256*1024];
    CHECK(nativeMatchArena_Begin(&arena,arena_bytes,sizeof(arena_bytes),71));
    CHECK(nativeHsdReservation_AdmitHistory(&arena));
    // Shared descriptor belongs to ObjAlloc's owner; include it in this fixture.
    CHECK(nativeMatchArena_AddRoot(&arena,&satellite,sizeof(satellite)));
    CHECK(nativeMatchArena_BindFrame(&arena,&frame,frame_bytes,sizeof(frame_bytes)));
    CHECK(nativeMatchArena_Save(&frame));
    CHECK(!nativeHsdReservation_Release(&owner));
    NativeHsdReservation extra={0};
    CHECK(!nativeHsdReservation_Bind(&extra,snapshot_bad,sizeof(snapshot_bad),plan,2,73));
    void* future=NULL;
    NativeHsdDemand next_plan[]={{NULL,64,1},{&satellite,satellite.size,1}};
    for(unsigned repeat=0;repeat<2;++repeat) {
        CHECK(nativeHsdReservation_Begin(&owner,next_plan,2));
        void* next=hsdAllocMemPiece(48); CHECK(next);
        if(future)CHECK(next==future); else future=next;
        void* next_obj=HSD_ObjAlloc(&satellite);CHECK(next_obj);
        CHECK(nativeHsdReservation_End(&owner));
        memset(a,0xaa,64);memset(c,0xbb,satellite.size);
        hsdFreeMemPiece(a,48);HSD_ObjFree(&satellite,c);
        CHECK(nativeMatchArena_Load(&frame));
        CHECK(nativeMatchPool_Resolve(a_ref)==a && nativeMatchPool_Resolve(c_ref)==c);
        CHECK(nativeHsdReservation_Save(snapshot_bad,sizeof(snapshot_bad),&written));
        CHECK(written==snapshot_packed_bytes && !memcmp(snapshot_data,snapshot_bad,written));
        CHECK(!memcmp(&satellite,&saved_counter,sizeof(satellite)));
    }
    // Retiring history permits the existing construction/lifetime tests below.
    arena.lifetime=0;

    // Exercise every truncation, including failure after a valid first owner.
    // Scratch may contain a partial candidate; no successful size is published.
    for (size_t capacity = 0; capacity < written; ++capacity) {
        size_t unpublished = SIZE_MAX;
        CHECK(!nativeHsdReservation_Save(snapshot_bad,capacity,&unpublished));
        CHECK(unpublished == SIZE_MAX);
    }
    CHECK(!memcmp(&owner,&saved_owner,sizeof(owner)) && !memcmp(&second,&saved_second,sizeof(second)));
    CHECK(!memcmp(&satellite,&saved_counter,sizeof(satellite)));
    size_t exact;
    CHECK(nativeHsdReservation_Save(snapshot_bad,written,&exact));
    CHECK(exact == written && !memcmp(snapshot_data,snapshot_bad,written));
    CHECK(nativeHsdReservation_ValidateSnapshot(snapshot_bad,exact));
    NativeMatchPool* last = &second.bins[1].pool;
    CHECK(last->free_count >= 2);
    uint32_t order_before = last->free_order[1];
    last->free_order[1] = last->free_order[0];
    exact = SIZE_MAX;
    CHECK(!nativeHsdReservation_Save(snapshot_bad,sizeof(snapshot_bad),&exact));
    CHECK(exact == SIZE_MAX);
    last->free_order[1] = order_before;
    CHECK(!memcmp(&owner,&saved_owner,sizeof(owner)) && !memcmp(&second,&saved_second,sizeof(second)));

    // Same slots, new generation. Other occupied payloads mutate in place.
    hsdFreeMemPiece(a,48); HSD_ObjFree(&satellite,c);
    CHECK(nativeHsdReservation_Begin(&owner,plan,2));
    CHECK(hsdAllocMemPiece(48) == a && HSD_ObjAlloc(&satellite) == c);
    CHECK(nativeHsdReservation_End(&owner));
    CHECK(!nativeMatchPool_Resolve(a_ref) && !nativeMatchPool_Resolve(c_ref));
    memset(a,0xab,64); memset(b,0xbc,128);
    memset(c,0xcd,satellite.size); memset(d,0xde,satellite.size);
    CHECK(!nativeHsdReservation_Failed());

    memcpy(snapshot_bad,snapshot_data,written); snapshot_reject(written-1);
    memcpy(snapshot_bad,snapshot_data,written); snapshot_bad[written]=0; snapshot_reject(written+1);
    NativeHsdReservationSnapshotHeader h;
    memcpy(&h,snapshot_data,sizeof(h));
    h.failed=2; memcpy(snapshot_bad,snapshot_data,written); memcpy(snapshot_bad,&h,sizeof(h)); snapshot_reject(written);
    h.failed=1; --h.owners; memcpy(snapshot_bad,snapshot_data,written); memcpy(snapshot_bad,&h,sizeof(h)); snapshot_reject(written);
    // Invalid final owner must not partially restore the valid first owner.
    size_t later = snapshot_pool_at(sizeof(h));
    NativeHsdReservationSnapshotOwner r;
    memcpy(&r,snapshot_data+later,sizeof(r));
    ++r.slot; memcpy(snapshot_bad,snapshot_data,written); memcpy(snapshot_bad+later,&r,sizeof(r)); snapshot_reject(written);
    size_t first_pool = sizeof(h)+sizeof(r)+sizeof(NativeHsdReservationSnapshotBin);
    NativeMatchPoolSnapshotHeader p;
    memcpy(&p,snapshot_data+first_pool,sizeof(p));
    ++p.lifetime; memcpy(snapshot_bad,snapshot_data,written); memcpy(snapshot_bad+first_pool,&p,sizeof(p)); snapshot_reject(written);
    memcpy(&p,snapshot_data+first_pool,sizeof(p));
    size_t order = first_pool+sizeof(p)+p.capacity*sizeof(uint32_t);
    uint32_t duplicate;
    memcpy(&duplicate,snapshot_data+order,sizeof(duplicate));
    memcpy(snapshot_bad,snapshot_data,written); memcpy(snapshot_bad+order+sizeof(duplicate),&duplicate,sizeof(duplicate)); snapshot_reject(written);
    NativeHsdReservationSnapshotBin bin;
    size_t bin_at = first_pool-sizeof(bin);
    memcpy(&bin,snapshot_data+bin_at,sizeof(bin)); bin.bytes=SIZE_MAX;
    memcpy(snapshot_bad,snapshot_data,written); memcpy(snapshot_bad+bin_at,&bin,sizeof(bin)); snapshot_reject(written);
    CHECK(!nativeHsdReservation_Load(backing,written));
    CHECK(nativeHsdReservation_Begin(&owner,&one,1));
    CHECK(!nativeHsdReservation_Load(snapshot_data,written));
    CHECK(nativeHsdReservation_End(&owner));

    CHECK(nativeHsdReservation_Load(snapshot_data,written));
    CHECK(nativeHsdReservation_Failed());
    CHECK(!memcmp(&owner,&saved_owner,sizeof(owner)) && !memcmp(&second,&saved_second,sizeof(second)));
    CHECK(nativeMatchPool_Resolve(a_ref) == a && nativeMatchPool_Resolve(c_ref) == c);
    CHECK(((unsigned char*)a)[63] == 0x34 && ((unsigned char*)b)[127] == 0x56);
    CHECK(((unsigned char*)c)[satellite.size-1] == 0x78 && ((unsigned char*)d)[satellite.size-1] == 0x9a);
    // This owner does not write shared descriptors. Aggregate restore must also
    // restore ordinary HSD allocations/class counts; here those are unchanged.
    CHECK(!memcmp(&satellite,&saved_counter,sizeof(satellite)));
    size_t repeat;
    CHECK(nativeHsdReservation_Save(snapshot_bad,sizeof(snapshot_bad),&repeat));
    CHECK(repeat == written && !memcmp(snapshot_data,snapshot_bad,repeat));
    hsdFreeMemPiece(a,48); hsdFreeMemPiece(b,128);
    HSD_ObjFree(&satellite,c); HSD_ObjFree(&satellite,d);
    CHECK(nativeHsdReservation_Release(&owner));
    memcpy(snapshot_bad,snapshot_data,written); snapshot_reject(written);
    CHECK(nativeHsdReservation_Bind(&owner,backing,sizeof(backing),plan,2,73));
    memcpy(snapshot_bad,snapshot_data,written); snapshot_reject(written);
    CHECK(nativeHsdReservation_Release(&owner) && nativeHsdReservation_Release(&second));
    CHECK(OSCheckHeap(HSD_GetHeap()) == heap_before);
    CHECK(nativeHsdReservation_SnapshotBytes() == sizeof(h));
    CHECK(nativeHsdReservation_Save(snapshot_bad,sizeof(snapshot_bad),&repeat));
    CHECK(nativeHsdReservation_Load(snapshot_bad,repeat));
    snapshot_checks = checks-first_check;
}
