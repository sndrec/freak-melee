#include "native_gobj_pool_storage.h"
#ifdef MELEE_NATIVE
#include "native_gobj_snapshot.h"
#include <melee/gm/native_match_snapshot_pool.h>
#include <melee/gm/native_match_arena.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/gr/native_ground_pool.h>
#include <melee/it/native_item_pool.h>
#include <melee/it/native_item_link_pool.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/ef/native_script_effect_actor.h>
#include <string.h>

// Live storage belongs to the scene arena. Descriptors and fixed roots remain
// in .mstate during migration and are admitted to arena history at construction.
NativeMatchPool nativeGObjPool, nativeGObjProcPool;
static HSD_GObj* objects;
static HSD_GObjProc* procs;
static NativeMatchPoolSlot *object_slots, *proc_slots;
static uint32_t *object_free, *proc_free;
static uint64_t scene_epoch;
static bool scene_active;
static HSD_GObjLibInitDataType scene_config;
static unsigned method_count;

// Preserve the full byte-sized public scheduling extents, rather than silently
// baking GM's current 64 links/25 priorities into the HSD library contract.
enum { Links = 256, GXLinks = 257, Priorities = 256, Methods = 256 };
static HSD_GObj **plink_heads, **plink_tails, **gx_heads, **gx_tails;
static HSD_GObjProc **proc_heads, **proc_link_tails;
static GObjFunc* method_table;

static bool boundary(void)
{
    return HSD_GObj_CurrentInvokedProcGObj == NULL &&
        HSD_GObj_CurrentInvokedProc == NULL && HSD_GObj_804D7818 == NULL &&
        HSD_GObj_RenderCallbackDepth == 0 && HSD_GObj_DelayedProcInfo.flags == 0;
}

static void clear_tables(void)
{
    size_t links=scene_config.p_link_max+1u,gx=scene_config.gx_link_max+2u;
    size_t priorities=scene_config.gproc_pri_max+1u;
    memset(plink_heads,0,links*sizeof(*plink_heads));memset(plink_tails,0,links*sizeof(*plink_tails));
    memset(gx_heads,0,gx*sizeof(*gx_heads));memset(gx_tails,0,gx*sizeof(*gx_tails));
    memset(proc_heads,0,priorities*sizeof(*proc_heads));memset(proc_link_tails,0,links*priorities*sizeof(*proc_link_tails));
    HSD_GObj_NextInvokedProc=NULL;
}

bool nativeGObjPool_RetireScene(void)
{
    if(!boundary() || !nativeScriptRuntime_Idle())return false;
    if(!scene_active)return true;
    if(!nativeScriptEffect_RetireScene() || !nativeScriptItem_RetireScene() || !nativeScriptFighter_RetireScene() || !nativeFighterPool_RetireScene() || !nativeGroundPool_RetireScene() || !nativeItemPool_RetireScene() || !nativeItemLinkPool_RetireScene())return false;
    clear_tables();
    nativeGObjPool.lifetime=0;nativeGObjPool.used=0;nativeGObjPool.free_count=0;
    nativeGObjProcPool.lifetime=0;nativeGObjProcPool.used=0;nativeGObjProcPool.free_count=0;
    scene_active=false;
    nativeMatchScene_Retire();
    return true;
}

bool nativeGObjPool_BeginScene(const HSD_GObjLibInitDataType* config)
{
    if(config==NULL || !boundary() || scene_epoch==UINT64_MAX)return false;
    const GObjFuncs* seen[Methods];unsigned lists=0,count=0;
    for(const GObjFuncs* list=config->funcs;list;list=list->next){
        if(lists==Methods || (list->size!=0 && list->funcs==NULL))return false;
        for(unsigned i=0;i<lists;++i)if(seen[i]==list)return false;
        seen[lists++]=list;
        if(list->size>Methods-count)return false;
        count+=list->size;
    }
    uint64_t epoch=scene_epoch+1;
    if(!nativeFighterPool_CanBeginScene(epoch) || !nativeGroundPool_CanBeginScene(epoch) || !nativeItemPool_CanBeginScene(epoch) || !nativeItemLinkPool_CanBeginScene(epoch) || !nativeScriptFighter_CanBeginScene(epoch) || !nativeScriptItem_CanBeginScene(epoch) || !nativeScriptEffect_CanBeginScene(epoch))return false;
    if(!nativeMatchScene_Begin())return false;
    scene_config=*config;method_count=count;
    size_t links=config->p_link_max+1u,gx=config->gx_link_max+2u;
    size_t priorities=config->gproc_pri_max+1u;
#define CLAIM(pointer,number) do { \
    pointer=nativeMatchArena_Allocate(&nativeMatchSceneArena,(number)*sizeof(*pointer),_Alignof(__typeof__(*pointer))); \
    if(!pointer){nativeMatchScene_Retire();return false;} \
} while(0)
    objects=nativeMatchActorStorage.objects;procs=nativeMatchActorStorage.procedures;
    if(!objects || !procs){nativeMatchScene_Retire();return false;}
    CLAIM(object_slots,NATIVE_GOBJ_CAPACITY);CLAIM(proc_slots,NATIVE_GOBJ_PROC_CAPACITY);
    CLAIM(object_free,NATIVE_GOBJ_CAPACITY);CLAIM(proc_free,NATIVE_GOBJ_PROC_CAPACITY);
    CLAIM(plink_heads,links);CLAIM(plink_tails,links);CLAIM(gx_heads,gx);CLAIM(gx_tails,gx);
    CLAIM(proc_heads,priorities);CLAIM(proc_link_tails,links*priorities);
    CLAIM(method_table,Methods);
#undef CLAIM
#define ROOT(value) do { if(!nativeMatchArena_AddRoot(&nativeMatchSceneArena,&(value),sizeof(value))){nativeMatchScene_Retire();return false;} } while(0)
    ROOT(nativeGObjPool);ROOT(nativeGObjProcPool);
    ROOT(HSD_GObj_DelayedProcInfo);ROOT(HSD_GObj_804D783C);ROOT(HSD_GObj_804D7814);
    ROOT(HSD_GObj_CurrentInvokedSLink);ROOT(HSD_GObj_CameraKind);ROOT(HSD_GObj_JObjKind);
    ROOT(HSD_GObj_LightKind);ROOT(HSD_GObj_FogKind);ROOT(HSD_GObj_804085F0);
    if(config->unk_2)ROOT(*config->unk_2);
#undef ROOT
    if(!nativeMatchPool_Bind(&nativeGObjPool,objects,(NATIVE_GOBJ_CAPACITY*sizeof(*objects)),object_slots,object_free,
            NATIVE_GOBJ_CAPACITY,sizeof(HSD_GObj),_Alignof(HSD_GObj),epoch) ||
       !nativeMatchPool_Bind(&nativeGObjProcPool,procs,(NATIVE_GOBJ_PROC_CAPACITY*sizeof(*procs)),proc_slots,proc_free,
            NATIVE_GOBJ_PROC_CAPACITY,sizeof(HSD_GObjProc),_Alignof(HSD_GObjProc),epoch))return false;
    if(!nativeMatchArena_AddCountedRoot(&nativeMatchSceneArena,objects,sizeof(*objects),
            NATIVE_GOBJ_CAPACITY,&nativeGObjPool.initialized_count) ||
       !nativeMatchArena_AddCountedRoot(&nativeMatchSceneArena,procs,sizeof(*procs),
            NATIVE_GOBJ_PROC_CAPACITY,&nativeGObjProcPool.initialized_count))return false;
    if(!nativeFighterPool_BeginScene(epoch) || !nativeGroundPool_BeginScene(epoch) || !nativeItemPool_BeginScene(epoch) || !nativeItemLinkPool_BeginScene(epoch) || !nativeScriptFighter_BeginScene(epoch) || !nativeScriptItem_BeginScene(epoch) || !nativeScriptEffect_BeginScene(epoch))return false;
    scene_epoch=epoch;scene_active=true;clear_tables();
    HSD_GObjPLinkHead=plink_heads;plinklow_gobjs=plink_tails;
    HSD_GObjGXLinkHead=gx_heads;HSD_GObj_804D7820=gx_tails;
    HSD_GObj_GObjProcHead=proc_heads;HSD_GObj_ProcList=proc_link_tails;
    memset(method_table,0,Methods*sizeof(*method_table));unsigned index=0;
    for(const GObjFuncs* list=config->funcs;list;list=list->next)
        for(unsigned i=0;i<list->size;++i)method_table[index++]=list->funcs[i];
    HSD_GObj_804D7810=count?method_table:NULL;
    scene_config=*config;method_count=count;
    return true;
}

// No per-frame/allocation work is added. Fixed validation scratch belongs to
// the host, not gameplay or the broad global-state oracle.
#pragma clang section data = "" bss = ""
static struct {
    const uint8_t* objects[NATIVE_GOBJ_CAPACITY];
    const uint8_t* procs[NATIVE_GOBJ_PROC_CAPACITY];
    uint8_t object_seen[NATIVE_GOBJ_CAPACITY], gx_seen[NATIVE_GOBJ_CAPACITY];
    uint8_t proc_seen[NATIVE_GOBJ_PROC_CAPACITY], child_seen[NATIVE_GOBJ_PROC_CAPACITY];
} snapshot_scratch;

static bool config_matches(const HSD_GObjLibInitDataType* a,const HSD_GObjLibInitDataType* b)
{
    return a->p_link_max==b->p_link_max && a->gx_link_max==b->gx_link_max &&
        a->gproc_pri_max==b->gproc_pri_max && a->funcs==b->funcs && a->unk_2==b->unk_2;
}
static bool snapshot_boundary(void)
{
    return scene_active && boundary() && nativeScriptRuntime_Idle() &&
        HSD_GObj_NextInvokedProc==NULL &&
        nativeGObjPool.lifetime==scene_epoch && nativeGObjProcPool.lifetime==scene_epoch &&
        nativeGObjPool.payload==(uint8_t*)objects && nativeGObjPool.slots==object_slots &&
        nativeGObjPool.free_order==object_free && nativeGObjPool.capacity==NATIVE_GOBJ_CAPACITY &&
        nativeGObjPool.object_size==sizeof(*objects) && nativeGObjPool.stride==sizeof(*objects) &&
        nativeGObjProcPool.payload==(uint8_t*)procs && nativeGObjProcPool.slots==proc_slots &&
        nativeGObjProcPool.free_order==proc_free && nativeGObjProcPool.capacity==NATIVE_GOBJ_PROC_CAPACITY &&
        nativeGObjProcPool.object_size==sizeof(*procs) && nativeGObjProcPool.stride==sizeof(*procs) &&
        HSD_GObjPLinkHead==plink_heads && plinklow_gobjs==plink_tails &&
        HSD_GObjGXLinkHead==gx_heads && HSD_GObj_804D7820==gx_tails &&
        HSD_GObj_GObjProcHead==proc_heads && HSD_GObj_ProcList==proc_link_tails &&
        HSD_GObj_804D7810==(method_count?method_table:NULL) &&
        config_matches(&HSD_GObjLibInitData,&scene_config);
}
static size_t table_bytes(void)
{
    size_t links=scene_config.p_link_max+1u,gx=scene_config.gx_link_max+2u;
    size_t priorities=scene_config.gproc_pri_max+1u;
    return (2*links+2*gx+priorities+links*priorities)*sizeof(void*);
}
static bool snapshot_overlap(const void* a,size_t an,const void* b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y || (x<y+bn && y<x+an);
}
static bool snapshot_buffer(const void* data,size_t bytes)
{
    if(!data || !bytes)return false;
#define EXCLUDE(value) if(snapshot_overlap(data,bytes,&(value),sizeof(value)))return false
    EXCLUDE(nativeGObjPool);EXCLUDE(nativeGObjProcPool);
    if(snapshot_overlap(data,bytes,nativeMatchSceneArena.memory,nativeMatchSceneArena.capacity) ||
       snapshot_overlap(data,bytes,nativeMatchActorStorage.memory,nativeMatchActorStorage.bytes))return false;
    EXCLUDE(objects);EXCLUDE(procs);EXCLUDE(object_slots);EXCLUDE(proc_slots);
    EXCLUDE(object_free);EXCLUDE(proc_free);EXCLUDE(method_table);
    EXCLUDE(plink_heads);EXCLUDE(plink_tails);EXCLUDE(gx_heads);EXCLUDE(gx_tails);
    EXCLUDE(proc_heads);EXCLUDE(proc_link_tails);
    EXCLUDE(scene_epoch);EXCLUDE(scene_active);
    EXCLUDE(scene_config);EXCLUDE(method_count);EXCLUDE(snapshot_scratch);
    EXCLUDE(HSD_GObjLibInitData);
    EXCLUDE(HSD_GObjPLinkHead);EXCLUDE(plinklow_gobjs);EXCLUDE(HSD_GObjGXLinkHead);
    EXCLUDE(HSD_GObj_804D7820);EXCLUDE(HSD_GObj_GObjProcHead);EXCLUDE(HSD_GObj_ProcList);
    EXCLUDE(HSD_GObj_804D7810);EXCLUDE(HSD_GObj_804D7814);EXCLUDE(HSD_GObj_804D7818);
    EXCLUDE(HSD_GObj_CurrentInvokedProcGObj);EXCLUDE(HSD_GObj_CurrentInvokedProc);
    EXCLUDE(HSD_GObj_NextInvokedProc);EXCLUDE(HSD_GObj_CurrentInvokedSLink);
    EXCLUDE(HSD_GObj_804D783C);EXCLUDE(HSD_GObj_DelayedProcInfo);EXCLUDE(HSD_GObj_804085F0);
    EXCLUDE(HSD_GObj_RenderCallbackDepth);
    EXCLUDE(HSD_GObj_CameraKind);EXCLUDE(HSD_GObj_JObjKind);EXCLUDE(HSD_GObj_LightKind);EXCLUDE(HSD_GObj_FogKind);
#undef EXCLUDE
    return !scene_config.unk_2 || !snapshot_overlap(data,bytes,scene_config.unk_2,sizeof(uint64_t));
}

size_t nativeGObjPool_SnapshotBytes(void)
{
    if(!snapshot_boundary())return 0;
    NativeMatchPoolReference context;
    // A new capture requires an owned live context. Validation/load instead
    // use the candidate's occupied records: a restore may resurrect this node.
    if (HSD_GObj_804D7814 && !nativeMatchPool_Reference(&nativeGObjPool,
            HSD_GObj_804D7814, &context)) return 0;
    size_t a=nativeMatchSnapshotPool_Bytes(&nativeGObjPool);
    size_t b=nativeMatchSnapshotPool_Bytes(&nativeGObjProcPool);
    return a && b ? sizeof(NativeGObjSnapshotHeader)+table_bytes()+a+b : 0;
}

static void copy_tables(uint8_t* buffer,bool load)
{
    size_t links=scene_config.p_link_max+1u,gx=scene_config.gx_link_max+2u;
    size_t priorities=scene_config.gproc_pri_max+1u;
    void* tables[]={plink_heads,plink_tails,gx_heads,gx_tails,proc_heads,proc_link_tails};
    size_t counts[]={links,links,gx,gx,priorities,links*priorities};
    for(unsigned i=0;i<6;++i){
        size_t bytes=counts[i]*sizeof(void*);
        if(load)memcpy(tables[i],buffer,bytes);else memcpy(buffer,tables[i],bytes);
        buffer+=bytes;
    }
}

static const uint8_t* read_pointer(const uint8_t* data,unsigned index)
{
    const uint8_t* pointer;memcpy(&pointer,data+index*sizeof(void*),sizeof(pointer));return pointer;
}
static int object_slot(const void* pointer)
{
    uintptr_t at=(uintptr_t)pointer,base=(uintptr_t)objects;
    if(at<base || at-base>=(NATIVE_GOBJ_CAPACITY*sizeof(*objects)) || (at-base)%sizeof(*objects))return -1;
    unsigned slot=(unsigned)((at-base)/sizeof(*objects));
    return snapshot_scratch.objects[slot]?(int)slot:-1;
}
static int proc_slot(const void* pointer)
{
    uintptr_t at=(uintptr_t)pointer,base=(uintptr_t)procs;
    if(at<base || at-base>=(NATIVE_GOBJ_PROC_CAPACITY*sizeof(*procs)) || (at-base)%sizeof(*procs))return -1;
    unsigned slot=(unsigned)((at-base)/sizeof(*procs));
    return snapshot_scratch.procs[slot]?(int)slot:-1;
}
static HSD_GObj read_object(unsigned slot)
{
    HSD_GObj value;memcpy(&value,snapshot_scratch.objects[slot],sizeof(value));return value;
}
static HSD_GObjProc read_proc(unsigned slot)
{
    HSD_GObjProc value;memcpy(&value,snapshot_scratch.procs[slot],sizeof(value));return value;
}
static uint32_t map_records(const uint8_t* data,const uint8_t** map,size_t object_size)
{
    NativeMatchPoolSnapshotHeader header;memcpy(&header,data,sizeof(header));
    const uint8_t* cursor=data+sizeof(header)+(header.capacity+header.free_count)*sizeof(uint32_t);
    for(unsigned i=0;i<header.used;++i){
        uint32_t slot;memcpy(&slot,cursor,sizeof(slot));cursor+=sizeof(slot);
        map[slot]=cursor;cursor+=object_size;
    }
    return header.used;
}
// Every pointer is resolved through serialized occupied records, never through
// the current graph. A prior snapshot may resurrect a currently free/reused node.
static bool snapshot_topology(const uint8_t* tables,const uint8_t* object_data,const uint8_t* proc_data)
{
    memset(&snapshot_scratch,0,sizeof(snapshot_scratch));
    unsigned object_count=map_records(object_data,snapshot_scratch.objects,sizeof(HSD_GObj));
    unsigned proc_count=map_records(proc_data,snapshot_scratch.procs,sizeof(HSD_GObjProc));
    unsigned links=scene_config.p_link_max+1u,gx=scene_config.gx_link_max+2u;
    unsigned priorities=scene_config.gproc_pri_max+1u;
    const uint8_t* pheads=tables,*ptails=pheads+links*sizeof(void*);
    const uint8_t* gheads=ptails+links*sizeof(void*),*gtails=gheads+gx*sizeof(void*);
    const uint8_t* qheads=gtails+gx*sizeof(void*),*qtails=qheads+priorities*sizeof(void*);
    unsigned seen_objects=0,seen_procs=0;
    for(unsigned link=0;link<links;++link){
        const void* previous=NULL;
        for(const void* pointer=read_pointer(pheads,link);pointer;){
            int slot=object_slot(pointer);
            if(slot<0 || snapshot_scratch.object_seen[slot])return false;
            snapshot_scratch.object_seen[slot]=1;++seen_objects;
            HSD_GObj object=read_object(slot);
            if(object.prev!=previous || object.p_link!=link || object.native_destroying ||
               object.native_userdata_removing || object.native_destroy_requested ||
               (object.obj_kind!=HSD_GOBJ_OBJ_NONE && object.obj_kind>=method_count))return false;
            previous=pointer;pointer=object.next;
        }
        if(read_pointer(ptails,link)!=previous)return false;
    }
    if(seen_objects!=object_count)return false;
    for(unsigned link=0;link<gx;++link){
        const void* previous=NULL;
        for(const void* pointer=read_pointer(gheads,link);pointer;){
            int slot=object_slot(pointer);
            if(slot<0 || snapshot_scratch.gx_seen[slot])return false;
            snapshot_scratch.gx_seen[slot]=1;
            HSD_GObj object=read_object(slot);
            if(object.gx_link!=link || object.prev_gx!=previous)return false;
            previous=pointer;pointer=object.next_gx;
        }
        if(read_pointer(gtails,link)!=previous)return false;
    }
    for(unsigned priority=0;priority<priorities;++priority){
        const void* previous=NULL;const void* tails[Links]={0};
        for(const void* pointer=read_pointer(qheads,priority);pointer;){
            int slot=proc_slot(pointer);
            if(slot<0 || snapshot_scratch.proc_seen[slot])return false;
            snapshot_scratch.proc_seen[slot]=1;++seen_procs;
            HSD_GObjProc proc=read_proc(slot);
            int owner=object_slot(proc.gobj);
            // flags_3 is 3 on a newly created proc; 0..2 mark completed passes.
            if(owner<0 || proc.prev!=previous || proc.s_link!=priority || !proc.on_invoke)return false;
            HSD_GObj object=read_object(owner);tails[object.p_link]=pointer;
            previous=pointer;pointer=proc.next;
        }
        for(unsigned link=0;link<links;++link)
            if(read_pointer(qtails,link+priority*links)!=tails[link])return false;
    }
    if(seen_procs!=proc_count)return false;
    unsigned child_count=0;
    for(unsigned i=0;i<NATIVE_GOBJ_CAPACITY;++i){
        if(!snapshot_scratch.objects[i])continue;
        HSD_GObj object=read_object(i);
        if(!snapshot_scratch.gx_seen[i] && object.gx_link!=HSD_GOBJ_GXLINK_NONE)return false;
        for(const void* pointer=object.proc;pointer;){
            int slot=proc_slot(pointer);
            if(slot<0 || snapshot_scratch.child_seen[slot])return false;
            snapshot_scratch.child_seen[slot]=1;++child_count;
            HSD_GObjProc proc=read_proc(slot);
            if(proc.gobj!=&objects[i])return false;
            pointer=proc.child;
        }
    }
    return child_count==proc_count;
}

bool nativeGObjPool_ValidateSnapshot(const void* data,size_t bytes)
{
    if(!snapshot_boundary() || bytes<sizeof(NativeGObjSnapshotHeader) || !snapshot_buffer(data,bytes))return false;
    NativeGObjSnapshotHeader h;memcpy(&h,data,sizeof(h));
    size_t prefix=sizeof(h)+table_bytes();
    if(h.epoch!=scene_epoch || !config_matches(&h.config,&scene_config) || h.delayed.flags ||
       h.invocation_phase<0 || h.invocation_phase>2 || h.current_priority<0 ||
       h.current_priority>=Priorities || bytes<prefix ||
       h.object_bytes>bytes-prefix || h.proc_bytes!=bytes-prefix-h.object_bytes)return false;
    const uint8_t* tables=(const uint8_t*)data+sizeof(h);
    const uint8_t* object_data=(const uint8_t*)data+prefix,*proc_data=object_data+h.object_bytes;
    return nativeMatchSnapshotPool_View(&nativeGObjPool,object_data,h.object_bytes).records &&
        nativeMatchSnapshotPool_View(&nativeGObjProcPool,proc_data,h.proc_bytes).records &&
        snapshot_topology(tables,object_data,proc_data) &&
        (!h.render_context || object_slot(h.render_context)>=0);
}

bool nativeGObjPool_Save(void* data,size_t capacity,size_t* written)
{
    if(!snapshot_boundary())return false;
    NativeMatchPoolReference context;
    if(HSD_GObj_804D7814 && !nativeMatchPool_Reference(&nativeGObjPool,HSD_GObj_804D7814,&context))return false;
    size_t prefix=sizeof(NativeGObjSnapshotHeader)+table_bytes();
    if(capacity<prefix || !snapshot_buffer(data,capacity) ||
       !snapshot_buffer(written,sizeof(*written)) || snapshot_overlap(data,capacity,written,sizeof(*written)))return false;
    NativeGObjSnapshotHeader h={0};h.epoch=scene_epoch;h.config=HSD_GObjLibInitData;
    h.process_mask=scene_config.unk_2?*scene_config.unk_2:0;
    h.delayed=HSD_GObj_DelayedProcInfo;h.invocation_phase=HSD_GObj_804D783C;
    h.render_context=HSD_GObj_804D7814;
    h.current_priority=HSD_GObj_CurrentInvokedSLink;h.camera_kind=HSD_GObj_CameraKind;
    h.joint_kind=HSD_GObj_JObjKind;h.light_kind=HSD_GObj_LightKind;h.fog_kind=HSD_GObj_FogKind;
    memcpy(h.render_flags,HSD_GObj_804085F0,sizeof(h.render_flags));
    uint8_t* cursor=(uint8_t*)data+sizeof(h);copy_tables(cursor,false);cursor+=table_bytes();
    size_t left=capacity-prefix;
    if(!nativeMatchSnapshotPool_Save(&nativeGObjPool,cursor,left,&h.object_bytes))return false;
    cursor+=h.object_bytes;
    left-=h.object_bytes;
    if(!nativeMatchSnapshotPool_Save(&nativeGObjProcPool,cursor,left,&h.proc_bytes))return false;
    size_t bytes=prefix+h.object_bytes+h.proc_bytes;
    memcpy(data,&h,sizeof(h));
    if(!nativeGObjPool_ValidateSnapshot(data,bytes))return false;
    *written=bytes;return true;
}

bool nativeGObjPool_Load(const void* data,size_t bytes)
{
    if(!nativeGObjPool_ValidateSnapshot(data,bytes))return false;
    NativeGObjSnapshotHeader h;memcpy(&h,data,sizeof(h));
    const uint8_t* object_data=(const uint8_t*)data+sizeof(h)+table_bytes();
    // All candidate components/topology validated before the first state write.
    if(!nativeMatchSnapshotPool_Load(&nativeGObjPool,object_data,h.object_bytes) ||
       !nativeMatchSnapshotPool_Load(&nativeGObjProcPool,object_data+h.object_bytes,h.proc_bytes))return false;
    copy_tables((uint8_t*)data+sizeof(h),true);
    HSD_GObj_804D783C=h.invocation_phase;HSD_GObj_CurrentInvokedSLink=h.current_priority;
    HSD_GObj_DelayedProcInfo=h.delayed;HSD_GObj_CameraKind=h.camera_kind;
    HSD_GObj_804D7814=h.render_context;
    HSD_GObj_JObjKind=h.joint_kind;HSD_GObj_LightKind=h.light_kind;HSD_GObj_FogKind=h.fog_kind;
    memcpy(HSD_GObj_804085F0,h.render_flags,sizeof(h.render_flags));
    if(scene_config.unk_2)*scene_config.unk_2=h.process_mask;
    return true;
}
#endif
