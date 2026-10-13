#include "native_fighter_target.h"
#include "ftlib.h"
#include "fighter.h"
#include "native_callback_lifetime.h"
#include <melee/lb/lb_00B0.h>
#include "types.h"
#include "kinds/ftCommon/ftCo_Guard.h"
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>

uint32_t nativeFighterTarget_Contact(Item_GObj* source)
{
    Item* item = source != NULL ? source->user_data : NULL;
    return item != NULL ? nativeFighterTarget_Id(item->xCF4_fighterGObjUnk) : 0;
}

uint32_t nativeFighterTarget_Id(HSD_GObj* object)
{
    if (object == NULL || HSD_GObjPLinkHead == NULL) return 0;
    // Compare identity in the live registry before reading the contact object:
    // a contact may be an item or an object removed earlier this tick.
    for (HSD_GObj* fighter = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];
         fighter != NULL; fighter = fighter->next) {
        if (fighter == object && fighter->user_data != NULL)
            return ftLib_80087460(fighter);
    }
    return 0;
}

uint32_t nativeFighterTarget_CurrentOwner(Item_GObj* source)
{
    Item* item = source != NULL ? source->user_data : NULL;
    return item != NULL ? nativeFighterTarget_Id(item->owner) : 0;
}

uint32_t nativeFighterTarget_Reflector(Item_GObj* source)
{
    Item* item = source != NULL ? source->user_data : NULL;
    return item != NULL ? nativeFighterTarget_Id(item->xC64_reflectGObj) : 0;
}

bool nativeFighterTarget_CameraPosition(uint32_t target, Vec3* position)
{
    if (position == NULL) return false;
    *position = (Vec3) { 0, 0, 0 };
    HSD_GObj* fighter = target != 0 ? ftLib_8008741C(target) : NULL;
    if (fighter == NULL) return false;
    ftLib_800866DC(fighter, position);
    return true;
}

bool nativeFighterTarget_IsFighter(uint32_t target)
{
    return target != 0 && ftLib_8008741C(target) != NULL;
}

bool nativeFighterTarget_Position(uint32_t target, Vec3* position)
{
    if (position == NULL || target == 0) return false;
    Fighter_GObj* object = ftLib_8008741C(target);
    if (object == NULL) return false;
    const Fighter* fighter = object->user_data;
    if (fighter == NULL) return false;
    *position = fighter->cur_pos;
    return true;
}

bool nativeFighterTarget_ControllerColor(uint32_t target, GXColor* color)
{
    if (color == NULL) return false;
    *color = (GXColor) { 0, 0, 0, 0 };
    if (target == 0 || HSD_GObjPLinkHead == NULL) return false;
    for (HSD_GObj* object = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];
         object != NULL; object = object->next) {
        const Fighter* fighter = object->user_data;
        if (fighter != NULL && (uint32_t)fighter->x8_spawnNum == target)
            return ftLib_800870BC(object, color);
    }
    return false;
}

bool nativeFighterTarget_Airborne(uint32_t target)
{
    if (target == 0 || HSD_GObjPLinkHead == NULL) return false;
    // Resolve only live creation IDs; a removed owner's retained address is
    // never read, and an address reused for another fighter does not match.
    for (HSD_GObj* object = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];
         object != NULL; object = object->next) {
        const Fighter* fighter = object->user_data;
        if (fighter != NULL && (uint32_t)fighter->x8_spawnNum == target)
            return fighter->ground_or_air == GA_Air;
    }
    return false;
}

bool nativeFighterTarget_SmashChargeActive(uint32_t target)
{
    if (target == 0 || HSD_GObjPLinkHead == NULL) return false;
    for (HSD_GObj* object = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];
         object != NULL; object = object->next) {
        const Fighter* fighter = object->user_data;
        if (fighter != NULL && (uint32_t)fighter->x8_spawnNum == target)
            return fighter->smash_attrs.state == 2;
    }
    return false;
}

int32_t nativeFighterTarget_Motion(uint32_t target)
{
    if (target == 0 || HSD_GObjPLinkHead == NULL) return -1;
    HSD_GObj* object = ftLib_8008741C(target);
    return object != NULL ? ((Fighter*)object->user_data)->motion_id : -1;
}

static Item* current_held_item(uint32_t target)
{
    if (target == 0 || HSD_GObjPLinkHead == NULL) return NULL;
    HSD_GObj* holder = ftLib_8008741C(target);
    if (holder == NULL || holder->user_data == NULL) return NULL;
    HSD_GObj* held = ((Fighter*)holder->user_data)->item_gobj;
    if (held == NULL) return NULL;
    for (HSD_GObj* live = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];
         live != NULL; live = live->next) {
        if (live != held) continue;
        Item* item = live->user_data;
        return item != NULL && !item->native_removal_in_progress ? item : NULL;
    }
    return NULL;
}

int32_t nativeFighterTarget_HeldId(uint32_t target)
{
    Item* item = current_held_item(target);
    return item != NULL ? (int32_t)item->x1C : -1;
}

int32_t nativeFighterTarget_HeldKind(uint32_t target)
{
    Item* item = current_held_item(target);
    return item != NULL ? (int32_t)item->kind : -1;
}

int32_t nativeFighterTarget_HeldMotion(uint32_t target)
{
    Item* item = current_held_item(target);
    return item != NULL ? (int32_t)item->msid : -1;
}

float nativeFighterTarget_HeldFrame(uint32_t target)
{
    Item* item = current_held_item(target);
    return item != NULL ? item->x5CC_currentAnimFrame : 0.0f;
}

bool nativeFighterTarget_GuardActive(uint32_t target)
{
    HSD_GObj* fighter = target != 0 ? ftLib_8008741C(target) : NULL;
    return fighter != NULL && ftLib_80086A18(fighter);
}

float nativeFighterTarget_ModelScale(uint32_t target)
{
    HSD_GObj* fighter = target != 0 ? ftLib_8008741C(target) : NULL;
    return fighter != NULL ? ftLib_800869D4(fighter) : 0.0f;
}

float nativeFighterTarget_InstanceScaleY(uint32_t target)
{
    HSD_GObj* fighter = target != 0 ? ftLib_8008741C(target) : NULL;
    return fighter != NULL ? ((Fighter*)fighter->user_data)->x34_scale.y : 1.0f;
}

float nativeFighterTarget_Shield(uint32_t target, Vec3* center)
{
    if (center == NULL) return 0.0f;
    *center = (Vec3) { 0, 0, 0 };
    HSD_GObj* fighter = target != 0 ? ftLib_8008741C(target) : NULL;
    return fighter != NULL ? ftCo_80094098(fighter, &center->x) : 0.0f;
}


bool nativeFighterTarget_BonePosition(uint32_t target, unsigned part, Vec3* position)
{
    if (position == NULL || target == 0) return false;
    Fighter_GObj* object = ftLib_8008741C(target);
    if (object == NULL) return false;
    Fighter* fighter = object->user_data;
    FighterKind kind = fighter->kind;
    ftData* data = fighter->ft_data;
    if (ftPartsTable == NULL || (unsigned)kind >= Ft_Kind_RuntimeCapacity ||
        ftPartsTable[kind] == NULL || part >= (unsigned)ftPartsTable[kind]->parts_num ||
        fighter->parts == NULL || fighter->parts[part].joint == NULL) return false;
    Vec3 result;
    lb_8000B1CC(fighter->parts[part].joint, NULL, &result);
    if (!nativeCallbackSource_Alive(object, target, NativeCallbackSource_Fighter) ||
        object->user_data != fighter || fighter->kind != kind || fighter->ft_data != data) return false;
    *position = result;
    return true;
}

bool nativeFighterTarget_PositionDelta(uint32_t target, Vec3* position)
{
    if (position == NULL || target == 0) return false;
    Fighter_GObj* object = ftLib_8008741C(target);
    if (object == NULL || object->user_data == NULL) return false;
    *position = ((Fighter*)object->user_data)->pos_delta;
    return true;
}

float nativeFighterTarget_Damage(uint32_t target)
{
    Fighter_GObj* object = target != 0 ? ftLib_8008741C(target) : NULL;
    return object != NULL ? ((Fighter*)object->user_data)->dmg.x1830_percent : 0.0f;
}

float nativeFighterTarget_Facing(uint32_t target)
{
    Fighter_GObj* object = target != 0 ? ftLib_8008741C(target) : NULL;
    return object != NULL ? ((Fighter*)object->user_data)->facing_dir : 0.0f;
}

bool nativeFighterTarget_Sleeping(uint32_t target)
{
    Fighter_GObj* object = target != 0 ? ftLib_8008741C(target) : NULL;
    return object != NULL && ((Fighter*)object->user_data)->x221F_b3;
}

int32_t nativeFighterTarget_FloorLine(uint32_t target)
{
    Fighter_GObj* object = target != 0 ? ftLib_8008741C(target) : NULL;
    return object != NULL ? ((Fighter*)object->user_data)->coll_data.floor.index : -1;
}

float nativeFighterTarget_CpuBodyHalfWidth(uint32_t target)
{
    Fighter_GObj* object = target != 0 ? ftLib_8008741C(target) : NULL;
    return object != NULL ? ((Fighter*)object->user_data)->cpu.x564 : 0.0f;
}
