#include "native_animation_pool.h"
#ifdef MELEE_NATIVE
#include <melee/gm/native_match_snapshot_pool.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <string.h>
#include <melee/gm/native_match_arena.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far

// Animation ownership follows heap recreation, independently of scene binds.
// Descriptors, globals and allocator metadata remain mutable game state.
NativeMatchPool nativeAObjPool, nativeFObjPool;
NativeAnimationGlobals nativeAnimationGlobals;
static HSD_AObj* controllers;
static HSD_FObj* decoders;
static NativeMatchPoolSlot controller_slots[NATIVE_AOBJ_CAPACITY];
static NativeMatchPoolSlot decoder_slots[NATIVE_FOBJ_CAPACITY];
static uint32_t controller_free[NATIVE_AOBJ_CAPACITY], decoder_free[NATIVE_FOBJ_CAPACITY];
_Static_assert(NATIVE_AOBJ_CAPACITY <= NATIVE_MATCH_POOL_MAX_SLOTS, "AObj bound");
_Static_assert(NATIVE_FOBJ_CAPACITY <= NATIVE_MATCH_POOL_MAX_SLOTS, "FObj bound");

typedef struct AnimationSnapshotHeader {
    uint64_t heap_epoch;
    size_t controller_bytes, decoder_bytes;
    HSD_SList* end_callbacks;
    int32_t finished, active;
} AnimationSnapshotHeader;

static bool overlaps(const void* a, size_t an, const void* b, size_t bn)
{
    uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    if(an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y)return true;
    return x<y+bn && y<x+an;
}
static bool boundary(void)
{
    return nativeAnimationGlobals.callback_depth==0 &&
        HSD_GObj_CurrentInvokedProcGObj==NULL && HSD_GObj_CurrentInvokedProc==NULL &&
        HSD_GObj_804D7818==NULL && HSD_GObj_DelayedProcInfo.flags==0;
}
static bool fixed_storage(void)
{
    // The same fixed spans passed to both Bind calls are preflighted together.
    // There is no caller-selected backing or second-stage allocation to fail.
    const void* spans[]={&nativeAObjPool,&nativeFObjPool,controllers,decoders,
        controller_slots,decoder_slots,controller_free,decoder_free};
    const size_t bytes[]={sizeof(nativeAObjPool),sizeof(nativeFObjPool),(NATIVE_AOBJ_CAPACITY*sizeof(*controllers)),
        (NATIVE_FOBJ_CAPACITY*sizeof(*decoders)),sizeof(controller_slots),sizeof(decoder_slots),
        sizeof(controller_free),sizeof(decoder_free)};
    if(nativeMatchPool_Stride(sizeof(HSD_AObj),_Alignof(HSD_AObj))!=sizeof(HSD_AObj) ||
       nativeMatchPool_Stride(sizeof(HSD_FObj),_Alignof(HSD_FObj))!=sizeof(HSD_FObj))return false;
    for(unsigned i=0;i<sizeof(spans)/sizeof(spans[0]);++i){
        if(bytes[i]>UINTPTR_MAX-(uintptr_t)spans[i])return false;
        for(unsigned j=i+1;j<sizeof(spans)/sizeof(spans[0]);++j)
            if(overlaps(spans[i],bytes[i],spans[j],bytes[j]))return false;
    }
    return (uintptr_t)controllers%_Alignof(HSD_AObj)==0 &&
        (uintptr_t)decoders%_Alignof(HSD_FObj)==0 &&
        (uintptr_t)controller_slots%_Alignof(NativeMatchPoolSlot)==0 &&
        (uintptr_t)decoder_slots%_Alignof(NativeMatchPoolSlot)==0 &&
        (uintptr_t)controller_free%_Alignof(uint32_t)==0 &&
        (uintptr_t)decoder_free%_Alignof(uint32_t)==0;
}
// Reservation identity is host state. Payload and metadata are captured by
// counted spans, and backing is tracked separately by the broad oracle.
#pragma clang section data = "" bss = ""
NativeAnimationStorage nativeAnimationStorage;
static bool reserve_storage(void)
{
    if(nativeAnimationStorage.memory)return true;
    size_t decoder_offset=(NATIVE_AOBJ_CAPACITY*sizeof(HSD_AObj)+63)&~(size_t)63;
    size_t bytes=decoder_offset+NATIVE_FOBJ_CAPACITY*sizeof(HSD_FObj);
    uint8_t* memory=VirtualAlloc(NULL,bytes,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    if(!memory)return false;
    memset(memory,0,bytes);
    nativeAnimationStorage=(NativeAnimationStorage){memory,bytes};
    controllers=(HSD_AObj*)memory;decoders=(HSD_FObj*)(memory+decoder_offset);
    return true;
}
bool nativeAnimationPool_CanBeginHeap(void)
{
    return boundary() && nativeAnimationGlobals.heap_epoch!=UINT64_MAX &&
        (!nativeAnimationStorage.memory || fixed_storage());
}
bool nativeAnimationPool_BeginHeap(void)
{
    if(!nativeAnimationPool_CanBeginHeap() || !reserve_storage() || !fixed_storage())return false;
    uint64_t epoch=nativeAnimationGlobals.heap_epoch+1;
    // Complete pair preflight makes both fixed bindings infallible here.
    bool a=nativeMatchPool_Bind(&nativeAObjPool,controllers,(NATIVE_AOBJ_CAPACITY*sizeof(*controllers)),
        controller_slots,controller_free,NATIVE_AOBJ_CAPACITY,sizeof(HSD_AObj),_Alignof(HSD_AObj),epoch);
    bool f=nativeMatchPool_Bind(&nativeFObjPool,decoders,(NATIVE_FOBJ_CAPACITY*sizeof(*decoders)),
        decoder_slots,decoder_free,NATIVE_FOBJ_CAPACITY,sizeof(HSD_FObj),_Alignof(HSD_FObj),epoch);
    if(!a || !f)return false;
    // ObjInit previously reset allocation ownership only. Counter reset and
    // callback-list forget remain at their actual AObj source call sites.
    nativeAnimationGlobals.heap_epoch=epoch;
    return true;
}
static bool typed_shape(void)
{
    return nativeAnimationGlobals.heap_epoch!=0 &&
        nativeAObjPool.payload==(uint8_t*)controllers && nativeAObjPool.slots==controller_slots &&
        nativeAObjPool.free_order==controller_free && nativeAObjPool.capacity==NATIVE_AOBJ_CAPACITY &&
        nativeAObjPool.object_size==sizeof(HSD_AObj) && nativeAObjPool.stride==sizeof(HSD_AObj) &&
        nativeAObjPool.lifetime==nativeAnimationGlobals.heap_epoch &&
        nativeFObjPool.payload==(uint8_t*)decoders && nativeFObjPool.slots==decoder_slots &&
        nativeFObjPool.free_order==decoder_free && nativeFObjPool.capacity==NATIVE_FOBJ_CAPACITY &&
        nativeFObjPool.object_size==sizeof(HSD_FObj) && nativeFObjPool.stride==sizeof(HSD_FObj) &&
        nativeFObjPool.lifetime==nativeAnimationGlobals.heap_epoch;
}
bool nativeAnimationPool_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || !boundary() || !typed_shape() ||
       !nativeMatchPool_Validate(&nativeAObjPool) || !nativeMatchPool_Validate(&nativeFObjPool))return false;
    if(!nativeMatchArena_AddRoot(arena,&nativeAnimationGlobals,sizeof(nativeAnimationGlobals)))return false;
    NativeMatchPool* pools[]={&nativeAObjPool,&nativeFObjPool};
    for(unsigned i=0;i<2;++i){
        NativeMatchPool* pool=pools[i];
        if(!nativeMatchArena_AddRoot(arena,pool,sizeof(*pool)) ||
           !nativeMatchArena_AddRoot(arena,pool->slots,pool->capacity*sizeof(*pool->slots)) ||
           !nativeMatchArena_AddRoot(arena,pool->free_order,pool->capacity*sizeof(*pool->free_order)) ||
           !nativeMatchArena_AddCountedRoot(arena,pool->payload,pool->stride,pool->capacity,&pool->initialized_count))return false;
    }
    return true;
}
static bool independent(const void* buffer,size_t bytes)
{
    return buffer!=NULL && bytes<=UINTPTR_MAX-(uintptr_t)buffer &&
        !overlaps(buffer,bytes,&nativeAObjPool,sizeof(nativeAObjPool)) &&
        !overlaps(buffer,bytes,&nativeFObjPool,sizeof(nativeFObjPool)) &&
        !overlaps(buffer,bytes,&nativeAnimationGlobals,sizeof(nativeAnimationGlobals)) &&
        !overlaps(buffer,bytes,controllers,(NATIVE_AOBJ_CAPACITY*sizeof(*controllers))) &&
        !overlaps(buffer,bytes,decoders,(NATIVE_FOBJ_CAPACITY*sizeof(*decoders))) &&
        !overlaps(buffer,bytes,controller_slots,sizeof(controller_slots)) &&
        !overlaps(buffer,bytes,decoder_slots,sizeof(decoder_slots)) &&
        !overlaps(buffer,bytes,controller_free,sizeof(controller_free)) &&
        !overlaps(buffer,bytes,decoder_free,sizeof(decoder_free));
}
static bool decoder_slot(const void* pointer,uint32_t* slot)
{
    uintptr_t p=(uintptr_t)pointer,base=(uintptr_t)decoders;
    if(p<base || p-base>=(NATIVE_FOBJ_CAPACITY*sizeof(*decoders)) || (p-base)%sizeof(HSD_FObj))return false;
    *slot=(uint32_t)((p-base)/sizeof(HSD_FObj));return true;
}
static bool valid_decoder(const HSD_FObj* f)
{
    if((f->flags&0xF)>6 || f->op>HSD_A_OP_KEY || f->op_intrp>HSD_A_OP_KEY)return false;
    // A newly installed track has no cursor until its first ReqAnim.
    if(f->ad==NULL)return true;
    uintptr_t head=(uintptr_t)f->ad_head,ad=(uintptr_t)f->ad;
    return head!=0 && f->length<=UINTPTR_MAX-head && ad>=head && ad<=head+f->length;
}
static bool valid_graph(const uint8_t* a_data,const uint8_t* f_data)
{
    const uint8_t* records[NATIVE_FOBJ_CAPACITY]={0};
    uint8_t color[NATIVE_FOBJ_CAPACITY]={0};
    NativeMatchPoolSnapshotHeader header;memcpy(&header,f_data,sizeof(header));
    const uint8_t* cursor=f_data+sizeof(header)+(header.capacity+header.free_count)*sizeof(uint32_t);
    for(unsigned i=0;i<header.used;++i){
        uint32_t slot;memcpy(&slot,cursor,sizeof(slot));cursor+=sizeof(slot);
        records[slot]=cursor;cursor+=sizeof(HSD_FObj);
    }
    for(unsigned i=0;i<NATIVE_FOBJ_CAPACITY;++i){
        if(!records[i])continue;
        HSD_FObj f;memcpy(&f,records[i],sizeof(f));if(!valid_decoder(&f))return false;
        uint32_t next;if(f.next && (!decoder_slot(f.next,&next) || !records[next]))return false;
    }
    // Each node has at most one next edge. Linear marking rejects cycles without
    // recursion or a quadratic walk; shared tails are not invented invalidity.
    for(unsigned i=0;i<NATIVE_FOBJ_CAPACITY;++i){
        if(!records[i] || color[i])continue;
        uint32_t slot=i;
        while(color[slot]==0){
            color[slot]=1;HSD_FObj f;memcpy(&f,records[slot],sizeof(f));
            if(!f.next)break;
            decoder_slot(f.next,&slot);
            if(color[slot]==1)return false;
        }
        slot=i;
        while(color[slot]==1){
            color[slot]=2;HSD_FObj f;memcpy(&f,records[slot],sizeof(f));
            if(!f.next)break;decoder_slot(f.next,&slot);
        }
    }
    memcpy(&header,a_data,sizeof(header));
    cursor=a_data+sizeof(header)+(header.capacity+header.free_count)*sizeof(uint32_t);
    for(unsigned i=0;i<header.used;++i){
        HSD_AObj a;cursor+=sizeof(uint32_t);memcpy(&a,cursor,sizeof(a));cursor+=sizeof(a);
        uint32_t slot;
        if(a.fobj && (!decoder_slot(a.fobj,&slot) || !records[slot]))return false;
    }
    return true;
}
size_t nativeAnimationPool_SnapshotBytes(void)
{
    if(!boundary() || !typed_shape())return 0;
    size_t a=nativeMatchSnapshotPool_Bytes(&nativeAObjPool),f=nativeMatchSnapshotPool_Bytes(&nativeFObjPool);
    if(!a || !f || a>SIZE_MAX-sizeof(AnimationSnapshotHeader) || f>SIZE_MAX-sizeof(AnimationSnapshotHeader)-a)return 0;
    return sizeof(AnimationSnapshotHeader)+a+f;
}
bool nativeAnimationPool_Save(void* snapshot,size_t capacity,size_t* written)
{
    if(!boundary() || !typed_shape() || capacity<sizeof(AnimationSnapshotHeader) ||
       !independent(snapshot,capacity) || !independent(written,sizeof(*written)) ||
       overlaps(snapshot,capacity,written,sizeof(*written)) || nativeAnimationGlobals.finished<0 ||
       nativeAnimationGlobals.active<0)return false;
    AnimationSnapshotHeader h={0};h.heap_epoch=nativeAnimationGlobals.heap_epoch;
    h.end_callbacks=nativeAnimationGlobals.end_callbacks;
    h.finished=nativeAnimationGlobals.finished;h.active=nativeAnimationGlobals.active;
    uint8_t* cursor=(uint8_t*)snapshot+sizeof(h);size_t a,f;
    // Each direct pool writer validates its source once. Use the resulting
    // byte counts instead of rescanning both free/occupied tables for sizing.
    if(!nativeMatchSnapshotPool_Save(&nativeAObjPool,cursor,capacity-sizeof(h),&a) ||
       !nativeMatchSnapshotPool_Save(&nativeFObjPool,cursor+a,capacity-sizeof(h)-a,&f) ||
       !valid_graph(cursor,cursor+a))return false;
    h.controller_bytes=a;h.decoder_bytes=f;
    memcpy(snapshot,&h,sizeof(h));*written=sizeof(h)+a+f;return true;
}
bool nativeAnimationPool_ValidateSnapshot(const void* snapshot,size_t bytes)
{
    if(!boundary() || !typed_shape() || bytes<sizeof(AnimationSnapshotHeader) || !independent(snapshot,bytes))return false;
    AnimationSnapshotHeader h;memcpy(&h,snapshot,sizeof(h));
    if(h.heap_epoch!=nativeAnimationGlobals.heap_epoch || h.finished<0 || h.active<0 ||
       h.controller_bytes>bytes-sizeof(h) || h.decoder_bytes!=bytes-sizeof(h)-h.controller_bytes)return false;
    const uint8_t* a=(const uint8_t*)snapshot+sizeof(h),*f=a+h.controller_bytes;
    return nativeMatchSnapshotPool_View(&nativeAObjPool,a,h.controller_bytes).records &&
        nativeMatchSnapshotPool_View(&nativeFObjPool,f,h.decoder_bytes).records && valid_graph(a,f);
}
bool nativeAnimationPool_Load(const void* snapshot,size_t bytes)
{
    if(!nativeAnimationPool_ValidateSnapshot(snapshot,bytes))return false;
    AnimationSnapshotHeader h;memcpy(&h,snapshot,sizeof(h));
    const uint8_t* a=(const uint8_t*)snapshot+sizeof(h),*f=a+h.controller_bytes;
    // Candidate is independent of both families and completely prevalidated.
    // Neither load invokes constructors, destructors, update callbacks or refs.
    if(!nativeMatchSnapshotPool_Load(&nativeAObjPool,a,h.controller_bytes) ||
       !nativeMatchSnapshotPool_Load(&nativeFObjPool,f,h.decoder_bytes))return false;
    nativeAnimationGlobals.end_callbacks=h.end_callbacks;
    nativeAnimationGlobals.finished=h.finished;nativeAnimationGlobals.active=h.active;
    return true;
}
#endif
