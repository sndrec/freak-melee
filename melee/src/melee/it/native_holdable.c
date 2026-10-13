#include "native_holdable.h"

#ifdef MELEE_NATIVE

#include <melee/ft/fighter.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/kinds/ftCommon/ftpickupitem.h>
#include <melee/it/item.h>
#include <melee/it/types.h>
#include <melee/it/it_2725.h>
#include <melee/it/itcoll.h>
#include <melee/it/itgroundcoll.h>
#include <melee/it/itmaplib.h>
#include <melee/it/kinds/inlines.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/gobj.h>

#include <string.h>

static const NativeItemDefinition* definition(const Item* item)
{
    const NativeFighterDefinition* fighter =
        nativeFighterRuntime_Definition(item->xDD4_itemVar.native_holdable.owner_kind);
    unsigned index = item->xDD4_itemVar.native_holdable.definition_index;
    return fighter != NULL && fighter->items != NULL &&
                   index < fighter->item_count ? &fighter->items[index] : NULL;
}

static bool expire(Item_GObj* gobj)
{
    Item* item = GET_ITEM(gobj);
    const NativeItemDefinition* spec = definition(item);
    if (spec == NULL) return true;
    ++item->xDD4_itemVar.native_holdable.age;
    if (spec->on_tick != NULL) spec->on_tick(gobj);
    return spec->lifetime_frames != 0 &&
           item->xDD4_itemVar.native_holdable.age >= spec->lifetime_frames;
}

static bool held_animation(Item_GObj* gobj) { return false; }
static void held_physics(Item_GObj* gobj) { }
static bool active_animation(Item_GObj* gobj) { return expire(gobj); }
static void flying_physics(Item_GObj* gobj)
{
    Item* item = GET_ITEM(gobj);
    const NativeItemDefinition* spec = definition(item);
    if (spec == NULL || spec->on_tick == NULL)
        Item_ApplyFallingPhysics(gobj);
}

static bool flying_collision(Item_GObj* gobj)
{
    Item* item = GET_ITEM(gobj);
    const NativeItemDefinition* spec = definition(item);
    int contact = it_8026DAA8(gobj);
    item->xDD4_itemVar.native_holdable.collision_flags = (u16) contact;
    if (spec != NULL && spec->on_ground_collision != NULL)
        return spec->on_ground_collision(gobj);
    if (contact & 0xE) {
        it_80276FC4(gobj, contact);
        Item_80268E5C(gobj, 1, ITEM_ANIM_UPDATE);
    } else if (contact & 3) {
        return true;
    }
    return false;
}

static ItemStateTable states[] = {
    { -1, held_animation, held_physics, NULL },
    { -1, active_animation, held_physics, NULL },
    { -1, active_animation, flying_physics, flying_collision },
    { -1, active_animation, flying_physics, flying_collision },
};

static void picked_up(Item_GObj* gobj)
{
    Item_80268E5C(gobj, 0, ITEM_ANIM_UPDATE);
    it_80273670(gobj, 0, 0.0f);
    Item_802694CC(gobj);
}

static void thrown(Item_GObj* gobj)
{
    Item* item = GET_ITEM(gobj);
    const NativeItemDefinition* spec = definition(item);
    if (spec == NULL) return;
    item->xDD4_itemVar.native_holdable.age = 0;
    Item_80268E5C(gobj, 2, ITEM_ANIM_UPDATE | ITEM_DROP_UPDATE);
    it_80273670(gobj, 0, 0.0f);
    HitCapsule* hitbox = &item->x5D4_hitboxes[0].hit;
    hitbox->state = HitCapsule_Enabled;
    hitbox->x4 = 0;
    it_8026FCF8(item, hitbox);
    hitbox->jobj = GET_JOBJ(gobj);
    hitbox->b_offset = (Vec3) { 0, 0, 0 };
    hitbox->scale = spec->hitbox_size / item->scl;
    hitbox->kb_angle = spec->angle;
    hitbox->x24 = spec->knockback_growth;
    hitbox->x28 = 0;
    hitbox->x2C = spec->base_knockback;
    hitbox->element = HitElement_Normal;
    hitbox->x40_b0 = 1;
    hitbox->x40_b2 = 1;
    hitbox->x40_b3 = 1;
    hitbox->x42_b5 = 1;
    hitbox->x42_b7 = 1;
    hitbox->x43_b2 = 0;
    item->xDC8_word.flags.x16 = 1;
    item->xDAA_flag.b2 = 0;
    it_80272460(hitbox, spec->damage, gobj);
    if (spec->on_thrown != NULL) spec->on_thrown(gobj);
}

static void dropped(Item_GObj* gobj)
{
    GET_ITEM(gobj)->xDD4_itemVar.native_holdable.age = 0;
    Item_80268E5C(gobj, 3, ITEM_ANIM_UPDATE | ITEM_DROP_UPDATE);
    it_80273670(gobj, 0, 0.0f);
}

static bool hit(Item_GObj* gobj)
{
    const NativeItemDefinition* spec = definition(GET_ITEM(gobj));
    if (spec != NULL && spec->on_hit != NULL) return spec->on_hit(gobj);
    itColl_BounceOffVictim(gobj);
    Item_80268E5C(gobj, 1, ITEM_ANIM_UPDATE);
    return false;
}

static bool clanked(Item_GObj* gobj)
{
    itColl_BounceOffVictim(gobj);
    return false;
}

static bool reflected(Item_GObj* gobj)
{
    it_80273030(gobj);
    return false;
}

static bool shield_bounced(Item_GObj* gobj)
{
    itColl_BounceOffShield(gobj);
    return false;
}

ItemLogicTable nativeHoldable_Logic = {
    .states = states,
    .picked_up = picked_up,
    .dropped = dropped,
    .thrown = thrown,
    .dmg_dealt = hit,
    .reflected = reflected,
    .clanked = clanked,
    .shield_bounced = shield_bounced,
    .hit_shield = clanked,
};

Item_GObj* nativeHoldable_Spawn(Fighter_GObj* fighter_gobj,
                               uint16_t definition_index)
{
    Fighter* fighter = GET_FIGHTER(fighter_gobj);
    const NativeFighterDefinition* custom =
        nativeFighterRuntime_Definition(fighter->kind);
    if (custom == NULL || custom->items == NULL ||
        definition_index >= custom->item_count || fighter->item_gobj != NULL)
        return NULL;

    Vec3 pos;
    Fighter_Part part = fighter->ft_data->x8->x10;
    HSD_JObj* joint = fighter->parts[part].joint;
    if (joint == NULL) return NULL;
    lb_8000B1CC(joint, NULL, &pos);
    SpawnItem spawn = { 0 };
    spawn.kind = It_Kind_NativeHoldable;
    Item_InitSpawn(&spawn, fighter_gobj, &pos, fighter->facing_dir);
    Item_GObj* gobj = Item_80268B18(&spawn);
    if (gobj == NULL) return NULL;
    Item* item = GET_ITEM(gobj);
    item->xDD4_itemVar.native_holdable.owner_kind = fighter->kind;
    item->xDD4_itemVar.native_holdable.definition_index = definition_index;
    item->xDD4_itemVar.native_holdable.age = 0;
    item->xDD4_itemVar.native_holdable.collision_flags = 0;
    memset(item->xDD4_itemVar.native_holdable.state, 0,
           sizeof(item->xDD4_itemVar.native_holdable.state));
    fighter->item_gobj = gobj;
    Item_8026AB54(gobj, fighter_gobj, part);
    ftpickupitem_80094818(fighter_gobj, false);
    return gobj;
}

Item_GObj* nativeHoldable_SpawnStockFood(Item_GObj* source)
{
    Item* item = GET_ITEM(source);
    SpawnItem spawn = { 0 };
    spawn.kind = It_Kind_Foods;
    Item_InitSpawn(&spawn, NULL, &item->pos, item->facing_dir);
    spawn.vel.x = item->x40_vel.x * 0.35f;
    spawn.vel.y = 1.8f;
    return Item_80268B18(&spawn);
}

#endif
