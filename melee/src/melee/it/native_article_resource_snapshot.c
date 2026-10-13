#include "native_article_resource_snapshot.h"
#include "native_article_resource_scene.h"
#include "native_article_resource_model.h"
#include "native_item_link_pool.h"
#include "native_item_pool.h"
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/native_script_runtime.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/native_hsd_reservation.h>
#include <sysdolphin/baselib/native_objalloc_snapshot.h>
#include <string.h>

enum { PREFIX=offsetof(ResourceCollection,nodes) };
static bool overlap(const void* a,size_t an,const void* b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y || (x<y+bn && y<x+an);
}
static bool independent_pool(const NativeMatchPool* p,const void* buffer,size_t bytes)
{
    return !overlap(buffer,bytes,p,sizeof(*p)) &&
        !overlap(buffer,bytes,p->payload,p->capacity*p->stride) &&
        !overlap(buffer,bytes,p->slots,p->capacity*sizeof(*p->slots)) &&
        !overlap(buffer,bytes,p->free_order,p->capacity*sizeof(*p->free_order));
}
static bool independent(const void* buffer,size_t bytes)
{
    return buffer && bytes<=UINTPTR_MAX-(uintptr_t)buffer &&
        !overlap(buffer,bytes,nativeArticleResourceCollections,sizeof(nativeArticleResourceCollections)) &&
        !overlap(buffer,bytes,&nativeArticleResourceGeneration,sizeof(nativeArticleResourceGeneration)) &&
        independent_pool(&nativeGObjPool,buffer,bytes) &&
        independent_pool(&nativeGObjProcPool,buffer,bytes) &&
        independent_pool(&nativeItemLinkPool,buffer,bytes) &&
        independent_pool(&nativeItemPool,buffer,bytes) &&
        independent_pool(&nativeFighterPool,buffer,bytes) &&
        nativeArticleResourceScene_BufferIndependent(buffer,bytes) &&
        nativeArticleResourceModel_BufferIndependent(buffer,bytes) &&
        nativeObjAlloc_BufferIndependent(buffer,bytes) &&
        nativeHsdReservation_BufferIndependent(buffer,bytes);
}
static bool boundary(void)
{
    if(!nativeScriptRuntime_Idle() || nativeHsdReservation_Active() ||
       HSD_GObj_CurrentInvokedProc || HSD_GObj_CurrentInvokedProcGObj ||
       HSD_GObj_RenderCallbackDepth || HSD_GObj_804D7818 || HSD_GObj_DelayedProcInfo.flags ||
       !nativeArticleResourceScene_SnapshotIdentity() ||
       !nativeArticleResourceModel_SnapshotIdentity())return false;
    for(unsigned i=0;i<NATIVE_ARTICLE_RESOURCE_COLLECTIONS;++i){
        const ResourceCollection* set=&nativeArticleResourceCollections[i];
        if(set->active_operations || set->releasing || set->destroying)return false;
    }
    return true;
}
static bool reference(NativeMatchPoolReference ref,const NativeMatchPool* p,const void* address)
{
    return ref.pool==p && ref.lifetime==p->lifetime && p->payload &&
        ref.slot<p->capacity && ref.generation &&
        p->payload+ref.slot*p->stride==(const uint8_t*)address;
}
static bool fighter_parts(const ResourceCollection* set)
{
    // Validate the candidate's physical addresses without resolving occupancy:
    // the saved Fighter may currently be destroyed or its slot reused.
    const NativeMatchPool* pool=&nativeFighterPool;
    uintptr_t base=(uintptr_t)pool->payload,address=(uintptr_t)set->attachment.data;
    if(!pool->payload || pool->stride!=sizeof(NativeFighterPoolSlot) ||
       address<base || address-base>=pool->capacity*pool->stride ||
       (address-base)%pool->stride || set->part_count>MAX_FT_PARTS)return false;
    const NativeFighterPoolSlot* slot=(const NativeFighterPoolSlot*)address;
    return set->parts==slot->parts;
}
static bool record(const ResourceCollection* set,uint32_t generation)
{
    if(!set->owner.object || !set->generation || set->generation>generation ||
       !set->owner.generation || !set->attachment.generation ||
       set->active_operations || set->releasing || set->destroying ||
       set->count<2 || set->count>NATIVE_ARTICLE_RESOURCE_NODES || set->created>set->count ||
       !reference(set->owner.reference,&nativeGObjPool,set->owner.object) ||
       !reference(set->attachment.reference,&nativeGObjPool,set->attachment.object) ||
       !set->owner.data || !set->attachment.data || !set->parts || !set->rig ||
       !fighter_parts(set) ||
       (unsigned)set->attachment.kind>=Ft_Kind_RuntimeCapacity || !ftPartsTable ||
       !set->parts_table || set->parts_table!=ftPartsTable[set->attachment.kind] ||
       set->parts_table->parts_num!=set->part_count ||
       !nativeArticleResourceModel_SnapshotProgram(&set->owner))return false;
    const NativeScriptItemProgram* p=set->owner.program;
    if(!nativeArticleResource_ValidateRecipe(p->resource_capacity,p->resource_reserved_part,set->part_count) ||
       set->count>p->resource_capacity || set->external_part!=p->resource_reserved_part)return false;
    for(unsigned i=0;i<set->created;++i){
        const ResourceNode* node=&set->nodes[i];
        if(node->generation!=set->generation || node->object_retired ||
           !reference(node->payload,&nativeItemLinkPool,node->link) ||
           node->object.pool!=&nativeGObjPool || node->object.lifetime!=nativeGObjPool.lifetime ||
           node->object.slot>=nativeGObjPool.capacity || !node->object.generation ||
           node->attachment_part>=set->part_count || !node->attachment_joint ||
           (node->bound_part&&set->external_part<0) ||
           !nativeArticleResourceModel_SnapshotGraph(node,p))return false;
        for(unsigned j=0;j<i;++j)
            if(set->nodes[j].payload.slot==node->payload.slot ||
               set->nodes[j].object.slot==node->object.slot)return false;
    }
    return true;
}
static size_t record_bytes(unsigned created)
{ return sizeof(uint32_t)+PREFIX+(size_t)created*sizeof(ResourceNode); }

size_t nativeArticleResource_SnapshotBytes(void)
{
    if(!boundary())return 0;
    size_t bytes=sizeof(NativeArticleResourceSnapshotHeader);
    for(unsigned i=0;i<NATIVE_ARTICLE_RESOURCE_COLLECTIONS;++i){
        const ResourceCollection* set=&nativeArticleResourceCollections[i];
        if(!set->owner.object)continue;
        if(set->created>NATIVE_ARTICLE_RESOURCE_NODES)return 0;
        bytes+=record_bytes(set->created);
    }
    return bytes;
}
bool nativeArticleResource_Save(void* buffer,size_t capacity,size_t* written)
{
    size_t bytes=nativeArticleResource_SnapshotBytes();
    if(!bytes || capacity<bytes || !independent(buffer,capacity) ||
       !independent(written,sizeof(*written)) || overlap(buffer,capacity,written,sizeof(*written)))return false;
    NativeArticleResourceSnapshotHeader header={.scene_identity=nativeArticleResourceScene_SnapshotIdentity(),
        .model_identity=nativeArticleResourceModel_SnapshotIdentity(),.object_epoch=nativeGObjPool.lifetime,
        .generation=nativeArticleResourceGeneration};
    // Refuse an inconsistent live registry before writing output. Candidate
    // validation later must not inspect the current graph, which can be reused.
    for(unsigned i=0;i<NATIVE_ARTICLE_RESOURCE_COLLECTIONS;++i){
        const ResourceCollection* set=&nativeArticleResourceCollections[i];
        if(!set->owner.object)continue;
        if(!record(set,header.generation) || !nativeArticleResource_AttachmentCurrent(set))return false;
        for(unsigned n=0;n<set->created;++n){ResourceCapture view;
            if(!nativeArticleResource_Capture(set->owner.object,n,&view))return false;}
        ++header.collections;
    }
    uint8_t* cursor=(uint8_t*)buffer+sizeof(header);
    for(uint32_t i=0;i<NATIVE_ARTICLE_RESOURCE_COLLECTIONS;++i){
        const ResourceCollection* set=&nativeArticleResourceCollections[i];
        if(!set->owner.object)continue;
        memcpy(cursor,&i,sizeof(i));cursor+=sizeof(i);
        size_t count=PREFIX+(size_t)set->created*sizeof(ResourceNode);
        memcpy(cursor,set,count);cursor+=count;
    }
    memcpy(buffer,&header,sizeof(header));*written=bytes;return true;
}
bool nativeArticleResource_ValidateSnapshot(const void* buffer,size_t bytes)
{
    if(!boundary() || bytes<sizeof(NativeArticleResourceSnapshotHeader) || !independent(buffer,bytes))return false;
    NativeArticleResourceSnapshotHeader h;memcpy(&h,buffer,sizeof(h));
    if(h.scene_identity!=nativeArticleResourceScene_SnapshotIdentity() ||
       h.model_identity!=nativeArticleResourceModel_SnapshotIdentity() ||
       h.object_epoch!=nativeGObjPool.lifetime || h.collections>NATIVE_ARTICLE_RESOURCE_COLLECTIONS)return false;
    const uint8_t* cursor=(const uint8_t*)buffer+sizeof(h);size_t left=bytes-sizeof(h);
    uint32_t previous=UINT32_MAX;
    NativeMatchPoolReference owners[NATIVE_ARTICLE_RESOURCE_COLLECTIONS];
    uint32_t generations[NATIVE_ARTICLE_RESOURCE_COLLECTIONS];
    uint8_t links[NATIVE_ITEM_LINK_CAPACITY/8]={0},objects[NATIVE_GOBJ_CAPACITY/8]={0};
    for(unsigned i=0;i<h.collections;++i){
        if(left<sizeof(uint32_t)+PREFIX)return false;
        uint32_t slot;memcpy(&slot,cursor,sizeof(slot));
        if(slot>=NATIVE_ARTICLE_RESOURCE_COLLECTIONS || (i&&slot<=previous))return false;
        ResourceCollection set={0};memcpy(&set,cursor+sizeof(slot),PREFIX);
        if(set.created>NATIVE_ARTICLE_RESOURCE_NODES || left<record_bytes(set.created))return false;
        memcpy(set.nodes,cursor+sizeof(slot)+PREFIX,set.created*sizeof(ResourceNode));
        if(!record(&set,h.generation))return false;
        for(unsigned n=0;n<set.created;++n){
            unsigned link=set.nodes[n].payload.slot,object=set.nodes[n].object.slot;
            if((links[link/8]&(1u<<(link%8))) || (objects[object/8]&(1u<<(object%8))))return false;
            links[link/8]|=(uint8_t)(1u<<(link%8));objects[object/8]|=(uint8_t)(1u<<(object%8));
        }
        for(unsigned j=0;j<i;++j)
            if(owners[j].slot==set.owner.reference.slot || generations[j]==set.generation)return false;
        owners[i]=set.owner.reference;generations[i]=set.generation;previous=slot;
        size_t count=record_bytes(set.created);cursor+=count;left-=count;
    }
    return left==0;
}
bool nativeArticleResource_Load(const void* buffer,size_t bytes)
{
    if(!nativeArticleResource_ValidateSnapshot(buffer,bytes))return false;
    NativeArticleResourceSnapshotHeader h;memcpy(&h,buffer,sizeof(h));
    const uint8_t* cursor=(const uint8_t*)buffer+sizeof(h);
    memset(nativeArticleResourceCollections,0,sizeof(nativeArticleResourceCollections));
    for(unsigned i=0;i<h.collections;++i){
        uint32_t slot;memcpy(&slot,cursor,sizeof(slot));cursor+=sizeof(slot);
        ResourceCollection* set=&nativeArticleResourceCollections[slot];
        memcpy(set,cursor,PREFIX);size_t count=PREFIX+set->created*sizeof(ResourceNode);
        memcpy(set,cursor,count);cursor+=count;
    }
    nativeArticleResourceGeneration=h.generation;return true;
}

// Empty collection prefixes are copied too, so a discarded future collection
// cannot become visible when the first-free search runs again. Nodes are read
// only below created and are overwritten in full before publishing that count.
bool nativeArticleResource_AdmitHistory(NativeMatchArena* arena)
{
    if (!boundary() || !arena || arena->lifetime!=nativeGObjPool.lifetime)
        return false;
    if (!nativeMatchArena_AddRoot(arena,&nativeArticleResourceGeneration,
            sizeof(nativeArticleResourceGeneration))) return false;
    for (unsigned i=0;i<NATIVE_ARTICLE_RESOURCE_COLLECTIONS;++i) {
        ResourceCollection* set=&nativeArticleResourceCollections[i];
        if (set->created>NATIVE_ARTICLE_RESOURCE_NODES ||
            !nativeMatchArena_AddRoot(arena,set,PREFIX) ||
            !nativeMatchArena_AddCountedRoot(arena,set->nodes,sizeof(*set->nodes),
                NATIVE_ARTICLE_RESOURCE_NODES,&set->created)) return false;
    }
    return true;
}
