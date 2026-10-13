#include <melee/gm/native_match_object_census.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/objalloc.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/gr/native_ground_pool.h>
#include <melee/it/native_item_pool.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/fobj.h>
#include <sysdolphin/baselib/robj.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

NativeMatchPool nativeGObjPool,nativeGObjProcPool;
NativeMatchPool nativeFighterPool,nativeGroundPool,nativeItemPool,nativeItemLinkPool,nativeAObjPool,nativeFObjPool;
static HSD_ObjAllocData extra_pool;
static HSD_ObjAllocData animation_pools[6];
HSD_ClassInfo hsdClass;
static HSD_ClassInfo joint_class,material_class;
HSD_GObjLibInitDataType HSD_GObjLibInitData;
static HSD_GObj* plink[4],*ptail[4],*gxlink[5],*gxtail[5];
static HSD_GObjProc* priorities[3],*priority_link[12];
HSD_GObj** HSD_GObjPLinkHead=plink;
HSD_GObj** plinklow_gobjs=ptail;
HSD_GObj** HSD_GObjGXLinkHead=gxlink;
HSD_GObj** HSD_GObj_804D7820=gxtail;
HSD_GObjProc** HSD_GObj_GObjProcHead=priorities;
HSD_GObjProc** HSD_GObj_ProcList=priority_link;
static HSD_GObj objects[3];
static HSD_GObjProc procs[3];
static unsigned checks,cases;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"line%u: %s\n",(unsigned)__LINE__,#x);abort();}}while(0)
// Registry-head accessor is a controlled fixture boundary; all records read by
// the actual census body are the real source HSD_ObjAllocData/Class/GObj types.
const HSD_ObjAllocData* HSD_ObjAllocRegistry(void){return &extra_pool;}
HSD_ObjAllocData* HSD_VecGetAllocData(void){return &extra_pool;}
HSD_ObjAllocData* HSD_MtxGetAllocData(void){return &animation_pools[2];}
HSD_ObjAllocData* HSD_RObjGetAllocData(void){return &animation_pools[3];}
HSD_ObjAllocData* HSD_RvalueObjGetAllocData(void){return &animation_pools[4];}
HSD_ObjAllocData* HSD_IDGetAllocData(void){return &animation_pools[5];}
void OSReport(char* format,...){(void)format;}

typedef struct GameBytes {
    NativeMatchPool pools[8]; HSD_ObjAllocData extra_pool,animation_pools[6];
    HSD_ClassInfo classes[3];
    HSD_GObj objects[3];HSD_GObjProc procs[3];
    HSD_GObj* plink[4],*ptail[4],*gxlink[5],*gxtail[5];
    HSD_GObjProc* priorities[3],*priority_link[12];
    HSD_GObjLibInitDataType init;
} GameBytes;
static GameBytes capture(void)
{
    GameBytes b;memset(&b,0,sizeof(b));
    b.pools[0]=nativeGObjPool;b.pools[1]=nativeGObjProcPool;b.pools[2]=nativeFighterPool;b.extra_pool=extra_pool;
    b.pools[3]=nativeAObjPool;b.pools[4]=nativeFObjPool;
    b.pools[5]=nativeGroundPool;b.pools[6]=nativeItemPool;b.pools[7]=nativeItemLinkPool;
    memcpy(b.animation_pools,animation_pools,sizeof(animation_pools));
    b.classes[0]=hsdClass;b.classes[1]=joint_class;b.classes[2]=material_class;
    memcpy(b.objects,objects,sizeof(objects));memcpy(b.procs,procs,sizeof(procs));
    memcpy(b.plink,plink,sizeof(plink));memcpy(b.ptail,ptail,sizeof(ptail));
    memcpy(b.gxlink,gxlink,sizeof(gxlink));memcpy(b.gxtail,gxtail,sizeof(gxtail));
    memcpy(b.priorities,priorities,sizeof(priorities));memcpy(b.priority_link,priority_link,sizeof(priority_link));
    b.init=HSD_GObjLibInitData;return b;
}
static void sample(unsigned frame,bool replaying)
{
    GameBytes before=capture();nativeMatchObjectCensus_Sample(frame,frame+10,replaying);
    GameBytes after=capture();CHECK(memcmp(&before,&after,sizeof(before))==0);++cases;
}
static void setup(void)
{
    memset(objects,0,sizeof(objects));memset(procs,0,sizeof(procs));
    memset(plink,0,sizeof(plink));memset(ptail,0,sizeof(ptail));
    memset(gxlink,0,sizeof(gxlink));memset(gxtail,0,sizeof(gxtail));
    memset(priorities,0,sizeof(priorities));memset(priority_link,0,sizeof(priority_link));
    HSD_GObjLibInitData=(HSD_GObjLibInitDataType){.p_link_max=3,.gx_link_max=3,.gproc_pri_max=2};
    nativeGObjPool=(NativeMatchPool){.object_size=sizeof(HSD_GObj),.capacity=8,.used=2,.free_count=3,.peak=5};
    nativeGObjProcPool=(NativeMatchPool){.object_size=sizeof(HSD_GObjProc),.capacity=8,.used=3,.free_count=1,.peak=4};
    nativeFighterPool=(NativeMatchPool){.object_size=sizeof(NativeFighterPoolSlot),.capacity=64,.used=1,.free_count=63,.peak=1};
    nativeItemPool=(NativeMatchPool){.object_size=sizeof(NativeItemPoolSlot),.capacity=NATIVE_ITEM_CAPACITY,.used=4,.free_count=NATIVE_ITEM_CAPACITY-4,.peak=5};
    nativeGroundPool=(NativeMatchPool){.object_size=sizeof(Ground),.capacity=NATIVE_GROUND_CAPACITY,.used=3,.free_count=NATIVE_GROUND_CAPACITY-3,.peak=4};
    nativeAObjPool=(NativeMatchPool){.object_size=sizeof(HSD_AObj),.capacity=4096,.used=3,.free_count=4093,.peak=5};
    nativeFObjPool=(NativeMatchPool){.object_size=sizeof(HSD_FObj),.capacity=16384,.used=2,.free_count=16382,.peak=3};
    extra_pool=(HSD_ObjAllocData){.size=sizeof(Vec3),.align=7,.used=3,.free=2,.peak=5};
    memset(animation_pools,0,sizeof(animation_pools));
    extra_pool.next=&animation_pools[2];
    for(unsigned i=2;i<6;++i){
        animation_pools[i].size=16;animation_pools[i].align=7;
        animation_pools[i].next=i==5?NULL:&animation_pools[i+1];
    }
    hsdClass=(HSD_ClassInfo){.head={.flags=1,.class_name="hsd_class",.obj_size=sizeof(HSD_Class),.child=&joint_class}};
    joint_class=(HSD_ClassInfo){.head={.flags=1,.class_name="fixture_joint",.obj_size=sizeof(HSD_JObj),.nb_exist=2,.nb_peak=4,.parent=&hsdClass,.next=&material_class}};
    material_class=(HSD_ClassInfo){.head={.flags=1,.class_name="fixture_material",.obj_size=sizeof(HSD_MObj),.nb_exist=1,.nb_peak=2,.parent=&hsdClass}};
    objects[0]=(HSD_GObj){.classifier=0x100,.p_link=1,.gx_link=2,.next=&objects[1],.next_gx=&objects[1],.proc=&procs[0]};
    objects[1]=(HSD_GObj){.classifier=7,.p_link=1,.gx_link=2,.prev=&objects[0],.prev_gx=&objects[0],.proc=&procs[1]};
    procs[0]=(HSD_GObjProc){.gobj=&objects[0],.s_link=0,.next=&procs[1],.child=&procs[2]};
    procs[1]=(HSD_GObjProc){.gobj=&objects[1],.s_link=0,.prev=&procs[0]};
    procs[2]=(HSD_GObjProc){.gobj=&objects[0],.s_link=2};
    plink[1]=&objects[0];ptail[1]=&objects[1];gxlink[2]=&objects[0];gxtail[2]=&objects[1];
    priorities[0]=&procs[0];priorities[2]=&procs[2];priority_link[1]=&procs[1];priority_link[9]=&procs[2];
}

int main(void)
{
    setup();GameBytes before=capture();
    nativeMatchObjectCensus_BeginScene(2);GameBytes after=capture();
    CHECK(memcmp(&before,&after,sizeof(before))==0);
    CHECK(nativeMatchObjectCensus_Stats()->samples==0);sample(0,false);
    const NativeMatchObjectCensusStats* stats=nativeMatchObjectCensus_Stats();
    CHECK(stats->errors==0&&stats->object_peak==5&&stats->proc_peak==4);
    CHECK(stats->linked_object_peak==2&&stats->linked_proc_peak==3);
    CHECK(stats->allocator_types==13&&stats->class_types==3&&stats->classifier_types==2);
    uint64_t expected_pool=2*sizeof(HSD_GObj)+3*sizeof(HSD_GObjProc)+3*sizeof(HSD_AObj)+3*sizeof(Vec3)+sizeof(NativeFighterPoolSlot)+3*sizeof(Ground)+4*sizeof(NativeItemPoolSlot)+2*sizeof(HSD_FObj);
    uint64_t expected_class=2*sizeof(HSD_JObj)+sizeof(HSD_MObj);
    CHECK(stats->allocator_payload_peak==expected_pool&&stats->class_payload_peak==expected_class);
    objects[2]=(HSD_GObj){.classifier=0x123,.p_link=0,.gx_link=0xFF};
    plink[0]=ptail[0]=&objects[2];nativeGObjPool.used=3;nativeGObjPool.peak=7;
    extra_pool.used=4;sample(1,true);
    CHECK(stats->errors==0&&stats->samples==2&&stats->replay_samples==1);
    CHECK(stats->object_peak==7&&stats->linked_object_peak==3&&stats->classifier_types==3);
    CHECK(stats->payload_peak==expected_pool+sizeof(HSD_GObj)+sizeof(Vec3)+expected_class);
    nativeMatchObjectCensus_FinishScene();CHECK(!nativeMatchObjectCensus_HasFailed());
    for(unsigned fault=0;fault<11;++fault){
        setup();nativeMatchObjectCensus_BeginScene(10+fault);
        if(fault==0)objects[1].next=&objects[0];
        if(fault==1)objects[1].prev=NULL;
        if(fault==2)procs[1].next=&procs[0];
        if(fault==3)procs[2].gobj=&objects[2];
        if(fault==4)objects[0].proc->child=&procs[0];
        if(fault==5)priority_link[1]=NULL;
        if(fault==6)gxtail[2]=&objects[0];
        if(fault==7)extra_pool.next=&extra_pool;
        if(fault==8)joint_class.head.child=&hsdClass;
        if(fault==9)nativeGObjPool.used=3;
        if(fault==10)extra_pool.next=NULL;
        sample(fault,false);CHECK(nativeMatchObjectCensus_Stats()->errors!=0);
        CHECK(nativeMatchObjectCensus_HasFailed());nativeMatchObjectCensus_FinishScene();
    }
    setup();nativeMatchObjectCensus_BeginScene(99);sample(0,false);
    CHECK(nativeMatchObjectCensus_Stats()->errors==0&&nativeMatchObjectCensus_Stats()->samples==1);
    // Forced close flushes an active scene and does not dereference a vanished
    // old registry after the last complete sample.
    memset(plink,0,sizeof(plink));nativeMatchObjectCensus_Close();
    CHECK(nativeMatchObjectCensus_HasFailed());
    printf("{\"cases\":%u,\"checks\":%u,\"malformed_topology_cases\":11,\"read_only\":true}\n",cases,checks);
    return 0;
}
