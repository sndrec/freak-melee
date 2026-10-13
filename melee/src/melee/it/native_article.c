#include "native_article.h"
#include "types.h"
#include "item.h"
#include "inlines.h"
#include "kinds/inlines.h"
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/types.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/it/it_26B1.h>
#include <melee/lb/lbcollision.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <math.h>
#include <string.h>

Fighter_GObj* nativeArticle_Creator(Item_GObj* object)
{
    Item* item = object != NULL ? object->user_data : NULL;
    if (item == NULL || item->native_article_author_kind == Ft_Kind_None ||
        item->xDD4_itemVar.native_article.creator_spawn_id == 0) return NULL;
    Fighter_GObj* creator = ftLib_8008741C(
        item->xDD4_itemVar.native_article.creator_spawn_id);
    if (creator == NULL || creator != item->xDD4_itemVar.native_article.attachment_owner ||
        creator->user_data == NULL) return NULL;
    Fighter* fighter = creator->user_data;
    const NativeFighterDefinition* definition =
        nativeFighterRuntime_Definition(item->native_article_author_kind);
    if (definition == NULL ||
        nativeFighterRuntime_Definition(fighter->kind) != definition) return NULL;
    return creator;
}

bool nativeArticle_OverlapsOwner(Item_GObj* object, unsigned index)
{
    Item* item = object->user_data;
    if (index >= 4) return false;
    Fighter_GObj* owner = nativeArticle_Creator(object);
    if (owner == NULL || owner != item->owner || owner->user_data == NULL) return false;
    Fighter* fighter = owner->user_data;
    for (unsigned i = 0; i < fighter->hurt_capsules_len; ++i)
        if (lbColl_80008248(&item->x5D4_hitboxes[index].hit,
                &fighter->hurt_capsules[i].capsule, ftCommon_8007F804(fighter),
                item->scl, fighter->x34_scale.y, fighter->cur_pos.z)) return true;
    return false;
}

// Links have no allocation and no ownership restriction. Reflection changes
// damage ownership, not the relationship or package that authored the item.
static Item_GObj* linked_item(Item_GObj* object, unsigned slot)
{
    if (object == NULL || slot >= 4 || HSD_GObjPLinkHead == NULL) return NULL;
    Item* item = object->user_data;
    if (item == NULL || item->native_article_author_kind == Ft_Kind_None) return NULL;
    uint32_t number = item->native_article_links[slot];
    if (number == UINT32_MAX) return NULL;
    for (Item_GObj* target = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];
         target != NULL; target = target->next) {
        Item* data = target->user_data;
        if (data != NULL && data->x1C == number &&
            data->native_article_author_kind == item->native_article_author_kind)
            return target;
    }
    return NULL;
}

static const NativeFighterArticleLogic* article_logic(FighterKind author, ItemKind kind)
{
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(author);
    if (definition == NULL) return NULL;
    for (unsigned i = 0; i < definition->article_logic_count; ++i)
        if (definition->article_logics[i].kind == kind) return &definition->article_logics[i];
    return NULL;
}

int32_t nativeArticle_Create(Fighter_GObj* owner, ItemKind kind, Vec3* position,
                             Vec3* velocity, float facing, bool origin_at_owner,
                             uint32_t collision_group, float creation_value,
                             bool initial_collision)
{
    if (owner == NULL || position == NULL || velocity == NULL ||
        !isfinite(velocity->x) || !isfinite(velocity->y) || !isfinite(velocity->z) ||
        !isfinite(position->x) ||
        !isfinite(position->y) || !isfinite(position->z) || !isfinite(facing) ||
        !isfinite(creation_value)) return -1;
    Fighter* fighter = owner->user_data;
    if (fighter == NULL) return -1;
    const NativeFighterArticleLogic* logic = article_logic(fighter->kind, kind);
    if (logic == NULL || nativeFighterRuntime_ArticleAssets(fighter->kind, kind) == NULL)
        return -1;
    SpawnItem spawn = { 0 };
    spawn.kind = kind;
    Item_InitSpawn(&spawn, owner, position, facing);
    spawn.vel = *velocity;
    spawn.x40 = collision_group;
    spawn.x44_flag.b0 = initial_collision;
    if (origin_at_owner) it_8026BB68(owner, &spawn.pos);
    Item_GObj* object = Item_CreateAuthored(&spawn, fighter->kind, 0);
    if (object == NULL) return -1;
    Item* item = object->user_data;
    item->xDD4_itemVar.native_article.attachment_owner = owner;
    item->xDD4_itemVar.native_article.creator_spawn_id = nativeFighterTarget_Id(owner);
    item->xDD4_itemVar.native_article.attachment_facing = facing;
    item->xDD4_itemVar.native_article.launch_origin = spawn.pos;
    item->xDD4_itemVar.native_article.creation_value = creation_value;
    int32_t number = item->x1C;
    if (logic->on_created != NULL) logic->on_created(object);
    // Created callbacks may remove the object. No subsequent dereference.
    return number;
}

// Spawn an unlinked article from another authored item. Damage ownership may
// have changed through reflection; the source's package still owns the assets
// and callbacks, while the item parent remains the actual creating item.
int32_t nativeArticle_CreateAtPosition(Item_GObj* parent, ItemKind kind,
                                      Vec3* position, float facing)
{
    if (parent == NULL || parent->user_data == NULL || position == NULL ||
        !isfinite(position->x) || !isfinite(position->y) ||
        !isfinite(position->z) || !isfinite(facing)) return -1;
    Item* source = parent->user_data;
    FighterKind author = source->native_article_author_kind;
    const NativeFighterArticleLogic* logic = article_logic(author, kind);
    if (logic == NULL || nativeFighterRuntime_ArticleAssets(author, kind) == NULL)
        return -1;
    SpawnItem spawn = { 0 };
    spawn.kind = kind;
    spawn.pos = spawn.prev_pos = *position;
    spawn.facing_dir = facing;
    spawn.x0_parent_gobj = source->owner;
    spawn.x4_parent_gobj2 = parent;
    spawn.x44_flag.b0 = 1;
    Fighter_GObj* owner = source->owner;
    Item_GObj* object = Item_CreateAuthored(&spawn, author, 0);
    if (object == NULL) return -1;
    Item* item = object->user_data;
    item->xDD4_itemVar.native_article.attachment_owner = owner;
    item->xDD4_itemVar.native_article.creator_spawn_id = nativeFighterTarget_Id(owner);
    item->xDD4_itemVar.native_article.attachment_facing = facing;
    item->xDD4_itemVar.native_article.launch_origin = spawn.pos;
    int32_t number = item->x1C;
    if (logic->on_created != NULL) logic->on_created(object);
    return number;
}

// A child retains its creating package and original fighter creator separately
// from damage ownership. All authored initialization precedes on_created.
int32_t nativeArticle_CreateChild(Item_GObj* parent, ItemKind kind,
                                  Vec3* position, float facing,
                                  uint32_t damage_owner,
                                  uint32_t collision_group,
                                  float creation_value, unsigned backlink)
{
    if (parent == NULL || HSD_GObjPLinkHead == NULL || position == NULL ||
        backlink >= 4 || !isfinite(position->x) || !isfinite(position->y) ||
        !isfinite(position->z) || !isfinite(facing) || !isfinite(creation_value))
        return -1;
    bool live_parent = false;
    for (Item_GObj* live = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];
         live != NULL; live = live->next) {
        if (live == parent) { live_parent = true; break; }
    }
    if (!live_parent || parent->user_data == NULL) return -1;
    Item* source = parent->user_data;
    FighterKind author = source->native_article_author_kind;
    const NativeFighterArticleLogic* logic = article_logic(author, kind);
    if (logic == NULL || nativeFighterRuntime_ArticleAssets(author, kind) == NULL)
        return -1;
    Fighter_GObj* owner = damage_owner != 0 ? ftLib_8008741C(damage_owner) : NULL;
    if (damage_owner != 0 && owner == NULL) return -1;
    Fighter_GObj* creator = nativeArticle_Creator(parent);
    uint32_t creator_id = nativeFighterTarget_Id(creator);
    uint32_t parent_id = source->x1C;
    SpawnItem spawn = { 0 };
    spawn.kind = kind;
    spawn.pos = spawn.prev_pos = *position;
    spawn.facing_dir = facing;
    spawn.x0_parent_gobj = owner;
    spawn.x4_parent_gobj2 = parent;
    spawn.x40 = collision_group;
    spawn.x44_flag.b0 = 1;
    Item_GObj* object = Item_CreateAuthored(&spawn, author, 0);
    if (object == NULL) return -1;
    Item* item = object->user_data;
    item->native_article_links[backlink] = parent_id;
    item->xDD4_itemVar.native_article.attachment_owner = creator;
    item->xDD4_itemVar.native_article.creator_spawn_id = creator_id;
    item->xDD4_itemVar.native_article.attachment_facing = facing;
    item->xDD4_itemVar.native_article.launch_origin = spawn.pos;
    item->xDD4_itemVar.native_article.creation_value = creation_value;
    int32_t number;
    memcpy(&number, &item->x1C, sizeof(number));
    if (logic->on_created != NULL) logic->on_created(object);
    // The callback may remove child or parent; do not touch either afterward.
    return number;
}

int32_t nativeArticle_CreateHeldAtCenter(Fighter_GObj* owner, ItemKind kind,
                                        Vec3* previous, unsigned bone, float facing)
{
    if (owner == NULL || owner->user_data == NULL || previous == NULL ||
        !isfinite(previous->x) || !isfinite(previous->y) ||
        !isfinite(previous->z) || !isfinite(facing)) return -1;
    Fighter* fighter = owner->user_data;
    FighterPartsTable* parts = ftPartsTable[fighter->kind];
    const NativeFighterArticleLogic* logic = article_logic(fighter->kind, kind);
    if (parts == NULL || bone >= parts->parts_num ||
        fighter->parts[bone].joint == NULL || logic == NULL ||
        nativeFighterRuntime_ArticleAssets(fighter->kind, kind) == NULL) return -1;
    uint32_t owner_number = (uint32_t) fighter->x8_spawnNum;
    SpawnItem spawn = { 0 };
    spawn.kind = kind;
    spawn.prev_pos = *previous;
    it_8026BB68(owner, &spawn.pos);
    spawn.facing_dir = facing;
    spawn.x0_parent_gobj = spawn.x4_parent_gobj2 = owner;
    spawn.x44_flag.b0 = 1;
    Item_GObj* object = Item_CreateAuthored(&spawn, fighter->kind, 0);
    if (object == NULL) return -1;
    if (!nativeCallbackSource_Alive(owner, owner_number, NativeCallbackSource_Fighter))
        return -1;
    Item* item = object->user_data;
    item->xDD4_itemVar.native_article.attachment_owner = owner;
    item->xDD4_itemVar.native_article.creator_spawn_id = nativeFighterTarget_Id(owner);
    item->xDD4_itemVar.native_article.attachment_facing = facing;
    item->xDD4_itemVar.native_article.launch_origin = spawn.pos;
    // Initialize authored state before pickup changes the animation.
    uint32_t number = item->x1C;
    if (logic->on_created != NULL) logic->on_created(object);
    if (!nativeCallbackSource_Alive(object, number, NativeCallbackSource_Item) ||
        !nativeCallbackSource_Alive(owner, owner_number, NativeCallbackSource_Fighter))
        return -1;
    Item_8026AB54(object, owner, bone);
    if (!nativeCallbackSource_Alive(object, number, NativeCallbackSource_Item) ||
        !nativeCallbackSource_Alive(owner, owner_number, NativeCallbackSource_Fighter))
        return -1;
    fighter->item_gobj = object;
    int32_t handle;
    memcpy(&handle, &number, sizeof(handle));
    return handle;
}

int32_t nativeArticle_CreateUnowned(Fighter_GObj* author, ItemKind kind,
                                   Vec3* position, Vec3* velocity, float facing)
{
    if (author == NULL || author->user_data == NULL || position == NULL ||
        velocity == NULL || !isfinite(position->x) || !isfinite(position->y) ||
        !isfinite(position->z) || !isfinite(velocity->x) ||
        !isfinite(velocity->y) || !isfinite(velocity->z) || !isfinite(facing)) return -1;
    Fighter* fighter = author->user_data;
    const NativeFighterArticleLogic* logic = article_logic(fighter->kind, kind);
    if (logic == NULL || nativeFighterRuntime_ArticleAssets(fighter->kind, kind) == NULL)
        return -1;
    SpawnItem spawn = { 0 };
    spawn.kind = kind;
    spawn.pos = spawn.prev_pos = *position;
    spawn.vel = *velocity;
    spawn.facing_dir = facing;
    spawn.x44_flag.b0 = 1;
    // Author identity selects assets/callbacks. It is independent of both
    // parent and damage ownership, which deliberately remain NULL here.
    Item_GObj* object = Item_CreateAuthored(&spawn, fighter->kind, 0);
    if (object == NULL) return -1;
    Item* item = object->user_data;
    item->xDD4_itemVar.native_article.attachment_facing = facing;
    item->xDD4_itemVar.native_article.launch_origin = spawn.pos;
    int32_t number = item->x1C;
    if (logic->on_created != NULL) logic->on_created(object);
    return number;
}

void nativeArticle_SetAnimationDescriptor(Item_GObj* object, unsigned index)
{
    if (object == NULL || object->user_data == NULL) return;
    Item* item = object->user_data;
    Article* article = item->xC4_article_data;
    if (article == NULL || article->xC_itemStates == NULL ||
        index >= article->xC_itemStates->count) return;
    item->xD0_itemStateDesc = &article->xC_itemStates->x0_itemStateDesc[index];
    Item_80268D34(object, item->xD0_itemStateDesc);
}

void nativeArticle_AddAnimationDescriptor(Item_GObj* object, unsigned index)
{
    Item* item = object != NULL ? object->user_data : NULL;
    Article* article = item != NULL ? item->xC4_article_data : NULL;
    if (article == NULL || article->xC_itemStates == NULL ||
        index >= article->xC_itemStates->count || object->hsd_obj == NULL) return;
    ItemStateDesc* descriptor = &article->xC_itemStates->x0_itemStateDesc[index];
    item->xD0_itemStateDesc = descriptor;
    HSD_JObjAddAnimAll(object->hsd_obj, descriptor->x0_anim_joint,
                      descriptor->x4_matanim_joint, descriptor->x8_parameters);
}

bool nativeArticle_ReadAttachmentMatrixRotation(Item_GObj* object, Vec3* output)
{
    if (output == NULL) return false;
    *output = (Vec3){ 0 };
    Fighter_GObj* creator = nativeArticle_Creator(object);
    if (creator == NULL) return false;
    Fighter* fighter = creator->user_data;
    Item* item = object->user_data;
    if ((unsigned)fighter->kind >= Ft_Kind_RuntimeCapacity || ftPartsTable == NULL ||
        ftPartsTable[fighter->kind] == NULL ||
        (unsigned)item->xDC4 >= ftPartsTable[fighter->kind]->parts_num) return false;
    HSD_JObj* joint = ftLib_80086630(creator, item->xDC4);
    if (joint == NULL) return false;
    // Read the existing world matrix, matching the caller's animation phase.
    HSD_MtxGetRotation(joint->mtx, output);
    return true;
}

bool nativeArticle_Link(Item_GObj* object, unsigned slot, Item_GObj* target)
{
    if (object == NULL || target == NULL || object == target || slot >= 4)
        return false;
    // Both addresses must still be live. Public handles resolve creation IDs
    // before this call; raw item pointers must not introduce removed objects.
    bool source_live = false, target_live = false;
    if (HSD_GObjPLinkHead == NULL) return false;
    for (Item_GObj* live = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM]; live; live = live->next) {
        source_live |= live == object;
        target_live |= live == target;
    }
    if (!source_live || !target_live) return false;
    Item* source = object->user_data;
    Item* destination = target->user_data;
    if (source == NULL || destination == NULL ||
        source->native_article_author_kind == Ft_Kind_None ||
        source->native_article_author_kind != destination->native_article_author_kind)
        return false;
    Item_GObj* current = linked_item(object, slot);
    if (current != NULL && current != target) return false;
    source->native_article_links[slot] = destination->x1C;
    return true;
}

bool nativeArticle_SpawnLink(Item_GObj* parent, unsigned slot, unsigned backlink,
                             ItemKind kind, Vec3* position, float facing)
{
    if (parent == NULL || slot >= 4 || backlink >= 4 || position == NULL ||
        !isfinite(position->x) || !isfinite(position->y) ||
        !isfinite(position->z) || !isfinite(facing) || linked_item(parent, slot) != NULL)
        return false;
    Item* source = parent->user_data;
    if (source == NULL) return false;
    const uint32_t parent_number = source->x1C;
    Fighter_GObj* owner = source->owner;
    const uint32_t owner_number = nativeFighterTarget_Id(owner);
    const NativeFighterArticleLogic* logic = article_logic(source->native_article_author_kind, kind);
    if (logic == NULL || nativeFighterRuntime_ArticleAssets(source->native_article_author_kind, kind) == NULL)
        return false;
    SpawnItem spawn = { 0 };
    spawn.kind = kind;
    Item_InitSpawnOnPlane(&spawn, source->owner, position, facing);
    Item_GObj* child = Item_CreateAuthored(&spawn, source->native_article_author_kind, 0);
    if (child == NULL) return false;
    Item* target = child->user_data;
    if (nativeCallbackSource_Alive(parent, parent_number, NativeCallbackSource_Item))
        source->native_article_links[slot] = target->x1C;
    target->native_article_links[backlink] = parent_number;
    target->xDD4_itemVar.native_article.attachment_owner = owner;
    target->xDD4_itemVar.native_article.creator_spawn_id = owner_number;
    target->xDD4_itemVar.native_article.attachment_facing = facing;
    target->xDD4_itemVar.native_article.launch_origin = spawn.pos;
    if (logic->on_created != NULL) logic->on_created(child);
    // A created callback can signal and remove its parent. Check the live list
    // before reading the parent again, including object-address reuse.
    for (Item_GObj* live = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];
         live != NULL; live = live->next) {
        if (live != parent) continue;
        Item* data = live->user_data;
        return data != NULL && data->x1C == parent_number &&
               linked_item(live, slot) != NULL;
    }
    return false;
}

void nativeArticle_Unlink(Item_GObj* object, unsigned slot)
{
    if (object == NULL || slot >= 4) return;
    Item* item = object->user_data;
    if (item == NULL) return;
    Item_GObj* target = linked_item(object, slot);
    if (target != NULL) {
        Item* other = target->user_data;
        for (unsigned i = 0; i < 4; ++i)
            if (other->native_article_links[i] == item->x1C)
                other->native_article_links[i] = UINT32_MAX;
    }
    item->native_article_links[slot] = UINT32_MAX;
}

bool nativeArticle_LinkReciprocal(Item_GObj* object, unsigned slot, unsigned backlink)
{
    Item_GObj* other = linked_item(object, slot);
    return other != NULL && linked_item(other, backlink) == object;
}

float nativeArticle_LinkValue(Item_GObj* object, unsigned slot, unsigned property)
{
    Item_GObj* other = linked_item(object, slot);
    if (other == NULL) return 0;
    Item* item = other->user_data;
    switch (property) {
    case NativeArticleLink_Exists: return 1;
    case NativeArticleLink_PositionX: return item->pos.x;
    case NativeArticleLink_PositionY: return item->pos.y;
    case NativeArticleLink_PositionZ: return item->pos.z;
    case NativeArticleLink_NormalX: return item->xDD4_itemVar.native_article.surface_normal.x;
    case NativeArticleLink_NormalY: return item->xDD4_itemVar.native_article.surface_normal.y;
    case NativeArticleLink_NormalZ: return item->xDD4_itemVar.native_article.surface_normal.z;
    case NativeArticleLink_Facing: return item->facing_dir;
    default: return 0;
    }
}

int32_t nativeArticle_LinkId(Item_GObj* object, unsigned slot)
{
    Item_GObj* linked = linked_item(object, slot);
    if (linked == NULL) return -1;
    int32_t number;
    memcpy(&number, &((Item*) linked->user_data)->x1C, sizeof(number));
    return number;
}

float nativeArticle_BoneValue(Item_GObj* object, unsigned bone, unsigned property)
{
    if (object == NULL || bone >= 100 || property >= 9) return 0;
    Item* item = object->user_data;
    if (item == NULL || item->xBBC_dynamicBoneTable == NULL) return 0;
    HSD_JObj* joint = item->xBBC_dynamicBoneTable->bones[bone];
    if (joint == NULL) return 0;
    Vec3 value;
    if (property < 3) lb_8000B1CC(joint, NULL, &value);
    else if (property < 6) {
        if (joint->flags & JOBJ_USE_QUATERNION) return 0;
        value = (Vec3) { joint->rotate.x, joint->rotate.y, joint->rotate.z };
    }
    else HSD_JObjGetScale(joint, &value);
    return property % 3 == 0 ? value.x : property % 3 == 1 ? value.y : value.z;
}

float nativeArticle_LinkBoneValue(Item_GObj* object, unsigned slot, unsigned bone,
                                  unsigned property)
{
    return nativeArticle_BoneValue(linked_item(object, slot), bone, property);
}

void nativeArticle_CopyLinkCollision(Item_GObj* object, unsigned slot)
{
    Item_GObj* other = linked_item(object, slot);
    if (other != NULL) {
        Item* source = other->user_data;
        Item* target = object->user_data;
        target->x378_itemColl = source->x378_itemColl;
    }
}

bool nativeArticle_SignalLink(Item_GObj* object, unsigned slot, uint32_t name, float value)
{
    if (!isfinite(value)) return false;
    Item_GObj* other = linked_item(object, slot);
    if (other == NULL) return false;
    Item* item = other->user_data;
    const NativeFighterArticleLogic* logic = article_logic(item->native_article_author_kind, item->kind);
    if (logic == NULL || logic->logic != item->xB8_itemLogicTable) return false;
    for (unsigned i = 0; i < logic->signal_count; ++i)
        if (logic->signals[i].name_hash == name) {
            if (logic->signals[i].callback == NULL || !nativeCallbackDispatch_Enter())
                return false;
            logic->signals[i].callback(other, value);
            nativeCallbackDispatch_Leave();
            return true;
        }
    return false;
}
