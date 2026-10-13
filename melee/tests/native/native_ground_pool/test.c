#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <melee/gr/native_ground_pool.h>
#include <melee/gm/native_match_snapshot_pool.h>
#include <melee/gm/native_rollback_state.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/fog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma clang section data = ""
static uint8_t saved[4*1024*1024], invalid[4*1024*1024], after[4*1024*1024];
static uint8_t object_saved[1024*1024], oracle[64*1024*1024];
static unsigned checks,cases,callbacks;
static size_t sparse_bytes,full_bytes;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"line%u: %s\n",(unsigned)__LINE__,#x);exit(1);}}while(0)
void OSReport(char* text,...){(void)text;}
__declspec(noreturn) void __assert(char* file,u32 line,char* reason)
{fprintf(stderr,"%s:%u: %s\n",file,(unsigned)line,reason);abort();}
void HSD_LObjRemoveAll(HSD_LObj* p){(void)p;abort();}
void HSD_JObjRemoveAll(HSD_JObj* p){(void)p;abort();}
void HSD_LObj_803668EC(HSD_LObj* p){(void)p;abort();}
HSD_CObj* HSD_CObjGetCurrent(void){abort();}
void HSD_LObjSetupInit(HSD_CObj* p){(void)p;abort();}
void HSD_JObjDispAll(HSD_JObj* p,Mtx m,u32 f,u32 r){(void)p;(void)m;(void)f;(void)r;abort();}
void HSD_FogSet(HSD_Fog* p){(void)p;abort();}
bool HSD_CObjSetCurrent(HSD_CObj* p){(void)p;abort();}
void HSD_CObjEndCurrent(void){abort();}

static void initialize(void)
{
    CHECK(nativeGObjPool_RetireScene());
    HSD_GObjLibInitDataType config;HSD_GObj_803912E0(&config);
    config.p_link_max=5;config.gx_link_max=3;config.gproc_pri_max=3;
    HSD_GObj_80391304(&config);
    CHECK(nativeGroundPool.lifetime==nativeGObjPool.lifetime);
    CHECK(nativeGroundPool.used==0 && nativeGroundPool.capacity==NATIVE_GOBJ_CAPACITY);
    CHECK(nativeMatchPool_Validate(&nativeGroundPool));
}
static Ground* create(int map_id)
{
    // Same allocation/publication sequence as both production Ground
    // constructors; model loading and stage-specific callbacks are not run.
    HSD_GObj* root=GObj_Create(HSD_GOBJ_CLASS_STAGE,5,0);if(!root)return NULL;
    NativeMatchPoolReference reference;
    Ground* actor=nativeMatchPool_Allocate(&nativeGroundPool,&reference);
    CHECK(actor!=NULL);
    Ground zero={0};CHECK(memcmp(actor,&zero,sizeof(zero))==0);
    actor->gobj=root;actor->map_id=map_id;
    GObj_InitUserData(root,3,nativeGroundPool_Release,actor);
    return actor;
}
static void deferred_remove(HSD_GObj* root)
{
    size_t bytes=123;
    CHECK(!nativeGroundPool_Save(after,sizeof(after),&bytes) && bytes==123);
    CHECK(!nativeGroundPool_RetireScene());
    unsigned used=nativeGroundPool.used;
    HSD_GObjFree(root);
    CHECK(nativeGroundPool.used==used && HSD_GObj_DelayedProcInfo.delay_remove_gobj);
    ++callbacks;
}
static void lifecycle(void)
{
    initialize();Ground* a=create(1),*b=create(2);
    NativeMatchPoolReference retired,live;
    CHECK(nativeMatchPool_Reference(&nativeGroundPool,a,&retired));
    memset(&a->u,0xAB,sizeof(a->u));
    HSD_GObj* root=a->gobj;HSD_GObjFree(root);
    CHECK(nativeGroundPool.used==1 && nativeMatchPool_Resolve(retired)==NULL);
    a=create(3);CHECK(a->gobj==root);
    CHECK(nativeMatchPool_Reference(&nativeGroundPool,a,&live));
    CHECK(live.slot==retired.slot && live.generation==retired.generation+1);
    GObj_RemoveUserData(root);CHECK(nativeGroundPool.used==1 && root->user_data==NULL);
    HSD_GObjFree(root);CHECK(nativeGroundPool.used==1);
    HSD_GObj_SetupProc(b->gobj,deferred_remove,0);HSD_GObj_80390CFC();
    CHECK(callbacks==1 && nativeGroundPool.used==0 && nativeGObjProcPool.used==0);
    CHECK(nativeMatchPool_Validate(&nativeGroundPool));++cases;
}
static void restore(void)
{
    initialize();Ground* a=create(11),*b=create(12),*c=create(13);
    a->x18=c->gobj;a->x1C_callback=deferred_remove;a->x20[3]=0x12345678;
    memset(&a->u,0x5A,sizeof(a->u));a->xC0=0.375f;
    HSD_GObjFree(b->gobj);Ground expected=*a;
    NativeMatchPoolReference reference,future;
    CHECK(nativeMatchPool_Reference(&nativeGroundPool,a,&reference));
    size_t object_bytes;CHECK(nativeGroundPool_Save(saved,sizeof(saved),&sparse_bytes));
    CHECK(nativeMatchSnapshotPool_Save(&nativeGObjPool,object_saved,sizeof(object_saved),&object_bytes));
    CHECK(sparse_bytes==sizeof(NativeMatchPoolSnapshotHeader)+
        (2*NATIVE_GROUND_CAPACITY-2)*sizeof(uint32_t)+2*(sizeof(uint32_t)+sizeof(Ground)));
    NativeRollbackStateRegion region;CHECK(nativeRollbackState_GetGlobalRegions(&region,1)==1);
    CHECK(region.size<sizeof(oracle));memcpy(oracle,region.memory,region.size);
    HSD_GObj* root=a->gobj;HSD_GObjFree(root);
    a=create(99);CHECK(a->gobj==root);a->x20[3]=-1;
    CHECK(nativeMatchPool_Reference(&nativeGroundPool,a,&future));
    CHECK(nativeGroundPool_ValidateSnapshot(saved,sparse_bytes));
    CHECK(nativeGroundPool_Load(saved,sparse_bytes));
    CHECK(nativeMatchPool_Resolve(reference)==a && nativeMatchPool_Resolve(future)==NULL);
    CHECK(memcmp(a,&expected,sizeof(expected))==0);
    CHECK(nativeMatchSnapshotPool_Load(&nativeGObjPool,object_saved,object_bytes));
    CHECK(a->gobj->user_data==a && a->gobj->user_data_remove_func==nativeGroundPool_Release);
    // The aggregate has not migrated scheduler/graph/stage roots yet. Restore
    // the broad oracle only after checking both typed family loads above.
    memcpy(region.memory,oracle,region.size);
    CHECK(nativeGroundPool_Save(after,sizeof(after),&object_bytes));
    CHECK(object_bytes==sparse_bytes && memcmp(saved,after,sparse_bytes)==0);
    HSD_GObjFree(a->gobj);a=create(99);
    CHECK(nativeMatchPool_Reference(&nativeGroundPool,a,&reference));
    CHECK(reference.slot==future.slot && reference.generation==future.generation);
    HSD_GObjFree(a->gobj);HSD_GObjFree(c->gobj);++cases;
}
static void reject(const void* snapshot,size_t bytes)
{
    size_t before_bytes,after_bytes;
    CHECK(nativeGroundPool_Save(after,sizeof(after),&before_bytes));
    CHECK(!nativeGroundPool_ValidateSnapshot(snapshot,bytes));
    CHECK(!nativeGroundPool_Load(snapshot,bytes));
    CHECK(nativeGroundPool_Save(object_saved,sizeof(object_saved),&after_bytes));
    CHECK(after_bytes==before_bytes && memcmp(after,object_saved,before_bytes)==0);
}
static void invalid_candidates(void)
{
    initialize();Ground* a=create(5);size_t bytes;
    CHECK(nativeGroundPool_Save(saved,sizeof(saved),&bytes));
    NativeMatchPoolSnapshotHeader header;memcpy(&header,saved,sizeof(header));
    size_t payload_offset=sizeof(header)+(header.capacity+header.free_count)*sizeof(uint32_t)+sizeof(uint32_t);
    HSD_GObj* bad_roots[]={NULL,(HSD_GObj*)1,(HSD_GObj*)(nativeGObjPool.payload+1),
        (HSD_GObj*)(nativeGObjPool.payload+nativeGObjPool.capacity*nativeGObjPool.stride)};
    for(unsigned i=0;i<sizeof(bad_roots)/sizeof(*bad_roots);++i) {
        memcpy(invalid,saved,bytes);memcpy(invalid+payload_offset+offsetof(Ground,gobj),&bad_roots[i],sizeof(bad_roots[i]));
        reject(invalid,bytes);++cases;
    }
    reject(saved,bytes-1);reject(saved,sizeof(header)-1);++cases;
    memcpy(invalid,saved,bytes);header.used=header.capacity+1;memcpy(invalid,&header,sizeof(header));reject(invalid,bytes);++cases;
    HSD_GObj* root=a->gobj;a->gobj=(HSD_GObj*)1;
    size_t untouched=456;CHECK(!nativeGroundPool_Save(invalid,sizeof(invalid),&untouched) && untouched==456);a->gobj=root;
    root->user_data_kind=4;CHECK(!nativeGroundPool_Save(invalid,sizeof(invalid),&untouched));root->user_data_kind=3;
    root->user_data=NULL;CHECK(!nativeGroundPool_Save(invalid,sizeof(invalid),&untouched));root->user_data=a;
    CHECK(nativeGroundPool_Save(invalid,sizeof(invalid),&untouched));++cases;
    HSD_GObj_804D7818=root;
    CHECK(!nativeGroundPool_Load(saved,bytes));HSD_GObj_804D7818=NULL;
    HSD_GObj_DelayedProcInfo.flags=1;
    CHECK(!nativeGroundPool_Save(invalid,sizeof(invalid),&untouched));HSD_GObj_DelayedProcInfo.flags=0;++cases;
    NativeMatchPoolReference reference;CHECK(nativeMatchPool_Reference(&nativeGroundPool,a,&reference));
    initialize();CHECK(nativeMatchPool_Resolve(reference)==NULL && !nativeGroundPool_Load(saved,bytes));++cases;
}
static void exhaustion(void)
{
    initialize();
    for(unsigned i=0;i<NATIVE_GROUND_CAPACITY;++i)CHECK(create((int)i));
    CHECK(nativeGObjPool.used==NATIVE_GOBJ_CAPACITY && nativeGroundPool.used==NATIVE_GROUND_CAPACITY);
    CHECK(create(1)==NULL);CHECK(nativeGroundPool_Save(saved,sizeof(saved),&full_bytes));
    CHECK(full_bytes==sizeof(NativeMatchPoolSnapshotHeader)+NATIVE_GROUND_CAPACITY*(2*sizeof(uint32_t)+sizeof(Ground)));
    CHECK(nativeMatchPool_Validate(&nativeGroundPool));
    while(HSD_GObjPLinkHead[5])HSD_GObjFree(HSD_GObjPLinkHead[5]);
    CHECK(nativeGroundPool.used==0 && nativeGObjPool.used==0);
    size_t bytes;CHECK(nativeGroundPool_Save(saved,sizeof(saved),&bytes));
    CHECK(bytes==sizeof(NativeMatchPoolSnapshotHeader)+2*NATIVE_GROUND_CAPACITY*sizeof(uint32_t));
    CHECK(nativeGroundPool_Load(saved,bytes));++cases;
}
int main(void)
{
    lifecycle();restore();invalid_candidates();exhaustion();CHECK(nativeGObjPool_RetireScene());
    CHECK(nativeGroundPool.lifetime==0 && nativeGroundPool.used==0);
    printf("{\"cases\":%u,\"checks\":%u,\"record_bytes\":%zu,\"capacity\":%u,\"sparse_snapshot_bytes\":%zu,\"full_snapshot_bytes\":%zu}\n",
        cases,checks,sizeof(Ground),NATIVE_GROUND_CAPACITY,sparse_bytes,full_bytes);
    return 0;
}
