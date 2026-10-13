#include "native_snapshot_actor_graph.h"
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/fighter.h>
#include <melee/gr/native_ground_pool.h>
#include <melee/gr/types.h>
#include <melee/it/native_item_pool.h>
#include <melee/it/item.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <string.h>

static bool shape(NativeMatchPoolSnapshotView view,const NativeMatchPool* pool,
                  size_t object_size,uint64_t epoch)
{
    return view.records && view.header.pool==pool &&
        view.header.object_size==object_size && view.header.lifetime==epoch &&
        pool->lifetime==epoch;
}
static uint32_t slot_number(const void* pointer,NativeMatchPoolSnapshotView view)
{
    uintptr_t at=(uintptr_t)pointer,base=(uintptr_t)view.header.payload;
    if(!pointer || at<base || !view.header.stride)return UINT32_MAX;
    uintptr_t offset=at-base;
    if(offset%view.header.stride || offset/view.header.stride>=view.header.capacity)return UINT32_MAX;
    return (uint32_t)(offset/view.header.stride);
}
static const void* root_pointer(const void* actor,size_t offset)
{
    const void* root;
    memcpy(&root,(const uint8_t*)actor+offset,sizeof(root));
    return root;
}
static bool actor_roots(NativeMatchPoolSnapshotView actors,NativeMatchPoolSnapshotView objects,
                        size_t root_offset,unsigned classifier,unsigned userdata_kind,
                        void (*remove)(void*))
{
    size_t record_bytes=sizeof(uint32_t)+actors.header.object_size;
    for(uint32_t i=0;i<actors.header.used;++i) {
        const uint8_t* record=actors.records+(size_t)i*record_bytes;
        uint32_t slot;memcpy(&slot,record,sizeof(slot));
        const void* root=root_pointer(record+sizeof(slot),root_offset);
        const void* payload=nativeMatchSnapshotPool_Find(objects,slot_number(root,objects));
        if(!payload)return false;
        HSD_GObj object;memcpy(&object,payload,sizeof(object));
        const void* actor= (const uint8_t*)actors.header.payload+(size_t)slot*actors.header.stride;
        if(object.classifier!=classifier || object.user_data_kind!=userdata_kind ||
           object.user_data!=actor || object.user_data_remove_func!=remove)return false;
    }
    return true;
}
static bool reverse_root(const HSD_GObj* object,const void* address,
                         NativeMatchPoolSnapshotView actors,size_t offset,
                         unsigned classifier,unsigned userdata_kind,void (*remove)(void*))
{
    if(object->classifier!=classifier || object->user_data_kind!=userdata_kind ||
       object->user_data_remove_func!=remove)return false;
    const void* actor=nativeMatchSnapshotPool_Find(actors,slot_number(object->user_data,actors));
    return actor && root_pointer(actor,offset)==address;
}
bool nativeSnapshotActorGraph_Validate(const NativeSnapshotActorGraph* graph)
{
    if(!graph || !graph->objects.records)return false;
    uint64_t epoch=graph->objects.header.lifetime;
    if(!shape(graph->objects,&nativeGObjPool,sizeof(HSD_GObj),epoch) ||
       !shape(graph->fighters,&nativeFighterPool,sizeof(NativeFighterPoolSlot),epoch) ||
       !shape(graph->ground,&nativeGroundPool,sizeof(Ground),epoch) ||
       !shape(graph->items,&nativeItemPool,sizeof(NativeItemPoolSlot),epoch))return false;
    if(!actor_roots(graph->fighters,graph->objects,offsetof(Fighter,gobj),
                    HSD_GOBJ_CLASS_FIGHTER,4,Fighter_Unload_8006DABC) ||
       !actor_roots(graph->ground,graph->objects,offsetof(Ground,gobj),
                    HSD_GOBJ_CLASS_STAGE,3,nativeGroundPool_Release) ||
       !actor_roots(graph->items,graph->objects,offsetof(Item,entity),
                    HSD_GOBJ_CLASS_ITEM,6,Item_OnUserDataRemove))return false;
    size_t record_bytes=sizeof(uint32_t)+graph->objects.header.object_size;
    for(uint32_t i=0;i<graph->objects.header.used;++i) {
        const uint8_t* record=graph->objects.records+(size_t)i*record_bytes;
        uint32_t slot;memcpy(&slot,record,sizeof(slot));
        HSD_GObj object;memcpy(&object,record+sizeof(slot),sizeof(object));
        const void* address=(const uint8_t*)graph->objects.header.payload+(size_t)slot*graph->objects.header.stride;
        if(object.classifier==HSD_GOBJ_CLASS_FIGHTER || object.user_data_remove_func==Fighter_Unload_8006DABC) {
            if(!reverse_root(&object,address,graph->fighters,offsetof(Fighter,gobj),
                             HSD_GOBJ_CLASS_FIGHTER,4,Fighter_Unload_8006DABC))return false;
        } else if(object.classifier==HSD_GOBJ_CLASS_ITEM || object.user_data_remove_func==Item_OnUserDataRemove) {
            if(!reverse_root(&object,address,graph->items,offsetof(Item,entity),
                             HSD_GOBJ_CLASS_ITEM,6,Item_OnUserDataRemove))return false;
        } else if(object.user_data_remove_func==nativeGroundPool_Release) {
            // Some other stage GObjs (e.g. Fountain's reflection camera) also
            // use userdata kind 3. The destructor identifies actual Ground.
            if(!reverse_root(&object,address,graph->ground,offsetof(Ground,gobj),
                             HSD_GOBJ_CLASS_STAGE,3,nativeGroundPool_Release))return false;
        }
    }
    return true;
}
