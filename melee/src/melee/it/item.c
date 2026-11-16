#include "item.h"
#include <melee/ft/native_callback_lifetime.h>

#include <melee/lb/forward.h>

#include <math.h>

#include "inlines.h"
#include "it_26B1.h"
#include "it_2725.h"
#include "it_279C.h"
#include "it_3F14.h"
#include "it_3F2F.h"
#include "itanimlist.h"
#include "itcoll.h"
#include "iteffect.h"
#include "itgroundcoll.h"
#include "itdraw.h"
#include "ithitbox.h"
#include "itmaplib.h"
#include "itmaterial.h"
#include "types.h"
#include <dolphin/mtx.h>
#include <melee/cm/camera.h>
#include <melee/db/db.h>
#include <melee/ef/efasync.h>
#include <melee/ef/eflib.h>
#include <melee/ft/ftlib.h>
#ifdef MELEE_NATIVE
#include <melee/ft/native_fighter_runtime.h>
#include <melee/gm/native_replay.h>
#include <melee/it/native_holdable.h>
#include <melee/it/native_article_model.h>
#include <melee/it/native_article_resource.h>
#include <melee/it/native_item_behavior.h>
#include <melee/it/native_item_owner.h>
#include <melee/it/native_item_pool.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/it/native_item_lifetime.h>
#include <melee/ft/native_script_receiver.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#endif
#include <melee/gm/gm_unsplit.h>
#include <melee/gr/grlib.h>
#include <melee/gr/stage.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lb_00F9.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/mp/mpcoll.h>
#include <melee/mp/mplib.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/jobj.h>

/* 267130 */ static void Item_80267130(HSD_GObj* gobj, SpawnItem* spawnItem);
/* 2674AC */ static void Item_802674AC(SpawnItem* spawnItem);
/* 2675A8 */ static void Item_802675A8(HSD_GObj* gobj);
/* 2676F4 */ static void Item_802676F4(HSD_GObj* gobj);
/* 26784C */ static bool Item_8026784C(enum_t dropItem, int);
/* 267AA8 */ static bool Item_80267AA8(HSD_GObj* gobj, SpawnItem* spawnItem,
    NativeItemBehaviorSelection selection, FighterKind author,uint32_t source_owner);
/* 26814C */ static void Item_8026814C(HSD_GObj* gobj);
/* 2682F0 */ static bool Item_802682F0(HSD_GObj* gobj);
/* 268560 */ static void Item_80268560(HSD_GObj* gobj);
/* 26862C */ static HSD_GObj* Item_8026862C(SpawnItem* spawnItem,
    NativeItemBehaviorSelection selection, FighterKind author,uint32_t source_owner);
/* 268BE0 */ static void Item_80268BE0(HSD_JObj* item_jobj,
                                       HSD_AnimJoint* anim_joint,
                                       HSD_MatAnimJoint* matanim_joint,
                                       HSD_ShapeAnimJoint* shapeanim_joint,
                                       Item* item_data);
/* 269528 */ static void Item_80269528(HSD_GObj* gobj);
/* 2696CC */ static bool Item_802696CC(HSD_GObj* gobj);
/* 269A9C */ static void Item_80269A9C(HSD_GObj* gobj);
/* 269B60 */ static void Item_80269B60(HSD_GObj* gobj);
/* 269BE4 */ static void Item_80269BE4(HSD_GObj* gobj);
/* 269C5C */ static void Item_80269C5C(HSD_GObj* gobj);
/* 269CC4 */ static void Item_80269CC4(HSD_GObj* gobj);
/* 269DC8 */ static bool Item_80269DC8(HSD_GObj* gobj);
/* 269F14 */ static bool Item_80269F14(HSD_GObj* gobj);
/* 26A0A0 */ static void Item_8026A0A0(HSD_GObj* gobj);
/* 26A0FC */ static void Item_8026A0FC(HSD_GObj* gobj);
/* 26A158 */ static void Item_8026A158(HSD_GObj* gobj);
/* 26A1E8 */ static void Item_8026A1E8(HSD_GObj* gobj);
/* 26A294 */ static void Item_8026A294(HSD_GObj* gobj);
/* 26A788 */ static void Item_8026A788(HSD_GObj* gobj);
/* 26A810 */ static bool Item_8026A810(HSD_GObj* gobj);
/* 26B0B4 */ static void Item_8026B0B4(HSD_GObj* gobj);
#ifdef MELEE_NATIVE
static bool processSourceCallback(HSD_GObj* gobj,uint32_t hook);
#endif

static bool itemCallbackCurrent(HSD_GObj* object,uint32_t generation)
{
    if(!nativeCallbackSource_Alive(object,generation,NativeCallbackSource_Item))return false;
#ifdef MELEE_NATIVE
    // HSD_GObjFree may defer physical removal until the current proc returns.
    if(((Item*)object->user_data)->native_removal_in_progress)return false;
#endif
    return true;
}

/// Check if items are enabled
bool Item_80266F3C(void)
{
    if (gm_GetItemFrequency() != -1) {
        return true;
    }

    return false;
}

/// Check to load ItCo.dat/usd
void Item_80266F70(void)
{
    it_8027870C(Item_80266F3C());
}

/// ItCo prefunction with 0
void Item_80266FA8(void)
{
    it_8027870C(false);
}

#ifndef MELEE_NATIVE
static HSD_ObjAllocData item_alloc_data;
static HSD_ObjAllocData item_dynamic_bones_alloc_data;
#endif

HSD_ObjAllocUnk Item_804A0C64;
Item_FtTrack Item_804A0CCC;
S32Vec3 Item_804A0E24;

/// Init item struct?
void Item_80266FCC(void)
{
#ifndef MELEE_NATIVE
    HSD_ObjAllocInit(&item_alloc_data, sizeof(Item), 4);
    HSD_ObjAllocInit(&item_dynamic_bones_alloc_data, sizeof(DynamicBoneTable),
                     4);
#endif

    Item_804A0C64.x0 = 0;
    Item_804A0C64.x8 = 0;
    Item_804A0C64.x10 = 0;
    Item_804A0C64.x18 = 0;
    Item_804A0C64.x1C = 0;
    Item_804A0C64.x24 = 0;
    Item_804A0C64.x2C = 0;
    Item_804A0C64.x34 = 0;
    Item_804A0C64.x3C = 0;
    Item_804A0C64.x40 = 0;
    Item_804A0C64.x48 = 0;
    Item_804A0C64.x50 = 0;
    Item_804A0C64.x58 = 0;
    Item_804A0C64.x60 = 0;

    Item_804A0C64.x4 = it_804D6D28->x0;
    Item_804A0C64.xC = it_804D6D28->x4;
    Item_804A0C64.x14 = it_804D6D28->x8;
    Item_804A0C64.x20 = it_804D6D28->xC;
    Item_804A0C64.x28 = it_804D6D28->x10;
    Item_804A0C64.x30 = it_804D6D28->x14;
    Item_804A0C64.x38 = it_804D6D28->x18;
    Item_804A0C64.x44 = it_804D6D28->x1C;
    Item_804A0C64.x4C = it_804D6D28->x24;
    Item_804A0C64.x54 = it_804D6D28->x20;
    Item_804A0C64.x5C = it_804D6D28->x28;
    Item_804A0C64.x64 = it_804D6D28->x148;

    Item_804A0CCC.x154.b0 = true;
    Item_804A0CCC.count = 1;

    Item_804A0E24.x = -1;
    Item_804A0E24.y = -1;
    Item_804A0E24.z = 0;

    it_804D6D00 = -1;
    it_804D6D14 = 1;
    it_804D6D10 = 0;
    it_804D6D0C = 0;
    it_804D6D08 = 0;

    it_804A0E60.total_weight = 0;
    it_804A0E50.total_weight = 0;
    it_804A0E30.pick_table.total_weight = 0;
    it_804A0E60.count = 0;
    it_804A0E50.count = 0;
    it_804A0E30.pick_table.count = 0;
}

static void ItUnkHoldKind(HSD_GObj* gobj)
{
    Item* it = HSD_GObjGetUserData(gobj);

    switch (it->hold_kind) {
    case 4:
    case 5:
    case 6:
    case 7: {
        int temp_r3 = gm_8017E068();
        if (temp_r3 >= 0) {
            it->xC3C = it_804D6D28->x80_float[temp_r3];
        }
    }
    }
}

static inline void HSD_JObjSetScaleItem(Item* it, HSD_JObj* jobj, Vec3* scl)
{
    scl->x = scl->y = scl->z = it->scl;
    HSD_JObjSetScale(jobj, scl);
}

static inline void HSD_JObjSetFacingDirItem(HSD_JObj* jobj, Item* it)
{
    if (it->xDC8_word.flags.x19 == true) {
        HSD_JObjSetRotationY(jobj, M_PI / 2 * it->facing_dir);
    }
}

static void Item_80267130(HSD_GObj* gobj, SpawnItem* spawnItem)
{
    Item* item_data = (Item*) HSD_GObjGetUserData(gobj);
    HSD_JObj* model = (HSD_JObj*) HSD_GObjGetHSDObj(gobj);

    item_data->pos = spawnItem->prev_pos;
    item_data->init_facing_dir = item_data->facing_dir = spawnItem->facing_dir;
    item_data->owner = spawnItem->x0_parent_gobj;

    {
        int facing_dir;

        if (item_data->facing_dir == -1.0F) {
            facing_dir = -1;
        } else {
            facing_dir = 1;
        }

        mpCollSetFacingDir(&item_data->x378_itemColl, facing_dir);
    }

    HSD_JObjSetFacingDirItem(model, item_data);
    it_80273500(gobj, &spawnItem->vel);

    item_data->x5CC_currentAnimFrame = 0.0f;
    item_data->x5D0_animFrameSpeed = 1.0f;

    it_802753BC(gobj, spawnItem->x3C_damage);
    it_802753DC(gobj);

    item_data->xC64_reflectGObj = NULL;
    item_data->xDCC_flag.b0 = false;

    item_data->xC90_absorbGObj = NULL;
    item_data->xD0C = 0;

    it_8026B390(gobj);
    item_data->xDC8_word.flags.x13 = false;

    if (spawnItem->x48_ground_or_air == GA_Air) {
        it_802762BC(item_data);
    } else {
        it_802762B0(item_data);
    }

    it_80275E98(gobj, spawnItem);
    it_80274DAC(gobj);
    HSD_JObjSetTranslate(model, &item_data->pos);
    it_80274658(gobj, it_804D6D28->x6C_float);
    it_802725D4(gobj);
    it_80271508(gobj, HurtCapsule_Enabled);
    it_80272280(gobj);

    item_data->on_accessory = NULL;
    item_data->touched = NULL;
    item_data->entered_hitlag = NULL;
    item_data->exited_hitlag = NULL;
    item_data->jumped_on = NULL;

    if (item_data->owner != NULL) {
        db_80225DD8(gobj, item_data->owner);
    } else {
        db_80225D64(gobj, item_data->owner);
    }

    it_8027B0C4(gobj, spawnItem);
    it_80279B64(item_data);
    ItUnkHoldKind(gobj);
}

/// Remove Camera Box
void Item_80267454(HSD_GObj* gobj)
{
    Item* item_data = gobj->user_data;

    if (item_data->xDCD_flag.b01 != 0 &&

        (item_data->x520_cameraBox != NULL))
    {
        Camera_800290D4(item_data->x520_cameraBox);
        item_data->x520_cameraBox = NULL;
        item_data->xDCD_flag.b01 = 0;
    }
}

static void Item_802674AC(SpawnItem* spawnItem)
{
    ItemKind kind = spawnItem->kind;

#ifdef MELEE_NATIVE
    if (kind == It_Kind_NativeHoldable) {
        // Authored snacks use ordinary pickup/throw ownership, not the
        // character-article ownership used by guns and other special items.
        spawnItem->hold_kind = ITEM_HOLD_0;
        return;
    }
#endif

    if (kind == It_Kind_Foods) {
        spawnItem->hold_kind = 2;
        return;
    }

    if (kind == It_PKind_Random) {
        spawnItem->hold_kind = 3;
        return;
    }

    if (kind < It_Kind_L_Gun_Ray) {
        spawnItem->hold_kind = 0;
        return;
    }

    if (kind < It_Kind_Kuriboh) {
        spawnItem->hold_kind = 1;
        return;
    }

    if (kind < It_Kind_Octarock_Stone) {
        spawnItem->hold_kind = 6;
        return;
    }

    if (kind < It_Kind_Mario_Fire) {
        spawnItem->hold_kind = 7;
        return;
    }

    if (kind < It_Kind_Unk4) {
        spawnItem->hold_kind = 8;
        return;
    }

    if (kind == It_Kind_Unk4) {
        spawnItem->hold_kind = 12;
        return;
    }

    if (kind < It_PKind_Random) {
        spawnItem->hold_kind = 11;
        return;
    }

    if (kind < It_PKind_Terminate) {
        spawnItem->hold_kind = 9;
        return;
    }

    if (kind < It_Kind_Old_Kuri) {
        spawnItem->hold_kind = 10;
        return;
    }

    if (kind < It_Kind_Arwing_Laser) {
        spawnItem->hold_kind = 4;
        return;
    }

    spawnItem->hold_kind = 5;
}

static void Item_802675A8(HSD_GObj* gobj)
{
    Item* item_data = gobj->user_data;

    switch (item_data->hold_kind) {
    case 0:
        Item_804A0C64.x0--;
        break;
    case 1:
        Item_804A0C64.x8--;
        break;
    case 2:
        Item_804A0C64.x10--;
        break;
    case 3:
        Item_804A0C64.x58--;
        break;
    case 4:
        Item_804A0C64.x3C--;
        break;
    case 5:
        Item_804A0C64.x40--;
        break;
    case 6:
        Item_804A0C64.x2C--;
        break;
    case 7:
        Item_804A0C64.x34--;
        break;
    case 8:
        Item_804A0C64.x18--;
        break;
    case 9:
        Item_804A0C64.x1C--;
        break;
    case 10:
        Item_804A0C64.x24--;
        break;
    case 11:
        Item_804A0C64.x48--;
        break;
    case 12:
        Item_804A0C64.x50--;
        break;
    }

    if (gm_8018841C() && item_data->x18 == 1) {
        Item_804A0C64.x60--;
    }
}

static void Item_802676F4(HSD_GObj* gobj)
{
    Item* item_data = gobj->user_data;
    switch (item_data->hold_kind) {
    case 0:
        Item_804A0C64.x0++;

        if (item_data->kind == It_Kind_M_Ball) {
            Item_804A0C64.x1C++;
        }

        break;
    case 1:
        Item_804A0C64.x8++;
        break;
    case 2:
        Item_804A0C64.x10++;
        it_80274EE8();
        break;
    case 3:
        Item_804A0C64.x58++;
        break;
    case 4:
        Item_804A0C64.x3C++;
        break;
    case 5:
        Item_804A0C64.x40++;
        break;
    case 6:
        Item_804A0C64.x2C++;
        break;
    case 7:
        Item_804A0C64.x34++;
        break;
    case 8:
        Item_804A0C64.x18++;
        break;
    case 10:
        Item_804A0C64.x24++;
        break;
    case 11:
        Item_804A0C64.x48++;
        break;
    case 12:
        Item_804A0C64.x50++;
        break;
    }

    if (gm_8018841C() && item_data->x18 == 1) {
        Item_804A0C64.x60++;
    }
}

/// @remarks #Item_8026862C loads two integers
static bool Item_8026784C(enum_t dropItem, int _)
{
    bool result = false;

    switch (dropItem) {
    case 0:
        if (Item_804A0C64.x0 >= Item_804A0C64.x4) {
            result = true;
        }

        break;

    case 1:
        if (Item_804A0C64.x8 >= Item_804A0C64.xC) {
            result = true;
        }

        break;

    case 2:
        if (Item_804A0C64.x10 >= Item_804A0C64.x14) {
            result = true;
        }

        break;

    case 3:
        if (Item_804A0C64.x58 >= Item_804A0C64.x5C) {
            result = true;
        }

        break;

    case 5:
        if (Item_804A0C64.x40 >= Item_804A0C64.x44) {
            result = true;
        }

        break;

    case 7:
        if (Item_804A0C64.x34 >= Item_804A0C64.x38) {
            result = true;
        }

        break;

    case 9:
        if (Item_804A0C64.x1C > Item_804A0C64.x20) {
            Item_804A0C64.x1C--;
            result = true;
        }

        break;

    case 10:
        if (Item_804A0C64.x24 >= Item_804A0C64.x28) {
            result = true;
        }

        break;

    case 11:
        if (Item_804A0C64.x48 >= Item_804A0C64.x4C) {
            result = true;
        }

        break;

    case 12:
        if (Item_804A0C64.x50 >= Item_804A0C64.x54) {
            result = true;
        }
    }

    return result;
}

void Item_80267978(HSD_GObj* gobj)
{
    Item* item_data = gobj->user_data;
#ifdef MELEE_NATIVE
    if (item_data->kind == It_Kind_NativeHoldable) {
        // Common item data is present even when no particular stock fighter
        // (such as Peach) participates in the match.
        item_data->xC4_article_data = it_804D6D24[It_Kind_Capsule];
        item_data->xB8_itemLogicTable = &nativeHoldable_Logic;
        item_data->xBC_itemStateContainer = nativeHoldable_Logic.states;
        return;
    }
#endif
    if (item_data->kind < It_Kind_Kuriboh) {
        // Common items
        item_data->xC4_article_data = it_804D6D24[item_data->kind];
        item_data->xB8_itemLogicTable = &it_803F14C4[item_data->kind];
    } else if (item_data->kind < It_PKind_Start) {
        // Character items
        int idx = item_data->kind - It_Kind_Kuriboh;
        item_data->xC4_article_data = it_804D6D38[idx];
        item_data->xB8_itemLogicTable = &it_803F3100[idx];
    } else if (item_data->kind < It_Kind_Old_Kuri) {
        // Pokemon
        int idx = item_data->kind - It_PKind_Start;
        item_data->xC4_article_data = it_804D6D30[idx];
        item_data->xB8_itemLogicTable = &it_803F23CC[idx];
    } else {
        // Stage items
        int idx = item_data->kind - It_Kind_Old_Kuri;
        item_data->xC4_article_data = it_804A0F60[idx];
        item_data->xB8_itemLogicTable = &it_803F4D20[idx];
        if (item_data->xC4_article_data == NULL) {
            HSD_ASSERTREPORT(
                686, 0, "not found zako model data! check ground dat file!\n");
        }
    }
    item_data->xBC_itemStateContainer = item_data->xB8_itemLogicTable->states;
}

static bool Item_80267AA8(HSD_GObj* gobj, SpawnItem* spawnItem,
    NativeItemBehaviorSelection selection, FighterKind author,uint32_t source_owner)
{
    ItemAttr* item_attr;
    Item* item_data = (Item*) HSD_GObjGetUserData(gobj);
    item_data->kind = spawnItem->kind;
    item_data->hold_kind = spawnItem->hold_kind;
    item_data->x18 = spawnItem->x10;
    item_data->x1C = it_804D6D10++;
    item_data->entity = gobj;
    item_data->model_layers = NULL;
    // Selector/actor attachment can fail before model construction. The actual
    // userdata destructor must see valid empty ownership even on that path.
    item_data->xBBC_dynamicBoneTable = NULL;
    if (!source_owner) Item_80267978(gobj);
#ifdef MELEE_NATIVE
    memset(item_data->native_models, 0, sizeof(item_data->native_models));
    item_data->native_model_draw_disabled_mask = 0;
    item_data->native_article_author_kind = Ft_Kind_None;
    item_data->native_script_owner = 0;
    item_data->native_capture_inert = false;
    item_data->native_removal_in_progress = false;
    for (unsigned i = 0; i < 4; ++i) item_data->native_article_links[i] = UINT32_MAX;
    Fighter_GObj* original_creator=spawnItem->x0_parent_gobj;
    uint32_t parent_generation;
    if(source_owner && nativeCallbackSource_Snapshot(spawnItem->x4_parent_gobj2,NativeCallbackSource_Item,&parent_generation)) {
        Item_GObj* parent=spawnItem->x4_parent_gobj2;
        const NativeScriptItemProgram* parent_program=nativeScriptItem_Spec(parent);
        const NativeScriptItemProgram* child_program=nativeScriptItem_Program(author,source_owner);
        // Damage ownership may have changed through reflection. The original
        // creator must already be correct when Attach/Spawned executes.
        nativeItem_CaptureCreator(gobj,NULL);
        if(!parent_program || !child_program || parent_program->bank!=child_program->bank ||
           parent_program->author_kind!=author || !parent_program->owner ||
           nativeScriptItem_Status(parent)!=NativeScriptVM_Ok)return false;
        original_creator=nativeScriptItem_Creator(parent);
        if(((Item*)parent->user_data)->native_creator && !original_creator)return false;
        if(!nativeCallbackSource_Alive(parent,parent_generation,NativeCallbackSource_Item))return false;
    } else if(source_owner && nativeCallbackSource_Snapshot(spawnItem->x4_parent_gobj2,NativeCallbackSource_Fighter,&parent_generation)) {
        // A source factory may deliberately create without damage ownership.
        // The creating Fighter remains original provenance before Attach.
        original_creator=spawnItem->x4_parent_gobj2;
    }
    nativeItem_CaptureCreator(gobj, original_creator);
    // Failed construction may invoke Remove/Destroyed while its original
    // physical owner still exists. Do not publish another child into that
    // retiring construction; complete Fighter teardown keeps native policy.
    if (original_creator &&
        nativeMatchPool_Resolve(item_data->native_creator_reference) == original_creator &&
        ((Fighter*)original_creator->user_data)->native_constructing &&
        (original_creator->native_destroying || original_creator->native_userdata_removing))
        return false;
    if (!nativeItemBehavior_Select(gobj, spawnItem, selection, author,source_owner)) return false;
    if(source_owner && nativeCallbackSource_Snapshot(spawnItem->x4_parent_gobj2,NativeCallbackSource_Item,&parent_generation) &&
       !nativeScriptItem_SetAttachmentOwner(gobj,nativeScriptItem_AttachmentOwner(spawnItem->x4_parent_gobj2)))return false;
#endif
    item_data->msid = -1;
    item_data->xC8_joint =
        item_data->xC4_article_data->x10_modelDesc->x0_joint;
    item_data->xCC_item_attr = item_data->xC4_article_data->x0_common_attr;
    item_data->owner = NULL;
    item_data->xDC8_word.flags.x0 = 0;
    item_data->ecb_lock = -1;
    item_data->xDC4 = 0;
    item_data->xDC8_word.flags.xF = 0;
    item_data->xCC4 = 0;
    item_data->xDCF_flag.b6 = 0;
    item_data->xDCF_flag.b7 = 0;
    item_data->xCB0_source_ply = 6;
    item_data->xCB4 = -1;
    item_data->xC38 = -1;
    item_data->xCE0.z = 0.0f;
    item_data->xCE0.y = 0.0f;
    item_data->xCE0.x = 0.0f;
    item_data->xCD4.z = 0.0f;
    item_data->xCD4.y = 0.0f;
    item_data->xCD4.x = 0.0f;
    item_data->xDD0_flag.b1 = 0;
    item_data->xDC8_word.flags.x14 = 0;
    item_data->xDC8_word.flags.xE = 0;
    item_data->xCEC_fighterGObj = NULL;
    item_data->xCF0_itemGObj = NULL;
    item_data->xCF4_fighterGObjUnk = NULL;
    item_data->toucher = NULL;
    item_data->xC64_reflectGObj = NULL;
    item_data->xCFC = 0;
    item_data->atk_victim = 0;
    item_data->grab_victim = 0;
    item_data->xCCC_incDamageDirection = 0.0f;
    item_data->xCB8_outDamageDirection = 0.0f;
    item_data->xC68 = 0.0f;
    item_data->xCD0 = 0.0f;
    it_80275158(gobj, it_804D6D28->x30_lifetime);
    item_data->xDD0_flag.b3 = false;
    item_data->spin_spd = item_data->xCC_item_attr->xC_spin_speed;
    item_data->xDC8_word.flags.x19 = item_data->xCC_item_attr->x1_3;
    item_data->xDC8_word.flags.x17 = item_data->xCC_item_attr->x1_1;
    item_attr = item_data->xCC_item_attr;
    item_data->xBCC_unk = item_attr->x30_unk;
    item_data->xBD4_grabRange = item_attr->x38_grab_range;
    item_data->xDC8_word.flags.x1A = item_data->xCC_item_attr->x1_4;
    item_attr = item_data->xCC_item_attr;
    item_data->xBEC = item_attr->x20;
    item_attr = item_data->xCC_item_attr;
    item_data->xBDC = item_attr->x20;
    item_data->xDC8_word.flags.x1C = 1;
    item_data->xDC8_word.flags.x1D = 1;
    item_data->xDC8_word.flags.x1E = 1;
    item_attr = item_data->xCC_item_attr;
    item_data->xC1C = item_attr->x40;
    item_data->xC0C = item_data->xC1C;
    item_data->xBFC = item_data->xC0C;
    item_data->xDC8_word.flags.xC = item_data->xCC_item_attr->x1_5;
    item_data->xDC8_word.flags.xD = 1;
    item_data->xDCD_flag.b01 = item_data->xCC_item_attr->x1_67_cam_kind;
    item_data->xDCE_flag.b3 = 0;
    item_data->xD54_throwNum = 0;
    item_data->xD50_landNum = 0;
    item_data->xD58 = 0;
    item_data->xD5C = 2;
    item_data->xDCC_flag.b3 = 1;
    item_data->xDCC_flag.b4567 = 15;
    item_data->xAC4_ignoreItemID = spawnItem->x40;
    item_data->destroy_type = 4;
    item_data->xDCE_flag.b5 = 0;
    item_data->xDCE_flag.b4 = 0;
    item_data->xC54 = 0.0f;
    item_data->xC58.x = 0.0f;
    item_data->xC58.x = 0.0f;
    item_data->xC58.x = 0.0f;
    item_data->xDC8_word.flags.x9 = 0;
    item_data->xDC8_word.flags.x3 = 0;
    item_data->xDC8_word.flags.x4 = 0;
    item_data->xDC8_word.flags.x5 = 0;
    item_data->xDC8_word.flags.x6 = 0;
    item_data->xDC8_word.flags.x7 = 0;
    item_data->xDC8_word.flags.x8 = 0;
    item_data->xDD0_flag.b5 = 0;
    item_data->xD09 = (s8) 0;
    item_data->xDD0_flag.b4 = 0;
    item_data->xCA8 = 0;
    item_data->xCBC_hitlagFrames = 0.0f;
    item_data->xCC0 = 0.0f;
    item_data->xDC8_word.flags.xA = 0;
    item_data->xDC8_word.flags.xB = 0;
    item_data->xDCC_flag.b1 = 0;
    item_data->xC70 = 1.0f;
    item_data->xC6C = 1.0f;
    item_data->xDCC_flag.b2 = 0;
    item_data->xDD0_flag.b0 = false;
    item_data->xDCE_flag.b6 = false;
    item_data->xDCF_flag.b0 = false;
    item_data->xDCF_flag.b1 = false;
    item_data->xDCF_flag.b2 = false;
    item_data->xDD1_flag.b0 = false;
    item_data->xDCE_flag.b7 = 1;
    item_data->scl = item_data->xCC_item_attr->x60_scale;
    item_data->throw_scale = 1.0f;
    item_data->x3C = 0.0f;
    item_data->xDCD_flag.b3 = false;
    item_data->xDCD_flag.b4 = false;
    item_data->xDD0_flag.b7 = false;
    item_data->xBC0 = 0;
    item_data->xDD1_flag.b1 = false;
    item_data->xDCD_flag.b2 = false;
    item_data->xD6C = -1;
    item_data->sfx_unk2 = SFX_NONE;
    item_data->sfx_unk1 = SFX_NONE;
    item_data->xD70 = item_data->xCC_item_attr->x6C;
    item_data->xD74 = item_data->xCC_item_attr->x70;
    item_data->xD78 = item_data->xCC_item_attr->x74;
    item_data->destroy_sfx = item_data->xCC_item_attr->destroy_sfx;
    item_data->xD80 = item_data->xCC_item_attr->x7C;
    item_attr = item_data->xCC_item_attr;
    item_data->xD84 = item_attr->x80;

    it_80275474(gobj);
    it_80275504(gobj);
    it_80274EF8(gobj);

    if (ftLib_80086960(spawnItem->x0_parent_gobj)) {
        item_data->xDC8_word.flags.x1 = false;
    } else {
        item_data->xDC8_word.flags.x1 = true;
    }

    item_data->x5C8 = 0;
    item_data->xBC4.a = 0;
    item_data->xBC4.b = 0;
    item_data->xBC4.g = 0;
    item_data->xBC4.r = 0;
    item_data->x5C9 = 255;

    it_80279B64(item_data);

    item_data->xDCF_flag.b3 = false;
    item_data->xDCF_flag.b4 = false;
    item_data->xDCF_flag.b5 = false;

    // The original raw value 1 enables b7 in GameCube bitfield order.
    item_data->xDAA_byte = 0;
    item_data->xDAA_flag.b7 = true;

    if (db_ShowItemPickupRange()) {
        item_data->xDAA_flag.b4 = true;
    }

    if (spawnItem->x0_parent_gobj == NULL) {
        item_data->x20_team_id = -1;
    } else if (ftLib_80086960(spawnItem->x0_parent_gobj)) {
        item_data->x20_team_id = ftLib_80086EB4(spawnItem->x0_parent_gobj);
    } else if (it_80272D1C(spawnItem->x0_parent_gobj)) {
        item_data->x20_team_id = itGetTeamId(spawnItem->x0_parent_gobj);
    } else {
        item_data->x20_team_id = -1;
    }
    return true;
}

/// Setup Item JObj
void Item_802680CC(HSD_GObj* gobj)
{
    Item* item_data = (Item*) HSD_GObjGetUserData(gobj);

    item_data->model_layers = NULL;
    item_data->xDCF_flag.b3 = false;

    if (item_data->xC8_joint != NULL) {
        HSD_JObj* jobj = HSD_JObjLoadJoint(item_data->xC8_joint);
        HSD_GObjObject_80390A70(gobj, HSD_GObj_JObjKind, jobj);
    } else {
        HSD_JObj* jobj = HSD_JObjAlloc();
        C_MTXIdentity(jobj->mtx);
        jobj->scl = NULL;
        HSD_GObjObject_80390A70(gobj, HSD_GObj_JObjKind, jobj);
    }
}

static void Item_8026814C(HSD_GObj* gobj)
{
    HSD_JObj* jobj = GET_JOBJ(gobj);
    PAD_STACK(16);
    while (jobj != NULL) {
        HSD_DObj* dobj = HSD_JObjGetDObj(jobj);
        while (1) {
            HSD_MObj* mobj;
            if (dobj == NULL) {
                break;
            }
            mobj = dobj->mobj;
            if (mobj != NULL) {
                hsdChangeClass(mobj, &it_mobj);
            }
            dobj = dobj != NULL ? dobj->next : NULL;
        }
        if (HSD_JObjGetChild(jobj) != NULL) {
            jobj = HSD_JObjGetChild(jobj);
        } else if (HSD_JObjGetNext(jobj) != NULL) {
            jobj = HSD_JObjGetNext(jobj);
        } else {
            while (1) {
                if (HSD_JObjGetParent(jobj) == NULL) {
                    jobj = NULL;
                    break;
                }
                if (HSD_JObjGetNext(HSD_JObjGetParent(jobj)) != NULL) {
                    jobj = HSD_JObjGetNext(HSD_JObjGetParent(jobj));
                    break;
                }
                jobj = HSD_JObjGetParent(jobj);
            }
        }
    }
}

static bool Item_802682F0(HSD_GObj* gobj)
{
    Item* ip = GET_ITEM(gobj);
    if (ip->xC4_article_data->x10_modelDesc->x4_bone_count != 0) {
        HSD_JObj* jobj;
        int i;
#ifdef MELEE_NATIVE
        NativeItemPoolSlot* slot=nativeItemPool_Slot(ip);
        if(slot==NULL)return false;
        ip->xBBC_dynamicBoneTable=&slot->bones;
#else
        ip->xBBC_dynamicBoneTable =
            HSD_ObjAlloc(&item_dynamic_bones_alloc_data);
#endif
        if (ip->xBBC_dynamicBoneTable == NULL) {
            return false;
        }
        memset(ip->xBBC_dynamicBoneTable->bones, 0,
               sizeof(ip->xBBC_dynamicBoneTable->bones));
        jobj = GET_JOBJ(gobj);
        i = 0;
        while (jobj != NULL) {
#ifdef MELEE_NATIVE
            if(i>=sizeof(ip->xBBC_dynamicBoneTable->bones)/sizeof(HSD_JObj*))return false;
#endif
            ip->xBBC_dynamicBoneTable->bones[i] = jobj;
            i++;
            if (HSD_JObjGetChild(jobj) != NULL) {
                jobj = HSD_JObjGetChild(jobj);
            } else if (HSD_JObjGetNext(jobj) != NULL) {
                jobj = HSD_JObjGetNext(jobj);
            } else {
                while (1) {
                    if (HSD_JObjGetParent(jobj) == NULL) {
                        jobj = NULL;
                        break;
                    }
                    if (HSD_JObjGetNext(HSD_JObjGetParent(jobj)) != NULL) {
                        jobj = HSD_JObjGetNext(HSD_JObjGetParent(jobj));
                        break;
                    }
                    jobj = HSD_JObjGetParent(jobj);
                }
            }
        }
    } else {
        ip->xBBC_dynamicBoneTable = NULL;
    }
    return true;
}

/// Set item model scale
void Item_8026849C(HSD_GObj* gobj)
{
    HSD_JObj* temp_jobj = (HSD_JObj*) HSD_GObjGetHSDObj(gobj);
    Item* item_data = (Item*) HSD_GObjGetUserData(gobj);
    f32 tmp = item_data->scl;
    Vec3 sp14;
    sp14.x = sp14.y = sp14.z = tmp;
    HSD_JObjSetScale(temp_jobj, &sp14);
}

static void Item_80268560(HSD_GObj* gobj)
{
    int i;
    Item* ip = GET_ITEM(gobj);
    Article* article_data = ip->xC4_article_data;
    if (article_data->x14_dynamics == NULL) {
        ip->x374_dynamicBonesNum = 0;
        ip->xB68 = 0;
        return;
    }
    ip->x374_dynamicBonesNum =
        article_data->x14_dynamics->flexible_bone_count;
    for (i = 0; i < article_data->x14_dynamics->flexible_bone_count; i++) {
        BoneDynamicsDesc* desc =
            &article_data->x14_dynamics->flexible_bones[i];
        HSD_JObj* jobj = ip->xBBC_dynamicBoneTable->bones[desc->bone_id];
        lb_8000FD48(jobj, &ip->xD4_dynamicBones[i].dyn_desc,
                    desc->dyn_desc.count);
        ip->xD4_dynamicBones[i].skeleton = jobj;
        ip->xD4_dynamicBones[i].flags = 0;
        lb_80011710(&article_data->x14_dynamics->flexible_bones[i].dyn_desc,
                    &ip->xD4_dynamicBones[i].dyn_desc);
    }
}

static void foobar(HSD_GObj* gobj)
{
    Item* it = (Item*) HSD_GObjGetUserData(gobj);
    switch (it->hold_kind) {
    case 0:
    case 6:
        if (it->xD0C == 2) {
            it->xDD0_flag.b7 = 1;
        }
        it->xD40 = it_804D6D28->x2C;
        it->xDD0_flag.b6 = 1;
        it_802756D0(gobj);
        it_80279B88(it, 3, 0);
        break;

    default:
        it->xD40 = 0.0f;
        it->xDD0_flag.b6 = 0;
    }
}

static void foobar2(HSD_GObj* gobj)
{
    Item* it = (Item*) HSD_GObjGetUserData(gobj);
    // Check if item is a character item with an owner
    if (it->kind >= It_Kind_Mario_Fire && it->kind < It_Kind_Unk4 &&
        ftLib_80086960(it->owner))
    {
        it->xDC8_word.flags.xE = 1;
        it->ecb_lock = ftLib_GetKind(it->owner);
        ftLib_8008702C(it->ecb_lock);
    }
}

static void foobar3(HSD_GObj* gobj)
{
    Item* it = (Item*) HSD_GObjGetUserData(gobj);
    CmSubject* cam_box;
    if (it->xDCD_flag.b01 != 0) {
        if (it->xDCD_flag.b01 == 1) {
            it->x520_cameraBox = Camera_80029044(0);
        } else {
            it->x520_cameraBox = Camera_80029044(2);
        }
        cam_box = it->x520_cameraBox;
        if (cam_box != NULL) {
            cam_box->target_ext.h.x = it_804D6D28->x14C;
            cam_box->target_ext.h.y = it_804D6D28->x150;
            cam_box->target_ext.v.x = it_804D6D28->x154;
            cam_box->target_ext.v.y = it_804D6D28->x158;
        }
    }
}

static HSD_GObj* Item_8026862C(SpawnItem* spawnItem,
    NativeItemBehaviorSelection selection, FighterKind author,uint32_t source_owner)
{
    HSD_GObj* gobj;
    void* user_data;
#ifdef MELEE_NATIVE
    const NativeScriptItemProgram* source_program = NULL;
    if (source_owner) {
        source_program = nativeScriptItem_Program(author, source_owner);
        if (selection != NativeItemBehavior_Author || !source_program ||
            source_program->owner != source_owner || source_program->author_kind != author ||
            source_program->kind != spawnItem->kind || !source_program->article ||
            !source_program->article->x10_modelDesc ||
            (source_program->recipe ? source_program->drawing != NativeScriptItemDrawing_Billboard :
                                     source_program->drawing != NativeScriptItemDrawing_Standard))
            return NULL;
    }
#endif

    if (Item_8026784C(spawnItem->hold_kind, spawnItem->kind) != 0) {
        return NULL;
    }
    gobj = GObj_Create(HSD_GOBJ_CLASS_ITEM, 9, 0);
    if (gobj == NULL) {
        return NULL;
    }
#ifdef MELEE_NATIVE
    if (source_program) {
        // Source drawing is selected from the pinned owned program, never the
        // physical kind's authored draw callback. Keep native GX registration
        // before userdata/model construction and the Spawned callback.
        GObj_SetupGXLink(gobj, source_program->drawing == NativeScriptItemDrawing_Billboard ?
                                  nativeHoldable_Draw : it_8026EECC, 6, 0);
    } else if (spawnItem->kind == It_Kind_NativeHoldable) {
        GObj_SetupGXLink(gobj, nativeHoldable_Draw, 6, 0);
    } else
#endif
    if (spawnItem->kind < It_Kind_Kuriboh) {
        // Common items
        GObj_SetupGXLink(gobj, it_803F1418[spawnItem->kind].x0_renderFunc, 6,
                         0);
    } else if (spawnItem->kind < It_PKind_Start) {
        // Character items
        int idx = spawnItem->kind - It_Kind_Kuriboh;
        GObj_SetupGXLink(gobj, it_803F2F28[idx].x0_renderFunc, 6, 0);
    } else if (spawnItem->kind < It_Kind_Old_Kuri) {
        // Pokemon
        int idx = spawnItem->kind - It_PKind_Start;
        GObj_SetupGXLink(gobj, it_803F2310[idx].x0_renderFunc, 6, 0);
    } else {
        // Stage items
        int idx = spawnItem->kind - It_Kind_Old_Kuri;
        GObj_SetupGXLink(gobj, it_803F4CA8[idx].x0_renderFunc, 6, 0);
    }
#ifdef MELEE_NATIVE
    NativeMatchPoolReference item_reference;
    user_data = nativeMatchPool_Allocate(&nativeItemPool,&item_reference);
#else
    user_data = HSD_ObjAlloc(&item_alloc_data);
#endif
    if (user_data == NULL) {
        HSD_GObjFree(gobj);
        return NULL;
    }
    GObj_InitUserData(gobj, 6, Item_OnUserDataRemove, user_data);
    if (!Item_80267AA8(gobj, spawnItem, selection, author,source_owner)) {
        HSD_GObjFree(gobj);
        return NULL;
    }
#ifdef MELEE_NATIVE
    if (source_program && nativeScriptItem_Spec(gobj) != source_program) {
        HSD_GObjFree(gobj);
        return NULL;
    }
#endif
    const uint32_t callback_generation = (uint32_t)((Item*)gobj->user_data)->x1C;
    Item_802680CC(gobj);
    if (Item_802682F0(gobj) != false) {
        Item_8026814C(gobj);
        Item_8026849C(gobj);
        it_8027163C(gobj);
        Item_80268560(gobj);
        HSD_GObj_SetupProc(gobj, Item_802693E4, 0);
        HSD_GObj_SetupProc(gobj, Item_80269528, 1);
        HSD_GObj_SetupProc(gobj, Item_802697D4, 4);
        HSD_GObj_SetupProc(gobj, Item_80269978, 5);
        HSD_GObj_SetupProc(gobj, Item_80269A9C, 9);
        HSD_GObj_SetupProc(gobj, Item_80269B60, 11);
        HSD_GObj_SetupProc(gobj, Item_80269BE4, 12);
        HSD_GObj_SetupProc(gobj, Item_80269C5C, 13);
        HSD_GObj_SetupProc(gobj, Item_8026A294, 14);
        HSD_GObj_SetupProc(gobj, Item_8026A788, 16);
        Item_80267130(gobj, spawnItem);
        if (!Item_8026A810(gobj)) {
            if (nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Item)) Item_8026A8EC(gobj);
            return NULL;
        }
        if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Item)) return NULL;
        if (selection == NativeItemBehavior_Engine &&
            ((Item*)gobj->user_data)->native_creator != NULL &&
            nativeItem_Creator(gobj) == NULL) {
            // Instance callbacks can remove or replace the creating fighter.
            // Do not continue through its cached owner in the ordinary tail.
            Item_8026A8EC(gobj);
            return NULL;
        }
        foobar(gobj);
        Item_802676F4(gobj);
        foobar2(gobj);
        foobar3(gobj);
    } else {
        HSD_GObjFree(gobj);
        return NULL;
    }
    return gobj;
}

/// Item spawn prefunction - spawn airborne
Item_GObj* Item_80268B18(SpawnItem* spawnItem)
{
    spawnItem->x48_ground_or_air = GA_Air;
    spawnItem->x10 = 0;
    Item_802674AC(spawnItem);
    return Item_8026862C(spawnItem, NativeItemBehavior_Parent, Ft_Kind_None,0);
}

// Create an ordinary common item, independent of the parent's article declarations.
Item_GObj* Item_CreateEngine(SpawnItem* spawnItem)
{
    if (spawnItem == NULL || !nativeItemBehavior_EngineAssetsAvailable(spawnItem->kind))
        return NULL;
    spawnItem->x48_ground_or_air = GA_Air;
    spawnItem->x10 = 0;
    Item_802674AC(spawnItem);
    return Item_8026862C(spawnItem, NativeItemBehavior_Engine, Ft_Kind_None,0);
}

Item_GObj* Item_CreateAuthored(SpawnItem* spawnItem, FighterKind author,uint32_t source_owner)
{
    if (spawnItem == NULL) return NULL;
    if(source_owner) {
        const NativeScriptItemProgram* program=nativeScriptItem_Program(author,source_owner);
        if(!program||program->kind!=spawnItem->kind||!program->article)return NULL;
    } else {
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(author);
    if (definition == NULL || nativeFighterRuntime_ArticleAssets(author, spawnItem->kind) == NULL)
        return NULL;
    bool has_logic = false;
    for (unsigned i = 0; i < definition->article_logic_count; ++i)
        if (definition->article_logics[i].kind == spawnItem->kind) {
            has_logic = true;
            break;
        }
    if (!has_logic) return NULL;
    }
    spawnItem->x48_ground_or_air = GA_Air;
    spawnItem->x10 = 0;
    Item_802674AC(spawnItem);
    return Item_8026862C(spawnItem, NativeItemBehavior_Author, author,source_owner);
}

/// Item spawn prefunction - spawn grounded
HSD_GObj* Item_80268B5C(SpawnItem* spawnItem)
{
    spawnItem->x48_ground_or_air = GA_Ground;
    spawnItem->x10 = 0;
    Item_802674AC(spawnItem);
    return Item_8026862C(spawnItem, NativeItemBehavior_Parent, Ft_Kind_None,0);
}

/// Item spawn prefunction - spawn grounded and toggle unknown true
void Item_80268B9C(SpawnItem* spawnItem)
{
    spawnItem->x48_ground_or_air = GA_Ground;
    spawnItem->x10 = 1;
    Item_802674AC(spawnItem);
    Item_8026862C(spawnItem, NativeItemBehavior_Parent, Ft_Kind_None,0);
}

static void Item_80268BE0(HSD_JObj* item_jobj, HSD_AnimJoint* anim_joint,
                          HSD_MatAnimJoint* matanim_joint,
                          HSD_ShapeAnimJoint* shapeanim_joint, Item* item_data)
{
    void* functionArg1;
    void* functionArg2;
    void* functionArg3;

    s32 temp_r0;

    if (item_jobj != NULL) {
        if (anim_joint != NULL) {
            for (temp_r0 = 0; temp_r0 < item_data->x374_dynamicBonesNum;
                 temp_r0++)
            {
                if (item_jobj == item_data->xD4_dynamicBones[temp_r0].skeleton)
                {
                    if (anim_joint->next != NULL) {
                        functionArg1 = NULL;
                        functionArg2 = NULL;
                        functionArg3 = NULL;
                        if (anim_joint != NULL) {
                            functionArg1 = anim_joint->next;
                        }
                        if (matanim_joint != NULL) {
                            functionArg2 = matanim_joint->next;
                        }
                        if (shapeanim_joint != NULL) {
                            functionArg3 = shapeanim_joint->next;
                        }
                        Item_80268BE0(item_jobj->next, functionArg1,
                                      functionArg2, functionArg3, item_data);
                    }
                    return;
                }
            }
        }
        HSD_JObjAddAnim(item_jobj, anim_joint, matanim_joint, shapeanim_joint);
        if (item_jobj->child != NULL) {
            functionArg1 = NULL;
            functionArg2 = NULL;
            functionArg3 = NULL;
            if (anim_joint != NULL) {
                functionArg1 = anim_joint->child;
            }
            if (matanim_joint != NULL) {
                functionArg2 = matanim_joint->child;
            }
            if (shapeanim_joint != NULL) {
                functionArg3 = shapeanim_joint->child;
            }
            Item_80268BE0(item_jobj->child, functionArg1, functionArg2,
                          functionArg3, item_data);
        }
        if (item_jobj->next != NULL) {
            functionArg1 = NULL;
            functionArg2 = NULL;
            functionArg3 = NULL;
            if (anim_joint != NULL) {
                functionArg1 = anim_joint->next;
            }
            if (matanim_joint != NULL) {
                functionArg2 = matanim_joint->next;
            }
            if (shapeanim_joint != NULL) {
                functionArg3 = shapeanim_joint->next;
            }
            Item_80268BE0(item_jobj->next, functionArg1, functionArg2,
                          functionArg3, item_data);
        }
    }
}

/// Unk Item AObj-related function
void Item_80268D34(HSD_GObj* gobj, struct ItemStateDesc* itemStateDesc)
{
    HSD_JObj* item_jobj = (HSD_JObj*) HSD_GObjGetHSDObj(gobj);
    Item* item_data = (Item*) HSD_GObjGetUserData(gobj);
    HSD_JObjRemoveAnimAll(item_jobj);
    if (item_data->xC8_joint != NULL) {
        UNK_T bonestruct_arg;
        if (item_jobj == NULL) {
            bonestruct_arg = NULL;
        } else {
            bonestruct_arg = item_jobj->child;
        }
        lb_8000B804(bonestruct_arg, item_data->xC8_joint->child);
    }
    Item_80268BE0(item_jobj, itemStateDesc->x0_anim_joint,
                  itemStateDesc->x4_matanim_joint,
                  itemStateDesc->x8_parameters, item_data);
    lb_8000BA0C(item_jobj, item_data->x5D0_animFrameSpeed);
    HSD_JObjReqAnimAll(item_jobj, 0.0f);
}

/// Advance item animation?
void Item_80268DD4(HSD_GObj* gobj, f32 frame)
{
    HSD_JObj* item_jobj;
    Item* item_data = (Item*) HSD_GObjGetUserData(gobj);
    item_jobj = (HSD_JObj*) HSD_GObjGetHSDObj(gobj);
    lb_8000BA0C(item_jobj, item_data->x5D0_animFrameSpeed);
    HSD_JObjReqAnimAll(item_jobj, frame);
    HSD_JObjAnimAll(item_jobj);
    item_data->x5CC_currentAnimFrame = lbGetJObjCurrFrame(item_jobj);
}

/// Copy item script
void Item_80268E40(Item* item_data, struct ItemStateDesc* itemStateDesc)
{
    item_data->x524_cmd.u = itemStateDesc->xC_script;
    item_data->x524_cmd.loop_count = 0;
    item_data->x524_cmd.timer = 0.0f;
}

#ifdef MELEE_NATIVE
// A reentrant native stage may replace this selection while the physical Item
// remains alive. Only the newest state setup may publish its callback reset.
static bool itemStateSetupCurrent(HSD_GObj* object,uint32_t generation,uint64_t revision)
{
    return itemCallbackCurrent(object,generation)&&
        (!revision||(nativeScriptItem_Status(object)==NativeScriptVM_Ok&&
            nativeScriptItem_Revision(object)==revision));
}
#endif

/// Change item state
void Item_80268E5C(HSD_GObj* gobj, enum_t msid, Item_StateChangeFlags flags)
{
    Vec3 sp4C;
    Vec3 scl;
    ItemStateTable* temp_r30;
    ItemStateDesc* temp_r29;

    u8 _[12];

    HSD_JObj* item_jobj1_2;

    UNK_T new_var;
    UNK_T new_var2;
    UNK_T new_var3;
    Item* item_data3;
    HSD_JObj* item_jobj;
    Item* item_data;
    ItemAttr* item_attr;
    ItemStateDesc* temp_r23;
    s32 temp_r0;

    item_jobj = (HSD_JObj*) HSD_GObjGetHSDObj(gobj);
    item_data = (Item*) HSD_GObjGetUserData(gobj);
#ifdef MELEE_NATIVE
    const uint32_t setup_generation=(uint32_t)item_data->x1C;
    const bool source_state=nativeScriptItem_IsAttached(gobj);
    int32_t source_animation=-1;
    if(source_state&&((!item_data->native_capture_inert&&!nativeScriptItem_StateAnimation(gobj,(uint32_t)msid,&source_animation))||
       !nativeScriptItem_SelectState(gobj,(uint32_t)msid)))return;
    const uint64_t setup_revision=source_state?nativeScriptItem_Revision(gobj):0;
#endif

    item_data->msid = msid;
    item_data->xDC8_word.flags.x14 = 0;
    HSD_JObjSetTranslate(gobj->hsd_obj, &item_data->pos);
    efAsync_QueueFlush(gobj, &item_data->xBC0);
#ifdef MELEE_NATIVE
    if(!itemStateSetupCurrent(gobj,setup_generation,setup_revision))return;
#endif

    if (item_data->xDC8_word.flags.x2 == 1) {
        it_8026BDCC(gobj);
#ifdef MELEE_NATIVE
        if(!itemStateSetupCurrent(gobj,setup_generation,setup_revision))return;
#endif
        it_80274EF8(gobj);
#ifdef MELEE_NATIVE
        if(!itemStateSetupCurrent(gobj,setup_generation,setup_revision))return;
#endif
    }

    if ((flags & ITEM_MODEL_UPDATE)) {
        it_80274740(gobj);
#ifdef MELEE_NATIVE
        if(!itemStateSetupCurrent(gobj,setup_generation,setup_revision))return;
#endif
    }

    if (!(flags & ITEM_COLANIM_PRESERVE) &&
        it_804D6D04[item_data->x548_colorOverlay.x28_colanim.i].unk5 != 0)
    {
        it_80279BBC(item_data);
#ifdef MELEE_NATIVE
        if(!itemStateSetupCurrent(gobj,setup_generation,setup_revision))return;
#endif
    }

    if (!(flags & ITEM_SFX_PRESERVE)) {
        Item_8026B074(item_data);
#ifdef MELEE_NATIVE
        if(!itemStateSetupCurrent(gobj,setup_generation,setup_revision))return;
#endif
    }

    HSD_JObjSetFacingDirItem(item_jobj, item_data);

    if ((flags & ITEM_DROP_UPDATE)) {
        item_data->xC40 = item_data->xC44;
    } else {
        item_data->xC40 = 1.0F;
    }

    if (!(flags & ITEM_HIT_PRESERVE) && item_data->xDC8_word.flags.x16) {
        it_802725D4(gobj);
#ifdef MELEE_NATIVE
        if(!itemStateSetupCurrent(gobj,setup_generation,setup_revision))return;
#endif
    }

    item_data->xCC8_knockback = 0.0F;
#ifdef MELEE_NATIVE
    if(source_state) {
        temp_r30=NULL;
        item_data->anim_id=source_animation;
    } else
#endif
    {
        temp_r30 = (new_var = &item_data->xBC_itemStateContainer[msid]);
        item_data->anim_id = temp_r30->anim_id;
    }
    item_data->x5CC_currentAnimFrame = 0.0F;
    temp_r0 = item_data->anim_id;

    if (temp_r0 != -1) {
        HSD_ASSERTREPORT(
            0, item_data->xC4_article_data->xC_itemStates != NULL &&
                   temp_r0 >= 0 &&
                   (u32) temp_r0 <
                       item_data->xC4_article_data->xC_itemStates->count,
            "item state out of range: kind=%d msid=%d anim=%d count=%lu\n",
            item_data->kind, item_data->msid, temp_r0,
            item_data->xC4_article_data->xC_itemStates != NULL
                ? item_data->xC4_article_data->xC_itemStates->count
                : 0);
        item_data->xD0_itemStateDesc =
            (temp_r23 =
                 (temp_r29 =
                      (new_var2 = &item_data->xC4_article_data->xC_itemStates
                                       ->x0_itemStateDesc[temp_r0])));

        temp_r23 = (new_var3 = item_data->xD0_itemStateDesc);

        HSD_ASSERTREPORT(
            0,
            (temp_r23->x0_anim_joint == NULL ||
             (uintptr_t) temp_r23->x0_anim_joint >= 4096) &&
                (temp_r23->x4_matanim_joint == NULL ||
                 (uintptr_t) temp_r23->x4_matanim_joint >= 4096) &&
                (temp_r23->x8_parameters == NULL ||
                 (uintptr_t) temp_r23->x8_parameters >= 4096),
            "invalid item state graph: kind=%d msid=%d anim=%d "
            "joint=%p mat=%p shape=%p\n",
            item_data->kind, item_data->msid, temp_r0,
            (void*) temp_r23->x0_anim_joint,
            (void*) temp_r23->x4_matanim_joint,
            (void*) temp_r23->x8_parameters);

        if (temp_r23 != NULL && (flags & ITEM_ANIM_UPDATE)) {
            Item_80268D34(gobj, temp_r23);
#ifdef MELEE_NATIVE
            if(!itemStateSetupCurrent(gobj,setup_generation,setup_revision))return;
#endif

            if ((flags & ITEM_UNK_UPDATE)) {
                item_jobj1_2 = gobj->hsd_obj;
                item_data3 = GetItemData(gobj);
                item_data3->scl = item_data3->xCC_item_attr->x60_scale;
                HSD_JObjSetScaleItem(item_data3, item_jobj1_2, &sp4C);
                item_attr = item_data3->xCC_item_attr;
                item_data3->xBCC_unk = item_attr->x30_unk;
                item_data3->xBD4_grabRange = item_attr->x38_grab_range;
                item_data3->xBEC = item_data3->xCC_item_attr->x20;
                item_data3->xBDC = item_data3->xCC_item_attr->x20;
            } else {
                HSD_JObjSetScaleItem(gobj->user_data, gobj->hsd_obj, &scl);
            }

            item_data->x524_cmd.u = temp_r29->xC_script;
            item_data->x524_cmd.loop_count = 0;
            item_data->x524_cmd.timer = 0.0F;
        } else if (temp_r23 != NULL && (flags & ITEM_CMD_UPDATE)) {
            item_data->x524_cmd.u = temp_r29->xC_script;
            item_data->x524_cmd.loop_count = 0;
            item_data->x524_cmd.timer = 0.0f;
        }

        HSD_JObjAnimAll(item_jobj);
#ifdef MELEE_NATIVE
        if(!itemStateSetupCurrent(gobj,setup_generation,setup_revision))return;
#endif
        it_80279BE0(gobj);
#ifdef MELEE_NATIVE
        if(!itemStateSetupCurrent(gobj,setup_generation,setup_revision))return;
#endif
        it_802799E4(gobj);
#ifdef MELEE_NATIVE
        if(!itemStateSetupCurrent(gobj,setup_generation,setup_revision))return;
#endif
    } else {
        HSD_JObjRemoveAnimAll(item_jobj);
        item_data->x524_cmd.u = NULL;
    }

#ifdef MELEE_NATIVE
    if(!itemStateSetupCurrent(gobj,setup_generation,setup_revision))return;
    if(source_state) {
        item_data->animated=NULL;
        item_data->physics_updated=NULL;
        item_data->collided=NULL;
        nativeScriptItem_ResetStateCallbacks(gobj);
    } else
#endif
    {
        item_data->animated = temp_r30->animated;
        item_data->physics_updated = temp_r30->physics_updated;
        item_data->collided = temp_r30->collided;
    }
    item_data->on_accessory = NULL;
    item_data->touched = NULL;
    item_data->entered_hitlag = NULL;
    item_data->exited_hitlag = NULL;
    item_data->jumped_on = NULL;

    item_data->xDD0_flag.b5 = false;
    item_data->xD09 = 0;

    it_802714C0(gobj);
}

void Item_802693E4(HSD_GObj* gobj)
{
    Item* item_data = gobj->user_data;
    const uint32_t callback_generation = (uint32_t)item_data->x1C;
#ifdef MELEE_NATIVE
    nativeScriptItem_BeginTick(gobj);
#endif
    if (item_data->xDC8_word.flags.x5 != 0) {
        Item_8026A0A0(gobj);
        if(!itemCallbackCurrent(gobj,callback_generation))return;
        item_data->xDC8_word.flags.x5 = 0;
    }
    if (item_data->xDC8_word.flags.x6 != 0) {
        Item_8026A0FC(gobj);
        if(!itemCallbackCurrent(gobj,callback_generation))return;
        item_data->xDC8_word.flags.x6 = 0;
    }
    if (item_data->xCBC_hitlagFrames > 0.0f) {
        item_data->xCBC_hitlagFrames -= 1.0f;
        if (item_data->xCBC_hitlagFrames <= 0.0f) {
            item_data->xCBC_hitlagFrames = 0.0f;
            item_data->xDC8_word.flags.xB = 0;
            item_data->xDC8_word.flags.xA = 0;
            if ((item_data->xDC8_word.flags.x8 == 0) &&
                (item_data->xDC8_word.flags.x7 == 0))
            {
                Item_8026A1E8(gobj);
            }
        }
    }
}

/// Advance item animation + script?
void Item_802694CC(HSD_GObj* gobj)
{
    HSD_JObj* item_jobj = (HSD_JObj*) HSD_GObjGetHSDObj(gobj);
    Item* item_data = (Item*) HSD_GObjGetUserData(gobj);
    HSD_JObjAnimAll(item_jobj);
    item_data->x5CC_currentAnimFrame = lbGetJObjCurrFrame(item_jobj);
    it_802799E4(gobj);
}

static void Item_80269528(HSD_GObj* gobj)
{
    u8 _[8];
    Item* item_data = (Item*) HSD_GObjGetUserData(gobj);
    const uint32_t callback_generation = (uint32_t)item_data->x1C;
    if (item_data->xDC8_word.flags.x9 == 0) {
        Item_802694CC(gobj);
        if(!itemCallbackCurrent(gobj,callback_generation))return;
        bool remove=false;
#ifdef MELEE_NATIVE
        if (item_data->native_capture_inert) {
            // Captured items retain their graph updates, but no normal phases.
        } else if (nativeScriptItem_IsAttached(gobj)) {
            NativeScriptValue result;
            NativeScriptVMStatus status=item_data->kind==It_Kind_NativeHoldable
                ? nativeHoldable_Animation(gobj,&result)
                : nativeScriptItem_RunPhase(gobj,NativeScriptItemPhase_Animation,&result);
            if (status!=NativeScriptVM_Ok) return;
            remove=result.as.u64!=0;
        } else
#endif
        if (item_data->animated != NULL) {
            remove = item_data->animated(gobj);
            if(!itemCallbackCurrent(gobj,callback_generation))return;
        }
        if (remove) {
            item_data->destroy_type = 0;
            Item_8026A8EC(gobj);
            return;
        }
    }
    if (item_data->xDD0_flag.b6 != 0) {
        item_data->xD40 -= 1.0f;
        if (item_data->xD40 <= 0.0f) {
            item_data->xDD0_flag.b6 = 0;
            item_data->xD40 = 0.0f;
            it_80279BBC(item_data);
            if (item_data->xDD0_flag.b7 == 0) {
                it_802756E0(gobj);
            }
        }
    }
    if (item_data->xDC8_word.flags.x15 != 0 &&
        item_data->kind < It_Kind_L_Gun_Ray && !item_data->xDD0_flag.b3)
    {
        item_data->xD44_lifeTimer -= 1.0f;
        if (item_data->xD44_lifeTimer <= 0.0f) {
            item_data->destroy_type = 0;
            Item_8026A8EC(gobj);
            return;
        }
        if (item_data->xD44_lifeTimer <= it_804D6D28->x34) {
            it_802728C8(gobj);
        }
    }
    it_802721B8(gobj);
    it_80272298(gobj);
    it_80279BE0(gobj);
}

static bool Item_802696CC(HSD_GObj* gobj)
{
    Item* item_data = (Item*) HSD_GObjGetUserData(gobj);
    if ((item_data->xDCC_flag.b4567 & 8) &&
        item_data->pos.x > Stage_GetBlastZoneRightOffset())
    {
        item_data->destroy_type = 3;
        Item_8026A8EC(gobj);
        return true;
    }
    if ((item_data->xDCC_flag.b4567 & 4) &&
        item_data->pos.x < Stage_GetBlastZoneLeftOffset())
    {
        item_data->destroy_type = 3;
        Item_8026A8EC(gobj);
        return true;
    }
    if ((item_data->xDCC_flag.b4567 & 1) && item_data->pos.y > 10000.0f) {
        item_data->destroy_type = 3;
        Item_8026A8EC(gobj);
        return true;
    }
    if ((item_data->xDCC_flag.b4567 & 2) &&
        item_data->pos.y < Stage_GetBlastZoneBottomOffset())
    {
        item_data->destroy_type = 3;
        Item_8026A8EC(gobj);
        return true;
    }
    return false;
}

void Item_802697D4(HSD_GObj* gobj)
{
    Vec3 sp1C;
    u8 _[12];

#ifdef MELEE_NATIVE
    NativeItemLifetime source;
    if(!nativeItemLifetime_Capture(gobj,&source))return;
    Item* item_data=source.data;
#else
    Item* item_data = (Item*) HSD_GObjGetUserData(gobj);
    const uint32_t callback_generation = (uint32_t)item_data->x1C;
#endif
    if (item_data->xDC8_word.flags.x9 == 0)
    {
#ifdef MELEE_NATIVE
        if (item_data->native_capture_inert) {
        } else if (nativeScriptItem_IsAttached(gobj)) {
            NativeScriptValue result;
            NativeScriptVMStatus status=item_data->kind==It_Kind_NativeHoldable
                ? nativeHoldable_Physics(gobj,&result)
                : nativeScriptItem_RunPhase(gobj,NativeScriptItemPhase_Physics,&result);
            if (status!=NativeScriptVM_Ok) return;
        } else
#endif
        if (item_data->physics_updated != NULL) item_data->physics_updated(gobj);
#ifdef MELEE_NATIVE
        if(!nativeItemLifetime_Current(&source))return;
#else
        if(!itemCallbackCurrent(gobj,callback_generation))return;
#endif
    }
    if (item_data->xDC8_word.flags.x13 == 0) {
        if (item_data->xDC8_word.flags.x9 == 0) {
            C_VECAdd(&item_data->x40_vel, &item_data->x70_nudge, &sp1C);
            if ((item_data->ground_or_air == 0) &&
                (item_data->xDCD_flag.b3 == 0))
            {
                it_8027737C(gobj, &sp1C);
            }
            C_VECAdd(&item_data->pos, &sp1C, &item_data->pos);
        }
        if (item_data->xDCC_flag.b3 == 1 &&
            (item_data->xDCC_flag.b4567 & 0xF) && Item_802696CC(gobj))
        {
            return;
        }
        if (item_data->xD3C_spinSpeed && item_data->ground_or_air == 1) {
            it_80274A64(gobj);
        }
    }
    if ((item_data->xDD1_flag.b1 != 0) && (item_data->ground_or_air == 1)) {
        grLib_801C9E60(&item_data->x58_vec_unk);
    }
    C_VECAdd(&item_data->pos, &item_data->x58_vec_unk, &item_data->pos);
    it_80273484(gobj);
    if (item_data->ground_or_air == 0) {
        if ((item_data->xDCE_flag.b3 != 0) && (it_8026D604(gobj) != 0)) {
            mpGetSpeed(item_data->xC30, &item_data->pos,
                       &item_data->x64_vec_unk2);
        } else {
            mpGetSpeed(item_data->x378_itemColl.floor.index, &item_data->pos,
                       &item_data->x64_vec_unk2);
        }
    }
    C_VECAdd(&item_data->pos, &item_data->x64_vec_unk2, &item_data->pos);
    it_8027346C(gobj);
    it_802714C0(gobj);
}

void Item_80269978(HSD_GObj* gobj)
{
#ifdef MELEE_NATIVE
    NativeItemLifetime source;
    if(!nativeItemLifetime_Capture(gobj,&source))return;
    Item* item_data=source.data;
#else
    Item* item_data = (Item*) HSD_GObjGetUserData(gobj);
    const uint32_t callback_generation = (uint32_t)item_data->x1C;
#endif
    bool remove = false;
#ifdef MELEE_NATIVE
    if (item_data->native_capture_inert) {
    } else if (nativeScriptItem_IsAttached(gobj)) {
        NativeScriptValue result;
        NativeScriptVMStatus status=item_data->kind==It_Kind_NativeHoldable
            ? nativeHoldable_Collision(gobj,&result)
            : nativeScriptItem_RunPhase(gobj,NativeScriptItemPhase_Collision,&result);
        if (status!=NativeScriptVM_Ok) return;
        remove=result.as.u64!=0;
    } else
#endif
    if (item_data->collided != NULL) {
        remove = item_data->collided(gobj);
#ifndef MELEE_NATIVE
        if(!itemCallbackCurrent(gobj,callback_generation))return;
#endif
    }
#ifdef MELEE_NATIVE
    if(!nativeItemLifetime_Current(&source))return;
#endif
    if (remove) {
        item_data->destroy_type = 1;
        Item_8026A8EC(gobj);
    } else {
        HSD_JObjSetTranslate((HSD_JObj*) HSD_GObjGetHSDObj(gobj),
                             &item_data->pos);
        it_8027574C(gobj);
        it_8026C368(gobj);
    }
}

/// this function is where the item accessory callback is called if it exists
static void Item_80269A9C(HSD_GObj* gobj)
{
    Item* item_data = (Item*) HSD_GObjGetUserData(gobj);
    const uint32_t callback_generation = (uint32_t)item_data->x1C;
    efAsync_QueueFlush(gobj, &item_data->xBC0);
    if(!itemCallbackCurrent(gobj,callback_generation))return;
    if (item_data->xDC8_word.flags.x9 == 0) {
#ifdef MELEE_NATIVE
        if(nativeScriptItem_IsAttached(gobj)&&nativeScriptItem_HookBound(gobj,NativeScriptItemHook_Accessory)) {
            NativeScriptValue result;
            if(nativeScriptItem_RunHook(gobj,NativeScriptItemHook_Accessory,NULL,0,&result)!=NativeScriptVM_Ok)return;
        } else
#endif
        if (item_data->on_accessory != NULL) {
            item_data->on_accessory(gobj);
            if(!itemCallbackCurrent(gobj,callback_generation))return;
        }
    }
    item_data = (Item*) HSD_GObjGetUserData(gobj);
    if (item_data->xDCD_flag.b01 != 0) {
        CmSubject* CmSubject = item_data->x520_cameraBox;
        if (CmSubject != NULL) {
            CmSubject->pos = item_data->pos;
            CmSubject->bone_pos = item_data->pos;
        }
    }
    it_80271A58(gobj);
    it_80272280(gobj);
    it_802722B0(gobj);
}

static void Item_80269B60(HSD_GObj* gobj)
{
    Item* item_data = gobj->user_data;
    const uint32_t callback_generation = (uint32_t)item_data->x1C;
    bool remove = false;
    if (item_data->xCFC != 0) {
#ifdef MELEE_NATIVE
        if(nativeScriptItem_IsAttached(gobj)&&nativeScriptItem_HookBound(gobj,NativeScriptItemHook_JumpedOn)) {
            NativeScriptValue result;
            if(nativeScriptItem_RunHook(gobj,NativeScriptItemHook_JumpedOn,NULL,0,&result)!=NativeScriptVM_Ok)return;
            remove=result.as.u64!=0;
        } else
#endif
        if(item_data->jumped_on!=NULL) {
            remove=item_data->jumped_on(gobj);
            if(!itemCallbackCurrent(gobj,callback_generation))return;
        }
    }
    if (remove) {
        item_data->destroy_type = 2;
        Item_8026A8EC(gobj);
        return;
    }
    it_8027137C(gobj);
    it_8027146C(gobj);
}

/// @remarks Somewhat arbitrary. Does not run on Hook Shot / Grapple
/// Beam, rather items such as the Barrel Cannon.
static void Item_80269BE4(HSD_GObj* gobj)
{
    Item* item_data = gobj->user_data;
    const uint32_t callback_generation = (uint32_t)item_data->x1C;
    if (item_data->xDD0_flag.b5) {
        it_802701BC(gobj);
        if(!itemCallbackCurrent(gobj,callback_generation))return;

        if (item_data->grab_victim != NULL) {
            HSD_GObj* victim = item_data->grab_victim;
            uint32_t victim_generation;
            if (!nativeCallbackSource_Snapshot(victim, NativeCallbackSource_Fighter, &victim_generation)) return;
#ifdef MELEE_NATIVE
            if(nativeScriptItem_IsAttached(gobj)&&nativeScriptItem_HookBound(gobj,NativeScriptItemHook_GrabDealt)) {
                NativeScriptValue result;
                if(nativeScriptItem_RunHook(gobj,NativeScriptItemHook_GrabDealt,NULL,0,&result)!=NativeScriptVM_Ok)return;
            } else
#endif
            if(item_data->grab_dealt!=NULL)item_data->grab_dealt(gobj);
            if(!itemCallbackCurrent(gobj,callback_generation))return;
            if (item_data->grab_victim == victim) {
                if (!nativeCallbackSource_Alive(victim, victim_generation, NativeCallbackSource_Fighter)) return;
            } else if (!nativeCallbackSource_Snapshot(item_data->grab_victim, NativeCallbackSource_Fighter, &victim_generation)) return;
#ifdef MELEE_NATIVE
            if(nativeScriptItem_IsAttached(gobj)&&nativeScriptItem_HookBound(gobj,NativeScriptItemHook_GrabbedForVictim)) {
                NativeMatchPoolReference source_reference;NativeScriptValue argument,result;NativeScriptDiagnostic diagnostic;
                if(!nativeMatchPool_Reference(&nativeGObjPool,gobj,&source_reference)||
                   !nativeScriptReceiver_CaptureExternal(item_data->grab_victim,NativeCallbackSource_Fighter,
                       source_reference.lifetime,&argument,&diagnostic))return;
                if(nativeScriptItem_RunHook(gobj,NativeScriptItemHook_GrabbedForVictim,&argument,1,&result)!=NativeScriptVM_Ok)return;
            } else
#endif
            if(item_data->grabbed_for_victim!=NULL)item_data->grabbed_for_victim(item_data->grab_victim,gobj);
        }
    }
}

static void Item_80269C5C(HSD_GObj* gobj)
{
    PAD_STACK(8);

    it_8026F9A0();
    it_802703E8(gobj);
    it_802706D0(gobj);
    it_80270E30(gobj);
}

/// Add to damage taken
void Item_80269CA0(Item* item_data, s32 damage)
{
    item_data->xC9C += damage;

    if (item_data->xC9C > 999) {
        item_data->xC9C = 999;
    }
}

static void Item_80269CC4(HSD_GObj* gobj)
{
    Item* temp_item = gobj->user_data;

    temp_item->xCA0 = 0;
    temp_item->xCA4 = 0;
    temp_item->xC94 = 0;
    temp_item->xCC8_knockback = 0.0f;
    temp_item->xC34_damageDealt = 0;
    temp_item->xC48 = 0;
    temp_item->xC4C = 0;
    temp_item->xC50 = 0;
    temp_item->xCCC_incDamageDirection = 0.0f;
    temp_item->xCB8_outDamageDirection = 0.0f;
    temp_item->xC68 = 0.0f;
    temp_item->xCD0 = 0.0f;
    temp_item->xCC0 = 0.0f;
    temp_item->xDCF_flag.b6 = false;
    temp_item->xCC4 = 0;
    temp_item->xCEC_fighterGObj = 0;
    temp_item->xCF0_itemGObj = 0;
    temp_item->xCF4_fighterGObjUnk = 0;
    temp_item->toucher = 0;
    temp_item->xC64_reflectGObj = 0;
    temp_item->xC90_absorbGObj = 0;
    temp_item->xCFC = 0;
    temp_item->xCB4 = -1;
    temp_item->xC38 = -1;
    temp_item->xCE0.z = 0.0f;
    temp_item->xCE0.y = 0.0f;
    temp_item->xCE0.x = 0.0f;
    temp_item->xCD4.z = 0.0f;
    temp_item->xCD4.y = 0.0f;
    temp_item->xCD4.x = 0.0f;
    temp_item->xDCE_flag.b5 = false;
    temp_item->xDCE_flag.b4 = false;
    temp_item->xDCC_flag.b1 = false;
    temp_item->xC70 = 1.0f;
    temp_item->xC6C = 1.0f;
    temp_item->xDCC_flag.b2 = false;
    temp_item->xDC8_word.flags.xB = false;
    temp_item->xCA8 = 0;
    temp_item->xDC8_word.flags.x3 = false;
    temp_item->xDC8_word.flags.x4 = false;
    temp_item->xDCE_flag.b6 = false;
}

static bool Item_80269DC8(HSD_GObj* gobj)
{
    HSD_GObjPredicate shield_bounced;
    HSD_GObjPredicate hit_shield;
    Item* item_data = gobj->user_data;
    const uint32_t callback_generation = (uint32_t)item_data->x1C;

    if (item_data->xDCE_flag.b5 && item_data->ground_or_air == 1 &&
        (item_data->xDCE_flag.b4 ||
         item_data->xC54 < (f32) M_PI / 180 * (90 + it_804D6D28->unk_degrees)))
    {
        if (item_data->xDCE_flag.b4 == 0) {
            item_data->xC54 -= M_PI / 2;
            if (item_data->xC54 < 0.0f) {
                item_data->xC54 = 0.0f;
            }
        }

#ifdef MELEE_NATIVE
        if(nativeScriptItem_IsAttached(gobj)) {
            if(processSourceCallback(gobj,NativeScriptItemHook_ShieldBounced))return true;
        } else
#endif
        {
        shield_bounced = item_data->xB8_itemLogicTable->shield_bounced;
        if (shield_bounced != NULL) {
            bool remove = shield_bounced(gobj);
            if(!itemCallbackCurrent(gobj,callback_generation))return true;
            if (remove) {
                item_data->destroy_type = 2;
                Item_8026A8EC(gobj);
                return true;
            }
        }
        }
    } else {

        if (it_80274C78(gobj)) {
            item_data->xCA8 = item_data->xC50;
        }

#ifdef MELEE_NATIVE
        if(nativeScriptItem_IsAttached(gobj)) {
            if(processSourceCallback(gobj,NativeScriptItemHook_HitShield))return true;
        } else
#endif
        {
        hit_shield = item_data->xB8_itemLogicTable->hit_shield;
        if (hit_shield != NULL) {
            bool remove = hit_shield(gobj);
            if(!itemCallbackCurrent(gobj,callback_generation))return true;
            if (remove) {
                item_data->destroy_type = 2;
                Item_8026A8EC(gobj);
                return true;
            }
        }
        }
    }

    return false;
}

static bool Item_80269F14(HSD_GObj* gobj)
{
    f32 temp_f30;
    bool (*cb_OnReflect)(HSD_GObj*);
    s32 i;
    u32 var_r27;
    Item* temp_item = (Item*) HSD_GObjGetUserData(gobj);
    const uint32_t callback_generation = (uint32_t)temp_item->x1C;

    if (temp_item->xDCC_flag.b1 == 0) {
        if (temp_item->kind == It_Kind_M_Ball) {
            if (temp_item->xDCC_flag.b2 == 0) {
                temp_item->owner = temp_item->xC64_reflectGObj;
                temp_item->x20_team_id = ftLib_80086EB4(temp_item->owner);
            }
        } else {
            temp_item->owner = temp_item->xC64_reflectGObj;
            temp_item->x20_team_id = ftLib_80086EB4(temp_item->owner);
        }
    }

    db_80225DD8(gobj, temp_item->owner);
    temp_item->xD90.x2070_int = temp_item->xC74;
    temp_item->xD94 = temp_item->xC78;
    temp_item->xD9C = temp_item->xC80;
    temp_item->xDA4_word = temp_item->xC88;
    temp_item->xDA8_short = temp_item->xC8C;
#ifdef MELEE_NATIVE
    if(nativeScriptItem_IsAttached(gobj)) {
        if(processSourceCallback(gobj,NativeScriptItemHook_Reflected))return true;
    } else
#endif
    {
    cb_OnReflect = temp_item->xB8_itemLogicTable->reflected;
    if (cb_OnReflect != NULL) {
        bool remove = cb_OnReflect(gobj);
        if(!itemCallbackCurrent(gobj,callback_generation))return true;
        if (remove) {
            temp_item->destroy_type = 2;
            Item_8026A8EC(gobj);
            return true;
        }
    }
    }
    if (temp_item->xDCC_flag.b0 == 0) {
        for (i = 0; i < 4; i++) // 4 here is the maximum amount of hitboxes
                                // available in the vanilla Item struct
        {
            if (temp_item->x5D4_hitboxes[i].hit.state != HitCapsule_Disabled) {
                temp_f30 =
                    temp_item->x5D4_hitboxes[i].hit.damage * temp_item->xC6C +
                    0.99f;
                var_r27 = temp_f30;
                if (var_r27 > it_804D6D28->xD8) {
                    var_r27 = it_804D6D28->xD8;
                }
                it_80272460(&temp_item->x5D4_hitboxes[i].hit, var_r27, gobj);
            }
        }
    }
    return false;
}

static void Item_8026A0A0(HSD_GObj* gobj)
{
    Item* item_data = (Item*) HSD_GObjGetUserData(gobj);
    const uint32_t callback_generation = (uint32_t)item_data->x1C;
    if (item_data->xDC8_word.flags.x7 != 0) {
        if ((item_data->xDC8_word.flags.xA == 0) &&
            (item_data->xDC8_word.flags.x8 == 0))
        {
            Item_8026A1E8(gobj);
            if(!itemCallbackCurrent(gobj,callback_generation))return;
        }
        item_data->xDC8_word.flags.x7 = 0;
    }
}

static void Item_8026A0FC(HSD_GObj* gobj)
{
    Item* item_data = (Item*) HSD_GObjGetUserData(gobj);
    const uint32_t callback_generation = (uint32_t)item_data->x1C;
    if (item_data->xDC8_word.flags.x8 != 0) {
        if ((item_data->xDC8_word.flags.xA == 0) &&
            (item_data->xDC8_word.flags.x7 == 0))
        {
            Item_8026A1E8(gobj);
            if(!itemCallbackCurrent(gobj,callback_generation))return;
        }
        item_data->xDC8_word.flags.x8 = 0;
    }
}

static void func_8026A158_helper(HSD_GObj* atkCollGObj)
{
    Item* temp_item = (Item*) HSD_GObjGetUserData(atkCollGObj);
    temp_item->xDC8_word.flags.x8 = 1;
    Item_8026A158(atkCollGObj);
}

static void Item_8026A158(HSD_GObj* gobj)
{
    Item* item_data = (Item*) HSD_GObjGetUserData(gobj);
    const uint32_t callback_generation = (uint32_t)item_data->x1C;
#ifdef MELEE_NATIVE
    if(nativeScriptItem_IsAttached(gobj)&&nativeScriptItem_HookBound(gobj,NativeScriptItemHook_EnteredHitlag)) {
        NativeScriptValue result;
        if(nativeScriptItem_RunHook(gobj,NativeScriptItemHook_EnteredHitlag,NULL,0,&result)!=NativeScriptVM_Ok)return;
    } else
#endif
    if (item_data->entered_hitlag != NULL) {
        item_data->entered_hitlag(gobj);
        if(!itemCallbackCurrent(gobj,callback_generation))return;
    }
    item_data->xDC8_word.flags.x9 = 1;
    if (item_data->atk_victim != NULL && it_80272D1C(item_data->atk_victim) &&
        item_data->xDC8_word.flags.x8 == 0)
    {
        func_8026A158_helper(item_data->atk_victim);
    }
}

static void func_8026A1E8_inline(HSD_GObj* atkCollGObj)
{
    Item* temp_item = (Item*) HSD_GObjGetUserData(atkCollGObj);
    const uint32_t callback_generation = (uint32_t)temp_item->x1C;
    if (temp_item->xDC8_word.flags.x8 != 0) {
        if ((temp_item->xDC8_word.flags.xA == 0) &&
            (temp_item->xDC8_word.flags.x7 == 0))
        {
            Item_8026A1E8(atkCollGObj);
            if(!itemCallbackCurrent(atkCollGObj,callback_generation))return;
        }
        temp_item->xDC8_word.flags.x8 = 0;
    }
}

static void Item_8026A1E8(HSD_GObj* gobj)
{
    Item* item_data = (Item*) HSD_GObjGetUserData(gobj);
    const uint32_t callback_generation = (uint32_t)item_data->x1C;
#ifdef MELEE_NATIVE
    if(nativeScriptItem_IsAttached(gobj)&&nativeScriptItem_HookBound(gobj,NativeScriptItemHook_ExitedHitlag)) {
        NativeScriptValue result;
        if(nativeScriptItem_RunHook(gobj,NativeScriptItemHook_ExitedHitlag,NULL,0,&result)!=NativeScriptVM_Ok)return;
    } else
#endif
    if (item_data->exited_hitlag != NULL) {
        item_data->exited_hitlag(gobj);
        if(!itemCallbackCurrent(gobj,callback_generation))return;
    }
    item_data->xDC8_word.flags.x9 = 0;
    if (item_data->atk_victim != NULL && it_80272D1C(item_data->atk_victim) &&
        item_data->xDC8_word.flags.x8 == 0)
    {
        func_8026A1E8_inline(item_data->atk_victim);
    }
}

#ifdef MELEE_NATIVE
// True means stop this native damage pipeline. A VM failure/retirement stops
// processing without setting a removal Bool or touching retired userdata.
static bool processSourceCallback(HSD_GObj* gobj,uint32_t hook)
{
    Item* item=gobj->user_data;NativeScriptValue result;NativeScriptVMStatus status;
    if(item->kind==It_Kind_NativeHoldable) {
        switch(hook) {
        case NativeScriptItemHook_DamageDealt:status=nativeHoldable_Hit(gobj,&result);break;
        case NativeScriptItemHook_Clanked:case NativeScriptItemHook_HitShield:status=nativeHoldable_Clanked(gobj,&result);break;
        case NativeScriptItemHook_Reflected:status=nativeHoldable_Reflected(gobj,&result);break;
        case NativeScriptItemHook_ShieldBounced:status=nativeHoldable_ShieldBounced(gobj,&result);break;
        default:status=nativeScriptItem_RunHook(gobj,hook,NULL,0,&result);break;
        }
    } else status=nativeScriptItem_RunHook(gobj,hook,NULL,0,&result);
    if(status!=NativeScriptVM_Ok)return true;
    if(result.as.u64) {
        item->destroy_type=2;
        Item_8026A8EC(gobj);
        return true;
    }
    return false;
}
#endif

static bool processCallback(HSD_GObjPredicate cb, HSD_GObj* gobj,
                            Item* item_data)
{
    const uint32_t callback_generation = (uint32_t)item_data->x1C;
    bool remove = false;
    if (cb != NULL) {
        remove = cb(gobj);
        if(!itemCallbackCurrent(gobj,callback_generation))return true;
    }
    if (remove) {
        item_data->destroy_type = 2;
        Item_8026A8EC(gobj);
        return true;
    }
    return false;
}

static bool OnTakeDamageThink(HSD_GObj* gobj, Item* item_data)
{
    item_data->xC9C += item_data->xCA0;
    if (item_data->xC9C > 999) {
        item_data->xC9C = 999;
    }
    item_data->xCA8 = item_data->xCA0;
    item_data->xDC8_word.flags.xB = 1;
#ifdef MELEE_NATIVE
    if(nativeScriptItem_IsAttached(gobj))return processSourceCallback(gobj,NativeScriptItemHook_DamageReceived);
#endif
    return processCallback(item_data->xB8_itemLogicTable->dmg_received, gobj,
                           item_data);
}

static bool OnClankThink(HSD_GObj* gobj, Item* item_data)
{
    bool (*cb_OnClank)(HSD_GObj*);
#ifdef MELEE_NATIVE
    nativeReplay_LogItemEvent(item_data->kind, "clank");
#endif
    if (it_80274C78(gobj)) {
        item_data->xCA8 = item_data->xC48;
    }
#ifdef MELEE_NATIVE
    if(nativeScriptItem_IsAttached(gobj))return processSourceCallback(gobj,NativeScriptItemHook_Clanked);
#endif
    cb_OnClank=item_data->xB8_itemLogicTable->clanked;
    return processCallback(cb_OnClank, gobj, item_data);
}

static bool OnGiveDamageThink(HSD_GObj* gobj, Item* item_data)
{
    bool (*cb_OnGiveDamage)(HSD_GObj*);
    if (it_80274C78(gobj)) {
        item_data->xCA8 = item_data->xC34_damageDealt;
    }
#ifdef MELEE_NATIVE
    if(nativeScriptItem_IsAttached(gobj))return processSourceCallback(gobj,NativeScriptItemHook_DamageDealt);
#endif
    cb_OnGiveDamage=item_data->xB8_itemLogicTable->dmg_dealt;
    return processCallback(cb_OnGiveDamage, gobj, item_data);
}

static void EnterHitlagThink(HSD_GObj* gobj, Item* item_data)
{
    const uint32_t callback_generation = (uint32_t)item_data->x1C;
    if (!item_data->xDC8_word.flags.x9) {
        item_data = GetItemData(gobj);
#ifdef MELEE_NATIVE
        if(nativeScriptItem_IsAttached(gobj)&&nativeScriptItem_HookBound(gobj,NativeScriptItemHook_EnteredHitlag)) {
            NativeScriptValue result;
            if(nativeScriptItem_RunHook(gobj,NativeScriptItemHook_EnteredHitlag,NULL,0,&result)!=NativeScriptVM_Ok)return;
        } else
#endif
        if (item_data->entered_hitlag) {
            item_data->entered_hitlag(gobj);
            if(!itemCallbackCurrent(gobj,callback_generation))return;
        }
        item_data->xDC8_word.flags.x9 = 1;

        if ((item_data->atk_victim) && it_80272D1C(item_data->atk_victim) &&
            !item_data->xDC8_word.flags.x8)
        {
            func_8026A158_helper(item_data->atk_victim);
        }
    }
}

static void checkHitLag(f32 min_value, Item* item_data)
{
    if (item_data->xCBC_hitlagFrames < min_value) {
        item_data->xCBC_hitlagFrames = min_value;
    }
    item_data->xDC8_word.flags.xA = 1;
}

static void Item_8026A294(HSD_GObj* gobj)
{
    Item* item_data = (Item*) HSD_GObjGetUserData(gobj);
    const uint32_t callback_generation = (uint32_t)item_data->x1C;

    if ((item_data->xCC8_knockback) || (item_data->xCA0)) {
        if (OnTakeDamageThink(gobj, item_data)) {
            return;
        }
    } else if (item_data->xC50) {
        if (Item_80269DC8(gobj)) {
            return;
        }
    } else if (item_data->xC48) {
        if (OnClankThink(gobj, item_data)) {
            return;
        }
    } else if ((item_data->xC34_damageDealt) || (item_data->xC4C)) {
        if (OnGiveDamageThink(gobj, item_data)) {
            return;
        }
    } else if (item_data->xC64_reflectGObj != NULL) {
        if (Item_80269F14(gobj)) {
            return;
        }
    } else if (item_data->xC90_absorbGObj != NULL) {
#ifdef MELEE_NATIVE
        nativeReplay_LogItemEvent(item_data->kind, "absorb");
#endif
#ifdef MELEE_NATIVE
        if(nativeScriptItem_IsAttached(gobj)) {
            if(processSourceCallback(gobj,NativeScriptItemHook_Absorbed))return;
        } else
#endif
        if (processCallback(item_data->xB8_itemLogicTable->absorbed, gobj,
                            item_data))
        {
            return;
        }
    } else if (item_data->xDCE_flag.b6) {
#ifdef MELEE_NATIVE
        if(nativeScriptItem_IsAttached(gobj)&&nativeScriptItem_HookBound(gobj,NativeScriptItemHook_Touched)) {
            if(processSourceCallback(gobj,NativeScriptItemHook_Touched))return;
        } else
#endif
        if (processCallback(item_data->touched, gobj, item_data)) {
            return;
        }
    }

    if ((item_data->xDC8_word.flags.xD) && (item_data->xCC0 > 0.0f)) {
        checkHitLag(item_data->xCC0, item_data);

        EnterHitlagThink(gobj, item_data);

        if(!itemCallbackCurrent(gobj,callback_generation))return;
    } else if (item_data->xCA8) {
        checkHitLag(it_8026B424(item_data->xCA8), item_data);

        EnterHitlagThink(gobj, item_data);

        if(!itemCallbackCurrent(gobj,callback_generation))return;
    }
    if ((item_data->xDC8_word.flags.x4) || (item_data->xDC8_word.flags.x3)) {
        EnterHitlagThink(gobj, item_data);
        if(!itemCallbackCurrent(gobj,callback_generation))return;

        if (item_data->xDC8_word.flags.x4) {
            item_data->xDC8_word.flags.x8 = 1;
            item_data->xDC8_word.flags.x4 = 0;
        } else {
            item_data->xDC8_word.flags.x7 = 1;
            item_data->xDC8_word.flags.x3 = 0;
        }
    }
    Item_80269CC4(gobj);
}

static void Item_8026A788(HSD_GObj* gobj)
{
    Item* item_data = gobj->user_data;
    int dynamicBonesNum = item_data->x374_dynamicBonesNum;
    Item_DynamicBones* dynamicBones = item_data->xD4_dynamicBones;
    int i;

    it_80272304(gobj);
    for (i = 0; i < dynamicBonesNum; i++, dynamicBones++) {
        lb_8001044C(&dynamicBones->dyn_desc, item_data->xB6C_vars,
                    item_data->xB68, 0.0f, 0, dynamicBones->flags, 0, 1);
    }
}

static bool Item_8026A810(HSD_GObj* gobj)
{
    Item* temp_item = (Item*) HSD_GObjGetUserData(gobj);
    const uint32_t generation=(uint32_t)temp_item->x1C;
#ifdef MELEE_NATIVE
    if (nativeScriptItem_IsAttached(gobj)) {
        NativeScriptValue result;
        return nativeScriptItem_RunHook(gobj,NativeScriptItemHook_Spawned,NULL,0,&result)==NativeScriptVM_Ok;
    }
#endif

    if (temp_item->xB8_itemLogicTable->spawned != NULL) {
        temp_item->xB8_itemLogicTable->spawned(gobj);
    }
    return itemCallbackCurrent(gobj,generation);
}

void Item_8026A848(HSD_GObj* gobj, HSD_GObj* fighter_gobj)
{
    Item* temp_item = (Item*) HSD_GObjGetUserData(gobj);

    if (temp_item->hold_kind == 8 && temp_item->kind != It_Kind_Link_Bomb &&
        temp_item->kind != It_Kind_CLink_Bomb &&
        temp_item->kind != It_Kind_Peach_Turnip &&
        temp_item->kind != It_Kind_Peach_Parasol)
    {
        if (ftLib_800867CC(fighter_gobj) == gobj) {
            ftLib_80086764(fighter_gobj);
        }
    } else if (ftLib_800867A0(fighter_gobj, gobj)) {
        ftLib_80086724(fighter_gobj, gobj);
    }
}

static void DestroyItemInline(HSD_GObj* gobj, Item* other_ip)
{
    Item* ip = gobj->user_data;
    HSD_GObj* other = other_ip->owner;

    if (ip->hold_kind == 8 && ip->kind != It_Kind_Link_Bomb &&
        ip->kind != It_Kind_CLink_Bomb && ip->kind != It_Kind_Peach_Turnip &&
        ip->kind != It_Kind_Peach_Parasol)
    {
        if (ftLib_800867CC(other) == gobj) {
            ftLib_80086764(other);
        }
    } else if (ftLib_800867A0(other, gobj)) {
        ftLib_80086724(other, gobj);
    }
}

static void ItemSwitch(HSD_GObj* gobj)
{
    Item* ip = gobj->user_data;
    enum_t destroy_type = ip->destroy_type;

    switch (destroy_type) {
    case 0:
        it_8027327C(gobj, ip->xCC_item_attr->destroy_gfx, ip->destroy_sfx);
        break;

    case 2:
    case 1:
        it_8027327C(gobj, ip->xCC_item_attr->x68, ip->xD80);
        break;

    case 3:
    case 4:
        break;
    }
}

static void func_8026A8EC_inline1(HSD_GObj* gobj)
{
    Item* ip = (Item*) HSD_GObjGetUserData(gobj);

    if (ip->xDC8_word.flags.xE) {
        ftLib_80087050(ip->ecb_lock);
        ip->xDC8_word.flags.xE = false;
        ip->ecb_lock = -1;
    }
}



static void func_8026A8EC_inline3(HSD_GObj* gobj)
{
    Item* it = (Item*) HSD_GObjGetUserData(gobj);

    if (it->xDCD_flag.b01 != 0 && it->x520_cameraBox != NULL) {
        Camera_800290D4(it->x520_cameraBox);
        it->x520_cameraBox = NULL;
        it->xDCD_flag.b01 = 0;
    }
}

void Item_8026A8EC(Item_GObj* gobj)
{
    uint32_t callback_generation;
    if (!nativeCallbackSource_Snapshot(gobj, NativeCallbackSource_Item, &callback_generation)) return;
    Item* ip = (Item*) HSD_GObjGetUserData(gobj);
    if (ip->native_removal_in_progress) return;
    ip->native_removal_in_progress = true;

    if (!it_80272D1C(gobj) || ip == NULL) {
        HSD_ASSERTREPORT(2405, 0, "===== Not Found Item_Struct!! =====\n");
    }

    it_802725D4(gobj);
    if(!nativeCallbackSource_Alive(gobj,callback_generation,NativeCallbackSource_Item))return;
    Item_802675A8(gobj);
    if(!nativeCallbackSource_Alive(gobj,callback_generation,NativeCallbackSource_Item))return;
    func_8026A8EC_inline1(gobj);
    if(!nativeCallbackSource_Alive(gobj,callback_generation,NativeCallbackSource_Item))return;
    efLib_DestroyAll(gobj);
    if(!nativeCallbackSource_Alive(gobj,callback_generation,NativeCallbackSource_Item))return;

    if (!ip->xDCF_flag.b2 && (!ip->xDC8_word.flags.x13 || ip->owner == NULL)) {
        ItemSwitch(gobj);
        if(!nativeCallbackSource_Alive(gobj,callback_generation,NativeCallbackSource_Item))return;
    }

#ifdef MELEE_NATIVE
    if(nativeScriptItem_IsAttached(gobj)) {
        NativeScriptValue result;
        // This retires owned storage after Destroyed (including a missing/error
        // hook), but the common destructor must finish while physically alive.
        (void)nativeScriptItem_RunHook(gobj,NativeScriptItemHook_Destroyed,NULL,0,&result);
        if(!nativeCallbackSource_Alive(gobj,callback_generation,NativeCallbackSource_Item))return;
    } else
#endif
    if (ip->xB8_itemLogicTable->destroyed != NULL) {
        ip->xB8_itemLogicTable->destroyed(gobj);
        if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Item)) return;
    }

    if (ip->xDC8_word.flags.x13 && ip->owner != NULL &&
        ftLib_80086960(ip->owner))
    {
        DestroyItemInline(gobj, ip);
        if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Item)) return;
    }

    ip->owner = NULL;
    func_8026A8EC_inline3(gobj);

    {
        int i;
        for (i = 0; i < ip->x374_dynamicBonesNum; i++) {
            lb_8000FD18(&ip->xD4_dynamicBones[i].dyn_desc);
        }
    }

    Item_8026B0B4(gobj);
    efAsync_QueueClear((struct EF_QueuedEffect*) &ip->xBC0);
#ifdef MELEE_NATIVE
    for (unsigned model = 1; model <= 3; ++model)
        nativeArticleModel_Destroy(gobj, model);
#endif
    HSD_GObjFree(gobj);
}

/// Pick up item
void Item_8026AB54(Item_GObj* gobj, HSD_GObj* owner_gobj, Fighter_Part part)
{
    Item* item_data = (Item*) HSD_GObjGetUserData(gobj);
    const uint32_t callback_generation = (uint32_t)item_data->x1C;
    PAD_STACK(16);

    it_80273168(gobj); // sets pickup sfx?
    it_802742F4(gobj, owner_gobj, part);
#ifdef MELEE_NATIVE
    if(nativeScriptItem_IsAttached(gobj)) {
        NativeScriptValue result;
        NativeScriptVMStatus status=item_data->kind==It_Kind_NativeHoldable
            ?nativeHoldable_PickedUp(gobj,&result)
            :nativeScriptItem_RunHook(gobj,NativeScriptItemHook_PickedUp,NULL,0,&result);
        if(status!=NativeScriptVM_Ok)return;
    } else
#endif
    if (item_data->xB8_itemLogicTable->picked_up != NULL) {
        item_data->xB8_itemLogicTable->picked_up(gobj);
        if(!itemCallbackCurrent(gobj,callback_generation))return;
    }
    Item_8026B074(item_data);
}

void Item_8026ABD8(Item_GObj* gobj, Vec3* pos, f32 arg2)
{
    Item* item_data = (Item*) HSD_GObjGetUserData(gobj);
#ifdef MELEE_NATIVE
    NativeItemLifetime lifetime;
    if (!nativeItemLifetime_Capture(gobj, &lifetime)) return;
#endif
    const uint32_t callback_generation = (uint32_t)item_data->x1C;
    PAD_STACK(8);

    item_data->xC44 = arg2;
    it_802731A4(gobj);
    it_80273B50(gobj, pos);
#ifdef MELEE_NATIVE
    if(nativeScriptItem_IsAttached(gobj)) {
        NativeScriptValue result;
        NativeScriptVMStatus status=item_data->kind==It_Kind_NativeHoldable
            ?nativeHoldable_Dropped(gobj,&result)
            :nativeScriptItem_RunHook(gobj,NativeScriptItemHook_Dropped,NULL,0,&result);
        if(status!=NativeScriptVM_Ok)return;
    } else
#endif
    if (item_data->xB8_itemLogicTable->dropped != NULL) {
        item_data->xB8_itemLogicTable->dropped(gobj);
        if(!itemCallbackCurrent(gobj,callback_generation))return;
    }
#ifdef MELEE_NATIVE
    if (!nativeItemLifetime_Current(&lifetime)) return;
#endif
    it_80274198(gobj, true);
#ifdef MELEE_NATIVE
    if (!nativeItemLifetime_Current(&lifetime)) return;
#endif
    it_802754D4(gobj);

    if (it_8026B6C8(gobj)) {
        it_80275390(gobj);
    }
}

void Item_8026AC74(HSD_GObj* gobj, Vec3* arg1, Vec3* arg2, f32 arg3)
{
    Item* item_data = GetItemData(gobj);
    const uint32_t callback_generation = (uint32_t)item_data->x1C;
    item_data->xC44 = arg3;
    it_802731A4(gobj);
    it_80273748(gobj, arg1, arg2);
#ifdef MELEE_NATIVE
    if(nativeScriptItem_IsAttached(gobj)) {
        NativeScriptValue result;
        NativeScriptVMStatus status=item_data->kind==It_Kind_NativeHoldable
            ?nativeHoldable_Dropped(gobj,&result)
            :nativeScriptItem_RunHook(gobj,NativeScriptItemHook_Dropped,NULL,0,&result);
        if(status!=NativeScriptVM_Ok)return;
    } else
#endif
    if (item_data->xB8_itemLogicTable->dropped != NULL) {
        item_data->xB8_itemLogicTable->dropped(gobj);
        if(!itemCallbackCurrent(gobj,callback_generation))return;
    }
    it_802741F4(gobj, true);
    it_802754D4(gobj);

    if (it_8026B6C8(gobj)) {
        it_80275390(gobj);
    }
}

void Item_8026AD20(HSD_GObj* gobj, Vec3* arg1, Vec3* arg2, f32 arg3, bool arg4)
{
    // What is arg4 used for? Was looking at ftCo_ItemThrow and it seems to
    // correspond to some kind of flag
    Item* item_data = GetItemData(gobj);
    const uint32_t callback_generation = (uint32_t)item_data->x1C;
    it_802731E0(gobj);
    item_data->xC44 = arg3;
    it_80273748(gobj, arg1, arg2);
#ifdef MELEE_NATIVE
    if(nativeScriptItem_IsAttached(gobj)) {
        NativeScriptValue result;
        NativeScriptVMStatus status=item_data->kind==It_Kind_NativeHoldable
            ?nativeHoldable_Thrown(gobj,&result)
            :nativeScriptItem_RunHook(gobj,NativeScriptItemHook_Thrown,NULL,0,&result);
        if(status!=NativeScriptVM_Ok)return;
    } else
#endif
    if (item_data->xB8_itemLogicTable->thrown != NULL) {
        item_data->xB8_itemLogicTable->thrown(gobj);
        if(!itemCallbackCurrent(gobj,callback_generation))return;
    }
    it_802741F4(gobj, true);
    it_802754D4(gobj);
}

void Item_8026ADC0(HSD_GObj* gobj)
{
    Item* item_data = (Item*) HSD_GObjGetUserData(gobj);
    const uint32_t callback_generation = (uint32_t)item_data->x1C;
#ifdef MELEE_NATIVE
    if(nativeScriptItem_IsAttached(gobj)) {
        NativeItemLifetime lifetime;
        if(!nativeItemLifetime_Capture(gobj,&lifetime))return;
        bool bound=nativeScriptItem_HookBound(gobj,NativeScriptItemHook_EnteredAir);
        NativeScriptValue result;
        if(nativeScriptItem_RunHook(gobj,NativeScriptItemHook_EnteredAir,NULL,0,&result)!=NativeScriptVM_Ok||
           !nativeItemLifetime_Current(&lifetime))return;
        if(bound)item_data->xDC8_word.flags.x1F=true;
    } else
#endif
    if (item_data->xB8_itemLogicTable->entered_air != NULL) {
        item_data->xB8_itemLogicTable->entered_air(gobj);
        if(!itemCallbackCurrent(gobj,callback_generation))return;
        item_data->xDC8_word.flags.x1F = true;
    }
}

void Item_OnUserDataRemove(void* user_data)
{
    Item* item_data = (Item*) user_data;

#ifdef MELEE_NATIVE
    nativeArticleResource_CancelGeneration(item_data->entity, item_data->x1C);
#endif
#ifndef MELEE_NATIVE
    if (item_data->xBBC_dynamicBoneTable != NULL) {
        HSD_ObjFree(&item_dynamic_bones_alloc_data,
                    item_data->xBBC_dynamicBoneTable);
    }
#endif

#ifdef MELEE_NATIVE
    nativeScriptItem_Detach(item_data->entity);
    NativeMatchPoolReference reference;
    HSD_ASSERTREPORT(0,nativeItemPool_Slot(item_data)!=NULL &&
        nativeMatchPool_Reference(&nativeItemPool,item_data,&reference) &&
        nativeMatchPool_Free(&nativeItemPool,reference),
        "Invalid native Item userdata retirement.\n");
#else
    HSD_ObjFree(&item_alloc_data, item_data);
#endif
}

u32 Item_8026AE60(void)
{
    u32 result = it_804D6D14++;

    if (it_804D6D14 == 0) {
        it_804D6D14++;
    }

    return result;
}

void Item_8026AE84(Item* item_data, enum_t sfx, u8 pan, u8 volume)
{
    if (sfx != 540000) {
        if (sfx != 540001) {
            item_data->xD6C =
                lbAudioAx_800237A8(lbAudioAx_800233EC(sfx), pan, volume);
        } else {
            item_data->xD6C = lbAudioAx_800237A8(540000, pan, volume);
        }
    }
}

void Item_8026AF0C(Item* item_data, enum_t sfx, u8 pan, u8 volume)
{
    if (sfx != 540000) {
        if (sfx != 540001) {
            if (item_data->sfx_unk1 != SFX_NONE) {
                lbAudioAx_800236B8(item_data->sfx_unk1);
            }
            item_data->sfx_unk1 =
                lbAudioAx_800237A8(lbAudioAx_800233EC(sfx), pan, volume);
        } else {
            Item_8026B034(item_data);
        }
    }
}

void Item_8026AFA0(Item* item_data, enum_t sfx, u8 pan, u8 volume)
{
    if (sfx != 540000) {
        if (sfx != 540001) {
            if (item_data->sfx_unk2 != SFX_NONE) {
                lbAudioAx_800236B8(item_data->sfx_unk2);
            }
            item_data->sfx_unk2 =
                lbAudioAx_800237A8(lbAudioAx_800233EC(sfx), pan, volume);
        } else {
            Item_8026B074(item_data);
        }
    }
}

void Item_8026B034(Item* item_data)
{
    if (item_data->sfx_unk1 != SFX_NONE) {
        lbAudioAx_800236B8(item_data->sfx_unk1);
    }

    item_data->sfx_unk1 = SFX_NONE;
}

void Item_8026B074(Item* item_data)
{
    if (item_data->sfx_unk2 != SFX_NONE) {
        lbAudioAx_800236B8(item_data->sfx_unk2);
    }

    item_data->sfx_unk2 = SFX_NONE;
}

static void Item_8026B0B4(HSD_GObj* gobj)
{
    Item* item_data = (Item*) HSD_GObjGetUserData(gobj);
    lbAudioAx_80026510(gobj);

    if (item_data->xD6C != SFX_NONE) {
        lbAudioAx_800265C4(gobj, item_data->xD6C);
        item_data->xD6C = SFX_NONE;
    }

    if (item_data->sfx_unk1 != SFX_NONE &&
        lbAudioAx_800265C4(gobj, item_data->sfx_unk1) == 1)
    {
        if (item_data->sfx_unk1 != SFX_NONE) {
            lbAudioAx_800236B8(item_data->sfx_unk1);
        }
        item_data->sfx_unk1 = SFX_NONE;
    }

    if (item_data->sfx_unk2 != SFX_NONE &&
        lbAudioAx_800265C4(gobj, item_data->sfx_unk2) == 1)
    {
        if (item_data->sfx_unk2 != SFX_NONE) {
            lbAudioAx_800236B8(item_data->sfx_unk2);
        }
        item_data->sfx_unk2 = SFX_NONE;
    }

    if (item_data->sfx_unk1 != SFX_NONE) {
        lbAudioAx_800236B8(item_data->sfx_unk1);
    }

    item_data->sfx_unk1 = SFX_NONE;

    if (item_data->sfx_unk2 != SFX_NONE) {
        lbAudioAx_800236B8(item_data->sfx_unk2);
    }

    item_data->sfx_unk2 = SFX_NONE;
    item_data->xD6C = SFX_NONE;
}

/// Check if item is grabbable
bool Item_IsGrabbable(Item_GObj* gobj)
{
    Item* temp_item;

    temp_item = gobj->user_data;

#ifdef MELEE_NATIVE
    if(nativeScriptItem_IsAttached(gobj))return temp_item->xDC8_word.flags.x15&&
        (temp_item->kind==It_Kind_NativeHoldable||nativeScriptItem_HookBound(gobj,NativeScriptItemHook_PickedUp));
#endif
    if ((temp_item->xDC8_word.flags.x15 != 0) &&
        (temp_item->xB8_itemLogicTable->picked_up != NULL))
    {
        return true;
    }

    return false;
}
