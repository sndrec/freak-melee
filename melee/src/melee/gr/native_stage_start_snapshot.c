#include <melee/gm/native_match_control_history.h>
#include "native_stage_start_snapshot.h"
#include "stage_state_internal.h"
#include <melee/gm/native_gm_loop_snapshot.h>
#include <stdlib.h>
#include <string.h>

// HOST definition lease. Actual nodes/count/phase are in stage_start_state.c.
static struct {
    uint64_t epoch;
    HSD_GObjEvent callbacks[NATIVE_GOBJ_CAPACITY];
    unsigned count;
} scene;
bool nativeStageStart_BeginScene(void)
{
    if(!nativeGObjPool.lifetime || !nativeMatchPool_Validate(&nativeGObjPool))return false;
    memset(&scene,0,sizeof(scene));scene.epoch=nativeGObjPool.lifetime;
    memset(&stage_start_state,0,sizeof(stage_start_state));return true;
}
bool nativeStageStart_AdmitCallback(HSD_GObjEvent callback)
{
    if(!callback || !scene.epoch || scene.epoch!=nativeGObjPool.lifetime)return false;
    for(unsigned i=0;i<scene.count;++i)if(scene.callbacks[i]==callback)return true;
    if(scene.count==NATIVE_GOBJ_CAPACITY)return false;
    scene.callbacks[scene.count++]=callback;return true;
}
static bool admitted(HSD_GObjEvent callback)
{
    for(unsigned i=0;i<scene.count;++i)if(scene.callbacks[i]==callback)return true;
    return false;
}
static bool boundary(void)
{return scene.epoch && scene.epoch==nativeGObjPool.lifetime && nativeGmLoop_SnapshotBoundary();}
static bool overlap(const void* a,size_t an,const void* b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y || (x<y+bn && y<x+an);
}
static bool independent(const void* p,size_t bytes)
{
    return p && bytes<=UINTPTR_MAX-(uintptr_t)p &&
        !overlap(p,bytes,&scene,sizeof(scene)) &&
        !overlap(p,bytes,&stage_start_state,sizeof(stage_start_state)) &&
        !overlap(p,bytes,&stage_info,sizeof(stage_info)) &&
        !overlap(p,bytes,&nativeGObjPool,sizeof(nativeGObjPool)) &&
        !overlap(p,bytes,nativeGObjPool.payload,nativeGObjPool.capacity*nativeGObjPool.stride) &&
        !overlap(p,bytes,nativeGObjPool.slots,nativeGObjPool.capacity*sizeof(*nativeGObjPool.slots)) &&
        !overlap(p,bytes,nativeGObjPool.free_order,nativeGObjPool.capacity*sizeof(*nativeGObjPool.free_order));
}
size_t nativeStageStart_SnapshotBytes(void)
{
    if(!boundary() || stage_start_state.count>NATIVE_GOBJ_CAPACITY)return 0;
    return sizeof(NativeStageStartSnapshotHeader)+stage_start_state.count*sizeof(NativeStageStartSnapshotRecord);
}
bool nativeStageStart_Validate(const void* data,size_t bytes)
{
    if(!boundary() || bytes<sizeof(NativeStageStartSnapshotHeader) || !independent(data,bytes))return false;
    NativeStageStartSnapshotHeader h;memcpy(&h,data,sizeof(h));
    if(h.epoch!=scene.epoch || h.count>NATIVE_GOBJ_CAPACITY || h.bytes!=bytes ||
       h.started>1 || h.reserved || bytes!=sizeof(h)+h.count*sizeof(NativeStageStartSnapshotRecord))return false;
    for(unsigned i=0;i<h.count;++i) {
        NativeStageStartSnapshotRecord r;memcpy(&r,(const uint8_t*)data+sizeof(h)+i*sizeof(r),sizeof(r));
        if(!admitted(r.callback) || r.object.pool!=&nativeGObjPool || r.object.lifetime!=h.epoch ||
           r.object.slot>=nativeGObjPool.capacity || !r.object.generation)return false;
    }
    return true;
}
bool nativeStageStart_Save(void* data,size_t capacity,size_t* written)
{
    size_t bytes=nativeStageStart_SnapshotBytes();unsigned count=stage_start_state.count;
    if(!bytes || bytes>capacity || !independent(data,bytes) || !independent(written,sizeof(*written)) ||
       overlap(data,bytes,written,sizeof(*written)) ||
       stage_info.x6A4!=(count ? &stage_start_state.nodes[count-1] : NULL))return false;
    // Complete preflight before publishing any record, including the source chain.
    for(unsigned i=0;i<count;++i) {
        const GroundStartCallback* node=&stage_start_state.nodes[i];NativeMatchPoolReference ref;
        if(node->next!=(i ? &stage_start_state.nodes[i-1] : NULL) || !admitted(node->callback) ||
           !nativeMatchPool_Reference(&nativeGObjPool,node->gobj,&ref))return false;
    }
    NativeStageStartSnapshotHeader h={scene.epoch,count,(uint32_t)bytes,stage_start_state.started,0};
    memcpy(data,&h,sizeof(h));
    for(unsigned i=0;i<count;++i) {
        NativeStageStartSnapshotRecord r={.callback=stage_start_state.nodes[i].callback};
        if(!nativeMatchPool_Reference(&nativeGObjPool,stage_start_state.nodes[i].gobj,&r.object))return false;
        memcpy((uint8_t*)data+sizeof(h)+i*sizeof(r),&r,sizeof(r));
    }
    *written=bytes;return true;
}
bool nativeStageStart_ValidateObjects(const void* data,size_t bytes,NativeMatchPoolSnapshotView objects)
{
    if(!nativeStageStart_Validate(data,bytes) || !objects.records)return false;
    NativeStageStartSnapshotHeader h;memcpy(&h,data,sizeof(h));
    for(unsigned i=0;i<h.count;++i) {
        NativeStageStartSnapshotRecord r;memcpy(&r,(const uint8_t*)data+sizeof(h)+i*sizeof(r),sizeof(r));
        if(!nativeMatchSnapshotPool_Resolve(objects,r.object))return false;
    }
    return true;
}
bool nativeStageStart_Load(const void* data,size_t bytes)
{
    if(!nativeStageStart_Validate(data,bytes))return false;
    NativeStageStartSnapshotHeader h;memcpy(&h,data,sizeof(h));
    for(unsigned i=0;i<h.count;++i) {
        NativeStageStartSnapshotRecord r;memcpy(&r,(const uint8_t*)data+sizeof(h)+i*sizeof(r),sizeof(r));
        if(!nativeMatchPool_Resolve(r.object))return false;
    }
    unsigned clear=stage_start_state.count>h.count ? stage_start_state.count : h.count;
    if(clear>NATIVE_GOBJ_CAPACITY)return false;
    memset(stage_start_state.nodes,0,clear*sizeof(*stage_start_state.nodes));
    for(unsigned i=0;i<h.count;++i) {
        NativeStageStartSnapshotRecord r;memcpy(&r,(const uint8_t*)data+sizeof(h)+i*sizeof(r),sizeof(r));
        stage_start_state.nodes[i]=(GroundStartCallback){i ? &stage_start_state.nodes[i-1] : NULL,
            nativeMatchPool_Resolve(r.object),r.callback};
    }
    stage_start_state.count=h.count;stage_start_state.started=h.started!=0;
    stage_info.x6A4=h.count ? &stage_start_state.nodes[h.count-1] : NULL;
    return true;
}

#ifdef MELEE_NATIVE
bool nativeStageStart_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || !arena->lifetime)return false;
#define ROOT(v) do { if(!nativeMatchArena_AddRoot(arena,&(v),sizeof(v)))return false; } while(0)
    ROOT(stage_start_state);
#undef ROOT
    return true;
}
#endif
