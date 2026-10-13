#include "native_article_resource.h"

#ifdef MELEE_NATIVE
#include "native_article_resource_internal.h"
#include "native_article_resource_scene.h"
#include "native_item_link_pool.h"
#include <sysdolphin/baselib/native_hsd_reservation.h>
#include "native_article_resource_model.h"
#include <melee/ft/fighter.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/ef/efsync.h>
#include <melee/mp/mpcoll.h>
#include <melee/mp/mplib.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/jobj.h>
#include <math.h>
#include <string.h>

// Real scene state. Its payload backing and this registry are covered by the
// production main-heap/.mstate page owner, not by Item actor snapshots alone.
ResourceCollection nativeArticleResourceCollections[NATIVE_ARTICLE_RESOURCE_COLLECTIONS];
uint32_t nativeArticleResourceGeneration;

static bool registered(const ResourceCollection* set)
{
    for (unsigned i = 0; i < NATIVE_ARTICLE_RESOURCE_COLLECTIONS; ++i)
        if (set == &nativeArticleResourceCollections[i]) return true;
    return false;
}

static bool same_reference(NativeMatchPoolReference a, NativeMatchPoolReference b)
{
    return a.pool == b.pool && a.lifetime == b.lifetime &&
        a.slot == b.slot && a.generation == b.generation;
}

static bool owner_current(const ResourceCollection* set, NativeItemLifetime* output)
{
    if (!registered(set) || !set->owner.object || !set->generation || set->releasing ||
        nativeMatchPool_Resolve(set->owner.reference) != set->owner.object ||
        set->owner.reference.lifetime != nativeGObjPool.lifetime ||
        set->owner.object->classifier != HSD_GOBJ_CLASS_ITEM ||
        set->owner.object->user_data != set->owner.data ||
        !nativeCallbackSource_Alive(set->owner.object, set->owner.generation, NativeCallbackSource_Item)) return false;
    // Removal is a live engine phase: Destroyed may release its own resources
    // before the Item storage logically retires. Preserve all other identities.
    NativeItemLifetime retained = set->owner;
    retained.removing = retained.data->native_removal_in_progress;
    if (!nativeItemLifetime_Current(&retained) || !retained.source_owned ||
        retained.program->owner != retained.data->native_script_owner ||
        retained.program->author_kind != retained.data->native_article_author_kind ||
        retained.program->article != retained.article || retained.program->kind != retained.kind ||
        retained.program->resource_capacity < set->count ||
        retained.program->resource_reserved_part != set->external_part) return false;
    if (output) *output = retained;
    return true;
}

ResourceCollection* nativeArticleResource_Collection(Item_GObj* owner)
{
    if (!owner) return NULL;
    for (unsigned i = 0; i < NATIVE_ARTICLE_RESOURCE_COLLECTIONS; ++i) {
        ResourceCollection* set = &nativeArticleResourceCollections[i];
        if (set->owner.object == owner && owner_current(set, NULL)) return set;
    }
    return NULL;
}

bool nativeArticleResource_AttachmentPhysical(const ResourceCollection* set)
{
    if (!registered(set) || !set->attachment.object || !set->attachment.data ||
        nativeMatchPool_Resolve(set->attachment.reference) != set->attachment.object ||
        set->attachment.reference.lifetime != nativeGObjPool.lifetime ||
        set->attachment.object->classifier != HSD_GOBJ_CLASS_FIGHTER ||
        set->attachment.object->user_data != set->attachment.data)
        return false;
    Fighter* fighter = set->attachment.data;
    // NewLife changes logical identity during construction. Failed teardown
    // still owns this exact rig and must detach its Load-era borrowed pointers.
    if (!nativeCallbackSource_Alive(set->attachment.object,
            set->attachment.generation, NativeCallbackSource_Fighter) &&
        !(fighter->native_constructing &&
          (set->attachment.object->native_destroying ||
           set->attachment.object->native_userdata_removing))) return false;
    return fighter->gobj == set->attachment.object && fighter->kind == set->attachment.kind &&
        fighter->ft_data == set->attachment.fighter_data && fighter->parts == set->parts &&
        set->attachment.object->hsd_obj == set->rig && set->attachment.object->obj_kind == set->rig_kind &&
        (unsigned)fighter->kind < Ft_Kind_RuntimeCapacity && ftPartsTable &&
        ftPartsTable[fighter->kind] == set->parts_table && set->parts_table &&
        set->parts_table->parts_num == set->part_count;
}

bool nativeArticleResource_AttachmentCurrent(const ResourceCollection* set)
{
    return owner_current(set, NULL) && nativeArticleResource_AttachmentPhysical(set) &&
        nativeFighterLifetime_Current(&set->attachment) &&
        nativeScriptItem_AttachmentOwner(set->owner.object) == set->attachment.object;
}

bool nativeArticleResource_CapturePhysical(ResourceCollection* set, ResourceNode* node,
                                         ResourceCapture* output)
{
    if (!output || !registered(set) || !set->generation || set->created > NATIVE_ARTICLE_RESOURCE_NODES)
        return false;
    unsigned index;
    for (index = 0; index < set->created; ++index) if (node == &set->nodes[index]) break;
    if (index == set->created || !node->link || node->object_retired ||
        node->generation != set->generation || nativeMatchPool_Resolve(node->payload) != node->link ||
        node->payload.lifetime != nativeGObjPool.lifetime) return false;
    HSD_GObj* object = nativeMatchPool_Resolve(node->object);
    if (!object || node->object.lifetime != nativeGObjPool.lifetime ||
        object->classifier != HSD_GOBJ_CLASS_ITEMLINK || object->user_data != node->link ||
        node->link->gobj != object || object->hsd_obj != node->model ||
        (node->model && (object->obj_kind != HSD_GObj_JObjKind ||
                        nativeMatchPool_Resolve(node->graph) != node->model || !node->descriptor)) ||
        (!node->model && (node->graph.pool || node->descriptor))) return false;
    *output = (ResourceCapture){.collection=set,.node=node,.owner=set->owner,
        .payload=node->payload,.object=node->object,.graph=node->graph,
        .link=node->link,.gobj=object,.model=node->model,.descriptor=node->descriptor,
        .generation=set->generation,.index=index,.root_index=node->root_index,
        .object_kind=object->obj_kind};
    return true;
}

bool nativeArticleResource_Capture(Item_GObj* owner, unsigned index, ResourceCapture* output)
{
    if (!output) return false;
    ResourceCollection* set = nativeArticleResource_Collection(owner);
    ResourceCapture value;
    if (!set || index >= set->created || !nativeArticleResource_CapturePhysical(set, &set->nodes[index], &value) ||
        !owner_current(set, &value.owner)) return false;
    *output = value;
    return true;
}

bool nativeArticleResource_Current(const ResourceCapture* saved)
{
    if (!saved || !registered(saved->collection) || saved->collection->generation != saved->generation ||
        saved->index >= saved->collection->created || saved->node != &saved->collection->nodes[saved->index] ||
        !nativeItemLifetime_Current(&saved->owner) || !owner_current(saved->collection, NULL)) return false;
    ResourceCapture now;
    return nativeArticleResource_CapturePhysical(saved->collection, saved->node, &now) &&
        same_reference(now.payload, saved->payload) && same_reference(now.object, saved->object) &&
        same_reference(now.graph, saved->graph) && now.link == saved->link && now.gobj == saved->gobj &&
        now.model == saved->model && now.descriptor == saved->descriptor &&
        now.root_index == saved->root_index && now.object_kind == saved->object_kind;
}

bool nativeArticleResource_BeginOperation(Item_GObj* owner, unsigned index, ResourceCapture* output)
{
    ResourceCapture value;
    if (!output || !nativeArticleResource_Capture(owner, index, &value) ||
        value.collection->active_operations == UINT16_MAX) return false;
    ++value.collection->active_operations;
    value.active = true;
    *output = value;
    return true;
}

bool nativeArticleResource_EndOperation(ResourceCapture* saved)
{
    if (!saved || !saved->active || !registered(saved->collection) ||
        saved->collection->generation != saved->generation || !saved->collection->active_operations) return false;
    bool current = nativeArticleResource_Current(saved);
    ResourceCollection* set = saved->collection;
    --set->active_operations;
    saved->active = false;
    if (set->releasing) nativeArticleResource_ReleaseCollection(set);
    return current;
}

static bool common_pull_state(const Fighter* fighter)
{
    return fighter->motion_id == ftCo_MS_Catch || fighter->motion_id == ftCo_MS_CatchDash ||
        fighter->motion_id == ftCo_MS_CatchPull || fighter->motion_id == ftCo_MS_CatchDashPull ||
        fighter->motion_id == ftCo_MS_CatchWait || fighter->motion_id == ftCo_MS_CatchAttack;
}

void nativeArticleResource_DetachBorrowers(ResourceCollection* set, ResourceNode* entry)
{
    if (!entry->model || !nativeArticleResource_AttachmentPhysical(set)) return;
    Fighter* fighter = set->attachment.data;
    HSD_JObj* model = entry->model;
    if (entry->bound_capture && common_pull_state(fighter) && fighter->mv.co.capturedamage.x18 == model) {
        unsigned part = fighter->ft_data && fighter->ft_data->x8 ? fighter->ft_data->x8->x11 : UINT32_MAX;
        fighter->mv.co.capturedamage.x18 = fighter->parts && part < set->part_count ? fighter->parts[part].joint : NULL;
    }
    if (entry->bound_part && set->external_part >= 0 && set->external_part < MAX_FT_PARTS &&
        fighter->parts && fighter->parts[set->external_part].joint == model)
        fighter->parts[set->external_part].joint = NULL;
    for (unsigned i = 0; i < 4; ++i) {
        HitCapsule* hit = &fighter->x914[i];
        if (hit->jobj == model) { hit->state = HitCapsule_Disabled; hit->jobj = NULL; }
    }
    if (fighter->x1064_thrownHitbox.jobj == model) {
        fighter->x1064_thrownHitbox.state = HitCapsule_Disabled;
        fighter->x1064_thrownHitbox.jobj = NULL;
    }
    entry->bound_part = entry->bound_capture = false;
}

static void free_link(void* payload)
{
    // Native GObj frees userdata before the object graph. Revoke borrowers and
    // the graph registry even on an unexpected direct endpoint GObj removal.
    for (unsigned i = 0; i < NATIVE_ARTICLE_RESOURCE_COLLECTIONS; ++i) {
        ResourceCollection* set = &nativeArticleResourceCollections[i];
        for (unsigned n = 0; n < set->created; ++n) {
            ResourceNode* entry = &set->nodes[n];
            if (entry->link != payload || nativeMatchPool_Resolve(entry->payload) != payload) continue;
            nativeArticleResource_DetachBorrowers(set, entry);
            bool removed = nativeArticleResourceModel_Remove(set, entry);
            HSD_ASSERTREPORT(0, removed, "Resource endpoint removal could not retire its admitted graph");
            entry->object_retired = true;
            set->releasing = true;
            if (set->active_operations) return;
            bool freed = nativeMatchPool_Free(&nativeItemLinkPool, entry->payload);
            HSD_ASSERTREPORT(0, freed, "Resource node payload retirement lost its allocation identity");
            entry->link = NULL;
            if (!set->destroying) nativeArticleResource_ReleaseCollection(set);
            return;
        }
    }
    HSD_ASSERTREPORT(0, false, "Unregistered source resource node userdata");
}

void nativeArticleResource_ReleaseCollection(ResourceCollection* set)
{
    if (!registered(set) || !set->owner.object || set->destroying) return;
    set->releasing = true;
    // Every borrowed pointer is detached before any owned graph is freed.
    // CancelCreator runs before the Fighter rig is freed, even when an outer
    // collision operation still retains the node's payload until it returns.
    for (unsigned i = 0; i < set->created; ++i)
        if (set->nodes[i].link) nativeArticleResource_DetachBorrowers(set, &set->nodes[i]);
    if (set->active_operations) return;
    set->destroying = true;
    for (unsigned i = set->created; i; --i) {
        ResourceNode* entry = &set->nodes[i-1];
        if (!entry->link || nativeMatchPool_Resolve(entry->payload) != entry->link) continue;
        if (!entry->object_retired) {
            HSD_GObj* object = nativeMatchPool_Resolve(entry->object);
            if (object && object->classifier == HSD_GOBJ_CLASS_ITEMLINK && object->user_data == entry->link) {
                if (!nativeArticleResourceModel_Remove(set, entry)) { set->destroying=false; return; }
                HSD_GObjFree(object);
                // A current proc can defer physical free. The payload callback
                // remains the sole allocation retire path; never free it twice.
                if (nativeMatchPool_Resolve(entry->payload)) continue;
            } else {
                // The graph registry retains its own allocation identity; it
                // must retire that graph without touching a substituted GObj.
                if (!nativeArticleResourceModel_Remove(set, entry)) { set->destroying=false; return; }
                entry->object_retired = true;
            }
        }
        if (entry->object_retired && nativeMatchPool_Resolve(entry->payload)) {
            bool freed = nativeMatchPool_Free(&nativeItemLinkPool, entry->payload);
            HSD_ASSERTREPORT(0, freed, "Resource node deferred payload retirement failed");
        }
        entry->link = NULL;
    }
    // Retain the teardown tombstone for any deferred GObj userdata callback.
    for (unsigned i = 0; i < set->created; ++i) if (set->nodes[i].link) { set->destroying=false; return; }
    memset(set, 0, sizeof(*set));
}

void nativeArticleResource_Release(Item_GObj* owner)
{
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(owner, NativeCallbackSource_Item, &generation)) return;
    nativeArticleResource_CancelGeneration(owner, generation);
}
void nativeArticleResource_CancelGeneration(Item_GObj* owner, uint32_t generation)
{
    for (unsigned i = 0; i < NATIVE_ARTICLE_RESOURCE_COLLECTIONS; ++i) {
        ResourceCollection* set = &nativeArticleResourceCollections[i];
        if (set->owner.object == owner && set->owner.generation == generation)
            nativeArticleResource_ReleaseCollection(set);
    }
}
void nativeArticleResource_CancelCreator(Fighter_GObj* creator, uint32_t generation)
{
    for (unsigned i = 0; i < NATIVE_ARTICLE_RESOURCE_COLLECTIONS; ++i) {
        ResourceCollection* set = &nativeArticleResourceCollections[i];
        if (set->attachment.object == creator && set->attachment.generation == generation)
            nativeArticleResource_ReleaseCollection(set);
    }
}
void nativeArticleResource_CancelConstructingAttachment(Fighter_GObj* object,
    NativeMatchPoolReference object_reference,
    NativeMatchPoolReference fighter_reference)
{
    if (!object || object_reference.pool != &nativeGObjPool ||
        fighter_reference.pool != &nativeFighterPool ||
        object_reference.lifetime != fighter_reference.lifetime ||
        nativeMatchPool_Resolve(object_reference) != object) return;
    Fighter* fighter = nativeMatchPool_Resolve(fighter_reference);
    if (!fighter || object->user_data != fighter || fighter->gobj != object ||
        !fighter->native_constructing ||
        !(object->native_destroying || object->native_userdata_removing)) return;
    for (unsigned i = 0; i < NATIVE_ARTICLE_RESOURCE_COLLECTIONS; ++i) {
        if (nativeMatchPool_Resolve(object_reference) != object ||
            nativeMatchPool_Resolve(fighter_reference) != fighter ||
            object->user_data != fighter) break;
        ResourceCollection* set = &nativeArticleResourceCollections[i];
        if (set->attachment.object == object && set->attachment.data == fighter &&
            same_reference(set->attachment.reference, object_reference))
            nativeArticleResource_ReleaseCollection(set);
    }
}
void nativeArticleResource_Clear(void)
{
    for (unsigned i = 0; i < NATIVE_ARTICLE_RESOURCE_COLLECTIONS; ++i)
        nativeArticleResource_ReleaseCollection(&nativeArticleResourceCollections[i]);
}

bool nativeArticleResource_ValidateRecipe(unsigned capacity, int reserved_part, unsigned actual_parts)
{
    if (actual_parts > MAX_FT_PARTS || reserved_part < -1 || reserved_part >= MAX_FT_PARTS) return false;
    if (!capacity) return reserved_part == -1;
    return capacity >= 2 && capacity <= NATIVE_ARTICLE_RESOURCE_NODES &&
        (reserved_part < 0 || (unsigned)reserved_part >= actual_parts);
}

bool nativeArticleResource_Begin(Item_GObj* owner, unsigned count)
{
    NativeItemLifetime source;
    if (!nativeArticleResourceScene_Ready() || !nativeItemLifetime_Capture(owner, &source) ||
        !source.source_owned || !nativeArticleResourceModel_OwnerReady(&source) ||
        source.program->owner != source.data->native_script_owner ||
        source.program->author_kind != source.data->native_article_author_kind ||
        source.program->article != source.article || source.program->kind != source.kind ||
        !source.program->resource_capacity || count < 2 || count > source.program->resource_capacity ||
        nativeArticleResourceGeneration == UINT32_MAX) return false;
    Fighter_GObj* attachment = nativeScriptItem_AttachmentOwner(owner);
    NativeMatchPoolReference reference;
    NativeFighterLifetime fighter;
    if (!nativeMatchPool_Reference(&nativeGObjPool, attachment, &reference) ||
        attachment->classifier != HSD_GOBJ_CLASS_FIGHTER || !attachment->user_data ||
        !nativeFighterLifetime_Capture(attachment->user_data, &fighter) ||
        (unsigned)fighter.kind >= Ft_Kind_RuntimeCapacity || !ftPartsTable || !ftPartsTable[fighter.kind] ||
        !nativeArticleResource_ValidateRecipe(source.program->resource_capacity,
            source.program->resource_reserved_part, ftPartsTable[fighter.kind]->parts_num)) return false;
    int external = source.program->resource_reserved_part;
    if (external >= 0 && !fighter.data->parts) return false;
    ResourceCollection* available = NULL;
    for (unsigned i = 0; i < NATIVE_ARTICLE_RESOURCE_COLLECTIONS; ++i) {
        ResourceCollection* set = &nativeArticleResourceCollections[i];
        if (set->owner.object == owner) return false;
        if (external >= 0 && set->owner.object && set->external_part == external &&
            same_reference(set->attachment.reference, fighter.reference)) return false;
        if (!set->owner.object && !available) available = set;
    }
    if (!available || !nativeItemLifetime_Current(&source) || !nativeFighterLifetime_Current(&fighter) ||
        nativeScriptItem_AttachmentOwner(owner) != attachment) return false;
    *available = (ResourceCollection){.owner=source,.attachment=fighter,.parts=fighter.data->parts,
        .parts_table=ftPartsTable[fighter.kind],.rig=attachment->hsd_obj,.rig_kind=attachment->obj_kind,
        .part_count=ftPartsTable[fighter.kind]->parts_num,.generation=++nativeArticleResourceGeneration,
        .count=(uint16_t)count,.external_part=(int16_t)external};
    return true;
}

bool nativeArticleResource_CreateNode(Item_GObj* owner, unsigned index, const NativeArticleResourceInit* init)
{
    ResourceCollection* set = nativeArticleResource_Collection(owner);
    if (!set || index != set->created || index >= set->count || !nativeArticleResource_AttachmentCurrent(set) ||
        !init || !set->parts || init->attachment_part >= set->part_count ||
        !set->parts[init->attachment_part].joint || !isfinite(init->radius) || init->radius <= 0 ||
        init->collision_category > 15 || !isfinite(init->position.x) ||
        !isfinite(init->position.y) || !isfinite(init->position.z)) return false;
    if (index) { ResourceCapture prior; if (!nativeArticleResource_Capture(owner,index-1,&prior)) return false; }
    NativeMatchPoolReference payload, object_ref;
    if (!nativeArticleResourceScene_Ready() || nativeHsdReservation_Active()) return false;
    ItemLink* link = nativeMatchPool_Allocate(&nativeItemLinkPool, &payload);
    if (!link) return false;
    HSD_GObj* object = GObj_Create(HSD_GOBJ_CLASS_ITEMLINK, 10, 0);
    if (!object || !nativeMatchPool_Reference(&nativeGObjPool, object, &object_ref)) {
        if (object) HSD_GObjFree(object);
        bool freed = nativeMatchPool_Free(&nativeItemLinkPool, payload);
        HSD_ASSERTREPORT(0, freed, "Unpublished resource payload allocation failed to unwind");
        return false;
    }
    link->gobj=object;link->jobj=set->parts[init->attachment_part].joint;link->x1CC=-1;
    link->pos=init->position;link->coll_data.cur_pos=init->position;
    mpColl_80041EE4(&link->coll_data);
    link->coll_data.x34_flags.b1234=init->collision_category;
    mpColl_SetECBSource_Fixed(&link->coll_data,NULL,init->radius,init->radius,init->radius,init->radius);
    ResourceNode* entry=&set->nodes[index];
    *entry=(ResourceNode){.link=link,.payload=payload,.object=object_ref,.generation=set->generation,
        .attachment_joint=link->jobj,.attachment_part=init->attachment_part,.root_index=UINT32_MAX};
    if(index){link->next=set->nodes[index-1].link;link->next->prev=link;}
    ++set->created;
    GObj_InitUserData(object,6,free_link,link);
    return true;
}

bool nativeArticleResource_SetDraw(Item_GObj* owner,unsigned index,bool enabled)
{
    ResourceCapture saved;if(!nativeArticleResource_Capture(owner,index,&saved))return false;
    saved.link->x2C_b0=enabled;return nativeArticleResource_Current(&saved);
}
unsigned nativeArticleResource_Count(Item_GObj* owner)
{ResourceCollection* set=nativeArticleResource_Collection(owner);return set?set->count:0;}
bool nativeArticleResource_Exists(Item_GObj* owner,unsigned index)
{ResourceCapture saved;return nativeArticleResource_Capture(owner,index,&saved);}

bool nativeArticleResource_BindExternalPart(Item_GObj* owner,unsigned index,unsigned external)
{
    ResourceCapture saved;if(!nativeArticleResource_Capture(owner,index,&saved))return false;
    ResourceCollection* set=saved.collection;
    if(!saved.model || !nativeArticleResource_AttachmentCurrent(set) || !set->parts ||
        set->external_part<0 || external!=(unsigned)set->external_part || external<set->part_count ||
        external>=MAX_FT_PARTS)return false;
    HSD_JObj** destination=&set->parts[external].joint;
    if(*destination && *destination!=saved.model)return false;
    *destination=saved.model;saved.node->bound_part=true;
    return nativeArticleResource_Current(&saved)&&nativeArticleResource_AttachmentCurrent(set);
}
bool nativeArticleResource_SetCaptureAnchor(Item_GObj* owner,unsigned index)
{
    ResourceCapture saved;if(!nativeArticleResource_Capture(owner,index,&saved) || !saved.model ||
        !nativeArticleResource_AttachmentCurrent(saved.collection) ||
        !common_pull_state(saved.collection->attachment.data))return false;
    saved.collection->attachment.data->mv.co.capturedamage.x18=saved.model;saved.node->bound_capture=true;
    return nativeArticleResource_Current(&saved)&&nativeArticleResource_AttachmentCurrent(saved.collection);
}

bool nativeArticleResource_ResolveBoundPart(Fighter_GObj* fighter,unsigned part,HSD_JObj** output)
{
    NativeMatchPoolReference reference;
    if(!output||part>=MAX_FT_PARTS||!nativeMatchPool_Reference(&nativeGObjPool,fighter,&reference)||
       fighter->classifier!=HSD_GOBJ_CLASS_FIGHTER||!fighter->user_data)return false;
    ResourceCapture selected={0};bool found=false;
    for(unsigned i=0;i<NATIVE_ARTICLE_RESOURCE_COLLECTIONS;++i){ResourceCollection* set=&nativeArticleResourceCollections[i];
        if(set->attachment.object!=fighter||set->external_part!=(int)part||
           !nativeArticleResource_AttachmentCurrent(set))continue;
        if(!set->parts||set->count>NATIVE_ARTICLE_RESOURCE_NODES||part<set->part_count)continue;
        for(unsigned n=0;n<set->count;++n){ResourceCapture saved;
            if(!nativeArticleResource_Capture(set->owner.object,n,&saved)||saved.collection!=set||
               !saved.node->bound_part||!saved.model||set->parts[part].joint!=saved.model)continue;
            if(found)return false;selected=saved;found=true;
        }
    }
    if(!found||nativeMatchPool_Resolve(reference)!=fighter||!nativeArticleResource_Current(&selected)||
       !nativeArticleResource_AttachmentCurrent(selected.collection))return false;
    *output=selected.model;return true;
}

static float* float_component(ItemLink* link,NativeArticleResourceFloat channel)
{
    if((unsigned)channel>=NativeArticleResource_FloatCount)return NULL;
    Vec3* v;switch((unsigned)channel/3){
    case 0:v=&link->pos;break;case 1:v=&link->vel;break;case 2:v=&link->coll_data.cur_pos;break;
    case 3:v=&link->coll_data.last_pos;break;case 4:v=&link->coll_data.floor.normal;break;
    case 5:v=&link->coll_data.left_facing_wall.normal;break;case 6:v=&link->coll_data.right_facing_wall.normal;break;
    default:v=&link->coll_data.ceiling.normal;break;}
    return (unsigned)channel%3==0?&v->x:(unsigned)channel%3==1?&v->y:&v->z;
}
float nativeArticleResource_ReadFloat(Item_GObj* owner,unsigned index,NativeArticleResourceFloat channel)
{
    ResourceCapture saved;if(!nativeArticleResource_Capture(owner,index,&saved))return 0;
    float* p=float_component(saved.link,channel);float value=p?*p:0;
    return nativeArticleResource_Current(&saved)?value:0;
}
bool nativeArticleResource_WriteFloat(Item_GObj* owner,unsigned index,NativeArticleResourceFloat channel,float value)
{
    ResourceCapture saved;if(!isfinite(value)||(unsigned)channel>=NativeArticleResource_FloorNormalX ||
        !nativeArticleResource_Capture(owner,index,&saved))return false;
    *float_component(saved.link,channel)=value;return nativeArticleResource_Current(&saved);
}
int32_t nativeArticleResource_ReadInt(Item_GObj* owner,unsigned index,NativeArticleResourceInt channel)
{
    int32_t value=channel==NativeArticleResource_EnvironmentFlags?0:-1;ResourceCapture saved;
    if(!nativeArticleResource_Capture(owner,index,&saved))return value;
    switch(channel){case NativeArticleResource_SurfaceLine:value=saved.link->x1CC;break;
    case NativeArticleResource_EnvironmentFlags:value=saved.link->coll_data.env_flags;break;
    case NativeArticleResource_FloorLine:value=saved.link->coll_data.floor.index;break;
    case NativeArticleResource_LeftWallLine:value=saved.link->coll_data.left_facing_wall.index;break;
    case NativeArticleResource_RightWallLine:value=saved.link->coll_data.right_facing_wall.index;break;
    case NativeArticleResource_CeilingLine:value=saved.link->coll_data.ceiling.index;break;default:break;}
    return nativeArticleResource_Current(&saved)?value:channel==NativeArticleResource_EnvironmentFlags?0:-1;
}
bool nativeArticleResource_WriteInt(Item_GObj* owner,unsigned index,NativeArticleResourceInt channel,int32_t value)
{
    ResourceCapture saved;if(channel!=NativeArticleResource_SurfaceLine || !nativeArticleResource_Capture(owner,index,&saved))return false;
    saved.link->x1CC=value;return nativeArticleResource_Current(&saved);
}

bool nativeArticleResource_SweepStage(Item_GObj* owner,unsigned index)
{
    ResourceCapture saved;if(!nativeArticleResource_BeginOperation(owner,index,&saved))return false;
    bool result=mpColl_800471F8(&saved.link->coll_data);
    bool current=nativeArticleResource_EndOperation(&saved);return current&&result;
}
bool nativeArticleResource_SweepPoint(Item_GObj* owner,unsigned index)
{
    ResourceCapture saved;if(!nativeArticleResource_BeginOperation(owner,index,&saved))return false;
    bool result=mpColl_80048844(&saved.link->coll_data);
    bool current=nativeArticleResource_EndOperation(&saved);return current&&result;
}

// Existing audited independent generator payloads. They do not retain the stack
// Vec3/scalar or interpret the endpoint userdata as Item/Fighter storage.
static bool resource_effect_payload(int32_t effect,bool facing)
{
    if(facing)return effect>=0x3EF&&effect<=0x3F2;
    if((uint32_t)effect<0x250||effect/1000==0x1E)return true;
    switch(effect){case 0x3E9:case 0x3EA:case 0x3F3:case 0x3F4:case 0x405:case 0x407:case 0x40C:case 0x40E:case 0x40F:
    case 0x410:case 0x411:case 0x416:case 0x41C:case 0x41F:case 0x420:case 0x421:case 0x425:case 0x426:case 0x42D:
    case 0x434:case 0x43A:case 0x43B:case 0x43C:case 0x441:case 0x442:case 0x444:case 0x445:
    case 0x474:case 0x475:case 0x476:case 0x477:return true;default:return false;}
}
bool nativeArticleResource_SpawnEffectAtPosition(Item_GObj* owner,unsigned index,int32_t effect,const Vec3* position)
{
    if(!resource_effect_payload(effect,false)||!position||!isfinite(position->x)||!isfinite(position->y)||!isfinite(position->z))return false;
    ResourceCapture saved;if(!nativeArticleResource_BeginOperation(owner,index,&saved))return false;
    Vec3 copy=*position;efSync_Spawn(effect,saved.gobj,&copy);return nativeArticleResource_EndOperation(&saved);
}
bool nativeArticleResource_SpawnEffectFacingAtPosition(Item_GObj* owner,unsigned index,int32_t effect,const Vec3* position,float facing)
{
    if(!resource_effect_payload(effect,true)||!position||!isfinite(position->x)||!isfinite(position->y)||!isfinite(position->z)||!isfinite(facing))return false;
    ResourceCapture saved;if(!nativeArticleResource_BeginOperation(owner,index,&saved))return false;
    // The audited generators consume Vec3* followed by f32*, not a promoted
    // scalar value. Keep both synchronous payloads alive through dispatch.
    Vec3 copy=*position;efSync_Spawn(effect,saved.gobj,&copy,&facing);return nativeArticleResource_EndOperation(&saved);
}
bool nativeArticleResource_FollowFloor(Item_GObj* owner,unsigned index)
{
    ResourceCapture saved;if(!nativeArticleResource_BeginOperation(owner,index,&saved))return false;
    bool result=mpColl_8004B108(&saved.link->coll_data);
    bool current=nativeArticleResource_EndOperation(&saved);return current&&result;
}
bool nativeArticleResource_ReadLineSpeed(Item_GObj* owner,unsigned index,int line,Vec3* output)
{
    ResourceCapture saved;if(!output||!nativeArticleResource_BeginOperation(owner,index,&saved))return false;
    // -1 is the native no-line sentinel; every other index must be bounded
    // before mpLib_80054ED8, which deliberately hangs on invalid line IDs.
    bool admitted=line==-1;
    if(!admitted){
        MapCollData* map=mpLib_8004D164();CollLine* lines=mpGetGroundCollLine();
        if(map&&lines&&line>=0&&line<map->line_count){
            admitted=true;
            if((lines[line].flags&LINE_FLAG_ENABLED)&&!(lines[line].flags&LINE_FLAG_HIDDEN)){
                MapLine* description=lines[line].x0;CollVtx* vertices=mpGetGroundCollVtx();
                admitted=description&&vertices&&description->v0_idx<map->vert_count&&
                    description->v1_idx<map->vert_count;
            }
        }
    }
    Vec3 value={0};bool result=admitted&&mpGetSpeed(line,&saved.link->pos,&value);
    bool current=nativeArticleResource_EndOperation(&saved);if(!current||!result)return false;
    *output=value;return true;
}

static bool attachment_joint(const ResourceCapture* saved)
{
    ResourceCollection* set=saved->collection;ResourceNode* node=saved->node;
    return nativeArticleResource_Current(saved)&&nativeArticleResource_AttachmentCurrent(set)&&
        set->parts&&node->attachment_part<set->part_count&&node->attachment_joint&&
        node->link->jobj==node->attachment_joint&&set->parts[node->attachment_part].joint==node->attachment_joint;
}
bool nativeArticleResource_ReadAttachmentMatrix(Item_GObj* owner,unsigned index,Mtx output)
{
    ResourceCapture saved;if(!output||!nativeArticleResource_BeginOperation(owner,index,&saved))return false;
    bool valid=attachment_joint(&saved);Mtx value;
    if(valid){HSD_JObjSetupMatrix(saved.node->attachment_joint);valid=attachment_joint(&saved);
        if(valid)C_MTXCopy(saved.node->attachment_joint->mtx,value);}
    bool current=nativeArticleResource_EndOperation(&saved);if(!valid||!current)return false;
    C_MTXCopy(value,output);return true;
}
bool nativeArticleResource_ReadAttachmentPoint(Item_GObj* owner,unsigned index,const Vec3* local,Vec3* world)
{
    if(!local||!world||!isfinite(local->x)||!isfinite(local->y)||!isfinite(local->z))return false;
    Vec3 input=*local,value;ResourceCapture saved;if(!nativeArticleResource_BeginOperation(owner,index,&saved))return false;
    bool valid=attachment_joint(&saved);
    if(valid){HSD_JObjSetupMatrix(saved.node->attachment_joint);valid=attachment_joint(&saved);
        if(valid)C_MTXMultVec(saved.node->attachment_joint->mtx,&input,&value);}
    bool current=nativeArticleResource_EndOperation(&saved);if(!valid||!current)return false;
    *world=value;return true;
}
#endif
