#include "native_item_owner.h"
#include "native_article.h"
#include "item.h"
#include "types.h"
#include "it_2725.h"
#include "it_26B1.h"
#include "kinds/inlines.h"
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/types.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftcommon.h>
#include <melee/db/db.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <math.h>
#ifdef MELEE_NATIVE
#include <melee/ft/native_fighter_pool.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#endif

void nativeItem_CaptureCreator(Item_GObj* object, Fighter_GObj* source)
{
    Item* item = object->user_data;
    item->native_creator = NULL;
    item->native_creator_generation = 0;
    item->native_creator_kind = Ft_Kind_None;
    item->native_creator_definition = NULL;
#ifdef MELEE_NATIVE
    item->native_creator_reference = (NativeMatchPoolReference){0};
    NativeMatchPoolReference creator_reference;
    if (!nativeMatchPool_Reference(&nativeGObjPool, source, &creator_reference))
        return;
#endif
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(source, NativeCallbackSource_Fighter, &generation))
        return;
    const Fighter* fighter = source->user_data;
    item->native_creator = source;
#ifdef MELEE_NATIVE
    item->native_creator_reference = creator_reference;
#endif
    item->native_creator_generation = generation;
    item->native_creator_kind = fighter->kind;
    item->native_creator_definition = nativeFighterRuntime_Definition(fighter->kind);
}

Fighter_GObj* nativeItem_Creator(Item_GObj* object)
{
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(object, NativeCallbackSource_Item, &generation))
        return NULL;
    const Item* item = object->user_data;
    Fighter_GObj* source = item->native_creator;
#ifdef MELEE_NATIVE
    if (nativeMatchPool_Resolve(item->native_creator_reference) != source ||
        source == NULL) return NULL;
#endif
    if (!nativeCallbackSource_Alive(source, item->native_creator_generation,
                                   NativeCallbackSource_Fighter)) return NULL;
    const Fighter* fighter = source->user_data;
    if (fighter->kind != item->native_creator_kind ||
        nativeFighterRuntime_Definition(fighter->kind) != item->native_creator_definition)
        return NULL;
    return source;
}

#ifdef MELEE_NATIVE
typedef struct ConstructingChildReference {
    uint32_t slot, pool_generation, item_generation;
} ConstructingChildReference;

// Nested synchronous teardown owns bounded scratch, outside rollback state.
#pragma clang section data = ""
static ConstructingChildReference constructing_children
    [NATIVE_CALLBACK_DISPATCH_LIMIT][NATIVE_GOBJ_CAPACITY];
static uint32_t constructing_depth;
#if !defined(MELEE_ROLLBACK_HOST_STATE)
#pragma clang section data = ".mstate$m"
#endif

static bool constructing_owner_current(Fighter_GObj* object,
    NativeMatchPoolReference object_reference,
    NativeMatchPoolReference fighter_reference)
{
    if (!object || object_reference.pool != &nativeGObjPool ||
        fighter_reference.pool != &nativeFighterPool ||
        object_reference.lifetime != fighter_reference.lifetime ||
        nativeMatchPool_Resolve(object_reference) != object) return false;
    Fighter* fighter = nativeMatchPool_Resolve(fighter_reference);
    return fighter && object->user_data == fighter && fighter->gobj == object &&
        fighter->native_constructing;
}

static bool constructing_child(const Item* item, Fighter_GObj* owner,
    NativeMatchPoolReference owner_reference)
{
    const NativeMatchPoolReference creator = item->native_creator_reference;
    return !item->native_removal_in_progress && item->native_creator == owner &&
        item->owner == owner && creator.pool == owner_reference.pool &&
        creator.lifetime == owner_reference.lifetime &&
        creator.slot == owner_reference.slot &&
        creator.generation == owner_reference.generation;
}

void nativeItem_RemoveConstructingOwnerChildren(Fighter_GObj* owner,
    NativeMatchPoolReference owner_reference,
    NativeMatchPoolReference fighter_reference)
{
    if (!constructing_owner_current(owner, owner_reference, fighter_reference) ||
        !HSD_GObjPLinkHead || constructing_depth == NATIVE_CALLBACK_DISPATCH_LIMIT)
        return;
    ConstructingChildReference* references =
        constructing_children[constructing_depth++];
    uint32_t count = 0, visited = 0;
    for (Item_GObj* object = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];
         object && visited++ < NATIVE_GOBJ_CAPACITY; object = object->next) {
        NativeMatchPoolReference reference;
        uint32_t generation;
        if (!nativeMatchPool_Reference(&nativeGObjPool, object, &reference)) break;
        if (reference.lifetime != owner_reference.lifetime ||
            !nativeCallbackSource_Snapshot(object, NativeCallbackSource_Item,
                                           &generation)) continue;
        if (constructing_child(object->user_data, owner, owner_reference))
            references[count++] = (ConstructingChildReference){
                reference.slot, reference.generation, generation};
    }
    for (uint32_t i = 0; i < count; ++i) {
        if (!constructing_owner_current(owner, owner_reference, fighter_reference))
            break;
        NativeMatchPoolReference reference = {&nativeGObjPool,
            owner_reference.lifetime, references[i].slot,
            references[i].pool_generation};
        Item_GObj* object = nativeMatchPool_Resolve(reference);
        if (!object || !nativeCallbackSource_Alive(object,
            references[i].item_generation, NativeCallbackSource_Item)) continue;
        if (constructing_child(object->user_data, owner, owner_reference))
            Item_8026A8EC(object);
    }
    --constructing_depth;
}
#endif

static Item_GObj* find_live_item(uint32_t handle)
{
    if (handle == UINT32_MAX || HSD_GObjPLinkHead == NULL) return NULL;
    for (Item_GObj* object = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];
         object != NULL; object = object->next) {
        const Item* item = object->user_data;
        if (item != NULL && item->x1C == handle) return object;
    }
    return NULL;
}

static Item_GObj* find_created_item(Fighter_GObj* source, uint32_t handle)
{
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(source, NativeCallbackSource_Fighter, &generation))
        return NULL;
    Item_GObj* object = find_live_item(handle);
    if (object == NULL) return NULL;
    const Item* item = object->user_data;
    if (item->native_article_author_kind != Ft_Kind_None) {
        const Fighter* fighter = source->user_data;
        if (item->native_article_author_kind != fighter->kind ||
            nativeArticle_Creator(object) != source) return NULL;
    } else if (nativeItem_Creator(object) != source) return NULL;
    return object;
}

bool nativeItem_BindExisting(Fighter_GObj* source, uint32_t handle, bool auxiliary)
{
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(source, NativeCallbackSource_Fighter, &generation))
        return false;
    Item_GObj* object = find_live_item(handle);
    if (object == NULL) return false;
    Fighter* fighter = source->user_data;
    if (auxiliary) fighter->x1984_heldItemSpec = object;
    else fighter->item_gobj = object;
    return true;
}

bool nativeItem_SetVisible(uint32_t handle, bool visible)
{
    Item_GObj* object = find_live_item(handle);
    if (object == NULL || object->hsd_obj == NULL) return false;
    if (visible) it_8026BB20(object);
    else it_8026BB44(object);
    return true;
}

bool nativeItem_SetPaused(uint32_t handle, bool paused)
{
    Item_GObj* object = find_live_item(handle);
    if (object == NULL) return false;
    if (paused) it_8026B724(object);
    else it_8026B73C(object);
    return true;
}

bool nativeItem_NotifyDrop(Fighter_GObj* source, uint32_t handle, bool normal)
{
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(source, NativeCallbackSource_Fighter, &generation))
        return false;
    Item_GObj* object = find_live_item(handle);
    if (object == NULL) return false;
    const FighterKind kind = ((Fighter*)source->user_data)->kind;
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(kind);
    ftCommon_8007E6DC(source, object, normal);
    if (!nativeCallbackSource_Alive(source, generation, NativeCallbackSource_Fighter) ||
        ((Fighter*)source->user_data)->kind != kind ||
        nativeFighterRuntime_Definition(((Fighter*)source->user_data)->kind) != definition)
        return false;
    return nativeCallbackSource_Alive(object, handle, NativeCallbackSource_Item);
}

bool nativeItem_BindFighter(Fighter_GObj* source, uint32_t handle, bool auxiliary)
{
    Item_GObj* object = find_created_item(source, handle);
    if (object == NULL) return false;
    Fighter* fighter = source->user_data;
    if (auxiliary) fighter->x1984_heldItemSpec = object;
    else fighter->item_gobj = object;
    return true;
}

bool nativeItem_ClearBinding(Fighter_GObj* source, bool auxiliary)
{
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(source, NativeCallbackSource_Fighter, &generation))
        return false;
    Fighter* fighter = source->user_data;
    if (auxiliary) fighter->x1984_heldItemSpec = NULL;
    else fighter->item_gobj = NULL;
    return true;
}

int32_t nativeItem_HeldId(Fighter_GObj* source, bool auxiliary)
{
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(source, NativeCallbackSource_Fighter, &generation))
        return -1;
    const Fighter* fighter = source->user_data;
    Item_GObj* object = auxiliary ? fighter->x1984_heldItemSpec : fighter->item_gobj;
    if (!nativeCallbackSource_Snapshot(object, NativeCallbackSource_Item, &generation))
        return -1;
    return (int32_t)generation;
}

int32_t nativeItem_CreateEngine(Fighter_GObj* source, ItemKind kind, Vec3* position,
                                Vec3* velocity, float facing, uint32_t group, float damage)
{
    uint32_t source_generation;
    if (position == NULL || velocity == NULL ||
        !isfinite(position->x) || !isfinite(position->y) || !isfinite(position->z) ||
        !isfinite(velocity->x) || !isfinite(velocity->y) || !isfinite(velocity->z) ||
        !isfinite(facing) || !isfinite(damage) || damage < INT16_MIN || damage > INT16_MAX ||
        !nativeCallbackSource_Snapshot(source, NativeCallbackSource_Fighter, &source_generation))
        return -1;
    const NativeFighterDefinition* definition =
        nativeFighterRuntime_Definition(((Fighter*)source->user_data)->kind);
    SpawnItem spawn = {0};
    spawn.kind = kind;
    Item_InitSpawn(&spawn, source, position, facing);
    spawn.vel = *velocity;
    spawn.x40 = group;
    spawn.x3C_damage = damage;
    Item_GObj* object = Item_CreateEngine(&spawn);
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(object, NativeCallbackSource_Item, &generation))
        return -1;
    if (!nativeCallbackSource_Alive(source, source_generation, NativeCallbackSource_Fighter) ||
        nativeFighterRuntime_Definition(((Fighter*)source->user_data)->kind) != definition ||
        nativeItem_Creator(object) != source) {
        Item_8026A8EC(object);
        return -1;
    }
    return (int32_t)generation;
}

bool nativeItem_Attach(Fighter_GObj* source, uint32_t handle, int32_t part,
                       bool notify_holder)
{
    Item_GObj* object = find_created_item(source, handle);
    if (object == NULL) return false;
    Fighter* fighter = source->user_data;
    if (ftPartsTable == NULL || (unsigned)fighter->kind >= Ft_Kind_RuntimeCapacity ||
        ftPartsTable[fighter->kind] == NULL || part < 0 || part >= MAX_FT_PARTS ||
        (unsigned)part >= ftPartsTable[fighter->kind]->parts_num ||
        fighter->parts[part].joint == NULL)
        return false;
    const uint32_t source_generation = (uint32_t)fighter->x8_spawnNum;
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(fighter->kind);
    Item_8026AB54(object, source, (Fighter_Part)part);
    if (!nativeCallbackSource_Alive(source, source_generation, NativeCallbackSource_Fighter) ||
        nativeFighterRuntime_Definition(((Fighter*)source->user_data)->kind) != definition ||
        !nativeCallbackSource_Alive(object, handle, NativeCallbackSource_Item)) return false;
    if (find_created_item(source, handle) != object) return false;
    if (notify_holder) {
        db_80225DD8(object, source);
        if (!nativeCallbackSource_Alive(source, source_generation, NativeCallbackSource_Fighter) ||
            nativeFighterRuntime_Definition(((Fighter*)source->user_data)->kind) != definition ||
            !nativeCallbackSource_Alive(object, handle, NativeCallbackSource_Item)) return false;
    }
    return find_created_item(source, handle) == object;
}

bool nativeItem_InheritOwnerScale(Fighter_GObj* source, uint32_t handle, bool remember)
{
    Item_GObj* object = find_created_item(source, handle);
    if (object == NULL || object->hsd_obj == NULL) return false;
    Item* item = object->user_data;
    uint32_t owner_generation;
    if (!nativeCallbackSource_Snapshot(item->owner, NativeCallbackSource_Fighter, &owner_generation))
        return false;
    it_80274594(object);
    if (remember) item->throw_scale = item->scl;
    return true;
}
