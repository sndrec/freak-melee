#include "native_match_object_census.h"
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/objalloc.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/gr/native_ground_pool.h>
#include <melee/it/native_item_pool.h>
#include <melee/it/native_item_link_pool.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/fobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/robj.h>
#include <sysdolphin/baselib/id.h>
#include <sysdolphin/baselib/native_animation_pool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { Nodes = 16384, HashSlots = Nodes * 2, Types = 256, Priorities = 256 };

typedef struct PoolCount {
    const void* identity;
    uint32_t size, align, used, peak, free_count, max_used, max_reserved, shape_changes;
    uint64_t samples, payload_peak;
} PoolCount;
typedef struct ClassCount {
    const void* identity;
    char name[64];
    uint32_t size, live, peak, max_live;
} ClassCount;
typedef struct GroupCount {
    uint32_t classifier, live, max_live;
    uint64_t samples;
} GroupCount;

// Host-only observation: never in .mstate or an explicit game snapshot.
static struct {
    NativeMatchObjectCensusStats stats;
    PoolCount pools[Types];
    ClassCount classes[Types];
    GroupCount groups[Types];
    uint32_t priority_peak[Priorities], link_peak[Priorities];
    const void* object_map[HashSlots];
    const void* proc_map[HashSlots];
    const HSD_ClassInfo* class_stack[Types];
    const void* seen_types[Types];
    FILE* output;
    uint64_t frame, input_sample, scene_index;
    bool active, failed, opened;
    char reason[128];
} census;

static void fail(const char* reason)
{
    ++census.stats.errors;
    census.failed = true;
    if (census.reason[0] == 0) {
        size_t i=0;for(;i+1<sizeof(census.reason)&&reason[i];++i)census.reason[i]=reason[i];
        census.reason[i]=0;
    }
}

static uint32_t maximum(uint32_t a,uint32_t b) { return a>b?a:b; }
static bool insert(const void** map,const void* pointer)
{
    size_t slot = (((uintptr_t)pointer >> 4) * UINT64_C(11400714819323198485)) & (HashSlots-1);
    for (unsigned probe=0;probe<HashSlots;++probe,slot=(slot+1)&(HashSlots-1)) {
        if (map[slot]==pointer) return false;
        if (map[slot]==NULL) { map[slot]=pointer; return true; }
    }
    return false;
}
static bool contains(const void** map,const void* pointer)
{
    if (pointer==NULL) return false;
    size_t slot = (((uintptr_t)pointer >> 4) * UINT64_C(11400714819323198485)) & (HashSlots-1);
    for (unsigned probe=0;probe<HashSlots;++probe,slot=(slot+1)&(HashSlots-1)) {
        if (map[slot]==pointer) return true;
        if (map[slot]==NULL) return false;
    }
    return false;
}

static GroupCount* group(uint32_t classifier)
{
    for(unsigned i=0;i<census.stats.classifier_types;++i)
        if(census.groups[i].classifier==classifier)return &census.groups[i];
    if(census.stats.classifier_types==Types){fail("classifier group capacity exceeded");return NULL;}
    GroupCount* result=&census.groups[census.stats.classifier_types++];
    *result=(GroupCount){.classifier=classifier};return result;
}

static bool read_objects(void)
{
    if(HSD_GObjPLinkHead==NULL || plinklow_gobjs==NULL ||
       HSD_GObj_GObjProcHead==NULL || HSD_GObj_ProcList==NULL ||
       HSD_GObjGXLinkHead==NULL || HSD_GObj_804D7820==NULL){fail("scheduler tables absent");return false;}
    memset(census.object_map,0,sizeof(census.object_map));
    memset(census.proc_map,0,sizeof(census.proc_map));
    for(unsigned i=0;i<census.stats.classifier_types;++i)census.groups[i].live=0;
    uint32_t objects=0,procs=0;
    for(unsigned link=0;link<=HSD_GObjLibInitData.p_link_max;++link){
        HSD_GObj* previous=NULL;uint32_t count=0;
        for(HSD_GObj* object=HSD_GObjPLinkHead[link];object;object=object->next){
            if(++objects>Nodes || !insert(census.object_map,object)){fail("GObj list cycle/duplicate/node bound");return false;}
            if(object->p_link!=link || object->prev!=previous){fail("GObj priority-link topology mismatch");return false;}
            GroupCount* entry=group(object->classifier);if(!entry)return false;
            ++entry->live;++count;previous=object;
        }
        if(plinklow_gobjs[link]!=previous){fail("GObj priority-link tail mismatch");return false;}
        census.link_peak[link]=maximum(census.link_peak[link],count);
    }
    for(unsigned priority=0;priority<=HSD_GObjLibInitData.gproc_pri_max;++priority){
        HSD_GObjProc* previous=NULL;uint32_t count=0;
        HSD_GObjProc* tails[Priorities]={0};
        for(HSD_GObjProc* proc=HSD_GObj_GObjProcHead[priority];proc;proc=proc->next){
            if(++procs>Nodes || !insert(census.proc_map,proc)){fail("proc list cycle/duplicate/node bound");return false;}
            if(proc->s_link!=priority || proc->prev!=previous || !contains(census.object_map,proc->gobj)){
                fail("proc priority/owner topology mismatch");return false;
            }
            tails[proc->gobj->p_link]=proc;
            previous=proc;++count;
        }
        for(unsigned link=0;link<=HSD_GObjLibInitData.p_link_max;++link)
            if(HSD_GObj_ProcList[link+priority*(HSD_GObjLibInitData.p_link_max+1)]!=tails[link]){
                fail("per-link priority proc table mismatch");return false;
            }
        census.priority_peak[priority]=maximum(census.priority_peak[priority],count);
    }
    uint32_t owner_procs=0;
    for(unsigned slot=0;slot<HashSlots;++slot){
        const HSD_GObj* object=census.object_map[slot];if(!object)continue;
        unsigned count=0;
        for(const HSD_GObjProc* proc=object->proc;proc;proc=proc->child){
            if(++count>Nodes || ++owner_procs>Nodes || !contains(census.proc_map,proc) || proc->gobj!=object){
                fail("proc owner chain missing/cyclic/mismatched");return false;
            }
        }
    }
    if(owner_procs!=procs){fail("priority and owner proc counts differ");return false;}
    memset(census.proc_map,0,sizeof(census.proc_map));uint32_t gx_objects=0;
    for(unsigned link=0;link<=HSD_GObjLibInitData.gx_link_max+1u;++link){
        HSD_GObj* previous=NULL;
        for(HSD_GObj* object=HSD_GObjGXLinkHead[link];object;object=object->next_gx){
            if(++gx_objects>Nodes || !insert(census.proc_map,object) ||
                !contains(census.object_map,object) || object->gx_link!=link || object->prev_gx!=previous){
                fail("GX object list cycle/owner/link topology mismatch");return false;
            }
            previous=object;
        }
        if(HSD_GObj_804D7820[link]!=previous){fail("GX object tail mismatch");return false;}
    }
    for(unsigned slot=0;slot<HashSlots;++slot){
        const HSD_GObj* object=census.object_map[slot];
        if(object && object->gx_link!=HSD_GOBJ_GXLINK_NONE && !contains(census.proc_map,object)){
            fail("GX-tagged object missing from GX registry");return false;
        }
    }
    // At a complete draw boundary these registries should cover allocated nodes.
    // Record disagreements as incomplete evidence rather than undercounting.
    if(objects!=nativeGObjPool.used || procs!=nativeGObjProcPool.used){
        fail("linked and actual allocator GObj/proc counts differ");return false;
    }
    census.stats.linked_object_peak=maximum(census.stats.linked_object_peak,objects);
    census.stats.linked_proc_peak=maximum(census.stats.linked_proc_peak,procs);
    for(unsigned i=0;i<census.stats.classifier_types;++i){
        GroupCount* entry=&census.groups[i];entry->max_live=maximum(entry->max_live,entry->live);
        entry->samples+=entry->live;
    }
    return true;
}

static uint64_t observe_pool(const void* identity,uint32_t size,uint32_t align,
                              uint32_t used,uint32_t free_count,uint32_t peak)
{
    unsigned index=0;for(;index<census.stats.allocator_types;++index)if(census.pools[index].identity==identity)break;
    if(index==census.stats.allocator_types){
        if(index==Types){fail("allocator census type capacity exceeded");return 0;}
        ++census.stats.allocator_types;census.pools[index]=(PoolCount){.identity=identity};
    }
    PoolCount* entry=&census.pools[index];
    if(entry->samples && (entry->size!=size || entry->align!=align))++entry->shape_changes;
    entry->size=size;entry->align=align;entry->used=used;
    entry->peak=maximum(entry->peak,peak);entry->free_count=free_count;
    entry->max_used=maximum(entry->max_used,used);
    if((uint64_t)used+free_count>UINT32_MAX){fail("allocator count overflow");return 0;}
    entry->max_reserved=maximum(entry->max_reserved,used+free_count);++entry->samples;
    uint64_t payload=(uint64_t)used*size;
    if(payload>entry->payload_peak)entry->payload_peak=payload;
    return payload;
}

static uint64_t read_pools(void)
{
    unsigned visited=0;
    uint64_t bytes=observe_pool(&nativeGObjPool,(uint32_t)nativeGObjPool.object_size,
        _Alignof(HSD_GObj),nativeGObjPool.used,nativeGObjPool.free_count,nativeGObjPool.peak);
    bytes+=observe_pool(&nativeGObjProcPool,(uint32_t)nativeGObjProcPool.object_size,
        _Alignof(HSD_GObjProc),nativeGObjProcPool.used,nativeGObjProcPool.free_count,nativeGObjProcPool.peak);
    bytes+=observe_pool(&nativeFighterPool,(uint32_t)nativeFighterPool.object_size,
        _Alignof(NativeFighterPoolSlot),nativeFighterPool.used,nativeFighterPool.free_count,nativeFighterPool.peak);
    bytes+=observe_pool(&nativeGroundPool,(uint32_t)nativeGroundPool.object_size,
        _Alignof(Ground),nativeGroundPool.used,nativeGroundPool.free_count,nativeGroundPool.peak);
    bytes+=observe_pool(&nativeItemPool,(uint32_t)nativeItemPool.object_size,
        _Alignof(NativeItemPoolSlot),nativeItemPool.used,nativeItemPool.free_count,nativeItemPool.peak);
    bytes+=observe_pool(&nativeItemLinkPool,(uint32_t)nativeItemLinkPool.object_size,
        _Alignof(ItemLink),nativeItemLinkPool.used,nativeItemLinkPool.free_count,nativeItemLinkPool.peak);
    bytes+=observe_pool(&nativeAObjPool,(uint32_t)nativeAObjPool.object_size,
        _Alignof(HSD_AObj),nativeAObjPool.used,nativeAObjPool.free_count,nativeAObjPool.peak);
    bytes+=observe_pool(&nativeFObjPool,(uint32_t)nativeFObjPool.object_size,
        _Alignof(HSD_FObj),nativeFObjPool.used,nativeFObjPool.free_count,nativeFObjPool.peak);
    for(const HSD_ObjAllocData* pool=HSD_ObjAllocRegistry();pool;pool=pool->next){
        if(visited==Types){fail("ObjAlloc registry bound/cycle");break;}
        for(unsigned j=0;j<visited;++j)if(census.seen_types[j]==pool){fail("ObjAlloc registry cycle");return bytes;}
        census.seen_types[visited++]=pool;
        uint64_t payload=observe_pool(pool,pool->size,pool->align+1,pool->used,pool->free,pool->peak);
        if(UINT64_MAX-bytes<payload){fail("ObjAlloc payload-byte overflow");break;}
        bytes+=payload;
    }
    // Heap recreation forgets the previous registry before ObjInit registers
    // the reset descriptors. Missing satellite owners are now a real fault,
    // rather than silently supplementing an incomplete registry.
    const HSD_ObjAllocData* known[]={HSD_VecGetAllocData(),HSD_MtxGetAllocData(),HSD_RObjGetAllocData(),
        HSD_RvalueObjGetAllocData(),HSD_IDGetAllocData()};
    for(unsigned i=0;i<sizeof(known)/sizeof(known[0]);++i){
        const HSD_ObjAllocData* pool=known[i];if(!pool){fail("named allocator unavailable");continue;}
        bool seen=false;for(unsigned j=0;j<visited;++j)if(census.seen_types[j]==pool)seen=true;
        if(!seen)fail("named allocator missing from registry");
    }
    return bytes;
}

static const char* pool_name(const void* identity)
{
    if(identity==&nativeGObjPool)return "gobj";
    if(identity==&nativeGObjProcPool)return "gobj_proc";
    if(identity==&nativeFighterPool)return "fighter_owner";
    if(identity==&nativeGroundPool)return "ground_owner";
    if(identity==&nativeItemPool)return "item_owner";
    if(identity==&nativeItemLinkPool)return "item_link_owner";
    if(identity==&nativeAObjPool)return "aobj";
    if(identity==&nativeFObjPool)return "fobj";
    if(identity==HSD_VecGetAllocData())return "vec";
    if(identity==HSD_MtxGetAllocData())return "mtx";
    if(identity==HSD_RObjGetAllocData())return "robj";
    if(identity==HSD_RvalueObjGetAllocData())return "rvalue";
    if(identity==HSD_IDGetAllocData())return "id";
    return "unnamed";
}

static uint64_t read_classes(void)
{
    unsigned count=0,stack=0;uint64_t bytes=0;
    census.class_stack[stack++]=&hsdClass;
    while(stack){
        const HSD_ClassInfo* info=census.class_stack[--stack];
        if(count==Types){fail("class registry bound/cycle");break;}
        for(unsigned j=0;j<count;++j)if(census.seen_types[j]==info){fail("class registry duplicate/cycle");return bytes;}
        census.seen_types[count++]=info;
        if(info->head.obj_size<0){fail("negative runtime class size");break;}
        unsigned index=0;for(;index<census.stats.class_types;++index)if(census.classes[index].identity==info)break;
        if(index==census.stats.class_types){
            if(index==Types){fail("class census capacity exceeded");break;}
            ++census.stats.class_types;census.classes[index]=(ClassCount){.identity=info};
        }
        ClassCount* entry=&census.classes[index];
        const char* name=info->head.class_name?info->head.class_name:"unnamed";
        size_t text=0;for(;text+1<sizeof(entry->name)&&name[text];++text)entry->name[text]=name[text];
        entry->name[text]=0;
        entry->size=(uint32_t)info->head.obj_size;entry->live=info->head.nb_exist;
        entry->peak=maximum(entry->peak,info->head.nb_peak);entry->max_live=maximum(entry->max_live,entry->live);
        bytes+=(uint64_t)entry->size*entry->live;
        if(info->head.next){if(stack==Types){fail("class traversal stack exceeded");break;}census.class_stack[stack++]=info->head.next;}
        if(info->head.child){if(stack==Types){fail("class traversal stack exceeded");break;}census.class_stack[stack++]=info->head.child;}
    }
    return bytes;
}

void nativeMatchObjectCensus_BeginScene(uint32_t scene)
{
    if(census.active)nativeMatchObjectCensus_FinishScene();
    FILE* output=census.output;bool failed=census.failed,opened=census.opened;uint64_t index=census.scene_index;
    memset(&census,0,sizeof(census));
    census.output=output;census.failed=failed;census.opened=opened;census.scene_index=index+1;
    census.stats.scene=scene;census.active=true;
    if(!census.opened){
        census.opened=true;const char* path=getenv("FREAK_MELEE_OBJECT_CENSUS");
        if(path&&path[0]){census.output=fopen(path,"wb");if(!census.output)fail("cannot open configured census output");}
    }
}

void nativeMatchObjectCensus_Sample(uint64_t frame,uint64_t input_sample,bool replaying)
{
    if(!census.active)return;
    census.frame=frame;census.input_sample=input_sample;
    ++census.stats.samples;census.stats.replay_samples+=replaying;
    census.stats.object_peak=maximum(census.stats.object_peak,nativeGObjPool.peak);
    census.stats.proc_peak=maximum(census.stats.proc_peak,nativeGObjProcPool.peak);
    (void)read_objects();
    uint64_t pool_bytes=read_pools(),class_bytes=read_classes();
    if(pool_bytes>census.stats.allocator_payload_peak)census.stats.allocator_payload_peak=pool_bytes;
    if(class_bytes>census.stats.class_payload_peak)census.stats.class_payload_peak=class_bytes;
    if(pool_bytes+class_bytes>census.stats.payload_peak)census.stats.payload_peak=pool_bytes+class_bytes;
}

void nativeMatchObjectCensus_FinishScene(void)
{
    if(!census.active)return;
    FILE* out=census.output?census.output:stdout;
    fprintf(out,"[object-census] scene_index=%llu scene=%u samples=%llu replay_samples=%llu last_frame=%llu last_input=%llu "
        "object_peak=%u proc_peak=%u linked_object_peak=%u linked_proc_peak=%u "
        "allocator_types=%u class_types=%u classifier_types=%u payload_peak=%llu allocator_payload_peak=%llu class_payload_peak=%llu errors=%u timing=UNTIMED\n",
        (unsigned long long)census.scene_index,census.stats.scene,(unsigned long long)census.stats.samples,
        (unsigned long long)census.stats.replay_samples,(unsigned long long)census.frame,(unsigned long long)census.input_sample,
        census.stats.object_peak,census.stats.proc_peak,census.stats.linked_object_peak,census.stats.linked_proc_peak,
        census.stats.allocator_types,census.stats.class_types,census.stats.classifier_types,
        (unsigned long long)census.stats.payload_peak,(unsigned long long)census.stats.allocator_payload_peak,
        (unsigned long long)census.stats.class_payload_peak,census.stats.errors);
    for(unsigned i=0;i<census.stats.allocator_types;++i){const PoolCount* p=&census.pools[i];
        fprintf(out,"[object-census] allocator=%p name=%s size=%u align=%u used_last=%u peak=%u free_last=%u max_used=%u max_reserved=%u shape_changes=%u payload_peak=%llu samples=%llu\n",
            p->identity,pool_name(p->identity),
            p->size,p->align,p->used,p->peak,p->free_count,p->max_used,p->max_reserved,p->shape_changes,
            (unsigned long long)p->payload_peak,(unsigned long long)p->samples);}
    for(unsigned i=0;i<census.stats.class_types;++i){const ClassCount* c=&census.classes[i];
        fprintf(out,"[object-census] class=%p name=%s size=%u live_last=%u peak=%u max_live=%u\n",c->identity,c->name,c->size,c->live,c->peak,c->max_live);}
    for(unsigned i=0;i<census.stats.classifier_types;++i){const GroupCount* g=&census.groups[i];
        fprintf(out,"[object-census] classifier=%u live_last=%u max_live=%u aggregate_live=%llu\n",g->classifier,g->live,g->max_live,(unsigned long long)g->samples);}
    for(unsigned i=0;i<Priorities;++i){
        if(census.priority_peak[i])fprintf(out,"[object-census] priority=%u max_procs=%u\n",i,census.priority_peak[i]);
        if(census.link_peak[i])fprintf(out,"[object-census] plink=%u max_objects=%u\n",i,census.link_peak[i]);}
    if(census.reason[0])fprintf(out,"[object-census] INVALID reason=%s\n",census.reason);
    if(fflush(out)!=0)fail("census output flush failed");
    OSReport("[object-census] Finished scene %u: GObj peak%u, proc peak%u, sampled payload%llu bytes, errors%u; untimed diagnostic.\n",
        census.stats.scene,census.stats.object_peak,census.stats.proc_peak,(unsigned long long)census.stats.payload_peak,census.stats.errors);
    census.active=false;
}
void nativeMatchObjectCensus_Close(void)
{
    nativeMatchObjectCensus_FinishScene();
    if(census.output){if(fclose(census.output)!=0)fail("census output close failed");census.output=NULL;}
}
const NativeMatchObjectCensusStats* nativeMatchObjectCensus_Stats(void){return &census.stats;}
bool nativeMatchObjectCensus_HasFailed(void){return census.failed;}
void nativeMatchObjectCensus_Abort(const char* reason)
{
    fail(reason?reason:"census aborted");
    OSReport("[object-census] INVALID: %s\n",census.reason);
}
