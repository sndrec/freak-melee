#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <melee/gm/native_match_snapshot_pool.h>
#include <melee/gm/native_rollback_state.h>
#include <melee/gm/native_match_arena.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/gr/native_ground_pool.h>
#include <melee/it/native_item_pool.h>
#include <melee/it/native_item_link_pool.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/native_gobj_snapshot.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/fog.h>
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <sysdolphin/baselib/native_random_snapshot.h>
void nativeRandom_TraceEvent(const char* name,void* caller,const void* object,int value)
{ (void)name;(void)caller;(void)object;(void)value; }
#include <stdlib.h>
#include <string.h>

// Fixture controls/results are deliberately outside the oracle section.
#pragma clang section data = ""
// Includes stable Fighter and Ground owner storage bound by scene entry.
static uint8_t oracle[64*1024*1024];
static uint8_t arena_oracle[NATIVE_MATCH_SCENE_BYTES];
static uint8_t payload_oracle[2*1024*1024];
static uint8_t object_snapshot[1024*1024],proc_snapshot[2*1024*1024];
static unsigned checks,cases,removed,events[64],event_count;
static jmp_buf panic_jump;
static bool expect_panic;
static unsigned panics;
static HSD_GObj *removal_peer, *removal_replacement;
static unsigned removal_mode, object_removed;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"line%u: %s\n",(unsigned)__LINE__,#x);exit(1);}}while(0)

void OSReport(char* format,...){(void)format;}
__declspec(noreturn) void __assert(char* file,u32 line,char* reason)
{if(expect_panic){++panics;longjmp(panic_jump,1);}fprintf(stderr,"assert %s:%u %s\n",file,line,reason);abort();}
void HSD_LObjRemoveAll(HSD_LObj* object){(void)object;abort();}
void HSD_JObjRemoveAll(HSD_JObj* object){(void)object;abort();}
void HSD_LObj_803668EC(HSD_LObj* object){(void)object;abort();}
HSD_CObj* HSD_CObjGetCurrent(void){abort();}
void HSD_LObjSetupInit(HSD_CObj* object){(void)object;abort();}
void HSD_JObjDispAll(HSD_JObj* object,Mtx matrix,u32 flags,u32 mode)
{(void)object;(void)matrix;(void)flags;(void)mode;abort();}
void HSD_FogSet(HSD_Fog* object){(void)object;abort();}
bool HSD_CObjSetCurrent(HSD_CObj* object){(void)object;abort();}
void HSD_CObjEndCurrent(void){abort();}

static void record(unsigned event){CHECK(event_count<64);events[event_count++]=event;}
static void cleanup(void* data){CHECK(data!=NULL);++removed;}
static void tick(HSD_GObj* object){record(object->classifier);}
static void remove_self(HSD_GObj* object)
{
    record(object->classifier);uint32_t before=nativeGObjPool.used;
    HSD_GObjFree(object);
    CHECK(nativeGObjPool.used==before&&HSD_GObj_DelayedProcInfo.delay_remove_gobj);
}
static void remove_proc(HSD_GObj* object)
{
    record(object->classifier);uint32_t before=nativeGObjProcPool.used;
    HSD_GObjProc_RemoveProc(HSD_GObj_CurrentInvokedProc);
    CHECK(nativeGObjProcPool.used==before&&HSD_GObj_DelayedProcInfo.delay_remove_proc);
}
static void draw(HSD_GObj* object,int pass){CHECK(pass==0);record(1000+object->classifier);}
static void initialize(void)
{
    CHECK(nativeGObjPool_RetireScene());
    HSD_GObjLibInitDataType config;HSD_GObj_803912E0(&config);
    config.p_link_max=3;config.gx_link_max=3;config.gproc_pri_max=3;
    HSD_GObj_80391304(&config);
    CHECK(nativeMatchPool_Validate(&nativeGObjPool));
    CHECK(nativeMatchPool_Validate(&nativeGObjProcPool));
    CHECK(nativeGObjPool.capacity==NATIVE_GOBJ_CAPACITY);
    CHECK(nativeGObjProcPool.capacity==NATIVE_GOBJ_PROC_CAPACITY);
    CHECK(nativeGObjPool.lifetime==nativeGObjProcPool.lifetime);
    event_count=0;removed=0;
}
static bool covered(NativeRollbackStateRegion region,const void* pointer,size_t bytes)
{
    uintptr_t begin=(uintptr_t)region.memory,at=(uintptr_t)pointer;
    bool global=at>=begin&&at-begin<=region.size&&bytes<=region.size-(at-begin);
    begin=(uintptr_t)nativeMatchSceneArena.memory;
    uintptr_t actor_begin=(uintptr_t)nativeMatchActorStorage.memory;
    bool actor=at>=actor_begin&&at-actor_begin<=nativeMatchActorStorage.bytes&&
        bytes<=nativeMatchActorStorage.bytes-(at-actor_begin);
    return global || actor || (at>=begin&&at-begin<=nativeMatchArena_Used(&nativeMatchSceneArena)&&
        bytes<=nativeMatchArena_Used(&nativeMatchSceneArena)-(at-begin));
}

static void actual_lifecycle(void)
{
    initialize();HSD_GObj* a=GObj_Create(10,1,10),*b=GObj_Create(20,1,20),*c=GObj_Create(30,2,0);
    CHECK(a&&b&&c&&a->next==b&&b->prev==a);
    HSD_GObjProc* ap=HSD_GObj_SetupProc(a,tick,0);
    HSD_GObjProc* bp=HSD_GObj_SetupProc(b,tick,0);
    HSD_GObjProc* cp=HSD_GObj_SetupProc(c,tick,0);
    CHECK(ap&&bp&&cp&&ap->next==bp&&bp->next==cp);
    CHECK(HSD_GObj_ProcList[1]==bp&&HSD_GObj_ProcList[2]==cp);
    GObj_InitUserData(a,6,cleanup,a);a->x34_unk=a;
    GObj_SetupGXLinkMax(a,draw,0);GObj_SetupGXLinkMax(b,draw,1);
    HSD_GObj_80390CFC();CHECK(event_count==3&&events[0]==10&&events[1]==20&&events[2]==30);
    HSD_GObj_80390FC0();CHECK(event_count==5&&events[3]==1010&&events[4]==1020);
    size_t object_bytes,proc_bytes;
    CHECK(nativeMatchSnapshotPool_Save(&nativeGObjPool,object_snapshot,sizeof(object_snapshot),&object_bytes));
    CHECK(nativeMatchSnapshotPool_Save(&nativeGObjProcPool,proc_snapshot,sizeof(proc_snapshot),&proc_bytes));
    NativeMatchPoolReference a_ref,ap_ref;
    CHECK(nativeMatchPool_Reference(&nativeGObjPool,a,&a_ref));
    CHECK(nativeMatchPool_Reference(&nativeGObjProcPool,ap,&ap_ref));
    NativeRollbackStateRegion region;CHECK(nativeRollbackState_GetGlobalRegions(&region,1)==1);
    CHECK(region.size<sizeof(oracle));
    CHECK(covered(region,nativeGObjPool.payload,nativeGObjPool.capacity*nativeGObjPool.stride));
    CHECK(covered(region,nativeGObjPool.slots,nativeGObjPool.capacity*sizeof(*nativeGObjPool.slots)));
    CHECK(covered(region,nativeGObjPool.free_order,nativeGObjPool.capacity*sizeof(uint32_t)));
    CHECK(covered(region,nativeGObjProcPool.payload,nativeGObjProcPool.capacity*nativeGObjProcPool.stride));
    CHECK(covered(region,nativeGObjProcPool.slots,nativeGObjProcPool.capacity*sizeof(*nativeGObjProcPool.slots)));
    CHECK(covered(region,nativeGObjProcPool.free_order,nativeGObjProcPool.capacity*sizeof(uint32_t)));
    CHECK(covered(region,&nativeGObjPool,sizeof(nativeGObjPool))&&covered(region,&nativeGObjProcPool,sizeof(nativeGObjProcPool)));
    CHECK(covered(region,HSD_GObjPLinkHead,4*sizeof(void*)));
    CHECK(covered(region,HSD_GObj_ProcList,16*sizeof(void*)));
    CHECK(covered(region,plinklow_gobjs,4*sizeof(void*)));
    CHECK(covered(region,HSD_GObjGXLinkHead,5*sizeof(void*)));
    CHECK(covered(region,HSD_GObj_804D7820,5*sizeof(void*)));
    CHECK(covered(region,HSD_GObj_GObjProcHead,4*sizeof(void*)));
    CHECK(covered(region,HSD_GObj_ProcList,4*4*sizeof(void*)));
    CHECK(covered(region,HSD_GObj_804D7810,4*sizeof(GObjFunc)));
    memcpy(oracle,region.memory,region.size);
    size_t arena_bytes=nativeMatchArena_Used(&nativeMatchSceneArena);
    memcpy(arena_oracle,nativeMatchSceneArena.memory,arena_bytes);
    size_t object_payload_bytes=nativeGObjPool.capacity*nativeGObjPool.stride;
    size_t proc_payload_bytes=nativeGObjProcPool.capacity*nativeGObjProcPool.stride;
    CHECK(object_payload_bytes+proc_payload_bytes<=sizeof(payload_oracle));
    memcpy(payload_oracle,nativeGObjPool.payload,object_payload_bytes);
    memcpy(payload_oracle+object_payload_bytes,nativeGObjProcPool.payload,proc_payload_bytes);
    HSD_GObjFree(a);CHECK(removed==1&&nativeGObjPool.used==2&&nativeGObjProcPool.used==2);
    CHECK(nativeMatchPool_Resolve(a_ref)==NULL&&nativeMatchPool_Resolve(ap_ref)==NULL);
    HSD_GObj* reused=GObj_Create(40,1,10);CHECK(reused==a);
    HSD_GObjProc* reused_proc=HSD_GObj_SetupProc(reused,tick,0);CHECK(reused_proc==ap);
    NativeMatchPoolReference future;
    CHECK(nativeMatchPool_Reference(&nativeGObjPool,reused,&future)&&future.generation==a_ref.generation+1);
    // Exact global-region oracle restores payload, both allocator descriptors,
    // metadata, tables, callback slots and source invocation marker together.
    memcpy(region.memory,oracle,region.size);
    memcpy(nativeMatchSceneArena.memory,arena_oracle,arena_bytes);
    memcpy(nativeGObjPool.payload,payload_oracle,object_payload_bytes);
    memcpy(nativeGObjProcPool.payload,payload_oracle+object_payload_bytes,proc_payload_bytes);
    CHECK(nativeMatchPool_Resolve(a_ref)==a&&nativeMatchPool_Resolve(ap_ref)==ap);
    CHECK(nativeMatchPool_Resolve(future)==NULL&&a->x34_unk==a&&a->next==b&&a->proc==ap);
    CHECK(nativeGObjPool.used==3&&nativeGObjProcPool.used==3);
    event_count=0;HSD_GObj_80390CFC();CHECK(event_count==3&&events[0]==10&&events[1]==20&&events[2]==30);
    HSD_GObjFree(a);CHECK(GObj_Create(40,1,10)==reused);
    CHECK(HSD_GObj_SetupProc(reused,tick,0)==reused_proc);
    CHECK(nativeMatchPool_Reference(&nativeGObjPool,reused,&future)&&future.generation==a_ref.generation+1);
    // Packed family restore has the same payload/generation semantics; complete
    // scheduling roots remain the central aggregate's responsibility.
    CHECK(nativeMatchSnapshotPool_View(&nativeGObjPool,object_snapshot,object_bytes).records);
    CHECK(nativeMatchSnapshotPool_View(&nativeGObjProcPool,proc_snapshot,proc_bytes).records);
    CHECK(nativeMatchSnapshotPool_Load(&nativeGObjPool,object_snapshot,object_bytes));
    CHECK(nativeMatchSnapshotPool_Load(&nativeGObjProcPool,proc_snapshot,proc_bytes));
    memcpy(region.memory,oracle,region.size);
    memcpy(nativeMatchSceneArena.memory,arena_oracle,arena_bytes);
    memcpy(nativeGObjPool.payload,payload_oracle,object_payload_bytes);
    memcpy(nativeGObjProcPool.payload,payload_oracle+object_payload_bytes,proc_payload_bytes);++cases;
}

static void deferred(void)
{
    initialize();HSD_GObj* a=GObj_Create(1,0,0),*b=GObj_Create(2,0,1);
    GObj_InitUserData(a,6,cleanup,a);
    HSD_GObj_SetupProc(a,remove_self,0);HSD_GObj_SetupProc(b,tick,0);
    HSD_GObj_80390CFC();CHECK(event_count==2&&events[0]==1&&events[1]==2);
    CHECK(removed==1&&nativeGObjPool.used==1&&nativeGObjProcPool.used==1);
    CHECK(HSD_GObjPLinkHead[0]==b&&b->prev==NULL&&HSD_GObj_DelayedProcInfo.flags==0);
    CHECK(GObj_Create(3,0,0)==a);++cases;
    initialize();a=GObj_Create(1,0,0);b=GObj_Create(2,0,1);
    HSD_GObjProc* retired=HSD_GObj_SetupProc(a,remove_proc,0);HSD_GObj_SetupProc(b,tick,0);
    HSD_GObj_80390CFC();CHECK(nativeGObjPool.used==2&&nativeGObjProcPool.used==1&&a->proc==NULL);
    CHECK(HSD_GObj_SetupProc(a,tick,0)==retired);++cases;
}

static void reordering(void)
{
    initialize();HSD_GObj* a=GObj_Create(1,0,1),*b=GObj_Create(2,0,2);
    HSD_GObjProc* ap=HSD_GObj_SetupProc(a,tick,0);HSD_GObjProc* bp=HSD_GObj_SetupProc(b,tick,0);
    HSD_GObjPLink_ChangeGObjPri_Unk(0,b,0,0,NULL);
    CHECK(HSD_GObjPLinkHead[0]==b&&b->next==a&&a->prev==b&&ap->prev==bp);
    HSD_GObj_80390CFC();CHECK(event_count==2&&events[0]==2&&events[1]==1);++cases;
}

static void exhaustion_and_epoch(void)
{
    initialize();for(unsigned i=0;i<NATIVE_GOBJ_CAPACITY;++i)CHECK(GObj_Create(1,0,0));
    CHECK(nativeGObjPool.used==NATIVE_GOBJ_CAPACITY&&GObj_Create(1,0,0)==NULL);
    CHECK(nativeMatchPool_Validate(&nativeGObjPool));++cases;
    initialize();HSD_GObj* a=GObj_Create(1,0,0);
    for(unsigned i=0;i<NATIVE_GOBJ_PROC_CAPACITY;++i)CHECK(HSD_GObj_SetupProc(a,tick,0));
    CHECK(nativeGObjProcPool.used==NATIVE_GOBJ_PROC_CAPACITY);
    expect_panic=true;if(setjmp(panic_jump)==0){HSD_GObj_SetupProc(a,tick,0);CHECK(false);}expect_panic=false;
    CHECK(panics==1&&nativeGObjProcPool.used==NATIVE_GOBJ_PROC_CAPACITY);
    CHECK(nativeMatchPool_Validate(&nativeGObjProcPool));++cases;
    NativeMatchPoolReference ref;CHECK(nativeMatchPool_Reference(&nativeGObjPool,a,&ref));
    HSD_GObj_CurrentInvokedProcGObj=a;CHECK(!nativeGObjPool_RetireScene());
    CHECK(nativeGObjPool.lifetime==ref.lifetime);HSD_GObj_CurrentInvokedProcGObj=NULL;
    unsigned before=removed;uint64_t epoch=nativeGObjPool.lifetime;
    CHECK(nativeGObjPool_RetireScene()&&removed==before&&nativeMatchPool_Resolve(ref)==NULL);
    CHECK(GObj_Create(1,0,0)==NULL);initialize();CHECK(nativeGObjPool.lifetime==epoch+1);
    CHECK(nativeMatchPool_Resolve(ref)==NULL);++cases;
}

static void initialization_rejection(void)
{
    initialize();uint64_t epoch=nativeGObjPool.lifetime;
    HSD_GObj* object=GObj_Create(1,0,0);CHECK(object);
    HSD_GObjLibInitDataType config=HSD_GObjLibInitData;
    GObjFunc methods[255]={0};GObjFuncs first={.size=255,.funcs=methods};
    GObjFuncs second={.size=2,.funcs=methods};first.next=&second;config.funcs=&first;
    CHECK(!nativeGObjPool_BeginScene(&config)&&nativeGObjPool.lifetime==epoch&&nativeGObjPool.used==1);
    first.size=0;second.size=0;second.next=&first;
    CHECK(!nativeGObjPool_BeginScene(&config)&&nativeGObjPool.lifetime==epoch&&HSD_GObjPLinkHead[0]==object);
    ++cases;
}

static void nested_cleanup(void* data)
{
    HSD_GObj* object=data;
    CHECK(object->user_data==data&&object->user_data_kind==6);
    CHECK(object->native_userdata_removing);
    ++removed;
    GObj_RemoveUserData(object);
    HSD_GObjFree(object);
    CHECK(object->user_data==data&&object->user_data_kind==6);
    if(removal_peer) {
        HSD_GObj* peer=removal_peer;removal_peer=NULL;
        HSD_GObjFree(peer);
        removal_replacement=GObj_Create(99,0,1);
        CHECK(removal_replacement==peer);
        GObj_InitUserData(removal_replacement,6,cleanup,removal_replacement);
    }
    if(removal_mode==1) {
        CHECK(nativeGObjPool_RetireScene());
        HSD_GObjLibInitDataType config;HSD_GObj_803912E0(&config);
        config.p_link_max=3;config.gx_link_max=3;config.gproc_pri_max=3;
        HSD_GObj_80391304(&config);
        removal_replacement=GObj_Create(99,0,1);
        CHECK(removal_replacement==object);
        GObj_InitUserData(removal_replacement,6,cleanup,removal_replacement);
    }
}

static void nested_object_cleanup(HSD_Obj* data)
{
    HSD_GObj* object=(HSD_GObj*)data;
    ++object_removed;
    HSD_GObjFree(object);
    CHECK(object->native_destroying);
    if(removal_mode==2) {
        CHECK(nativeGObjPool_RetireScene());
        HSD_GObjLibInitDataType config;HSD_GObj_803912E0(&config);
        config.p_link_max=3;config.gx_link_max=3;config.gproc_pri_max=3;
        HSD_GObj_80391304(&config);
        removal_replacement=GObj_Create(99,0,1);
        CHECK(removal_replacement==object);
        GObj_InitUserData(removal_replacement,6,cleanup,removal_replacement);
        removal_replacement->obj_kind=0;
        removal_replacement->hsd_obj=data;
    }
}

static void recursive_removal(void)
{
    // Free and standalone userdata removal both keep data published during
    // the hook, but run it once. Delayed scheduler destruction follows suit.
    for(unsigned mode=0;mode<3;++mode) {
        initialize();removal_mode=0;removal_peer=NULL;removal_replacement=NULL;
        HSD_GObj* object=GObj_Create(1,0,0);
        GObj_InitUserData(object,6,nested_cleanup,object);
        NativeMatchPoolReference reference;
        CHECK(nativeMatchPool_Reference(&nativeGObjPool,object,&reference));
        if(mode==0)HSD_GObjFree(object);
        if(mode==1)GObj_RemoveUserData(object);
        if(mode==2) {
            HSD_GObj_SetupProc(object,remove_self,0);
            HSD_GObj_80390CFC();
        }
        CHECK(removed==1&&nativeGObjPool.used==0&&nativeGObjProcPool.used==0);
        CHECK(nativeMatchPool_Resolve(reference)==NULL);
        CHECK(HSD_GObj_DelayedProcInfo.flags==0&&HSD_GObjPLinkHead[0]==NULL);
        HSD_GObj* reused=GObj_Create(2,0,0);
        CHECK(reused==object&&!reused->native_destroying&&!reused->native_userdata_removing&&!reused->native_destroy_requested);
        HSD_GObjFree(reused);CHECK(nativeMatchPool_Validate(&nativeGObjPool));++cases;
    }
    initialize();removal_mode=0;
    HSD_GObj* object=GObj_Create(1,0,0);
    removal_peer=GObj_Create(2,0,1);removal_replacement=NULL;
    GObj_InitUserData(object,6,nested_cleanup,object);
    GObj_InitUserData(removal_peer,6,cleanup,removal_peer);
    HSD_GObjFree(object);
    CHECK(removed==2&&nativeGObjPool.used==1&&HSD_GObjPLinkHead[0]==removal_replacement);
    CHECK(removal_replacement->user_data==removal_replacement);
    HSD_GObjFree(removal_replacement);CHECK(removed==3&&nativeGObjPool.used==0);++cases;

    // A native callback can retire/rebind the scene outside scheduler/VM
    // execution. Neither old userdata clearing nor outer unlink may touch
    // the same-address object that belongs to the next scene.
    for(unsigned mode=0;mode<2;++mode) {
        initialize();removal_mode=1;removal_replacement=NULL;
        object=GObj_Create(1,0,0);GObj_InitUserData(object,6,nested_cleanup,object);
        NativeMatchPoolReference reference;
        CHECK(nativeMatchPool_Reference(&nativeGObjPool,object,&reference));
        if(mode==0)HSD_GObjFree(object);else GObj_RemoveUserData(object);
        CHECK(nativeMatchPool_Resolve(reference)==NULL&&nativeGObjPool.used==1&&removed==1);
        CHECK(HSD_GObjPLinkHead[0]==removal_replacement&&removal_replacement->user_data==removal_replacement);
        removal_mode=0;HSD_GObjFree(removal_replacement);CHECK(removed==2&&nativeGObjPool.used==0);++cases;
    }
    for(unsigned mode=0;mode<2;++mode) {
        initialize();removal_mode=mode?2:0;object_removed=0;
        GObjFunc functions[]={nested_object_cleanup};
        GObjFuncs methods={.size=1,.funcs=functions};
        HSD_GObjLibInitDataType config=HSD_GObjLibInitData;
        config.funcs=&methods;CHECK(nativeGObjPool_RetireScene());HSD_GObj_80391304(&config);
        object=GObj_Create(1,0,0);
        object->obj_kind=0;object->hsd_obj=(HSD_Obj*)object;
        HSD_GObjFree(object);CHECK(object_removed==1);
        if(mode) {
            CHECK(nativeGObjPool.used==1&&HSD_GObjPLinkHead[0]==removal_replacement);
            CHECK(removal_replacement->obj_kind==0&&removal_replacement->hsd_obj==(HSD_Obj*)object);
            removal_replacement->obj_kind=HSD_GOBJ_OBJ_NONE;
            HSD_GObjFree(removal_replacement);
        }
        CHECK(nativeGObjPool.used==0);++cases;
    }
    removal_mode=0;removal_peer=NULL;removal_replacement=NULL;
}

#include "snapshot_cases.h"
#include "arena_cases.h"

int main(void)
{
    actual_lifecycle();deferred();reordering();exhaustion_and_epoch();initialization_rejection();recursive_removal();
    explicit_scheduler();contiguous_scheduler_history();contiguous_actor_history();
    CHECK(nativeGObjPool_RetireScene());
    printf("{\"cases\":%u,\"checks\":%u,\"object_capacity\":%u,\"proc_capacity\":%u,\"expected_exhaustion_panics\":%u,\"global_oracle\":true,\"scheduler_checks\":%u,\"scheduler_refusals\":%u,\"scheduler_bytes\":%zu,\"arena_copy_bytes\":%zu}\n",
        cases,checks,NATIVE_GOBJ_CAPACITY,NATIVE_GOBJ_PROC_CAPACITY,panics,scheduler_checks,scheduler_refusals,scheduler_bytes,arena_copy_bytes);
    return 0;
}
