#include <melee/gm/native_match_pool.h>
#include <melee/gm/native_match_snapshot_pool.h>
#include <melee/ft/types.h>
#include <melee/it/types.h>
#include <melee/it/itCharItems.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { Slots = 4, Storage = 4 * sizeof(Fighter) + 64, Snapshot = Storage + 1024 };
typedef struct Fixture {
    NativeMatchPool pool;
    _Alignas(16) uint8_t payload[Storage];
    NativeMatchPoolSlot slots[Slots];
    uint32_t order[Slots];
} Fixture;
static Fixture objects, procedures, links;
static uint8_t saved[Snapshot], other[Snapshot], invalid[Snapshot];
static unsigned checks, lifecycles, failures;
static unsigned writer_refusals;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"line%u: %s\n",(unsigned)__LINE__,#x); exit(1); } } while (0)
static unsigned callbacks;
static void callback(HSD_GObj* object) { (void)object; ++callbacks; }
static void destroy(void* data) { (void)data; ++callbacks; }

static void bind(Fixture* fixture, size_t size, uint64_t lifetime)
{
    memset(fixture, 0xA5, sizeof(*fixture));
    CHECK(nativeMatchPool_Bind(&fixture->pool, fixture->payload, sizeof(fixture->payload),
        fixture->slots, fixture->order, Slots, size, 8, lifetime));
    CHECK(nativeMatchPool_Validate(&fixture->pool));
}

static void gobj_lifecycle(void)
{
    bind(&objects, sizeof(HSD_GObj), 10);
    bind(&procedures, sizeof(HSD_GObjProc), 11);
    NativeMatchPoolReference a_ref,b_ref,proc_ref,c_ref;
    HSD_GObj* a = nativeMatchPool_Allocate(&objects.pool,&a_ref);
    HSD_GObj* b = nativeMatchPool_Allocate(&objects.pool,&b_ref);
    HSD_GObjProc* proc = nativeMatchPool_Allocate(&procedures.pool,&proc_ref);
    CHECK(a&&b&&proc);
    *a=(HSD_GObj){.classifier=7,.p_link=10,.next=b,.proc=proc,.render_cb=NULL,
                  .user_data=a,.user_data_remove_func=destroy,.x34_unk=b};
    *b=(HSD_GObj){.classifier=4,.p_link=6,.prev=a,.user_data=b};
    *proc=(HSD_GObjProc){.gobj=a,.next=proc,.child=proc,.s_link=2,.on_invoke=callback};
    proc->flags_3=2;
    HSD_GObj a_value=*a,b_value=*b;HSD_GObjProc proc_value=*proc;
    size_t bytes,proc_bytes;
    CHECK(nativeMatchSnapshotPool_Save(&objects.pool,saved,sizeof(saved),&bytes));
    CHECK(nativeMatchSnapshotPool_Save(&procedures.pool,other,sizeof(other),&proc_bytes));
    CHECK(nativeMatchPool_Free(&objects.pool,a_ref));
    CHECK(nativeMatchPool_Resolve(a_ref)==NULL);
    HSD_GObj* c=nativeMatchPool_Allocate(&objects.pool,&c_ref);
    CHECK(c==a&&c_ref.generation==a_ref.generation+1);
    for(size_t i=0;i<sizeof(*c);++i)CHECK(((uint8_t*)c)[i]==0);
    c->classifier=0xFFFF;b->prev=c;b->user_data=c;
    proc->on_invoke=NULL;proc->gobj=b;proc->flags_3=1;
    NativeMatchPoolSnapshotView view=nativeMatchSnapshotPool_View(&objects.pool,saved,bytes);
    CHECK(view.records && view.header.used==2);
    CHECK(nativeMatchSnapshotPool_Resolve(view,a_ref)==a);
    CHECK(nativeMatchPool_Resolve(a_ref)==NULL); // Current slot has a new life.
    CHECK(nativeMatchSnapshotPool_Resolve(view,c_ref)==NULL);
    const void* candidate=nativeMatchSnapshotPool_Find(view,a_ref.slot);
    CHECK(candidate && !memcmp(candidate,&a_value,sizeof(a_value)));
    CHECK(c->classifier==0xFFFF); // Candidate lookup never restores live bytes.
    CHECK(!nativeMatchSnapshotPool_Find(view,Slots));
    CHECK(!nativeMatchSnapshotPool_Find(view,Slots-1));
    NativeMatchPoolReference foreign=a_ref;foreign.pool=(void*)1;
    CHECK(!nativeMatchSnapshotPool_Resolve(view,foreign));
    foreign=a_ref;++foreign.lifetime;CHECK(!nativeMatchSnapshotPool_Resolve(view,foreign));
    foreign=a_ref;foreign.slot=UINT32_MAX;CHECK(!nativeMatchSnapshotPool_Resolve(view,foreign));
    NativeMatchPoolSnapshotView rejected=nativeMatchSnapshotPool_View(&objects.pool,saved,bytes-1);
    CHECK(!rejected.records && !nativeMatchSnapshotPool_Find(rejected,0));
    CHECK(!nativeMatchSnapshotPool_Resolve(rejected,a_ref));
    CHECK(nativeMatchSnapshotPool_View(&objects.pool,saved,bytes).records);
    CHECK(nativeMatchSnapshotPool_View(&procedures.pool,other,proc_bytes).records);
    // Both pools validated before either is restored. No callback can run
    // between these loads; central multi-owner atomicity is a later layer.
    CHECK(nativeMatchSnapshotPool_Load(&objects.pool,saved,bytes));
    CHECK(nativeMatchSnapshotPool_Load(&procedures.pool,other,proc_bytes));
    CHECK(memcmp(a,&a_value,sizeof(*a))==0&&memcmp(b,&b_value,sizeof(*b))==0);
    CHECK(memcmp(proc,&proc_value,sizeof(*proc))==0);
    CHECK(a->user_data==a&&a->next==b&&b->prev==a&&a->proc->gobj==a);
    CHECK(a->proc->child==a->proc&&a->proc->next==a->proc&&a->proc->on_invoke==callback);
    CHECK(callbacks==0&&nativeMatchPool_Resolve(a_ref)==a&&nativeMatchPool_Resolve(c_ref)==NULL);
    CHECK(nativeMatchPool_Free(&objects.pool,a_ref));
    NativeMatchPoolReference replay;
    CHECK(nativeMatchPool_Allocate(&objects.pool,&replay)==c);
    CHECK(replay.slot==c_ref.slot&&replay.generation==c_ref.generation);
    objects.pool.lifetime=12; // Retire this binding without touching its backing.
    CHECK(!nativeMatchSnapshotPool_Find(view,a_ref.slot));
    CHECK(!nativeMatchSnapshotPool_Resolve(view,a_ref));
    CHECK(callbacks==0);++lifecycles;
}

static void item_link_chain(void)
{
    bind(&links,sizeof(ItemLink),12);
    NativeMatchPoolReference refs[4];ItemLink* chain[4];
    for(unsigned i=0;i<4;++i){chain[i]=nativeMatchPool_Allocate(&links.pool,&refs[i]);CHECK(chain[i]);}
    for(unsigned i=0;i<4;++i){
        chain[i]->next=i?chain[i-1]:NULL;chain[i]->prev=i<3?chain[i+1]:NULL;
        chain[i]->gobj=(HSD_GObj*)chain[i];chain[i]->x1CC=100+(int)i;
        chain[i]->pos=(Vec3){(float)i,-2,3};chain[i]->vel=(Vec3){1,2,3};
    }
    ItemLink values[4];for(unsigned i=0;i<4;++i)values[i]=*chain[i];
    size_t bytes;CHECK(nativeMatchSnapshotPool_Save(&links.pool,saved,sizeof(saved),&bytes));
    for(unsigned i=0;i<4;++i)CHECK(nativeMatchPool_Free(&links.pool,refs[i]));
    CHECK(nativeMatchPool_Validate(&links.pool));
    CHECK(nativeMatchSnapshotPool_Load(&links.pool,saved,bytes));
    for(unsigned i=0;i<4;++i)CHECK(memcmp(chain[i],&values[i],sizeof(ItemLink))==0);
    ++lifecycles;
}

static void real_actor_state(void)
{
    bind(&objects,sizeof(Fighter),13);bind(&links,sizeof(Item),14);
    NativeMatchPoolReference f_ref,i_ref;
    Fighter* fighter=nativeMatchPool_Allocate(&objects.pool,&f_ref);
    Item* item=nativeMatchPool_Allocate(&links.pool,&i_ref);CHECK(fighter&&item);
    fighter->x8_spawnNum=0x7654321;fighter->motion_id=357;
    fighter->cur_pos=(Vec3){4,5,6};fighter->native_module_state[255]=0xBC;
    item->x1C=0x81234567;item->native_creator_generation=fighter->x8_spawnNum;
    item->xDD4_itemVar.native_article.state[511]=0xCD;
    Fighter expected_f=*fighter;Item expected_i=*item;
    size_t fbytes,ibytes;
    CHECK(nativeMatchSnapshotPool_Save(&objects.pool,saved,sizeof(saved),&fbytes));
    CHECK(nativeMatchSnapshotPool_Save(&links.pool,other,sizeof(other),&ibytes));
    fighter->motion_id=14;fighter->native_module_state[255]=0;
    item->x1C=7;item->xDD4_itemVar.native_article.state[511]=0;
    CHECK(nativeMatchSnapshotPool_Load(&objects.pool,saved,fbytes));
    CHECK(nativeMatchSnapshotPool_Load(&links.pool,other,ibytes));
    CHECK(memcmp(fighter,&expected_f,sizeof(Fighter))==0&&memcmp(item,&expected_i,sizeof(Item))==0);
    ++lifecycles;
}

static void large_class_piece_capacity(void)
{
    // Actual Ness resource-layout class-piece requirement, above the old 16K
    // ceiling. Metadata and free-order offsets remain 32-bit owned indices.
    enum { Count = 47553 };
    static uint32_t payload[Count], order[Count];
    static NativeMatchPoolSlot metadata[Count];
    static NativeMatchPoolReference references[Count];
    NativeMatchPool pool;
    CHECK(nativeMatchPool_Bind(&pool, payload, sizeof(payload), metadata, order,
                              Count, sizeof(uint32_t), _Alignof(uint32_t), 41));
    for (unsigned i = 0; i < Count; ++i) {
        uint32_t* value = nativeMatchPool_Allocate(&pool, &references[i]);
        CHECK(value); *value = i;
    }
    CHECK(pool.used == Count && nativeMatchPool_Validate(&pool));
    NativeMatchPoolReference exhausted;
    CHECK(!nativeMatchPool_Allocate(&pool, &exhausted));
    CHECK(nativeMatchPool_Free(&pool, references[Count - 1]));
    NativeMatchPoolReference replacement;
    uint32_t* value = nativeMatchPool_Allocate(&pool, &replacement);
    CHECK(value == &payload[Count - 1] && *value == 0);
    CHECK(!nativeMatchPool_Resolve(references[Count - 1]));
    CHECK(nativeMatchPool_Free(&pool, replacement));
    for (unsigned i = 0; i < Count - 1; ++i)
        CHECK(nativeMatchPool_Free(&pool, references[i]));
    CHECK(pool.used == 0 && nativeMatchPool_Validate(&pool));
}

static void full_resource_capacity(void)
{
    // Actual declared grapple collection capacity, independent of current WIP
    // constructor acceptance. Exercise packed offsets well past four slots.
    enum { Count = 90 };
    static ItemLink payload[Count];
    static NativeMatchPoolSlot metadata[Count];
    static uint32_t order[Count];
    NativeMatchPool pool;
    NativeMatchPoolReference references[Count];ItemLink* records[Count];
    CHECK(nativeMatchPool_Bind(&pool,payload,sizeof(payload),metadata,order,
                              Count,sizeof(ItemLink),_Alignof(ItemLink),40));
    for(unsigned i=0;i<Count;++i){
        records[i]=nativeMatchPool_Allocate(&pool,&references[i]);CHECK(records[i]);
        records[i]->next=i?records[i-1]:NULL;records[i]->x1CC=(int)i;
        records[i]->pos=(Vec3){(float)i,-(float)i,0};
    }
    size_t bytes;CHECK(nativeMatchSnapshotPool_Save(&pool,saved,sizeof(saved),&bytes));
    for(unsigned i=0;i<Count;++i)CHECK(nativeMatchPool_Free(&pool,references[i]));
    for(unsigned i=0;i<Count;++i){NativeMatchPoolReference reused;ItemLink* value=nativeMatchPool_Allocate(&pool,&reused);CHECK(value==records[Count-1-i]);value->x1CC=-1;}
    CHECK(nativeMatchSnapshotPool_Load(&pool,saved,bytes));
    for(unsigned i=0;i<Count;++i){CHECK(nativeMatchPool_Resolve(references[i])==records[i]);CHECK(records[i]->next==(i?records[i-1]:NULL));CHECK(records[i]->x1CC==(int)i);}
    CHECK(nativeMatchPool_Validate(&pool));++lifecycles;
}

static void ordering(void)
{
    // All 24 free orders of four actual GObj slots, saved at all five states.
    for(unsigned a=0;a<4;++a)for(unsigned b=0;b<4;++b)for(unsigned c=0;c<4;++c)for(unsigned d=0;d<4;++d){
        unsigned permutation[]={a,b,c,d};
        if(a==b||a==c||a==d||b==c||b==d||c==d)continue;
        for(unsigned release=0;release<=4;++release){
            bind(&objects,sizeof(HSD_GObj),20);
            NativeMatchPoolReference refs[4],future[4],replay[4];void* ptrs[4];
            for(unsigned i=0;i<4;++i){ptrs[i]=nativeMatchPool_Allocate(&objects.pool,&refs[i]);CHECK(ptrs[i]);}
            for(unsigned i=0;i<release;++i)CHECK(nativeMatchPool_Free(&objects.pool,refs[permutation[i]]));
            size_t bytes;CHECK(nativeMatchSnapshotPool_Save(&objects.pool,saved,sizeof(saved),&bytes));
            CHECK(nativeMatchSnapshotPool_Bytes(&objects.pool)==bytes);
            for(unsigned i=0;i<release;++i){CHECK(nativeMatchPool_Allocate(&objects.pool,&future[i])==ptrs[permutation[release-1-i]]);}
            NativeMatchPoolReference sentinel={0};CHECK(nativeMatchPool_Allocate(&objects.pool,&sentinel)==NULL);
            CHECK(nativeMatchSnapshotPool_Load(&objects.pool,saved,bytes));
            CHECK(nativeMatchPool_Validate(&objects.pool));
            for(unsigned i=0;i<release;++i){
                CHECK(nativeMatchPool_Allocate(&objects.pool,&replay[i])==ptrs[future[i].slot]);
                CHECK(replay[i].slot==future[i].slot&&replay[i].generation==future[i].generation);
            }
            ++lifecycles;
        }
    }
}

static uint32_t get_u32(uint8_t* p){uint32_t value;memcpy(&value,p,4);return value;}
static void put_u32(uint8_t* p,uint32_t value){memcpy(p,&value,4);}
static void rejects(void)
{
    bind(&objects,sizeof(HSD_GObj),30);
    NativeMatchPoolReference a,b;CHECK(nativeMatchPool_Allocate(&objects.pool,&a));CHECK(nativeMatchPool_Allocate(&objects.pool,&b));
    size_t bytes;CHECK(nativeMatchSnapshotPool_Save(&objects.pool,saved,sizeof(saved),&bytes));
    Fixture before=objects;
    NativeMatchPoolSnapshotHeader initial;memcpy(&initial,saved,sizeof(initial));
    for(unsigned fault=0;fault<19;++fault){
        memcpy(invalid,saved,bytes);
        NativeMatchPoolSnapshotHeader h=initial;size_t candidate_bytes=bytes;
        size_t free_start=sizeof(h)+Slots*4;
        size_t active_start=free_start+h.free_count*4;
        if(fault==0)h.pool=&procedures.pool;
        if(fault==1)++h.lifetime;
        if(fault==2)++h.capacity;
        if(fault==3)++h.object_size;
        if(fault==4)++h.stride;
        if(fault==5)h.used=Slots+1;
        if(fault==6)++h.free_count;
        if(fault==7)h.peak=1;
        if(fault==8)h.peak=Slots+1;
        if(fault==9)--candidate_bytes;
        if(fault==10)++candidate_bytes;
        if(fault==11)put_u32(invalid+free_start+4,get_u32(invalid+free_start));
        if(fault==12)put_u32(invalid+free_start,Slots);
        if(fault==13)put_u32(invalid+active_start+4+sizeof(HSD_GObj),get_u32(invalid+active_start));
        if(fault==14)put_u32(invalid+active_start,get_u32(invalid+free_start));
        if(fault==15)put_u32(invalid+sizeof(h),0);
        if(fault==16)h.payload=procedures.payload;
        if(fault==17)h.initialized_count=0;
        if(fault==18)h.initialized_count=Slots+1;
        memcpy(invalid,&h,sizeof(h));
        CHECK(!nativeMatchSnapshotPool_View(&objects.pool,invalid,candidate_bytes).records);
        CHECK(!nativeMatchSnapshotPool_Load(&objects.pool,invalid,candidate_bytes));
        CHECK(memcmp(&objects,&before,sizeof(objects))==0);++failures;
    }
    memset(invalid,0xFE,sizeof(invalid));size_t written=SIZE_MAX;
    CHECK(!nativeMatchSnapshotPool_Save(&objects.pool,invalid,bytes-1,&written));
    CHECK(written==SIZE_MAX);for(size_t i=0;i<sizeof(invalid);++i)CHECK(invalid[i]==0xFE);
    CHECK(!nativeMatchSnapshotPool_Save(&objects.pool,objects.payload,sizeof(objects.payload),&written));
    CHECK(memcmp(&objects,&before,sizeof(objects))==0);
    CHECK(!nativeMatchPool_Free(&objects.pool,(NativeMatchPoolReference){&procedures.pool,30,a.slot,a.generation}));
    CHECK(!nativeMatchPool_Free(&objects.pool,(NativeMatchPoolReference){&objects.pool,30,a.slot,a.generation+1}));
    CHECK(memcmp(&objects,&before,sizeof(objects))==0);
    CHECK(!nativeMatchPool_Bind(&objects.pool,objects.payload,sizeof(objects.payload),objects.slots,objects.order,Slots,sizeof(HSD_GObj),3,31));
    CHECK(!nativeMatchPool_Bind(&objects.pool,objects.payload+1,sizeof(objects.payload)-1,objects.slots,objects.order,Slots,sizeof(HSD_GObj),8,31));
    CHECK(!nativeMatchPool_Bind(&objects.pool,objects.payload,sizeof(objects.payload),objects.slots,(uint32_t*)objects.slots,Slots,sizeof(HSD_GObj),8,31));
    CHECK(memcmp(&objects,&before,sizeof(objects))==0);
    CHECK(nativeMatchPool_Free(&objects.pool,a));before=objects;
    CHECK(!nativeMatchPool_Free(&objects.pool,a));CHECK(memcmp(&objects,&before,sizeof(objects))==0);
    CHECK(nativeMatchSnapshotPool_Load(&objects.pool,saved,bytes));
    CHECK(nativeMatchPool_Bind(&objects.pool,objects.payload,sizeof(objects.payload),objects.slots,objects.order,Slots,sizeof(HSD_GObj),8,31));
    before=objects;CHECK(!nativeMatchSnapshotPool_Load(&objects.pool,saved,bytes));
    CHECK(memcmp(&objects,&before,sizeof(objects))==0&&nativeMatchPool_Resolve(a)==NULL);
    objects.slots[0].generation=UINT32_MAX;before=objects;
    CHECK(nativeMatchPool_Allocate(&objects.pool,&a)==NULL);CHECK(memcmp(&objects,&before,sizeof(objects))==0);
}

static void writer_bounds(void)
{
    bind(&objects,sizeof(HSD_GObj),40);
    NativeMatchPoolReference refs[2];
    CHECK(nativeMatchPool_Allocate(&objects.pool,&refs[0]));
    CHECK(nativeMatchPool_Allocate(&objects.pool,&refs[1]));
    Fixture valid=objects;
    for(unsigned fault=0;fault<8;++fault) {
        objects=valid;
        if(fault==0){objects.pool.used=0;objects.pool.free_count=Slots;}
        if(fault==1){objects.pool.used=3;objects.pool.free_count=1;}
        if(fault==2)objects.slots[refs[1].slot].generation=0;
        if(fault==3)objects.order[1]=objects.order[0];
        if(fault==4)objects.order[0]=Slots;
        if(fault==5)objects.order[0]=refs[0].slot;
        if(fault==6)objects.pool.peak=1;
        if(fault==7)objects.pool.free_count=1;
        Fixture malformed=objects;
        size_t capacity=sizeof(NativeMatchPoolSnapshotHeader)+
            (Slots+objects.pool.free_count)*sizeof(uint32_t)+
            objects.pool.used*(sizeof(uint32_t)+objects.pool.object_size);
        CHECK(capacity<sizeof(invalid));
        memset(invalid,0xCD,sizeof(invalid));
        size_t unpublished=SIZE_MAX;
        CHECK(!nativeMatchSnapshotPool_Save(&objects.pool,invalid,capacity,&unpublished));
        CHECK(unpublished==SIZE_MAX && !memcmp(&objects,&malformed,sizeof(objects)));
        for(size_t i=capacity;i<sizeof(invalid);++i)CHECK(invalid[i]==0xCD);
        ++writer_refusals;
    }
    objects=valid;
    size_t bytes=nativeMatchSnapshotPool_Bytes(&objects.pool),written;
    CHECK(nativeMatchSnapshotPool_Save(&objects.pool,invalid,bytes,&written));
    CHECK(written==bytes && nativeMatchSnapshotPool_View(&objects.pool,invalid,bytes).records);
    CHECK(!memcmp(&objects,&valid,sizeof(objects)));
}

int main(void)
{
    gobj_lifecycle();item_link_chain();real_actor_state();full_resource_capacity();large_class_piece_capacity();ordering();rejects();writer_bounds();
    printf("{\"lifecycles\":%u,\"malformed_snapshot_cases\":%u,\"writer_refusals\":%u,\"checks\":%u,\"callback_invocations\":%u,\"sizes\":{\"gobj\":%zu,\"gobj_proc\":%zu,\"item_link\":%zu,\"fighter\":%zu,\"item\":%zu}}\n",
        lifecycles,failures,writer_refusals,checks,callbacks,sizeof(HSD_GObj),sizeof(HSD_GObjProc),sizeof(ItemLink),sizeof(Fighter),sizeof(Item));
    return 0;
}
