#include "itmariofireball.h"

#include <math.h>
#include <stdio.h>

#ifdef MELEE_NATIVE
#include <melee/ft/ftparts.h>
#include <melee/ft/types.h>
#include <melee/gm/native_match_launch.h>
#include <melee/gm/native_replay.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/jobj.h>
#endif

#include <dolphin/mtx.h>
#include <melee/db/db.h>
#include <melee/ef/efasync.h>
#include <melee/it/inlines.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/itCommonItems.h>
#include <melee/it/item.h>
#include <melee/it/itgroundcoll.h>
#include <melee/it/itmaplib.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/mtx.h>

ItemStateTable it_803F6788[] = {
    { 0, itMariofireball_UnkMotion0_Anim, itMariofireball_UnkMotion0_Phys,
      itMariofireball_UnkMotion0_Coll },
};

void it_8029B6F8(Item_GObj* gobj, Vec3* pos, ItemKind kind, f32 facing_dir)
{
    Item_GObj* spawned_gobj;

    SpawnItem spawn_item;
    spawn_item.kind = kind;
    spawn_item.prev_pos = *pos;
    spawn_item.prev_pos.z = 0.0f;
    it_8026BB68(gobj, &spawn_item.pos);
    spawn_item.facing_dir = facing_dir;
    spawn_item.x3C_damage = 0;
    spawn_item.vel.x = spawn_item.vel.y = spawn_item.vel.z = 0.0f;
    spawn_item.x0_parent_gobj = gobj;
    spawn_item.x4_parent_gobj2 = spawn_item.x0_parent_gobj;
    spawn_item.x44_flag.b0 = true;
    spawn_item.x40 = 0;

    spawned_gobj = Item_80268B18(&spawn_item);
    it_8029B7C0(spawned_gobj);
#ifdef MELEE_NATIVE
    if (nativeMatchLaunch_TraceActions()) {
        Item* spawned = GET_ITEM(spawned_gobj);
        Fighter* owner = gobj->user_data;
        Fighter_Part bone = ftParts_GetBoneIndex(owner, FtPart_L1stNb);
        HSD_JObj* hand = owner->parts[bone].joint;
        fprintf(stderr,
                "[fireball] sample %llu owner=%d costume=%d "
                "anim=%d source=%d mask=%x bone=%d frame=%.2f "
                "joint-frame=%.2f "
                "fighter=(%.5f,%.5f) "
                "hand-local=(%.5f,%.5f,%.5f) "
                "pos=(%.5f,%.5f) "
                "vel=(%.5f,%.5f) life=%.2f\n",
                (unsigned long long) nativeReplay_SampleCount(),
                owner->kind, owner->x619_costume_id, owner->anim_id,
                owner->x597_bits, owner->x594_bits, bone,
                owner->cur_anim_frame,
                hand->aobj != NULL ? hand->aobj->curr_frame : -1.0f,
                owner->cur_pos.x, owner->cur_pos.y,
                hand->translate.x, hand->translate.y, hand->translate.z,
                spawned->pos.x, spawned->pos.y,
                spawned->x40_vel.x, spawned->x40_vel.y,
                spawned->xD44_lifeTimer);
    }
#endif
    db_80225DD8(spawned_gobj, gobj);
    it_802750F8(spawned_gobj);
}

void it_8029B7C0(Item_GObj* gobj)
{
    Item* it = GET_ITEM(gobj);
    itUnkAttributes* ap = it->xC4_article_data->x4_specialAttributes;
    f32 x0 = ap->x0_float;
    f32 x4 = ap->x4_float;

    it->x40_vel.x = it->facing_dir * (x0 * cosf(x4));
    it->x40_vel.y = x0 * sinf(x4);
    it->x40_vel.z = 0.0f;

    it_80275158(gobj, ap->x8);
    Item_80268E5C(gobj, 0, ITEM_ANIM_UPDATE);
}

bool itMariofireball_UnkMotion0_Anim(Item_GObj* gobj)
{
    Item* item = GET_ITEM(gobj);
    --item->xD44_lifeTimer;
    return item->xD44_lifeTimer <= 0 ? true : false;
}

void itMariofireball_UnkMotion0_Phys(Item_GObj* gobj)
{
    Item_ApplyFallingPhysics(gobj);
}

static double calc_dist_2d_accurate(Vec3* v)
{
    return sqrtf_accurate(VEC2_SQ_LEN(*v));
}

bool itMariofireball_UnkMotion0_Coll(Item_GObj* gobj)
{
    it_8026D9A0(gobj);
    if (it_8027781C(gobj)) {
        Item* it = GET_ITEM(gobj);
        HSD_JObj* jobj = GET_JOBJ(gobj);
        itUnkAttributes* attrs = it->xC4_article_data->x4_specialAttributes;
        if (calc_dist_2d_accurate(&it->x40_vel) < attrs->x10) {
            return true;
        }
        if (it->kind == It_Kind_Mario_Fire) {
            Item_8026AE84(it, ~0xFFFD40C6, 0x7f,
                          0x40); // What the fuck is this ID?
            efAsync_Spawn(gobj, &(GET_ITEM(gobj)->xBC0), 1, 1147, jobj);
        } else {
            efAsync_Spawn(gobj, &(GET_ITEM(gobj)->xBC0), 1, 1184, jobj);
        }
    }
    return false;
}

bool itMarioFireball_Logic87_DmgDealt(Item_GObj* gobj)
{
    return true;
}

bool itMarioFireball_Logic87_Reflected(Item_GObj* gobj)
{
    return it_80273030(gobj);
}

bool itMarioFireball_Logic87_Clanked(Item_GObj* gobj)
{
    return true;
}

bool itMarioFireball_Logic87_HitShield(Item_GObj* gobj)
{
    return true;
}

bool itMarioFireball_Logic87_Absorbed(Item_GObj* gobj)
{
    return true;
}

bool itMarioFireball_Logic87_ShieldBounced(Item_GObj* gobj)
{
    return itColl_BounceOffShield(gobj);
}

void itMarioFireball_Logic87_EvtUnk(Item_GObj* gobj, HSD_GObj* referenced_gobj)
{
    it_8026B894(gobj, referenced_gobj);
}
