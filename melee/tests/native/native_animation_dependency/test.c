#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/fobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/id.h>
#include <sysdolphin/baselib/list.h>
#include <sysdolphin/baselib/random.h>
#include <melee/lb/lbanim.h>
#include <melee/lb/types.h>
#include <sysdolphin/baselib/native_animation_pool.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/initialize.h>
#include <sysdolphin/baselib/memory.h>
#include <melee/gm/native_match_snapshot_pool.h>
#include <melee/gm/native_match_arena.h>
#include <sysdolphin/baselib/native_hsd_arena.h>
#include <melee/gm/native_rollback_state.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#pragma clang section data = ""
static HSD_JObj target;
static unsigned checks,cases,updates,ends,refs,init_calls,heap_step,heap_resets,fallback_loads,panic_count;
static bool expect_panic,probe_callback,registry_live;
static jmp_buf panic_jump;
static uint8_t packed[2*1024*1024],invalid[2*1024*1024],before[2*1024*1024],oracle[4*1024*1024];
static uint8_t game_before[2*1024*1024],game_after[2*1024*1024];
static uint8_t animation_oracle[2*1024*1024],arena_saved[20*1024*1024];
HSD_GObj* HSD_GObj_CurrentInvokedProcGObj,*HSD_GObj_804D7818;
HSD_GObjProc* HSD_GObj_CurrentInvokedProc;
struct _unk_gobj_struct HSD_GObj_DelayedProcInfo;
static void* hsd_heap_next_arena_lo=(void*)0x10000;
static void* hsd_heap_next_arena_hi=(void*)0x20000;
static OSHeapHandle heap=1;
static PreloadCache preloadCache;
static unsigned preload_stages;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"line%u: %s\n",(unsigned)__LINE__,#x);abort();}}while(0)
void OSReport(char* text,...){(void)text;}
__declspec(noreturn) void __assert(char* file,u32 line,char* reason)
{++panic_count;if(expect_panic)longjmp(panic_jump,1);fprintf(stderr,"%s:%u: %s\n",file,(unsigned)line,reason);abort();}
__declspec(noreturn) void HSD_Panic(char* file,u32 line,char* reason){__assert(file,line,reason);}
void* HSD_ObjAlloc(HSD_ObjAllocData* pool){(void)pool;CHECK(false);return NULL;}
void HSD_ObjFree(HSD_ObjAllocData* pool,void* pointer){(void)pool;(void)pointer;CHECK(false);}
void HSD_ListInitAllocData(void){CHECK(nativeAObjPool.lifetime&&nativeFObjPool.lifetime);CHECK(heap_step==0||heap_step==5);if(heap_step==5)CHECK(!registry_live);registry_live=true;++init_calls;}
#define INIT(name) void name(void){++init_calls;}
INIT(HSD_IDInitAllocData) INIT(HSD_VecInitAllocData) INIT(HSD_MtxInitAllocData)
INIT(HSD_RObjInitAllocData) INIT(HSD_RenderInitAllocData) INIT(HSD_ShadowInitAllocData) INIT(HSD_ZListInitAllocData)
void hsdForgetClassLibrary(const char* name){CHECK(!strcmp(name,"sysdolphin_base_library")&&heap_step==0);heap_step=1;}
void _HSD_DispForgetMemory(void* low,void* high){(void)low;(void)high;CHECK(heap_step==1);heap_step=2;}
void _HSD_IDForgetMemory(void* low,void* high){(void)low;(void)high;CHECK(heap_step==2);heap_step=3;}
void _HSD_ObjAllocForgetMemory(void* low,void* high){(void)low;(void)high;CHECK(heap_step==3);registry_live=false;heap_step=4;}
void _HSD_RObjForgetMemory(void* low,void* high){(void)low;(void)high;CHECK(heap_step==4);heap_step=5;}
OSHeapHandle HSD_GetHeap(void){return heap;}
void HSD_SetHeap(OSHeapHandle value){heap=value;}
void OSDestroyHeap(int value){CHECK(value==heap&&heap_step==5&&registry_live);heap_step=6;}
int OSCreateHeap(void* low,void* high){CHECK(low==hsd_heap_next_arena_lo&&high==hsd_heap_next_arena_hi&&heap_step==6);heap_step=7;return 2;}
int OSSetCurrentHeap(int value){CHECK(value==heap&&heap_step==7);heap_step=8;return 1;}
void HSD_ObjSetHeap(u32 size,void* pointer){CHECK(size==0x10000&&pointer==NULL&&heap_step==8);heap_step=9;++heap_resets;}
// Full actual preload function runs below. Its unchanged-cache branch must not
// reach any of these separately owned asset/heap stages.
#define PRELOAD_STAGE(name) static void name(void){++preload_stages;CHECK(false);}
PRELOAD_STAGE(inline_preload_entries) PRELOAD_STAGE(inline1) PRELOAD_STAGE(inline2)
PRELOAD_STAGE(lbDvd_80017960) PRELOAD_STAGE(inline_pad) PRELOAD_STAGE(inline_cleanup_entries) PRELOAD_STAGE(lbDvd_80017CC4)
void* HSD_IDGetDataFromTable(HSD_IDTable* table,uintptr_t id,s32* success)
{(void)table;if(success)*success=id==1;CHECK(id==1||id==2);return id==1?(HSD_Obj*)&target:NULL;}
HSD_JObj* HSD_JObjLoadJoint(HSD_Joint* desc){CHECK(desc==(void*)2);++fallback_loads;return &target;}
void HSD_JObjUnref(HSD_JObj* joint){CHECK(joint==&target&&target.object.ref_count>0);--target.object.ref_count;++refs;}
// Model flag propagation is outside this decoder/constructor fixture.
void HSD_JObjSetFlags(HSD_JObj* joint,u32 flags){CHECK(joint==&target);joint->flags|=flags;}
void HSD_JObjClearFlags(HSD_JObj* joint,u32 flags){CHECK(joint==&target);joint->flags&=~flags;}

#include "heap.inc"
#include "hermite.inc"
#include "snapshot_layout.inc"
static void frame(HSD_AObj* a);

static size_t unchanged_save(void)
{
    size_t bytes;CHECK(nativeAnimationPool_Save(before,sizeof(before),&bytes));return bytes;
}
static size_t full_owner(uint8_t* output)
{
    const void* spans[]={&nativeAObjPool,&nativeFObjPool,&nativeAnimationGlobals,
        nativeAObjPool.payload,nativeFObjPool.payload,nativeAObjPool.slots,nativeFObjPool.slots,
        nativeAObjPool.free_order,nativeFObjPool.free_order};
    const size_t sizes[]={sizeof(nativeAObjPool),sizeof(nativeFObjPool),sizeof(nativeAnimationGlobals),
        nativeAObjPool.capacity*nativeAObjPool.stride,nativeFObjPool.capacity*nativeFObjPool.stride,
        NATIVE_AOBJ_CAPACITY*sizeof(NativeMatchPoolSlot),NATIVE_FOBJ_CAPACITY*sizeof(NativeMatchPoolSlot),
        NATIVE_AOBJ_CAPACITY*sizeof(uint32_t),NATIVE_FOBJ_CAPACITY*sizeof(uint32_t)};
    size_t bytes=0;for(unsigned i=0;i<sizeof(spans)/sizeof(spans[0]);++i){memcpy(output+bytes,spans[i],sizes[i]);bytes+=sizes[i];}
    CHECK(bytes<=sizeof(game_before));return bytes;
}
static void reject_unchanged(size_t bytes,size_t candidate_bytes)
{
    size_t old=unchanged_save(),all=full_owner(game_before);CHECK(!nativeAnimationPool_ValidateSnapshot(invalid,candidate_bytes));
    CHECK(!nativeAnimationPool_Load(invalid,candidate_bytes));size_t now;
    CHECK(full_owner(game_after)==all&&!memcmp(game_before,game_after,all));
    CHECK(nativeAnimationPool_Save(packed,sizeof(packed),&now)&&now==old&&!memcmp(packed,before,now));
    CHECK(bytes<=sizeof(invalid));++cases;
}
static uint8_t* first_record(uint8_t* data,NativeMatchPoolSnapshotHeader* header)
{
    memcpy(header,data,sizeof(*header));
    return data+sizeof(*header)+(header->capacity+header->free_count)*sizeof(uint32_t)+sizeof(uint32_t);
}
static void owner_cases(HSD_FObjDesc* track,HSD_AObjDesc* recipe)
{
    unsigned initial_cases=cases;
    CHECK(nativeAObjPool.used==0&&nativeFObjPool.used==0);
    size_t empty;CHECK(nativeAnimationPool_Save(packed,sizeof(packed),&empty));
    CHECK(empty==sizeof(AnimationSnapshotHeader)+2*sizeof(NativeMatchPoolSnapshotHeader)+
        8*(NATIVE_AOBJ_CAPACITY+NATIVE_FOBJ_CAPACITY));
    HSD_AObj* first=HSD_AObjAlloc();HSD_FObj* loose=HSD_FObjAlloc();loose->p0=123;
    CHECK(nativeAnimationPool_Load(packed,empty)&&nativeAObjPool.used==0&&nativeFObjPool.used==0);++cases;
    CHECK(HSD_AObjAlloc()==first&&HSD_FObjAlloc()==loose&&loose->p0==0);
    HSD_AObjFree(first);HSD_FObjFree(loose);

    target=(HSD_JObj){0};target.object.ref_count=1;recipe->obj_id=1;recipe->flags=AOBJ_LOOP;
    HSD_FObjDesc second=*track;second.next=NULL;track->next=&second;
    HSD_AObj* a=HSD_AObjLoadDesc(recipe);HSD_FObj* x=a->fobj,*y=x->next;
    HSD_AObj* b=HSD_AObjAlloc();HSD_FObj* z=HSD_FObjAlloc();b->fobj=z;
    HSD_AObjReqAnim(a,0);frame(a);HSD_JObj saved_target=target;
    u32 saved_rng=*seed_ptr;unsigned saved_updates=updates,saved_ends=ends;
    NativeMatchPoolReference ar,yr;CHECK(nativeMatchPool_Reference(&nativeAObjPool,a,&ar));
    CHECK(nativeMatchPool_Reference(&nativeFObjPool,y,&yr));
    size_t bytes;CHECK(nativeAnimationPool_Save(packed,sizeof(packed),&bytes));
    CHECK(nativeAnimationPool_ValidateSnapshot(packed,bytes));
    HSD_AObjRemove(a);CHECK(target.object.ref_count==1&&nativeMatchPool_Resolve(ar)==NULL);
    HSD_AObj* reused=HSD_AObjAlloc();HSD_FObj* reused_x=HSD_FObjAlloc(),*reused_y=HSD_FObjAlloc();
    CHECK(reused==a&&reused_x==x&&reused_y==y&&nativeMatchPool_Resolve(yr)==NULL);
    CHECK(nativeAnimationPool_Load(packed,bytes));
    CHECK(nativeMatchPool_Resolve(ar)==a&&nativeMatchPool_Resolve(yr)==y&&a->fobj==x&&x->next==y&&b->fobj==z);
    // External target ownership is restored by its own owner/page snapshot.
    target=saved_target;*seed_ptr=saved_rng;updates=saved_updates;ends=saved_ends;
    HSD_AObjRemove(a);HSD_AObj* again=HSD_AObjAlloc();HSD_FObj* again_x=HSD_FObjAlloc(),*again_y=HSD_FObjAlloc();
    CHECK(again==reused&&again_x==reused_x&&again_y==reused_y);
    HSD_AObjFree(again);HSD_FObjFree(again_x);HSD_FObjFree(again_y);HSD_AObjRemove(b);++cases;

    // An unknown registry ID leaves the animation unattached.
    track->next=NULL;recipe->obj_id=2;target.object.ref_count=1;
    a=HSD_AObjLoadDesc(recipe);CHECK(!fallback_loads&&!a->hsd_obj);
    HSD_AObjRemove(a);
    // A relocated joint owns the newly loaded target's initial ref.
    recipe->obj_id_is_joint=true;
    a=HSD_AObjLoadDesc(recipe);CHECK(fallback_loads==1&&a->hsd_obj==(HSD_Obj*)&target&&target.object.ref_count==1);
    HSD_AObjRemove(a);CHECK(target.object.ref_count==0);recipe->obj_id=1;recipe->obj_id_is_joint=false;++cases;

    target.object.ref_count=1;a=HSD_AObjLoadDesc(recipe);HSD_AObjReqAnim(a,0);probe_callback=true;frame(a);probe_callback=false;++cases;
    b=HSD_AObjAlloc();HSD_FObj* foreign=HSD_FObjAlloc();
    CHECK(nativeAnimationPool_Save(packed,sizeof(packed),&bytes));
    AnimationSnapshotHeader h;memcpy(&h,packed,sizeof(h));
    NativeMatchPoolSnapshotHeader ah,fh;
    size_t a_offset=(size_t)(first_record(packed+sizeof(h),&ah)-packed);
    size_t f_offset=(size_t)(first_record(packed+sizeof(h)+h.controller_bytes,&fh)-packed);
#define BAD(change, length) do{memcpy(invalid,packed,bytes);change;reject_unchanged(bytes,length);}while(0)
#define FIELD(type, offset, member, value) do{type record;memcpy(&record,invalid+(offset),sizeof(record));record.member=(value);memcpy(invalid+(offset),&record,sizeof(record));}while(0)
    BAD(FIELD(AnimationSnapshotHeader,0,finished,-1),bytes);
    BAD(FIELD(AnimationSnapshotHeader,0,heap_epoch,h.heap_epoch+1),bytes);
    BAD(FIELD(AnimationSnapshotHeader,0,decoder_bytes,h.decoder_bytes+1),bytes);
    BAD(FIELD(NativeMatchPoolSnapshotHeader,sizeof(h),pool,&nativeFObjPool),bytes);
    BAD(FIELD(HSD_AObj,a_offset,fobj,(void*)a),bytes);
    BAD(FIELD(HSD_FObj,f_offset,next,(void*)b),bytes);
    HSD_FObj* record_pointer=(HSD_FObj*)(nativeFObjPool.payload+nativeFObjPool.stride*(*(uint32_t*)(packed+f_offset-sizeof(uint32_t))));
    BAD(FIELD(HSD_FObj,f_offset,next,record_pointer),bytes);
    BAD(FIELD(HSD_FObj,f_offset,next,(void*)(nativeFObjPool.payload+nativeFObjPool.stride*(NATIVE_FOBJ_CAPACITY-1))),bytes);
    BAD(FIELD(HSD_FObj,f_offset,ad,(void*)((uintptr_t)track->ad+track->length+1)),bytes);
    BAD(FIELD(HSD_FObj,f_offset,flags,15),bytes);
    BAD(FIELD(HSD_FObj,f_offset,op,7),bytes);
    BAD(FIELD(HSD_AObj,a_offset,curr_frame,1234);FIELD(HSD_FObj,f_offset,op,7),bytes);
    BAD((void)0,bytes-1);
    uint8_t* free_ids=packed+sizeof(h)+h.controller_bytes+sizeof(fh)+fh.capacity*sizeof(uint32_t);
    size_t free_offset=(size_t)(free_ids-packed);
    BAD(memcpy(invalid+free_offset+4,invalid+free_offset,4),bytes);
#undef BAD
#undef FIELD
    CHECK(!nativeAnimationPool_Save(packed,bytes-1,&(size_t){0}));
    CHECK(!nativeAnimationPool_Save(nativeAObjPool.payload,sizeof(HSD_AObj),&(size_t){0}));
    CHECK(!nativeAnimationPool_Save(packed,sizeof(packed),(size_t*)packed));++cases;
    // Corrupted live metadata/links reject save, while repair restores validity.
    HSD_FObj* prior=a->fobj->next;a->fobj->next=a->fobj;CHECK(!nativeAnimationPool_Save(packed,sizeof(packed),&(size_t){0}));a->fobj->next=prior;
    size_t unpublished=SIZE_MAX;
    uint32_t used=nativeAObjPool.used;++nativeAObjPool.used;
    CHECK(!nativeAnimationPool_Save(packed,sizeof(packed),&unpublished) && unpublished==SIZE_MAX);
    nativeAObjPool.used=used;
    u8 operation=a->fobj->op;a->fobj->op=7;
    CHECK(!nativeAnimationPool_Save(packed,sizeof(packed),&unpublished) && unpublished==SIZE_MAX);
    a->fobj->op=operation;
    CHECK(nativeAnimationPool_Save(packed,sizeof(packed),&unpublished));
    CHECK(nativeAnimationPool_ValidateSnapshot(packed,unpublished));++cases;
    uint64_t epoch=nativeAnimationGlobals.heap_epoch;nativeAnimationGlobals.heap_epoch=UINT64_MAX;
    CHECK(!nativeAnimationPool_CanBeginHeap()&&!nativeAnimationPool_BeginHeap());nativeAnimationGlobals.heap_epoch=epoch;
    HSD_GObj_CurrentInvokedProcGObj=(void*)1;
    CHECK(!nativeAnimationPool_CanBeginHeap()&&!nativeAnimationPool_BeginHeap()&&!nativeAnimationPool_Load(packed,bytes));
    HSD_GObj_CurrentInvokedProcGObj=NULL;HSD_GObj_DelayedProcInfo.flags=1;
    CHECK(!nativeAnimationPool_CanBeginHeap());HSD_GObj_DelayedProcInfo.flags=0;++cases;
    HSD_AObjRemove(a);HSD_AObjFree(b);HSD_FObjFree(foreign);
    a=HSD_AObjAlloc();HSD_AObjFree(a);expect_panic=true;
    if(setjmp(panic_jump)==0){HSD_AObjFree(a);CHECK(false);}expect_panic=false;
    foreign=HSD_FObjAlloc();expect_panic=true;
    if(setjmp(panic_jump)==0){HSD_AObjFree((HSD_AObj*)foreign);CHECK(false);}expect_panic=false;
    CHECK(nativeAObjPool.used==0&&nativeFObjPool.used==1);HSD_FObjFree(foreign);++cases;

    // Exhaustion is tested through the actual constructors, including their
    // explicit panic branch; no legacy allocation is available to fall back to.
    static HSD_AObj* all_a[NATIVE_AOBJ_CAPACITY];static HSD_FObj* all_f[NATIVE_FOBJ_CAPACITY];
    for(unsigned i=0;i<NATIVE_AOBJ_CAPACITY;++i)all_a[i]=HSD_AObjAlloc();
    for(unsigned i=0;i<NATIVE_FOBJ_CAPACITY;++i)all_f[i]=HSD_FObjAlloc();
    CHECK(nativeAnimationPool_Save(packed,sizeof(packed),&bytes));
    CHECK(bytes==empty+NATIVE_AOBJ_CAPACITY*sizeof(HSD_AObj)+NATIVE_FOBJ_CAPACITY*sizeof(HSD_FObj));
    memcpy(before,packed,bytes);expect_panic=true;
    if(setjmp(panic_jump)==0){HSD_AObjAlloc();CHECK(false);}expect_panic=false;
    expect_panic=true;if(setjmp(panic_jump)==0){HSD_FObjAlloc();CHECK(false);}expect_panic=false;
    size_t unchanged;CHECK(nativeAnimationPool_Save(packed,sizeof(packed),&unchanged)&&unchanged==bytes&&!memcmp(packed,before,bytes));
    for(unsigned i=0;i<NATIVE_AOBJ_CAPACITY;++i)HSD_AObjFree(all_a[i]);
    for(unsigned i=0;i<NATIVE_FOBJ_CAPACITY;++i)HSD_FObjFree(all_f[i]);
    CHECK(nativeAnimationPool_Load(before,bytes)&&nativeAObjPool.used==NATIVE_AOBJ_CAPACITY&&nativeFObjPool.used==NATIVE_FOBJ_CAPACITY);
    for(unsigned i=0;i<NATIVE_AOBJ_CAPACITY;++i)HSD_AObjFree(all_a[i]);
    for(unsigned i=0;i<NATIVE_FOBJ_CAPACITY;++i)HSD_FObjFree(all_f[i]);++cases;

    // The actual production global-region accessor must see backing and metadata.
    NativeRollbackStateRegion region;CHECK(nativeRollbackState_GetGlobalRegions(&region,1)==1&&region.size<=sizeof(oracle));
#define IN_ORACLE(pointer, span) CHECK((uintptr_t)(pointer)>=(uintptr_t)region.memory&&(uintptr_t)(pointer)+(span)<=(uintptr_t)region.memory+region.size)
    CHECK((uintptr_t)nativeAObjPool.payload>=(uintptr_t)nativeAnimationStorage.memory &&
        (uintptr_t)nativeFObjPool.payload+nativeFObjPool.stride*nativeFObjPool.capacity<=
        (uintptr_t)nativeAnimationStorage.memory+nativeAnimationStorage.bytes);
    IN_ORACLE(nativeAObjPool.slots,sizeof(NativeMatchPoolSlot)*NATIVE_AOBJ_CAPACITY);
    IN_ORACLE(nativeFObjPool.slots,sizeof(NativeMatchPoolSlot)*NATIVE_FOBJ_CAPACITY);
    IN_ORACLE(nativeAObjPool.free_order,sizeof(uint32_t)*NATIVE_AOBJ_CAPACITY);
    IN_ORACLE(nativeFObjPool.free_order,sizeof(uint32_t)*NATIVE_FOBJ_CAPACITY);
    IN_ORACLE(&nativeAObjPool,sizeof(nativeAObjPool));IN_ORACLE(&nativeFObjPool,sizeof(nativeFObjPool));
    IN_ORACLE(&nativeAnimationGlobals,sizeof(nativeAnimationGlobals));
#undef IN_ORACLE
    a=HSD_AObjAlloc();foreign=HSD_FObjAlloc();a->fobj=foreign;foreign->p0=42;
    memcpy(oracle,region.memory,region.size);
    CHECK(nativeAnimationStorage.bytes<=sizeof(animation_oracle));
    memcpy(animation_oracle,nativeAnimationStorage.memory,nativeAnimationStorage.bytes);HSD_AObjRemove(a);b=HSD_AObjAlloc();CHECK(b==a);
    memcpy(region.memory,oracle,region.size);
    memcpy(nativeAnimationStorage.memory,animation_oracle,nativeAnimationStorage.bytes);CHECK(a->fobj==foreign&&foreign->p0==42&&nativeAObjPool.used==1&&nativeFObjPool.used==1);
    HSD_AObjRemove(a);++cases;

    // Heap reset bulk-retires still occupied graph storage without destructor
    // callbacks. Existing snapshots and generation refs become wrong-epoch.
    target.object.ref_count=1;a=HSD_AObjLoadDesc(recipe);CHECK(nativeAnimationPool_Save(packed,sizeof(packed),&bytes));
    CHECK(nativeMatchPool_Reference(&nativeAObjPool,a,&ar));unsigned prior_refs=refs;
    int32_t prior_finished=nativeAnimationGlobals.finished,prior_active=nativeAnimationGlobals.active;
    heap_step=0;CHECK(HSD_CreateMainHeap(NULL,NULL)==2&&heap_step==9&&heap_resets==1);
    CHECK(nativeAnimationGlobals.heap_epoch==epoch+1&&nativeAObjPool.used==0&&nativeFObjPool.used==0&&refs==prior_refs&&nativeAnimationGlobals.end_callbacks==NULL);
    CHECK(nativeAnimationGlobals.finished==prior_finished&&nativeAnimationGlobals.active==prior_active);
    CHECK(nativeMatchPool_Resolve(ar)==NULL&&!nativeAnimationPool_Load(packed,bytes));++cases;
    // A rejected actual heap reset must fail before class/heap mutation.
    nativeAnimationGlobals.callback_depth=1;heap_step=0;expect_panic=true;
    if(setjmp(panic_jump)==0){HSD_CreateMainHeap(NULL,NULL);CHECK(false);}expect_panic=false;
    CHECK(heap_step==0&&heap_resets==1&&nativeAnimationGlobals.heap_epoch==epoch+1);nativeAnimationGlobals.callback_depth=0;++cases;
    // Real unchanged-preload early return skips heap recreation; owner identity
    // must remain live. The other asset/heap stages are explicitly trapped.
    a=HSD_AObjAlloc();ar=(NativeMatchPoolReference){0};CHECK(nativeMatchPool_Reference(&nativeAObjPool,a,&ar));
    lbDvd_80018254();CHECK(preload_stages==0&&nativeMatchPool_Resolve(ar)==a&&nativeAnimationGlobals.heap_epoch==epoch+1);
    HSD_AObjFree(a);++cases;
    printf("{\"owner_cases\":%u,\"empty_packed_bytes\":%zu,\"full_packed_bytes\":%zu,\"heap_resets\":%u,\"fallback_loads\":%u,\"panics_trapped\":%u,\"oracle_bytes\":%zu}\n",cases-initial_cases,empty,empty+NATIVE_AOBJ_CAPACITY*sizeof(HSD_AObj)+NATIVE_FOBJ_CAPACITY*sizeof(HSD_FObj),heap_resets,fallback_loads,panic_count,region.size);
}

static void update(void* object,enum_t channel,HSD_ObjData* value)
{
    CHECK(object==&target&&(channel==HSD_A_J_TRAX||channel==HSD_A_J_ROTX));
    if(probe_callback){CHECK(!nativeAnimationPool_CanBeginHeap()&&!nativeAnimationPool_BeginHeap());
        CHECK(!nativeAnimationPool_Save(packed,sizeof(packed),&(size_t){0}));}
    if(channel==HSD_A_J_TRAX)target.translate.x=value->fv;else target.rotate.x=value->fv;
    target.flags|=JOBJ_MTX_DIRTY;++updates;(void)HSD_Rand();
}
static void ended(void){CHECK(nativeAnimationGlobals.callback_depth>0);++ends;(void)HSD_Rand();}
typedef struct State {
    HSD_AObj controller;HSD_FObj decoder;HSD_JObj joint;
    u32 rng;s32 ended,active;unsigned updates,ends;
} State;
static State capture(HSD_AObj* a)
{
    State s;memset(&s,0,sizeof(s));s.controller=*a;s.decoder=*a->fobj;s.joint=target;
    s.rng=*seed_ptr;s.ended=nativeAnimationGlobals.finished;s.active=nativeAnimationGlobals.active;s.updates=updates;s.ends=ends;return s;
}
static void restore(HSD_AObj* a,const State* s)
{
    *a=s->controller;*a->fobj=s->decoder;target=s->joint;*seed_ptr=s->rng;
    nativeAnimationGlobals.finished=s->ended;nativeAnimationGlobals.active=s->active;updates=s->updates;ends=s->ends;
}
static void frame(HSD_AObj* a)
{
    HSD_AObjInitEndCallBack();HSD_AObjInterpretAnim(a,&target,update);HSD_AObjInvokeCallBacks();
}
static void float_bytes(uint8_t* output,float value){memcpy(output,&value,4);}
// Copy the actual complete decoder/controller owner without serializers or
// rebinding, then replay the source interpreter including callback/RNG state.
static void copy_history(HSD_FObjDesc* track,HSD_AObjDesc* recipe)
{
    CHECK(nativeAnimationPool_BeginHeap());
    CHECK(nativeHsdArena_ResetHistory());
    target=(HSD_JObj){0};target.object.ref_count=1;updates=ends=0;*seed_ptr=0x12345678;
    recipe->flags=AOBJ_LOOP;recipe->fobjdesc=track;
    HSD_AObj* a=HSD_AObjLoadDesc(recipe);CHECK(a&&a->fobj);HSD_AObjReqAnim(a,0);
    for(unsigned i=0;i<3;++i)frame(a);
    NativeMatchArena* arena=&nativeHsdArena;NativeMatchArenaFrame saved;
    CHECK(nativeAnimationPool_AdmitHistory(arena));
    CHECK(nativeMatchArena_AddRoot(arena,&target,sizeof(target)));
    CHECK(nativeMatchArena_AddRoot(arena,seed_ptr,sizeof(*seed_ptr)));
    CHECK(nativeMatchArena_AddRoot(arena,&updates,sizeof(updates)));
    CHECK(nativeMatchArena_AddRoot(arena,&ends,sizeof(ends)));
    CHECK(nativeMatchArena_BindFrame(arena,&saved,arena_saved,sizeof(arena_saved)));
    CHECK(nativeMatchArena_Save(&saved));
    size_t copied=saved.bytes;for(unsigned i=0;i<arena->root_count;++i)copied+=saved.root_bytes[i];
    CHECK(copied<300000); // Metadata plus one initialized controller/decoder.
    for(unsigned i=0;i<8;++i)frame(a);State expected=capture(a);
    for(unsigned round=0;round<2;++round){
        CHECK(nativeMatchArena_Load(&saved));
        for(unsigned i=0;i<8;++i)frame(a);State replay=capture(a);
        CHECK(!memcmp(&expected,&replay,sizeof(replay)));
    }
    CHECK(nativeMatchArena_Load(&saved));NativeMatchPoolReference future_a,future_f;
    HSD_AObj* next_a=HSD_AObjAlloc();HSD_FObj* next_f=HSD_FObjAlloc();
    CHECK(nativeMatchPool_Reference(&nativeAObjPool,next_a,&future_a));
    CHECK(nativeMatchPool_Reference(&nativeFObjPool,next_f,&future_f));
    memset(next_a,0xFE,sizeof(*next_a));memset(next_f,0xFE,sizeof(*next_f));
    CHECK(nativeMatchArena_Load(&saved));
    CHECK(nativeAObjPool.initialized_count==1&&nativeFObjPool.initialized_count==1);
    CHECK(!nativeMatchPool_Resolve(future_a)&&!nativeMatchPool_Resolve(future_f));
    CHECK(HSD_AObjAlloc()==next_a&&HSD_FObjAlloc()==next_f);
    NativeMatchPoolReference replay_a,replay_f;
    CHECK(nativeMatchPool_Reference(&nativeAObjPool,next_a,&replay_a)&&replay_a.generation==future_a.generation);
    CHECK(nativeMatchPool_Reference(&nativeFObjPool,next_f,&replay_f)&&replay_f.generation==future_f.generation);
    CHECK(next_a->fobj==NULL&&next_f->next==NULL);
    HSD_AObjFree(next_a);HSD_FObjFree(next_f);HSD_AObjRemove(a);
    // Heap recreation invalidates prior private history even at reused addresses.
    CHECK(nativeHsdArena_BeginHeap());CHECK(nativeAnimationPool_BeginHeap());
    CHECK(!nativeMatchArena_Load(&saved));++cases;
}

int main(void)
{
    CHECK(nativeHsdArena_BeginHeap());
    CHECK(nativeAnimationPool_CanBeginHeap());HSD_ObjInit();CHECK(init_calls==8&&nativeAnimationGlobals.heap_epoch==1);
    uint8_t* stream=VirtualAlloc(NULL,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);CHECK(stream);
    // Three LINEAR packet values, two waits. Actual decoder consumes/updates
    // packet count, data cursor, interpolation slope/state and term history.
    stream[0]=0x22;float_bytes(stream+1,0);stream[5]=4;
    float_bytes(stream+6,10);stream[10]=4;float_bytes(stream+11,20);
    // Same value packets exercise CONSTANT, SPL0 and KEY. SPL adds explicit
    // nonzero slopes, exposing saved Hermite state as well as stream cursors.
    const unsigned opcodes[]={HSD_A_OP_CON,HSD_A_OP_LIN,HSD_A_OP_SPL0,HSD_A_OP_SPL,HSD_A_OP_KEY};
    const unsigned offsets[]={32,0,64,96,160};
    for(unsigned shape=0;shape<5;++shape){
        uint8_t* bytes=stream+offsets[shape];bytes[0]=(uint8_t)(0x20|opcodes[shape]);unsigned pos=1;
        for(unsigned value=0;value<3;++value){float_bytes(bytes+pos,(float)(value*10));pos+=4;
            if(opcodes[shape]==HSD_A_OP_SPL){float_bytes(bytes+pos,value==1?-2.f:3.f);pos+=4;}
            if(value<2)bytes[pos++]=4;
        }
        CHECK(pos==(opcodes[shape]==HSD_A_OP_SPL?27u:15u));
    }
    DWORD prior;CHECK(VirtualProtect(stream,4096,PAGE_READONLY,&prior));
    HSD_FObjDesc track={.length=15,.type=HSD_A_J_TRAX,.frac_value=HSD_A_FRAC_FLOAT,.ad=stream};
    HSD_AObjDesc recipe={.flags=0,.end_frame=8,.fobjdesc=&track,.obj_id=1};
    HSD_SList callback={.data=ended};nativeAnimationGlobals.end_callbacks=&callback;
    for(unsigned shape=0;shape<5;++shape)for(unsigned looping=0;looping<2;++looping)for(unsigned rate=0;rate<3;++rate){
        track.ad=stream+offsets[shape];track.length=opcodes[shape]==HSD_A_OP_SPL?27:15;
        target=(HSD_JObj){0};target.object.ref_count=1;updates=ends=0;*seed_ptr=0x12345678;
        recipe.flags=looping?AOBJ_LOOP:0;HSD_AObj* a=HSD_AObjLoadDesc(&recipe);CHECK(a&&a->fobj&&target.object.ref_count==2);
        HSD_AObjSetRate(a,rate==0?.5f:rate==1?1.f:2.f);HSD_AObjReqAnim(a,0);
        for(unsigned i=0;i<3;++i)frame(a);
        State saved=capture(a);CHECK(a->fobj->ad>track.ad&&a->fobj->ad<=track.ad+track.length);
        size_t bytes;CHECK(nativeAnimationPool_Save(packed,sizeof(packed),&bytes));
        for(unsigned i=0;i<8;++i)frame(a);State expected=capture(a);
        CHECK(nativeAnimationPool_Load(packed,bytes));target=saved.joint;*seed_ptr=saved.rng;updates=saved.updates;ends=saved.ends;
        for(unsigned i=0;i<8;++i)frame(a);State replay=capture(a);
        CHECK(memcmp(&expected,&replay,sizeof(expected))==0);++cases;
        // Current-frame-only restoration leaves consumed decoder/callback/RNG
        // state behind. It cannot reproduce the same next eight frames.
        a->curr_frame=saved.controller.curr_frame;
        for(unsigned i=0;i<8;++i)frame(a);State incomplete=capture(a);
        CHECK(memcmp(&expected,&incomplete,sizeof(expected))!=0);++cases;
        HSD_AObjRemove(a);CHECK(target.object.ref_count==1&&nativeAObjPool.used==0&&nativeFObjPool.used==0);
    }
    // Isolate decoder ownership: restore target, RNG, counters and the complete
    // AObj, but leave only the consumed FObj behind. Even a short nonloop replay
    // differs, independently of stale callback counters or RNG.
    target=(HSD_JObj){0};target.object.ref_count=1;updates=ends=0;recipe.flags=0;track.ad=stream;track.length=15;
    HSD_AObj* isolated=HSD_AObjLoadDesc(&recipe);HSD_AObjReqAnim(isolated,0);
    for(unsigned i=0;i<3;++i)frame(isolated);State start=capture(isolated);
    for(unsigned i=0;i<2;++i)frame(isolated);State correct=capture(isolated);
    for(unsigned i=0;i<4;++i)frame(isolated);HSD_FObj consumed=*isolated->fobj;
    restore(isolated,&start);*isolated->fobj=consumed;
    for(unsigned i=0;i<2;++i)frame(isolated);State decoder_omitted=capture(isolated);
    CHECK(correct.joint.translate.x!=decoder_omitted.joint.translate.x);
    HSD_AObjRemove(isolated);++cases;
    // NO_UPDATE still consumes animation/cursors while suppressing mutation
    // callbacks. These decoder writes cannot be discarded as drawing scratch.
    target=(HSD_JObj){0};target.object.ref_count=1;updates=ends=0;
    recipe.flags=AOBJ_LOOP;HSD_AObj* a=HSD_AObjLoadDesc(&recipe);HSD_AObjSetFlags(a,AOBJ_NO_UPDATE);HSD_AObjReqAnim(a,0);
    uint8_t* cursor=a->fobj->ad;u32 rng=*seed_ptr;frame(a);
    CHECK(a->fobj->ad!=cursor&&updates==0&&*seed_ptr==rng);
    HSD_AObjRemove(a);++cases;
    // Real fighter FigaTree constructors take a second direct allocation path,
    // install ordered channels, remove an existing AObj, and optionally filter
    // translation from the interpolation tree.
    FigaTree tree={.type=1,.flags=AOBJ_LOOP,.frames=8};
    FigaTrack channels[2]={{.length=15,.obj_type=HSD_A_J_TRAX,.frac_value=HSD_A_FRAC_FLOAT,.ad_head=stream},
        {.length=15,.obj_type=HSD_A_J_ROTX,.frac_value=HSD_A_FRAC_FLOAT,.ad_head=stream}};
    target=(HSD_JObj){0};target.object.ref_count=1;
    lbAnim_8001E6D8(&target,&tree,channels,2);
    CHECK(target.aobj&&target.aobj->fobj&&target.aobj->fobj->next&&
        target.aobj->fobj->obj_type==HSD_A_J_TRAX&&target.aobj->fobj->next->obj_type==HSD_A_J_ROTX);
    CHECK(nativeAObjPool.used==1&&nativeFObjPool.used==2&&(target.flags&JOBJ_CLASSICAL_SCALE));
    HSD_AObjReqAnim(target.aobj,0);frame(target.aobj);++cases;
    tree.type=0;lbAnim_8001E7E8(&target,&tree,channels,2);
    CHECK(target.aobj&&target.aobj->fobj&&target.aobj->fobj->next==NULL&&target.aobj->fobj->obj_type==HSD_A_J_ROTX);
    CHECK(nativeAObjPool.used==1&&nativeFObjPool.used==1&&!(target.flags&JOBJ_CLASSICAL_SCALE));
    HSD_AObjRemove(target.aobj);target.aobj=NULL;++cases;
    CHECK(refs==32&&nativeAObjPool.used==0&&nativeFObjPool.used==0);
    unsigned decoder_cases=cases;owner_cases(&track,&recipe);
    copy_history(&track,&recipe);
    CHECK(VirtualFree(stream,0,MEM_RELEASE));
    printf("{\"total_cases\":%u,\"decoder_cases\":%u,\"checks\":%u,\"aobj_bytes\":%zu,\"fobj_bytes\":%zu,\"reference_releases\":%u,\"resident_stream_read_only\":true,\"full_state_replay\":true,\"frame_only_replay_diverges\":true,\"decoder_only_omission_diverges\":true,\"decoder_opcodes\":5,\"figatree_constructor_paths\":2}\n",
        cases,decoder_cases,checks,sizeof(HSD_AObj),sizeof(HSD_FObj),refs);
    return 0;
}
