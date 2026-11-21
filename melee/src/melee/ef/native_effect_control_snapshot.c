#include <melee/gm/native_match_control_history.h>
#include "native_effect_control_snapshot.h"
#include "efdata.h"
#include <melee/gm/native_gm_loop_snapshot.h>
#include <melee/ft/native_script_runtime.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/native_objalloc_snapshot.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdlib.h>
#include <string.h>

// Host lease. The actual counters and tables remain in efdata.c.
static struct { uint64_t epoch; NativeMatchPool objects; bool active; } scene;
void nativeEffectControl_BeginScene(void)
{
    if(scene.epoch==UINT64_MAX)abort();
    ++scene.epoch;scene.active=false;
    if(!nativeMatchPool_Validate(&nativeGObjPool) || !nativeGObjPool.lifetime)return;
    scene.objects=nativeGObjPool;scene.active=true;
}
void nativeEffectControl_EndScene(void) {scene.active=false;}
static bool boundary(void)
{
    const NativeMatchPool* p=&scene.objects;
    return scene.active && nativeGmLoop_SnapshotBoundary() && nativeScriptRuntime_Idle() &&
        p->lifetime==nativeGObjPool.lifetime && p->payload==nativeGObjPool.payload &&
        p->slots==nativeGObjPool.slots && p->free_order==nativeGObjPool.free_order &&
        p->capacity==nativeGObjPool.capacity && p->stride==nativeGObjPool.stride && p->object_size==nativeGObjPool.object_size;
}
static bool overlap(const void* a,size_t an,const void* b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y || (x<y+bn && y<x+an);
}
static bool independent(const void* data,size_t bytes)
{
    if(!data || !bytes || bytes>UINTPTR_MAX-(uintptr_t)data)return false;
#define EXCLUDE(v) if(overlap(data,bytes,&(v),sizeof(v)))return false
    EXCLUDE(scene);EXCLUDE(nativeGObjPool);EXCLUDE(efLib_LoadKind);EXCLUDE(efLib_EffectCount);
    EXCLUDE(efLib_AnimCount);EXCLUDE(efLib_AllocationSerial);EXCLUDE(efLib_AnimQueue);EXCLUDE(efLib_ParamTable);
    EXCLUDE(efLib_Usage);
#undef EXCLUDE
    if(efLib_AnimCount<0 || efLib_AnimCount>EFFECT_ANIMATION_QUEUE)return false;
    for(unsigned i=0;i<(unsigned)efLib_AnimCount;++i) {
        NativeClassReference reference;
        if(!(reference=nativeClass_Reference(efLib_AnimQueue[i],sizeof(HSD_JObj))).bytes)return false;
        HSD_ClassInfo* type=HSD_CLASS_METHOD(efLib_AnimQueue[i]);
        if(!nativeClass_TypeKnown(type,sizeof(HSD_JObj)) ||
            overlap(data,bytes,efLib_AnimQueue[i],sizeof(HSD_JObj)) ||
            overlap(data,bytes,type,type->head.info_size))return false;
    }
    const NativeMatchPool* p=&scene.objects;
    return !overlap(data,bytes,p->payload,p->capacity*p->stride) &&
        !overlap(data,bytes,p->slots,p->capacity*sizeof(*p->slots)) &&
        !overlap(data,bytes,p->free_order,p->capacity*sizeof(*p->free_order)) &&
        nativeObjAlloc_BufferIndependent(data,bytes);
}
static bool zero_object(NativeMatchPoolReference r)
{return !r.pool && !r.lifetime && !r.slot && !r.generation;}
static bool zero_joint(NativeClassReference r)
{return !r.epoch && !r.block && !r.reserved && !r.offset && !r.bytes;}
bool nativeEffectControl_Validate(const void* data,size_t bytes)
{
    if(!boundary() || bytes!=sizeof(NativeEffectControlSnapshot) || !independent(data,bytes))return false;
    NativeEffectControlSnapshot s;memcpy(&s,data,sizeof(s));
    if(s.epoch!=scene.epoch || s.load_kind>1 || s.animation_count<0 || s.animation_count>EFFECT_ANIMATION_QUEUE)return false;
    if (s.usage.model_units_this_tick > EF_MODEL_UNITS_PER_TICK ||
        s.usage.requests_this_tick > EF_MAX_QUEUED_EFFECTS ||
        s.usage.live_model_units > EF_MODEL_LIVE_UNITS) return false;
    for(unsigned i=0;i<EFFECT_PARAMETER_SLOTS;++i) {
        NativeMatchPoolReference r=s.objects[i];
        if(!s.parameters[i].gobj) {if(!zero_object(r))return false;}
        else if(r.pool!=&nativeGObjPool || r.lifetime!=scene.objects.lifetime || r.slot>=scene.objects.capacity ||
            !r.generation || s.parameters[i].gobj!=scene.objects.payload+(size_t)r.slot*scene.objects.stride)return false;
    }
    for(unsigned i=0;i<EFFECT_ANIMATION_QUEUE;++i) {
        if(i>=(unsigned)s.animation_count) {
            // End-of-frame drain keeps old pointer bits beyond the live prefix.
            if(!zero_joint(s.joints[i]) || s.joint_types[i])return false;
        } else if(!s.animations[i] || s.joints[i].bytes!=sizeof(HSD_JObj) ||
            nativeClass_Resolve(s.joints[i])!=s.animations[i] ||
            !nativeClass_TypeKnown(s.joint_types[i],sizeof(HSD_JObj)) ||
            overlap(data,bytes,s.animations[i],sizeof(HSD_JObj)))return false;
    }
    return true;
}
static bool live(const NativeEffectControlSnapshot* s)
{
    for(unsigned i=0;i<EFFECT_PARAMETER_SLOTS;++i)
        if(s->parameters[i].gobj && nativeMatchPool_Resolve(s->objects[i])!=s->parameters[i].gobj)return false;
    for(unsigned i=0;i<(unsigned)s->animation_count;++i)
        if(HSD_CLASS_METHOD(s->animations[i])!=s->joint_types[i])return false;
    return true;
}
bool nativeEffectControl_Save(void* data,size_t capacity,size_t* written)
{
    size_t bytes=sizeof(NativeEffectControlSnapshot);
    if(!boundary() || capacity<bytes || !independent(data,bytes) || !independent(written,sizeof(*written)) ||
        overlap(data,bytes,written,sizeof(*written)) || efLib_AnimCount<0 || efLib_AnimCount>EFFECT_ANIMATION_QUEUE)return false;
    NativeEffectControlSnapshot s;memset(&s,0,sizeof(s));s.epoch=scene.epoch;
    s.load_kind=efLib_LoadKind;s.effect_count=efLib_EffectCount;s.animation_count=efLib_AnimCount;s.allocation_serial=efLib_AllocationSerial;
    s.usage = efLib_Usage;
    memcpy(s.parameters,efLib_ParamTable,sizeof(s.parameters));memcpy(s.animations,efLib_AnimQueue,sizeof(s.animations));
    for(unsigned i=0;i<EFFECT_PARAMETER_SLOTS;++i)if(s.parameters[i].gobj)
        if(!nativeMatchPool_Reference(&nativeGObjPool,s.parameters[i].gobj,&s.objects[i]))return false;
    for(unsigned i=0;i<(unsigned)s.animation_count;++i) {
        if(!(s.joints[i]=nativeClass_Reference(s.animations[i],sizeof(HSD_JObj))).bytes)return false;
        s.joint_types[i]=HSD_CLASS_METHOD(s.animations[i]);
    }
    if(!nativeEffectControl_Validate(&s,bytes) || !live(&s))return false;
    memcpy(data,&s,bytes);*written=bytes;return true;
}
bool nativeEffectControl_Load(const void* data,size_t bytes)
{
    if(!nativeEffectControl_Validate(data,bytes))return false;
    NativeEffectControlSnapshot s;memcpy(&s,data,sizeof(s));
    // GObj/class payload companions restore first; aggregate candidate graph
    // agreement is still needed before any owner performs writes.
    if(!live(&s))return false;
    efLib_LoadKind=s.load_kind;efLib_EffectCount=s.effect_count;efLib_AnimCount=s.animation_count;efLib_AllocationSerial=s.allocation_serial;
    efLib_Usage = s.usage;
    memcpy(efLib_ParamTable,s.parameters,sizeof(s.parameters));memcpy(efLib_AnimQueue,s.animations,sizeof(s.animations));
    return true;
}

#ifdef MELEE_NATIVE
bool nativeEffectControl_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || !arena->lifetime)return false;
#define ROOT(v) do { if(!nativeMatchArena_AddRoot(arena,&(v),sizeof(v)))return false; } while(0)
    ROOT(efLib_LoadKind);
    ROOT(efLib_EffectCount);
    ROOT(efLib_AnimCount);
    ROOT(efLib_AllocationSerial);
    ROOT(efLib_Usage);
    ROOT(efLib_ParamTable);
    ROOT(efLib_AnimQueue);
#undef ROOT
    return true;
}
#endif
