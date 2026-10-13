#include <melee/gr/stage_events.h>
#include <melee/gr/stage.h>
#include <melee/pl/player.h>
#include <melee/pl/plbonus.h>
#include <melee/pl/plbonuslib.h>
#include "native_fighter_module.h"
#include "native_cpu_history.h"
#include "native_fighter_relations.h"
#include "native_script_math.h"
#include "native_fighter_geometry.h"
#include "native_fighter_visuals.h"
#include "native_fighter_model_graph.h"
#include "native_fighter_pose.h"
#include "native_fighter_contact.h"
#include "native_fighter_capture.h"
#include "ft_0CDD.h"
#include "ft_0DF1.h"
#include "kinds/ftCommon/ftCo_AttackAir.h"
#include "native_fighter_state.h"
#include "native_fighter_target.h"
#include "native_fighter_events.h"
#include "native_fighter_damage.h"
#include "native_fighter_jump.h"
#include "native_costume_runtime.h"
#include <melee/it/native_article_refs.h>
#include <melee/it/native_item_owner.h>
#include <melee/it/kinds/itfoods.h>
#include <melee/ef/native_model_effect.h>
#include <melee/lb/native_model_joint.h>
#include <melee/lb/lbcollision.h>

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "types.h"
#include "fighter.h"
#include "inlines.h"
#include "ftanim.h"
#include "ftdata.h"
#include "ftcpuattack.h"
#include "kinds/ftCommon/ftCo_0A01.h"
#include "ftwalkcommon.h"
#include "ft_081B.h"
#include "ft_084E.h"
#include "ft_0892.h"
#include "ft_0881.h"
#include "ft_0877.h"
#include "ftchangeparam.h"
#include "ftcolanim.h"
#include <melee/cm/camera.h>
#include <melee/gr/grbigblue.h>
#include <melee/gr/grinishie1.h>
#include <melee/gr/grcorneria.h>
#include <melee/gr/grvenom.h>
#include <melee/mp/mplib.h>
#include <melee/mp/mpcoll.h>
#include <melee/mp/mpisland.h>
#include <melee/mp/types.h>
#include "ft_0BF0.h"
#include "ftcommon.h"
#include "kinds/ftCommon/ftCo_Fall.h"
#include "kinds/ftCommon/ftCo_FallAerial.h"
#include "kinds/ftCommon/ftCo_Guard.h"
#include "kinds/ftCommon/ftCo_Dash.h"
#include "kinds/ftCommon/ftCo_SpecialAir.h"
#include "kinds/ftCommon/ftCo_FallSpecial.h"
#include "kinds/ftCommon/ftCo_DownBound.h"
#include "kinds/ftCommon/ftCo_Landing.h"
#include "kinds/ftCommon/ftCo_Wait.h"
#include "kinds/ftCommon/ftCo_SquatWait.h"
#include "kinds/ftCommon/ftCo_Squat.h"
#include "kinds/ftCommon/ftCo_Walk.h"
#include "kinds/ftCommon/ftCo_AppealS.h"
#include "ftparts.h"
#include "ftlib.h"
#include "ftmaterial.h"
#include <melee/it/itmaterial.h>
#include <melee/it/article_model_layers.h>
#include "ftcoll.h"
#include "ftcharacter_assets.h"
#include "ftcliffcommon.h"
#include "ftwalljump.h"
#include "kinds/ftCommon/ftCo_JumpAerial.h"
#include "kinds/ftCommon/ftCo_Jump.h"
#include "kinds/ftCommon/ftCo_KneeBend.h"
#include "kinds/ftCommon/ftCo_Shouldered.h"
#include "kinds/ftCommon/ftCo_Damage.h"
#include "kinds/ftCommon/ftCo_DamageFall.h"
#include "kinds/ftCommon/ftCo_CliffJump.h"
#include <melee/lb/lbaudio_ax.h>
#include "kinds/ftCommon/ftCo_Pass.h"
#include "kinds/ftCommon/ftCo_Turn.h"
#include "kinds/ftCommon/ftCo_Escape.h"
#include "kinds/ftCommon/ftCo_Attack100.h"
#include "kinds/ftCommon/ftCo_Attack1.h"
#include "kinds/ftCommon/ftCo_LandingAir.h"
#include "kinds/ftCommon/ftCo_Attack1.h"
#include "kinds/ftCommon/ftCo_AttackS3.h"
#include "kinds/ftCommon/ftCo_AttackHi3.h"
#include "kinds/ftCommon/ftCo_AttackLw3.h"
#include "kinds/ftCommon/ftCo_AttackS4.h"
#include "kinds/ftCommon/ftCo_AttackHi4.h"
#include "kinds/ftCommon/ftCo_AttackLw4.h"
#include "kinds/ftCommon/ftCo_SpecialS.h"
#include "kinds/ftCommon/ftCo_Lift.h"
#include "kinds/ftCommon/ftCo_ItemThrow.h"
#include "kinds/ftCommon/ftCo_ItemParasolOpen.h"
#include <melee/it/native_held_article.h>
#include "kinds/ftCommon/ftCo_Throw.h"
#include "kinds/ftCommon/ftCo_Thrown.h"
#include "kinds/ftCommon/ftCo_CaptureKoopa.h"
#include "kinds/ftCommon/ftCo_CaptureDamageKoopa.h"
#include "kinds/ftCommon/ftCo_ThrownKoopa.h"
#include "kinds/ftCommon/ftpickupitem.h"
#include <melee/ef/eflib.h>
#include <melee/ef/efasync.h>
#include <melee/ef/efsync.h>
#include <melee/it/kinds/itmariofireball.h>
#include <melee/it/kinds/itdrmariopill.h>
#include <melee/it/kinds/types.h>
#include <melee/it/kinds/inlines.h>
#include <melee/it/native_holdable.h>
#include <melee/it/native_article_model.h>
#include <melee/it/native_article.h>
#include <melee/it/native_capture_item.h>
#include "native_capture.h"
#include <melee/it/inlines.h>
#include <melee/it/item.h>
#include <melee/it/itcoll.h>
#include <melee/it/it_2725.h>
#include <melee/it/it_26B1.h>
#include <melee/it/ithitbox.h>
#include <melee/it/iteffect.h>
#include <melee/it/itgroundcoll.h>
#include <melee/it/itmaplib.h>
#include <melee/db/db.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lb_00F9.h>
#include <melee/lb/lbvector.h>
#include <melee/lb/lbanim.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/random.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// Stock native call sites use the CRT float operation. Taking its address
// must bind the import directly; lbtrigf's console math symbols otherwise
// pull an unused console implementation into the native link.
__declspec(dllimport) float __cdecl atan2f(float, float);

typedef const NativeFighterDefinition* (__cdecl* DefinitionEntry)(
    const NativeFighterDefinition* base, const NativeFighterHost* host);
typedef uint64_t (__cdecl* LayoutEntry)(void);

static float air_drift_threshold(void)
{
    return p_ftCommonData->x258;
}

static unsigned bone_count(Fighter* fighter)
{
    if (fighter == NULL || ftPartsTable == NULL ||
        (unsigned) fighter->kind >= Ft_Kind_RuntimeCapacity ||
        ftPartsTable[fighter->kind] == NULL) return 0;
    return ftPartsTable[fighter->kind]->parts_num;
}

// Named compiler constants select shared input thresholds; scripts cannot
// supply arbitrary data offsets or depend on a fighter's extended DAT layout.
static float common_attribute(NativeFighterCommonAttribute id)
{
    switch (id) {
    case NativeFighterCommon_ThrowHorizontal: return p_ftCommonData->x98;
    case NativeFighterCommon_ThrowUp: return p_ftCommonData->attackhi3_stick_threshold_y;
    case NativeFighterCommon_ThrowDown: return p_ftCommonData->xB0;
    case NativeFighterCommon_AerialJumpReverse: return p_ftCommonData->x78;
    case NativeFighterCommon_TapJumpThreshold: return p_ftCommonData->tap_jump_threshold;
    case NativeFighterCommon_AerialActionDownThreshold: return p_ftCommonData->x88;
    case NativeFighterCommon_ShieldStartHealth: return p_ftCommonData->x260_startShieldHealth;
    case NativeFighterCommon_ShieldMinimumHold: return p_ftCommonData->x268;
    case NativeFighterCommon_ShieldPushbackDamage: return p_ftCommonData->x28C;
    case NativeFighterCommon_ShieldPushbackBase: return p_ftCommonData->x290;
    case NativeFighterCommon_ShieldPushbackVelocity: return p_ftCommonData->x294;
    case NativeFighterCommon_ShieldPushbackLightMin: return p_ftCommonData->x2E4;
    case NativeFighterCommon_ShieldPushbackLightMax: return p_ftCommonData->x2E8;
    case NativeFighterCommon_ShieldReflectDuration: return p_ftCommonData->x2A4;
    case NativeFighterCommon_ShieldReflectRadius: return p_ftCommonData->x2A8;
    case NativeFighterCommon_ShieldReflectDamage: return p_ftCommonData->x2AC;
    case NativeFighterCommon_ShieldReflectSpeed: return p_ftCommonData->x2B0;
    case NativeFighterCommon_ShieldPowershieldDuration: return p_ftCommonData->x2B4;
    case NativeFighterCommon_HorizontalSmashThreshold: return p_ftCommonData->dash_smash_stick_threshold;
    case NativeFighterCommon_HorizontalSmashWindow: return p_ftCommonData->dash_smash_window;
    case NativeFighterCommon_HorizontalSmashExtension: return p_ftCommonData->x44;
    case NativeFighterCommon_SmashShakeCount: return Fighter_SmashChargeShakeTable->x4;
    case NativeFighterCommon_HorizontalStickDeadzone: return p_ftCommonData->horizontal_stick_deadzone;
    case NativeFighterCommon_StickSmoothing: return p_ftCommonData->x44C;
    default: return 0;
    }
}

static void mark_effect_active(Fighter_GObj* gobj)
{
    Fighter* fighter = gobj->user_data;
    fighter->x2219_b0 = true;
    Fighter_SetEffectHitlagCallbacks(fighter);
}

static void copy_ext_attributes(Fighter_GObj* gobj)
{
    Fighter* fighter = GET_FIGHTER(gobj);
    memcpy(fighter->dat_attrs_backup, fighter->ft_data->ext_attr,
           FT_CHARACTER_EXT_ATTR_CAPACITY);
    fighter->dat_attrs = fighter->dat_attrs_backup;
}

static void reload_ext_attributes(Fighter_GObj* gobj)
{
    Fighter* fighter = GET_FIGHTER(gobj);
    if (fighter->dat_attrs != NULL)
        memcpy(fighter->dat_attrs, fighter->ft_data->ext_attr,
               FT_CHARACTER_EXT_ATTR_CAPACITY);
}

static void item_land(Item_GObj* gobj)
{
    Item_80268E5C(gobj, 1, ITEM_ANIM_UPDATE);
}

static void item_change_state(Item_GObj* gobj, int state)
{
    Item_80268E5C(gobj, state, ITEM_ANIM_UPDATE);
}

static void item_play_sound(Item_GObj* gobj, s32 sound, u8 pan, u8 volume)
{
    Item_8026AE84(GET_ITEM(gobj), sound, pan, volume);
}

static void item_set_uniform_scale(Item_GObj* gobj, float scale)
{
    HSD_JObj* jobj = GET_JOBJ(gobj);
    if (jobj == NULL) return;
    Vec3 size = { scale, scale, scale };
    HSD_JObjSetScale(jobj, &size);
}

static float fighter_root_scale_y(Fighter_GObj* gobj)
{
    HSD_JObj* root = GET_JOBJ(gobj);
    return root != NULL ? HSD_JObjGetScaleY(root) : 1.0f;
}

static void item_set_child_visible(Item_GObj* gobj, bool visible)
{
    HSD_JObj* root = GET_JOBJ(gobj);
    if (root == NULL || root->child == NULL) return;
    if (visible) HSD_JObjClearFlags(root->child, JOBJ_HIDDEN);
    else HSD_JObjSetFlags(root->child, JOBJ_HIDDEN);
}

static s32 random_int(s32 upper_bound)
{
    return upper_bound > 0 ? HSD_Randi(upper_bound) : 0;
}

static s32 random_int_excluding(s32 upper_bound, s32 first, s32 second)
{
    if (upper_bound <= 0) return -1;
    s32 count = 0;
    for (s32 value = 0; value < upper_bound; ++value)
        if (value != first && value != second) ++count;
    if (count == 0) return -1;
    s32 choice = HSD_Randi(count);
    for (s32 value = 0; value < upper_bound; ++value) {
        if (value == first || value == second) continue;
        if (choice-- == 0) return value;
    }
    return -1;
}

static void spawn_projectile(Fighter_GObj* owner, Vec3* position,
                             ItemKind kind, s32 variant, float facing)
{
    switch (kind) {
    case It_Kind_Mario_Fire:
        it_8029B6F8(owner, position, kind, facing);
        break;
    case It_Kind_DrMario_Vitamin:
        itDrMarioPill_Spawn(owner, position, variant, kind, facing);
        break;
    default:
        break;
    }
}

static void spawn_authored_projectile(Fighter_GObj* owner, Vec3* position,
                                     ItemKind kind, s32 variant, float facing,
                                     const NativeFighterProjectileLaunch* launch)
{
    if (owner == NULL || position == NULL || !isfinite(position->x) ||
        !isfinite(position->y) || !isfinite(position->z) || !isfinite(facing))
        return;
    Fighter* fighter = GET_FIGHTER(owner);
    const NativeFighterDefinition* definition =
        fighter != NULL ? nativeFighterRuntime_Definition(fighter->kind) : NULL;
    if (definition == NULL) return;
    const uint32_t owner_number = (uint32_t) fighter->x8_spawnNum;
    const NativeFighterProjectile* setup = NULL;
    unsigned state_count = 0;
    for (unsigned i = 0; i < definition->article_logic_count; ++i)
        if (definition->article_logics[i].kind == kind) {
            setup = definition->article_logics[i].projectile;
            state_count = definition->article_logics[i].state_count;
            break;
        }
    Article* article = NULL;
    if (setup == NULL ||
        !nativeFighterRuntime_ResolveArticleForItem(fighter, kind, &article) ||
        article == NULL || setup->read_values == NULL ||
        article->xC_itemStates == NULL ||
        (setup->appearance_state >= 0 &&
         (u32) setup->appearance_state >= article->xC_itemStates->count))
        return;
    float values[3];
    setup->read_values(article, values);
    float speed = values[0], angle = values[1], lifetime = values[2];
    unsigned item_state = setup->item_state;
    if (launch != NULL) {
        speed = launch->speed;
        angle = launch->angle;
        item_state = launch->item_state;
    }
    if (!isfinite(speed) || !isfinite(angle) || !isfinite(lifetime) ||
        lifetime <= 0 || item_state >= state_count) return;
    if (launch != NULL) {
        angle = nativeScriptMath_WrapAngle(angle);
        if (!launch->preserve_facing)
            facing = angle < M_PI_2 || angle > M_PI * 3 / 2 ? 1 : -1;
    }
    SpawnItem spawn = { 0 };
    spawn.kind = kind;
    spawn.prev_pos = *position;
    spawn.prev_pos.z = 0;
    it_8026BB68(owner, &spawn.pos);
    spawn.facing_dir = facing;
    spawn.x40 = launch != NULL ? launch->hit_group : 0;
    spawn.x0_parent_gobj = owner;
    spawn.x4_parent_gobj2 = owner;
    spawn.x44_flag.b0 = 1;
    Item_GObj* gobj = Item_80268B18(&spawn);
    if (gobj == NULL) return;
    Item* item = GET_ITEM(gobj);
    const uint32_t number = item->x1C;
    if (setup->initial_velocity) {
        item->x40_vel.x = (launch != NULL ? 1 : facing) * (speed * cosf(angle));
        item->x40_vel.y = speed * sinf(angle);
        item->x40_vel.z = 0;
    }
    item->xDD4_itemVar.native_article.launch_angle = launch != NULL
        ? angle : nativeScriptMath_WrapAngle(facing >= 0 ? angle : M_PI - angle);
    item->xDD4_itemVar.native_article.launch_speed = speed;
    item->xDD4_itemVar.native_article.launch_origin = spawn.pos;
    it_80275158(gobj, lifetime);
    Item_80268E5C(gobj, item_state, ITEM_ANIM_UPDATE);
    if (!setup->auto_lifetime) it_8026B3A8(gobj);
    if (setup->appearance_state >= 0)
        it_80273670(gobj, setup->appearance_state, (float) variant);
    if (!nativeCallbackSource_Alive(gobj, number, NativeCallbackSource_Item) ||
        !nativeCallbackSource_Alive(owner, owner_number, NativeCallbackSource_Fighter)) return;
    item->xDD4_itemVar.native_article.attachment_owner = owner;
    item->xDD4_itemVar.native_article.creator_spawn_id = nativeFighterTarget_Id(owner);
    item->xDD4_itemVar.native_article.attachment_facing = GET_FIGHTER(owner)->facing_dir;
    const NativeFighterArticleLogic* logic = nativeFighterRuntime_ArticleLogic(fighter, kind);
    if (logic->on_created != NULL) logic->on_created(gobj);
    if (!nativeCallbackSource_Alive(gobj, number, NativeCallbackSource_Item) ||
        !nativeCallbackSource_Alive(owner, owner_number, NativeCallbackSource_Fighter)) return;
    db_80225DD8(gobj, owner);
    if (setup->initial_update) it_802750F8(gobj);
    if (!nativeCallbackSource_Alive(gobj, number, NativeCallbackSource_Item)) return;
    if (!setup->auto_lifetime) it_8026B3A8(gobj);
    if (setup->on_launched != NULL) setup->on_launched(gobj);
}

// Unattached owned article, with authored initialization after engine creation
// and state selection, before the constructor's first physics/update pass.
static s32 spawn_article(Fighter_GObj* owner, ItemKind kind, s32 state)
{
    if (owner == NULL) return -1;
    Fighter* fighter = GET_FIGHTER(owner);
    const NativeFighterArticleLogic* logic =
        nativeFighterRuntime_ArticleLogic(fighter, kind);
    Article* article = NULL;
    if (logic == NULL || state < 0 || state >= logic->state_count ||
        !nativeFighterRuntime_ResolveArticleForItem(fighter, kind, &article) ||
        article == NULL) return -1;
    const uint32_t owner_number = (uint32_t) fighter->x8_spawnNum;
    SpawnItem spawn = { 0 };
    spawn.kind = kind;
    Item_InitSpawn(&spawn, owner, &fighter->cur_pos, fighter->facing_dir);
    Item_GObj* object = Item_80268B18(&spawn);
    if (object == NULL) return -1;
    if (!nativeCallbackSource_Alive(owner, owner_number, NativeCallbackSource_Fighter)) return -1;
    Item* item = GET_ITEM(object);
    const s32 number = item->x1C;
    item->xDD4_itemVar.native_article.attachment_owner = owner;
    item->xDD4_itemVar.native_article.creator_spawn_id = nativeFighterTarget_Id(owner);
    item->xDD4_itemVar.native_article.attachment_facing = fighter->facing_dir;
    Item_80268E5C(object, state, ITEM_ANIM_UPDATE);
    if (!nativeCallbackSource_Alive(object, (uint32_t) number, NativeCallbackSource_Item) ||
        !nativeCallbackSource_Alive(owner, owner_number, NativeCallbackSource_Fighter)) return -1;
    if (logic->on_created != NULL) logic->on_created(object);
    if (!nativeCallbackSource_Alive(object, (uint32_t) number, NativeCallbackSource_Item) ||
        !nativeCallbackSource_Alive(owner, owner_number, NativeCallbackSource_Fighter)) return -1;
    db_80225DD8(object, owner);
    it_802750F8(object);
    return number;
}

static void follow_attached_article(Item_GObj* gobj)
{
    Item* item = GET_ITEM(gobj);
    Fighter_GObj* owner = nativeArticle_Creator(gobj);
    if (owner == NULL || item->owner != owner) {
        item->on_accessory = NULL;
        return;
    }
    Vec3 position = GET_FIGHTER(owner)->cur_pos;
    item->pos = position;
    HSD_JObj* joint = gobj->hsd_obj;
    HSD_JObjSetTranslate(joint, &position);
    HSD_JObjSetRotationY(joint,
        (float) (M_PI_2 * item->xDD4_itemVar.native_article.attachment_facing));
    HSD_JObjSetRotationZ(joint, 0);
    it_8026B3A8(gobj);
}

// This creates an authored presentation article. Scripts choose its kind,
// source joint, appearance frame, item states and lifetime; the shared engine
// only performs ordinary item setup and follows the original fighter origin.
static s32 spawn_attached_article(Fighter_GObj* owner, ItemKind kind,
                                 Fighter_Part part, s32 variant,
                                 s32 appearance_state, s32 initial_state,
                                 s32 attached_state, float lifetime)
{
    if (owner == NULL || appearance_state < 0 || initial_state < 0 ||
        attached_state < 0 || initial_state >= 32 || attached_state >= 32 ||
        !isfinite(lifetime) || lifetime <= 0)
        return -1;
    Fighter* fighter = GET_FIGHTER(owner);
    Article* article = NULL;
    if (fighter == NULL ||
        nativeFighterRuntime_ArticleLogic(fighter, kind) == NULL ||
        !nativeFighterRuntime_ResolveArticleForItem(fighter, kind, &article) ||
        article == NULL || article->xC_itemStates == NULL ||
        (u32) appearance_state >= article->xC_itemStates->count)
        return -1;
    Fighter_Part joint_index = ftParts_GetBoneIndex(fighter, part);
    FighterPartsTable* parts = ftPartsTable[fighter->kind];
    if (parts == NULL || (u32) joint_index >= parts->parts_num ||
        fighter->parts[joint_index].joint == NULL) return -1;
    Vec3 position;
    lb_8000B1CC(fighter->parts[joint_index].joint, NULL, &position);
    const uint32_t owner_number = (uint32_t) fighter->x8_spawnNum;
    SpawnItem spawn = { 0 };
    spawn.kind = kind;
    spawn.prev_pos = position;
    spawn.prev_pos.z = 0;
    it_8026BB68(owner, &spawn.pos);
    spawn.facing_dir = fighter->facing_dir;
    spawn.x0_parent_gobj = owner;
    spawn.x4_parent_gobj2 = owner;
    spawn.x44_flag.b0 = 1;
    Item_GObj* gobj = Item_80268B18(&spawn);
    if (gobj == NULL) return -1;
    if (!nativeCallbackSource_Alive(owner, owner_number, NativeCallbackSource_Fighter)) return -1;
    Item* item = GET_ITEM(gobj);
    const s32 number = item->x1C;
    it_80275158(gobj, lifetime);
    Item_80268E5C(gobj, initial_state, ITEM_ANIM_UPDATE);
    it_8026B3A8(gobj);
    it_8026B3A8(gobj);
    it_80273670(gobj, appearance_state, (float) variant);
    it_8026B3A8(gobj);
    Item_80268E5C(gobj, attached_state, ITEM_ANIM_UPDATE);
    if (!nativeCallbackSource_Alive(gobj, (uint32_t) number, NativeCallbackSource_Item) ||
        !nativeCallbackSource_Alive(owner, owner_number, NativeCallbackSource_Fighter)) return -1;
    item->xDAC_itcmd_var0 = item->xDB0_itcmd_var1 = 0;
    item->xDB4_itcmd_var2 = item->xDB8_itcmd_var3 = 0;
    item->xDC8_word.flags.x19 = 0;
    item->xDD4_itemVar.native_article.attachment_owner = owner;
    item->xDD4_itemVar.native_article.creator_spawn_id = nativeFighterTarget_Id(owner);
    item->xDD4_itemVar.native_article.attachment_facing = fighter->facing_dir;
    item->on_accessory = follow_attached_article;
    item->xDC8_word.flags.x13 = 1;
    item->owner = owner;
    it_80274740(gobj);
    it_802756D0(gobj);
    it_8026B3A8(gobj);
    const NativeFighterArticleLogic* logic = nativeFighterRuntime_ArticleLogic(fighter, kind);
    if (logic->on_created != NULL) logic->on_created(gobj);
    if (!nativeCallbackSource_Alive(gobj, (uint32_t) number, NativeCallbackSource_Item) ||
        !nativeCallbackSource_Alive(owner, owner_number, NativeCallbackSource_Fighter)) return -1;
    db_80225DD8(gobj, owner);
    it_802750F8(gobj);
    if (!nativeCallbackSource_Alive(gobj, (uint32_t) number, NativeCallbackSource_Item)) return -1;
    it_8026B3A8(gobj);
    return number;
}

static bool stage_same_line_island(int32_t first, int32_t second)
{
    MapCollData* collision = mpLib_8004D164();
    if (collision == NULL || first < 0 || second < 0 ||
        first >= collision->line_count || second >= collision->line_count) return false;
    mp_UnkStruct0* island = mpIsland_8005AB54(first);
    return island != NULL && island == mpIsland_8005AB54(second);
}

static bool stage_cpu_floor_excluded(int32_t line)
{
    MapCollData* collision = mpLib_8004D164();
    if (collision == NULL || line < 0 || line >= collision->line_count) return false;
    return grBigBlue_801EF844(line) || grInishie1_801FCAAC(line) ||
        grCorneria_801E2D90(line) || grVenom_80206D10(line);
}

static bool stage_cpu_island_blocked(int32_t line)
{
    MapCollData* collision = mpLib_8004D164();
    if (collision == NULL || line < 0 || line >= collision->line_count) return false;
    return ftCo_CpuIslandBlocked(mpIsland_8005AB54(line));
}

static bool stage_line_island_bounds(int32_t line, Vec3* minimum, Vec3* maximum)
{
    if (minimum == NULL || maximum == NULL) return false;
    *minimum = (Vec3){ 0 };
    *maximum = (Vec3){ 0 };
    MapCollData* collision = mpLib_8004D164();
    if (collision == NULL || line < 0 || line >= collision->line_count) return false;
    mp_UnkStruct0* island = mpIsland_8005AB54(line);
    if (island == NULL) return false;
    *minimum = island->x8;
    *maximum = island->x14;
    return true;
}

// Script handles are item creation numbers, never saved GObj pointers. Resolve
// them only against live items still owned by the requesting fighter.
static Item_GObj* find_owned_article(Fighter_GObj* owner, s32 item_number)
{
    if (owner == NULL || item_number == -1 || HSD_GObjPLinkHead == NULL) return NULL;
    for (HSD_GObj* gobj = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];
         gobj != NULL; gobj = gobj->next) {
        Item* item = gobj->user_data;
        if (item != NULL && item->x1C == item_number && item->owner == owner)
            return (Item_GObj*) gobj;
    }
    return NULL;
}


static s32 spawn_bone_attached_article(Fighter_GObj* owner, ItemKind kind,
                                      Fighter_Part part)
{
    if (owner == NULL) return -1;
    Fighter* fighter = GET_FIGHTER(owner);
    Article* article = NULL;
    if (fighter == NULL ||
        nativeFighterRuntime_ArticleLogic(fighter, kind) == NULL ||
        !nativeFighterRuntime_ResolveArticleForItem(fighter, kind, &article) ||
        article == NULL) return -1;
    Fighter_Part bone = ftParts_GetBoneIndex(fighter, part);
    FighterPartsTable* parts = ftPartsTable[fighter->kind];
    if (parts == NULL || (u32) bone >= parts->parts_num ||
        fighter->parts[bone].joint == NULL) return -1;
    Vec3 position;
    const NativeFighterDefinition* definition =
        nativeFighterRuntime_Definition(fighter->kind);
    const NativeFighterAttachmentSpawn* setup = NULL;
    if (definition != NULL)
        for (unsigned i = 0; i < definition->article_logic_count; ++i)
            if (definition->article_logics[i].kind == kind) {
                setup = definition->article_logics[i].attachment;
                break;
            }
    Vec3 previous;
    if (setup == NULL || (!setup->origin_at_owner && !setup->origin_at_center) ||
        setup->previous_at_bone) {
        lb_8000B1CC(fighter->parts[bone].joint, NULL, &previous);
        position = previous;
    }
    if (setup != NULL && setup->origin_at_center)
        ftLib_80086990(owner, &position);
    else if (setup != NULL && setup->origin_at_owner)
        position = fighter->cur_pos;
    if (setup != NULL && setup->flatten_z) position.z = 0;
    const uint32_t owner_number = (uint32_t) fighter->x8_spawnNum;
    SpawnItem spawn;
    spawn.kind = kind;
    Item_InitSpawn(&spawn, owner, &position, fighter->facing_dir);
    if (setup != NULL && setup->previous_at_bone) {
        if (setup->flatten_z) previous.z = 0;
        spawn.prev_pos = previous;
    }
    if (setup != NULL) spawn.x44_flag.b0 = setup->initial_collision;
    Item_GObj* item = Item_80268B18(&spawn);
    if (item == NULL) return -1;
    if (!nativeCallbackSource_Alive(owner, owner_number, NativeCallbackSource_Fighter)) return -1;
    Item* attached = GET_ITEM(item);
    attached->xDD4_itemVar.native_article.attachment_owner = owner;
    attached->xDD4_itemVar.native_article.creator_spawn_id = nativeFighterTarget_Id(owner);
    attached->xDD4_itemVar.native_article.attachment_facing = fighter->facing_dir;
    const s32 number = attached->x1C;
    const NativeFighterArticleLogic* logic = nativeFighterRuntime_ArticleLogic(fighter, kind);
    if (logic->on_created != NULL) logic->on_created(item);
    if (!nativeCallbackSource_Alive(owner, owner_number, NativeCallbackSource_Fighter))
        return -1;
    // Authored construction may remove the item. Resolve the saved creation
    // number before attaching or registering it, never reuse a freed pointer.
    item = find_owned_article(owner, number);
    if (item == NULL) return -1;
    Item_8026AB54(item, owner, bone);
    if (!nativeCallbackSource_Alive(item, (uint32_t) number, NativeCallbackSource_Item) ||
        !nativeCallbackSource_Alive(owner, owner_number, NativeCallbackSource_Fighter)) return -1;
    if (setup == NULL || setup->register_as_held) {
        db_80225DD8(item, owner);
        fighter->x1984_heldItemSpec = item;
    }
    return number;
}

static bool owned_article_exists(Fighter_GObj* owner, s32 item_number)
{
    return find_owned_article(owner, item_number) != NULL;
}

static bool link_owned_articles(Fighter_GObj* owner, s32 source, unsigned slot, s32 target)
{
    return nativeArticle_Link(find_owned_article(owner, source), slot,
                              find_owned_article(owner, target));
}

// Resolve the caller before dereferencing it. An item's author is immutable;
// reflection may change damage ownership without changing its script package.
static const NativeFighterDefinition* article_signal_author(HSD_GObj* caller)
{
    if (caller == NULL || HSD_GObjPLinkHead == NULL) return NULL;
    for (HSD_GObj* live = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];
         live != NULL; live = live->next) {
        if (live != caller) continue;
        Fighter* fighter = live->user_data;
        return fighter != NULL && fighter->kind >= Ft_Kind_CustomFirst &&
            fighter->kind < Ft_Kind_RuntimeCapacity
            ? nativeFighterRuntime_Definition(fighter->kind) : NULL;
    }
    for (HSD_GObj* live = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];
         live != NULL; live = live->next) {
        if (live != caller) continue;
        Item* item = live->user_data;
        if (item == NULL || item->native_article_author_kind < Ft_Kind_CustomFirst ||
            item->native_article_author_kind >= Ft_Kind_RuntimeCapacity) return NULL;
        const NativeFighterDefinition* definition =
            nativeFighterRuntime_Definition(item->native_article_author_kind);
        if (definition == NULL || definition->article_logics == NULL) return NULL;
        for (unsigned i = 0; i < definition->article_logic_count; ++i) {
            const NativeFighterArticleLogic* logic = &definition->article_logics[i];
            if (logic->kind == item->kind && logic->logic != NULL &&
                logic->logic == item->xB8_itemLogicTable) return definition;
        }
        return NULL;
    }
    return NULL;
}

static bool signal_article(HSD_GObj* caller, uint32_t handle,
                           uint32_t name, float value)
{
    if (handle == UINT32_MAX || !isfinite(value)) return false;
    const NativeFighterDefinition* definition = article_signal_author(caller);
    if (definition == NULL || definition->article_logics == NULL) return false;
    for (HSD_GObj* live = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];
         live != NULL; live = live->next) {
        Item* item = live->user_data;
        if (item == NULL || item->x1C != handle ||
            item->native_article_author_kind < Ft_Kind_CustomFirst ||
            item->native_article_author_kind >= Ft_Kind_RuntimeCapacity ||
            nativeFighterRuntime_Definition(item->native_article_author_kind) != definition)
            continue;
        for (unsigned i = 0; i < definition->article_logic_count; ++i) {
            const NativeFighterArticleLogic* logic = &definition->article_logics[i];
            if (logic->kind != item->kind || logic->logic == NULL ||
                logic->logic != item->xB8_itemLogicTable) continue;
            for (unsigned j = 0; j < logic->signal_count; ++j) {
                if (logic->signals[j].name_hash != name) continue;
                if (logic->signals[j].callback == NULL || !nativeCallbackDispatch_Enter())
                    return false;
                logic->signals[j].callback(live, value);
                nativeCallbackDispatch_Leave();
                // Either caller or recipient may have been removed/reused.
                return true;
            }
            return false;
        }
        return false;
    }
    return false;
}

static bool signal_owned_article(Fighter_GObj* owner, s32 item_number,
                                  uint32_t name_hash, float value)
{
    if (!isfinite(value)) return false;
    Item_GObj* object = find_owned_article(owner, item_number);
    if (object == NULL) return false;
    Item* item = object->user_data;
    Fighter* fighter = owner->user_data;
    const NativeFighterDefinition* definition = fighter != NULL
        ? nativeFighterRuntime_Definition(fighter->kind) : NULL;
    if (definition == NULL) return false;
    for (unsigned i = 0; i < definition->article_logic_count; ++i) {
        const NativeFighterArticleLogic* article = &definition->article_logics[i];
        if (article->kind != item->kind) continue;
        // Confirm authored logic before interpreting its private item state.
        if (item->xB8_itemLogicTable != article->logic ||
            item->xDD4_itemVar.native_article.attachment_owner != owner) return false;
        for (unsigned j = 0; j < article->signal_count; ++j) {
            if (article->signals[j].name_hash != name_hash) continue;
            if (article->signals[j].callback == NULL || !nativeCallbackDispatch_Enter())
                return false;
            article->signals[j].callback(object, value);
            nativeCallbackDispatch_Leave();
            // A signal may remove the item. Do not dereference it afterwards.
            return true;
        }
        return false;
    }
    return false;
}

static bool remove_owned_article(Fighter_GObj* owner, s32 item_number)
{
    Item_GObj* item = find_owned_article(owner, item_number);
    if (item == NULL) return false;
    Item_8026A8EC(item);
    return true;
}

static bool pause_owned_article(Fighter_GObj* owner, s32 item_number)
{
    Item_GObj* item = find_owned_article(owner, item_number);
    if (item == NULL) return false;
    it_8026B724(item);
    return true;
}

static bool resume_owned_article(Fighter_GObj* owner, s32 item_number)
{
    Item_GObj* item = find_owned_article(owner, item_number);
    if (item == NULL) return false;
    it_8026B73C(item);
    return true;
}

static void item_spawn_effect_on_bone(Item_GObj* gobj, s32 effect,
                                      u32 bone_index, u8 mode)
{
    Item* item = GET_ITEM(gobj);
    if (item == NULL || item->xC4_article_data == NULL ||
        item->xC4_article_data->x10_modelDesc == NULL || bone_index >= 100 || mode > 1)
        return;
    HSD_JObj* joint = GET_JOBJ(gobj);
    if (item->xC4_article_data->x10_modelDesc->x4_bone_count != 0) {
        if (item->xBBC_dynamicBoneTable == NULL ||
            bone_index >= item->xC4_article_data->x10_modelDesc->x4_bone_count)
            return;
        joint = item->xBBC_dynamicBoneTable->bones[bone_index];
    } else {
        // Items without an indexed bone table still have a model root and
        // optional child chain, as in it_80272CC0's engine attachment lookup.
        for (u32 i = 0; i < bone_index && joint != NULL; ++i)
            joint = joint->child;
    }
    if (joint != NULL)
        efAsync_Spawn(gobj, &item->xBC0, mode, effect, joint);
}

static void item_activate_attached(Item_GObj* gobj)
{
    Item_802694CC(gobj);
    it_80274574(gobj);
}

static void spawn_wind_at_fighter(Fighter_GObj* gobj, int kind,
                                  float width, float height, float depth)
{
    Fighter* fighter = GET_FIGHTER(gobj);
    lb_800119DC(&fighter->cur_pos, kind, width, height, depth);
}

static void spawn_effect_between_parts(Fighter_GObj* gobj, s32 effect,
                                       s32 first, s32 second)
{
    Fighter* fighter = GET_FIGHTER(gobj);
    FighterPartsTable* parts = ftPartsTable[fighter->kind];
    if (parts == NULL || first < 0 || second < 0 ||
        (u32) first >= parts->parts_num ||
        (u32) second >= parts->parts_num) return;
    efSync_Spawn(effect, gobj, fighter->parts[first].joint,
                 fighter->parts[second].joint);
}

static void spawn_effect_on_part(Fighter_GObj* gobj, s32 effect, s32 part)
{
    Fighter* fighter = GET_FIGHTER(gobj);
    FighterPartsTable* parts = ftPartsTable[fighter->kind];
    if (parts == NULL || part < 0 || (u32) part >= parts->parts_num) return;
    efSync_Spawn(effect, gobj, fighter->parts[part].joint);
}

static void spawn_effect_with_facing(Fighter_GObj* gobj, s32 effect,
                                     s32 part)
{
    Fighter* fighter = GET_FIGHTER(gobj);
    FighterPartsTable* parts = ftPartsTable[fighter->kind];
    if (parts == NULL || part < 0 || (u32) part >= parts->parts_num) return;
    efSync_Spawn(effect, gobj, fighter->parts[part].joint,
                 &fighter->facing_dir);
}

static s32 detected_item_kind(Fighter* fighter)
{
    HSD_GObj* target = fighter->unk_gobj;
    return target != NULL && target->classifier == HSD_GOBJ_CLASS_ITEM
        ? itGetKind(target) : -1;
}

static const NativeFighterSoundChannel sound_channels[NATIVE_FIGHTER_SOUND_CHANNEL_COUNT] = {
    ft_PlaySFX, ft_80088478, ft_800881D8, ft_80088510,
    ft_800885A8, ft_80088640, ft_80088328,
};

static NativeFighterHost fighter_host = {
    .activate_partner = ftCommon_ActivatePartner,
    .family_member_id = nativeFighterRuntime_MemberId,
    .link_hitlag = nativeFighterRelations_Link,
    .linked_hitlag_id = nativeFighterRelations_LinkedId,
    .request_hitlag_release = Fighter_UnkSetFlag_8006CFBC,
    .copy_collision_from = nativeFighterRelations_CopyCollision,
    .target_bone_position = nativeFighterTarget_BonePosition,
    .target_position = nativeFighterTarget_Position,
    .cpu_nearest_enemy_at = ftCo_CpuNearestEligibleTarget,
    .cpu_record_input = nativeCpuHistory_Record,
    .cpu_read_history_pose = nativeCpuHistory_ReadPose,
    .cpu_apply_recorded_input = nativeCpuHistory_Apply,
    .cpu_prepare_combat_plan = ftCo_CpuPrepareCombatPlan,
    .cpu_prepare_destination_update = ftCo_CpuPrepareDestinationUpdate,
    .cpu_update_recovery_destination = ftCo_CpuUpdateRecoveryDestination,
    .cpu_plan_attack = ftCo_CpuPlanAttack,
    .cpu_plan_without_target = ftCo_CpuPlanWithoutTarget,
    .cpu_set_destination = ftCo_CpuSetDestination,
    .stage_same_line_island = stage_same_line_island,
    .stage_cpu_floor_excluded = stage_cpu_floor_excluded,
    .stage_cpu_island_blocked = stage_cpu_island_blocked,
    .stage_read_floor = mpCheckFloor,
    .target_position_delta = nativeFighterTarget_PositionDelta,
    .target_damage = nativeFighterTarget_Damage,
    .target_facing = nativeFighterTarget_Facing,
    .target_sleeping = nativeFighterTarget_Sleeping,
    .target_floor_line = nativeFighterTarget_FloorLine,
    .target_cpu_body_half_width = nativeFighterTarget_CpuBodyHalfWidth,

    .article_ref_primary_translation = nativeArticleRef_PrimaryModelTranslation,
    .target_call = nativeFighterEvent_Call,
    .target_call_bool = nativeFighterEvent_CallBool,
    .target_call_int = nativeFighterEvent_CallInt,
    .target_call_float = nativeFighterEvent_CallFloat,
    .callback_source_alive = nativeCallbackSource_Alive,
    .article_ref_exists = nativeArticleRef_Exists,
    .article_ref_float = nativeArticleRef_Float,
    .article_ref_int = nativeArticleRef_Int,
    .article_ref_bool = nativeArticleRef_Bool,
    .item_link_id = nativeArticle_LinkId,
    .item_create_child = nativeArticle_CreateChild,
    .enter_wait_blend = ft_8008A348,
    .enter_held_open = ftCo_ItemParasolOpen_Enter,
    .notify_held_neutral = nativeHeldArticle_NotifyNeutral,
    .stage_event = Stage_Event,
    .get_action_stats = Player_GetActionStats,
    .action_count = pl_800386D8,
    .notify_taunt = pl_80040120,
    .vector_angle_xy = lbVector_AngleXY,
    .skip_soft_floor = ftCo_8009A134,
    .check_turn_input = ftCo_800C97A8,
    .check_taunt = ftCo_800DE9D8,
    .check_crouch = ftCo_800D5FB0,
    .check_turn = ftCo_Turn_CheckInput,
    .check_walk = ftCo_Walk_CheckInput,
    .check_ground_jump = ftCo_Jump_CheckInput,
    .check_air_jump = ftCo_800CB870,
    .check_platform_drop = ftCo_80099F1C,
    .enter_platform_drop = ftCo_8009A184,
    .get_bone_rotation_y = ftPartGetRotY,
    .finish_action = ftCommon_8007D92C,
    .update_throw = ftCo_800DD724,
    .throw_motion = ftCo_ThrowF_Phys,
    .walk_type = ftWalkCommon_GetWalkType,
    .walk_requested = ftWalkCommon_800DFC70,
    .walk_stop_requested = ft_8008A1FC,
    .enter_walk = ftWalkCommon_800DFCA4,
    .update_walk_animation = ftWalkCommon_800DFDDC,
    .walk_physics = ftWalkCommon_800E0060,
    .animation_length = ftAnim_8006F484,
    .animation_clip = ftData_80085E50,
    .clip_length = lbAnim_8001E8F8,
    .jump_input = ftCo_Jump_GetInput,
    .check_short_hop = ftCo_KneeBend_Check_ShortHop,
    .launch_jump = ftCo_800CB110,
    .begin_aerial_jump = ftCo_JumpAerial_Begin,
    .jump_physics = ftCo_Jump_Phys_Inner,
    .enter_landing = ftCo_Landing_Enter,
    .initialize_hold = ftCo_8009C640,
    .set_hold_motion = ftCo_8009C5A4,
    .set_throw_motion = ftCo_800DE3FC,
    .common_attribute = common_attribute,
    .update_damage_timer = ftCo_8008F744,
    .damage_physics = ftCo_Damage_Phys,
    .check_heavy_item_throw = ftCo_80094EA4,
    .damage_reaction_suppressed = ftCo_8008E984,
    .scale_knockback = ftCo_ScaleBy154,
    .knockback_level = ftCo_Damage_CalcKnockbackLevel,
    .enter_damage_motion = ftCo_8008DCE0,
    .enter_damage_reaction = ftCo_8008E908,
    .release_held_item = Item_8026ABD8,
    .cpu_command = ftCo_800B463C,
    .cpu_command_arg = ftCo_800B46B8,
    .cpu_command_pair = ftCo_800B4778,
    .cpu_run_script = ftCo_800B4880,
    .common_motion_states = ftData_MotionStateList,
    .cpu_ranged_unsafe = ftCo_CpuRangedAttackUnsafe,
    .fighters_share_floor = ftCo_800A2170,
    .cpu_add_candidate = ftCo_CpuAddAttackCandidate,
    .animation_family = nativeFighterRuntime_AnimationFamily,
    .create_article_at = nativeArticle_Create,
    .stage_segment_intersection = mpCheckAllRemap,
    .play_global_sound = lbAudioAx_800237A8,
    .check_fastfall = ftCommon_CheckFallFast,
    .fastfall = ftCommon_FallFast,
    .probe_ledge = nativeFighterGeometry_ProbeLedge,
    .find_ledge_occupant = ft_80082E3C,
    .enter_cliff_jump_phase2 = ftCo_8009B390,
    .bind_fighter_item = nativeItem_BindFighter,
    .bind_existing_item = nativeItem_BindExisting,
    .set_item_visible = nativeItem_SetVisible,
    .set_item_paused = nativeItem_SetPaused,
    .notify_item_drop = nativeItem_NotifyDrop,
    .clear_fighter_item = nativeItem_ClearBinding,
    .held_item_id = nativeItem_HeldId,
    .create_engine_item = nativeItem_CreateEngine,
    .attach_created_item = nativeItem_Attach,
    .inherit_created_item_scale = nativeItem_InheritOwnerScale,
    .item_set_appearance = it_80273670,
    .item_reset_spin = it_80274740,
    .item_refresh_physics = it_802750F8,
    .item_spawn_link = nativeArticle_SpawnLink,
    .item_unlink = nativeArticle_Unlink,
    .item_link_reciprocal = nativeArticle_LinkReciprocal,
    .item_link_value = nativeArticle_LinkValue,
    .item_bone_value = nativeArticle_BoneValue,
    .item_link_bone_value = nativeArticle_LinkBoneValue,
    .item_copy_link_collision = nativeArticle_CopyLinkCollision,
    .item_signal_link = nativeArticle_SignalLink,
    .item_change_state_flags = Item_80268E5C,
    .item_set_debug_owner = db_80225DD8,
    .clear_kinetics = ftCommon_8007E2FC,
    .air_decel_normal = ftCommon_CalcSelfAccel_DeaccelAir,
    .update_facing_threshold = ftCommon_8007DA24,
    .new_article_collision_group = Item_8026AE60,
    .link_owned_articles = link_owned_articles,
    .item_check_ground = it_8026DA70,
    .item_show_model = it_8026BB20,
    .item_hide_model = it_8026BB44,
    .item_attach_to_owner = Item_8026AB54,
    .item_release_attachment = it_8027429C,
    .item_spawn_joint_effect = nativeArticleModel_SpawnEffect,
    .item_spawn_async_joint_effect = nativeArticleModel_SpawnAsyncEffect,
    .item_stage_collision_land = it_8026DA08,
    .item_stage_response = it_8026E71C,
    .item_begin_expiry = it_8027518C,
    .item_expire_tick = it_802751D8,
    .item_stop_motion = it_80273454,
    .item_find_fighter = it_8026B634,
    .item_find_target = it_8026C258,
    .item_center_position = it_8026BB88,
    .vector_normalize_xy = lbVector_NormalizeXY,
    .vector_rotate = lbVector_Rotate,
    .item_owner_overlap = nativeArticle_OverlapsOwner,
    .item_follow_ground = it_8026D62C,
    .item_bounce_to_ground = it_8026E248,
    .item_slide_on_ground = it_8026E8C4,
    .item_align_facing = it_80272980,
    .model_animation_rate = lb_8000BA0C,
    .model_animation_active = lb_8000B09C,
    .model_advance_animation = HSD_JObjAnimAll,
    .item_sweep_floor = it_8026E0F4,
    .item_stage_contact_response = it_8026E5A0,
    .try_wall_jump = ftWallJump_8008169C,
    .init_hurtbox = ftColl_HurtboxInit,
    .body_hit_status = ftColl_8007B868,
    .reset_damage_callbacks = ftCommon_8007DB58,
    .enter_crouch_wait = ftCo_800D638C,
    .check_side_special = ftCo_SpecialS_CheckInput,
    .check_up_special = ftCo_Attack100_CheckInput,
    .check_neutral_special = ftCo_800D6824,
    .check_down_special = ftCo_800D68C0,
    .check_catch = ftCo_Catch_CheckInput,
    .check_side_smash = ftCo_AttackS4_CheckInput,
    .check_up_smash = ftCo_AttackHi4_CheckInput,
    .check_down_smash = ftCo_AttackLw4_CheckInput,
    .check_side_tilt = ftCo_AttackS3_CheckInput,
    .check_up_tilt = ftCo_AttackHi3_CheckInput,
    .check_down_tilt = ftCo_AttackLw3_CheckInput,
    .check_jab = ftCo_Attack1_CheckInput,
    .check_ground_roll = ftCo_80099794,
    .check_guard_spotdodge = ftCo_8009980C,
    .find_floor_island = mpIsland_8005AC14,
    .island_bounds = mpIsland_8005ACE8,
    .item_is_heavy = itIsHeavy,
    .item_hold_kind = itGetHoldKind,
    .item_use_category = itGetUseKind,
    .cstick_attack_edge = ftCo_800DF478,
    .aerial_attack_motion = ftCo_AttackAir_GetMsidFromCStick,
    .enter_air_gun = ftCo_800CDDA0,
    .check_air_item_throw = ftCo_80095328,
    .contact_rebound_allowed = nativeFighterContact_ReboundAllowed,
    .set_part_animation = ftAnim_80070FB4,
    .show_part_animation = ftAnim_80070C48,
    .hide_part_animation = ftAnim_80070CC4,
    .has_model_root = nativeFighterAttachment_HasRoot,
    .load_model_attachment = nativeFighterAttachment_Load,
    .add_attachment_dynamics = nativeFighterAttachment_AddDynamics,
    .clear_model_attachments = nativeFighterAttachment_Clear,
    .set_model_scale = HSD_JObjSetScale,
    .clear_hitboxes = ftColl_8007AFF8,
    .clear_attack_color = ftCo_800BFFAC,
    .start_color = ftCo_800BFFD0,
    .set_animation_rate = ftAnim_SetAnimRate,
    .play_sound = ft_PlaySFX,
    .loop_sound = ft_80088510,
    .sound_channels = sound_channels,
    .floor_friction = mpLib_800569EC,
    .ground_box_edges = ft_80082978,
    .air_box_ledge = ft_8008239C,
    .camera_quake = Camera_RequestQuake,
    .signal_article = signal_article,
    .item_rumble = it_80273598,
    .item_stop_loop_sound = Item_8026B074,
    .article_resource_begin = nativeArticleResource_Begin,
    .article_resource_create = nativeArticleResource_CreateNode,
    .article_resource_load_model = nativeArticleResource_LoadModel,
    .article_resource_set_draw = nativeArticleResource_SetDraw,
    .article_resource_count = nativeArticleResource_Count,
    .article_resource_exists = nativeArticleResource_Exists,
    .article_resource_bind_external_part = nativeArticleResource_BindExternalPart,
    .article_resource_set_capture_anchor = nativeArticleResource_SetCaptureAnchor,
    .article_resource_release = nativeArticleResource_Release,
    .article_resource_read_joint = nativeArticleResource_ReadJoint,
    .article_resource_write_joint = nativeArticleResource_WriteJoint,
    .article_resource_set_world_matrix = nativeArticleResource_SetWorldMatrix,
    .article_resource_read_float = nativeArticleResource_ReadFloat,
    .article_resource_write_float = nativeArticleResource_WriteFloat,
    .article_resource_read_int = nativeArticleResource_ReadInt,
    .article_resource_write_int = nativeArticleResource_WriteInt,
    .article_resource_sweep_stage = nativeArticleResource_SweepStage,
    .article_resource_sweep_point = nativeArticleResource_SweepPoint,
    .article_resource_follow_floor = nativeArticleResource_FollowFloor,
    .article_resource_read_line_speed = nativeArticleResource_ReadLineSpeed,
    .article_resource_read_attachment_point = nativeArticleResource_ReadAttachmentPoint,
    .article_resource_read_attachment_matrix = nativeArticleResource_ReadAttachmentMatrix,
    .article_resource_read_world_matrix = nativeArticleResource_ReadWorldMatrix,
    .article_resource_add_animation = nativeArticleResource_AddAnimation,
    .article_resource_request_animation = nativeArticleResource_RequestAnimation,
    .article_resource_advance_animation = nativeArticleResource_AdvanceAnimation,
    .article_resource_remove_animation = nativeArticleResource_RemoveAnimation,
    .article_resource_set_animation_rate = nativeArticleResource_SetAnimationRate,
    .article_resource_spawn_effect_at_position = nativeArticleResource_SpawnEffectAtPosition,
    .article_resource_spawn_effect_facing_at_position = nativeArticleResource_SpawnEffectFacingAtPosition,
    .rumble = ftCommon_8007EBAC,
    .refresh_attack = ft_80089824,
    .record_attack = ft_800892A0,
    .change_motion_state = Fighter_ChangeMotionState,
    .start_animation = ftAnim_8006EBA4,
    .ground_collision = ft_80082708,
    .air_collision = ft_80081D0C,
    .special_ground_collision = ft_800827A0,
    .follow_floor = mpLib_80056C54,
    .set_hitbox_position = ftColl_8007B8A8,
    .check_ground_escape = ftCo_8009917C,
    .create_guard = ftColl_8007B1B8,
    .ground_box_collision = ft_80082888,
    .ground_standard_box_collision = ft_800847D0,
    .air_box_collision = ft_800824A0,
    .ground_to_air = ftCommon_8007D5D4,
    .air_to_ground = ftCommon_8007D7FC,
    .animation_frames_remaining = ftAnim_IsFramesRemaining,
    .finish_ground_action = ft_8008A2BC,
    .enter_taunt = ftCo_800DEAE8,
    .enter_fall = ftCo_Fall_Enter,
    .enter_freefall = ftCo_80096900,
    .ground_actions = ftCo_Wait_IASA,
    .taunt_actions = ftCo_AppealS_IASA,
    .air_actions = ftCo_Fall_IASA_Inner,
    .ground_friction = ft_80084F3C,
    .air_drift = ft_80084DB0,
    .ground_motion = ft_80084FA8,
    .ground_animation_velocity = ft_80085088,
    .air_motion = ft_80085154,
    .air_gravity_and_friction = ft_80084EEC,
    .clamp_air_drift = ftCommon_ClampAirDrift,
    .fall = ftCommon_Fall,
    .quick_air_decel = ftCommon_CalcSelfAccel_DeaccelQuickAir,
    .quick_decel = ftCommon_CalcSelfAccel_DeaccelQuick,
    .air_drift_threshold = air_drift_threshold,
    .air_decel = ftCommon_CalcSelfAccel_Deaccel,
    .fall_basic = ftCommon_FallBasic,
    .ascend = ftCommon_Ascend,
    .ground_accel = ftCommon_CalcGroundAccel_AccelToLStickX,
    .air_accel = ftCommon_CalcSelfAccel_DriftSimple_NoFriction,
    .air_accel_friction = ftCommon_CalcSelfAccel_DriftSimple,
    .air_accel_normal = ftCommon_CalcSelfAccel_Drift,
    .air_accel_from = ftCommon_CalcSelfAccel_DriftFrom,
    .ground_to_self_movement = ftCommon_SetSelfMovementFromGroundedMovement,
    .ground_decelerate = ftCommon_CalcGroundAccel_Deaccel,
    .clamp_self_vel_x = ftCommon_ClampSelfVelX,
    .clamp_ground_vel = ftCommon_ClampGroundVel,
    .clamp_fall_speed = ftCommon_ClampFallSpeed,
    .air_standard_collision = ft_80083B68,
    .air_catch_hit_collision = ftCo_AirCatchHit_Coll,
    .ground_standard_collision = ft_80084104,
    .ground_nudge_collision = ft_80084280,
    .air_special_landing_collision = ft_800831CC,
    .fall_special_ledge_check = ftCo_80096CC8,
    .enter_landing_fall_special = ftCo_LandingFallSpecial_Enter,
    .update_facing = ftCommon_UpdateFacing,
    .set_root_rotation_y = ftPartSetRotY,
    .set_root_rotation_x = ftPartSetRotX,
    .set_root_rotation_z = ftPartSetRotZ,
    .align_root_to_floor = ftCommon_8007E5AC,
    .angle_atan2 = atan2,
    .resolve_bone = ftParts_GetBoneIndex,
    .joint_position = lb_8000B1CC,
    .spawn_projectile = spawn_projectile,
    .spawn_authored_projectile = spawn_authored_projectile,
    .spawn_attached_article = spawn_attached_article,
    .spawn_bone_attached_article = spawn_bone_attached_article,
    .owned_article_exists = owned_article_exists,
    .remove_owned_article = remove_owned_article,
    .pause_owned_article = pause_owned_article,
    .resume_owned_article = resume_owned_article,
    .signal_owned_article = signal_owned_article,
    .spawn_effect = efSync_Spawn,
    .spawn_model_effect = nativeModelEffect_Spawn,
    .model_effect_joint = nativeModelEffect_Joint,
    .model_effect_creator = nativeModelEffect_Creator,
    .model_effect_cancel = nativeModelEffect_Cancel,
    .multi_jump_vertical_speed = nativeFighterJump_MultiVerticalSpeed,
    .configure_multi_jump = nativeFighterJump_ConfigureMulti,
    .basic_landing_velocity_threshold = ftCo_800D0EC8,
    .air_drift_with_limits = ft_80084E1C,
    .aerial_jump_actions = ftCo_JumpAerial_IASA,
    .enter_fall_aerial = ftCo_FallAerial_Enter,
    .model_joint_resolve = nativeModelJoint_Resolve,
    .model_joint_read = nativeModelJoint_Read,
    .model_joint_write = nativeModelJoint_Write,
    .spawn_wind = lb_800119DC,
    .spawn_wind_at_fighter = spawn_wind_at_fighter,
    .spawn_effect_between_parts = spawn_effect_between_parts,
    .clear_effects = ftCommon_8007DB24,
    .destroy_effects = efLib_DestroyAll,
    .spawn_effect_on_part = spawn_effect_on_part,
    .spawn_effect_with_facing = spawn_effect_with_facing,
    .align_velocity_to_facing = ftCommon_8007F76C,
    .mark_effect_active = mark_effect_active,
    .set_hitbox_damage = ftColl_8007ABD0,
    .clear_hitbox_victims = lbColl_80008440,
    .initialize_collision = mpColl_80041EE4,
    .sweep_collision_box = mpColl_8004730C,
    .spawn_async_effect = efAsync_Spawn,
    .force_air_and_use_jumps = ftCommon_8007D60C,
    .detected_item_kind = detected_item_kind,
    .pause_effects = efLib_PauseAll,
    .resume_effects = efLib_ResumeAll,
    .create_reflector = ftColl_CreateReflectHit,
    .update_reflector = ftColl_8007AEF8,
    .copy_ext_attributes = copy_ext_attributes,
    .reload_ext_attributes = reload_ext_attributes,
    .set_visibility_group = ftParts_80074A4C,
    .set_current_visibility_group = ftParts_80074B0C,
    .costume_variant = nativeCostumeRuntime_ModelVariant,
    .apply_part_animation = ftAnim_ApplyPartAnim,
    .random_float = HSD_Randf,
    .random_int = random_int,
    .random_int_excluding = random_int_excluding,
    .animation_velocity_y = ft_800851C0,
    .animation_velocity_y_adjusted = ft_800851D0,
    .animation_velocity_x = ft_80085134,
    .check_ground_and_ledge = ft_CheckGroundAndLedge,
    .check_cliff_catch = ftCliffCommon_80081298,
    .enter_cliff_catch = ftCliffCommon_80081370,
    .capture_attach_item = nativeCaptureItem_Attach,
    .capture_release_item = nativeCaptureItem_Release,
    .capture_release_into_enclosure = nativeCapture_ReleaseIntoEnclosure,
    .create_unowned_article_at = nativeArticle_CreateUnowned,
    .arm_grab = ftCommon_8007E2D0,
    .begin_capture = nativeFighterCapture_Begin,
    .set_capture_mask = ftCommon_8007E2F4,
    .clear_capture_movement = ftCommon_8007E2FC,
    .capture_victim_to_air = ftCo_800DB368,
    .follow_capture_victim = nativeFighterCapture_FollowVictim,
    .release_capture_throw = nativeFighterCapture_ReleaseThrow,
    .enter_landing_basic = ftCo_Landing_Enter_Basic,
    .aerial_attack_actions = ftCo_800CB870,
    .spawn_holdable_item = nativeHoldable_Spawn,
    .self_damage = nativeFighterDamage_Self,
    .fighter_item_pickup = Fighter_OnItemPickup,
    .fighter_item_drop = Fighter_OnItemDrop,
    .fighter_item_visible = Fighter_OnItemVisible,
    .fighter_item_invisible = Fighter_OnItemInvisible,
    .fighter_knockback_enter = Fighter_OnKnockbackEnter,
    .fighter_knockback_exit = Fighter_OnKnockbackExit,
    .item_gravity = Item_ApplyFallingPhysics,
    .item_spawn_food = nativeHoldable_SpawnStockFood,
    .item_land = item_land,
    .item_bounce = itColl_BounceOffVictim,
    .item_change_state = item_change_state,
    .item_advance_animation = Item_802694CC,
    .item_pause = it_8026B724,
    .item_resume = it_8026B73C,
    .item_remove = Item_8026A8EC,
    .item_scale_by_attribute = it_8026BAE8,
    .item_set_instance_scale = it_80274484,
    .item_respond_stage_mask = it_80276FC4,
    .item_spawn_directional_effect = nativeFighterGeometry_ItemEffect,
    .notify_item_pickup = ftpickupitem_80094818,
    .bone_segment_angle = nativeFighterGeometry_BoneSegmentAngle,
    .item_set_lifetime = it_80275158,
    .item_reflect = it_80273030,
    .item_stage_collision = it_8026D9A0,
    .item_bounce_stage = it_8027781C,
    .item_shield_bounce = itColl_BounceOffShield,
    .item_play_sound = item_play_sound,
    .item_loop_sound = Item_8026AF0C,
    .item_fall = it_80272860,
    .item_spawn_effect_on_bone = item_spawn_effect_on_bone,
    .item_activate_attached = item_activate_attached,
    .fighter_model_scale = ftLib_800869D4,
    .fighter_root_scale_y = fighter_root_scale_y,
    .fighter_incapacitated = ftCo_800BF228,
    .item_set_uniform_scale = item_set_uniform_scale,
    .item_set_child_visible = item_set_child_visible,
    .item_interact = it_8026B894,
    .item_joint_value = nativeArticleModel_Read,
    .item_sweep_stage = it_8026E9A4,
    .vector_mirror = lbVector_Mirror,
    .wrap_angle = nativeScriptMath_WrapAngle,
    .item_set_joint_value = nativeArticleModel_Write,
    .item_segment_sweep = it_8026EA20,
    .stage_line_exists = mpLib_80054ED8,
    .set_collision_line = mpColl_80043558,
    .stage_line_speed = mpGetSpeed,
    .stage_line_normal = mpLineGetNormal,
    .item_set_animation_frame = Item_80268DD4,
    .item_set_hitbox_damage = it_80272460,
    .item_spawn_effect = it_80278800,
    .item_spawn_joint_effect_at = nativeArticleModel_SpawnEffectAt,
    .item_contact_fighter = nativeFighterTarget_Contact,
    .item_creator = nativeArticle_Creator,
    .item_current_owner = nativeFighterTarget_CurrentOwner,
    .item_reflector = nativeFighterTarget_Reflector,
    .target_camera_target_position = nativeFighterTarget_CameraPosition,
    .create_absorber = ftColl_CreateAbsorbHit,
    .heal_damage = nativeFighterDamage_Heal,
    .item_add_animation_descriptor = nativeArticle_AddAnimationDescriptor,
    .item_read_attachment_matrix_rotation = nativeArticle_ReadAttachmentMatrixRotation,
    .check_guard = ftCo_80091A4C,
    .check_dash = ftCo_Dash_CheckInput,
    .check_air_special = ftCo_SpecialAir_CheckInput,
    .target_is_fighter = nativeFighterTarget_IsFighter,
    .target_airborne = nativeFighterTarget_Airborne,
    .spawn_food = it_8028FAF4,
    .article_ref_current_owner = nativeArticleRef_CurrentOwner,
    .target_smash_charge_active = nativeFighterTarget_SmashChargeActive,
    .enter_jab = ftCo_Attack11_EnterMotion,
    .enter_rapid_jab = ftCo_800D6B00,
    .rapid_jab_should_finish = ftCo_RapidJab_ShouldFinish,
    .try_ground_pickup = ftpickupitem_80094790,
    .air_aerial_landing_collision = ft_80082C74,
    .enter_landing_aerial = ftCo_LandingAir_EnterWithMsidLag,
    .debug_level = &DbLevel,
    .item_joint_world_position = nativeArticleModel_ReadWorldPosition,
    .set_fighter_diffuse_color = ftMaterial_800BFB4C,
    .set_item_diffuse_color = it_80278574,
    .target_controller_color = nativeFighterTarget_ControllerColor,
    .install_outline_visibility = nativeFighterRuntime_InstallOutlineVisibility,
    .set_item_layered = articleModelLayers_Bind,
    .item_disable_hitbox = it_80272560,
    .target_guard_active = nativeFighterTarget_GuardActive,
    .target_model_scale = nativeFighterTarget_ModelScale,
    .target_instance_scale_y = nativeFighterTarget_InstanceScaleY,
    .target_motion = nativeFighterTarget_Motion,
    .target_held_id = nativeFighterTarget_HeldId,
    .target_held_kind = nativeFighterTarget_HeldKind,
    .target_held_motion = nativeFighterTarget_HeldMotion,
    .target_held_frame = nativeFighterTarget_HeldFrame,
    .target_shield = nativeFighterTarget_Shield,
    .spawn_article = spawn_article,
    .get_bone_rotation_x = ftPartGetRotX,
    .item_inherit_owner_scale = it_80274594,
    .item_use_manual_lifetime = it_8026B3A8,
    .ground_animation_motion = ft_80085088,
    .item_load_model = nativeArticleModel_Load,
    .item_add_model_animation = nativeArticleModel_AddAnimation,
    .item_request_model_animation = nativeArticleModel_RequestAnimation,
    .item_advance_model_animation = nativeArticleModel_AdvanceAnimation,
    .item_remove_model_animation = nativeArticleModel_RemoveAnimation,
    .item_set_model_animation_rate = nativeArticleModel_SetAnimationRate,
    .item_read_joint_matrix_rotation = nativeArticleModel_ReadMatrixRotation,
    .item_set_joint_transform = nativeArticleModel_SetTransform,
    .item_clone_model = nativeArticleModel_Clone,
    .item_destroy_model = nativeArticleModel_Destroy,
    .sin_float = sinf,
    .cos_float = cosf,
    .abs_float = fabsf,
    .sqrt_float = sqrtf,
    .min_float = fminf,
    .max_float = fmaxf,
    .set_hurtboxes = ftColl_8007B0C0,
    .set_body_status = ftColl_8007B62C,
    .set_group_material_frame = nativeFighterVisuals_SetGroupMaterialFrame,
    .bone_count = bone_count,
    .held_item_kind = itGetKind,
    .throw_held_item = ftCo_800957F4,
    .create_held_article_at_center = nativeArticle_CreateHeldAtCenter,
    .item_dynamic_joint_value = nativeArticleModel_ReadDynamic,
    .item_set_dynamic_joint_value = nativeArticleModel_WriteDynamic,
    .item_set_animation_descriptor = nativeArticle_SetAnimationDescriptor,
    .item_sweep_stage_flags = it_8026DAA8,
    .item_wall_contact_flags = it_80276308,
    .item_ceiling_contact_flags = it_802763E0,
    .item_align_to_floor = it_80276CB8,
    .item_reflect_from_wall = it_8027770C,
    .item_reset_default_ecb = it_80275D5C,
    .item_set_grounded = it_802762B0,
    .item_set_airborne = it_802762BC,
    .enter_damage_fall = ftCo_80090780,
    .item_bounce_stage_to_ground = it_8026E15C,
    .set_collision_facing = mpCollSetFacingDir,
    .set_bone_hurtbox = ftColl_8007B128,
    .set_held_item_visible = ftCommon_8007F5CC,
    .unlock_ecb = ftCommon_UnlockECB,
    .vector_normalize = lbVector_Normalize,
    .vector_cross_normalized = lbVector_CrossprodNormalized,
    .stage_internal_kind = Stage_80225194,
    .item_release_camera = Item_80267454,
    .vector_angle = lbVector_Angle,
    .vector_rotate_unit_axis = lbVector_RotateAboutUnitAxis,
    .enter_freefall_blend = ftCo_800969D8,
    .enter_down_bound = ftCo_80097D40,
    .stage_line_island_bounds = stage_line_island_bounds,
    .wrap_signed_angle = nativeScriptMath_WrapSignedAngle,
    .item_disable_hitboxes = it_802725D4,
    .initialize_attached_capture = ftCo_InitializeAttachedCapture,
    .enter_attached_capture_motion = ftCo_EnterAttachedCaptureMotion,
    .enter_attached_throw = ftCo_EnterAttachedThrow,
    .unlink_capture = ftCo_800DC920,
    .apply_throw_damage = ftCo_800DE2A8,
    .launch_thrown_victim = ftCo_800DE7C0,
    .ground_animation_facing_motion = ft_800850B4,
    .initialize_group_material_animation = nativeFighterVisuals_InitializeGroupMaterialAnimation,
    .current_model_group = nativeFighterVisuals_CurrentGroup,
    .previous_model_group = nativeFighterVisuals_PreviousGroup,
    .install_model_graph = nativeFighterModelGraph_Install,
    .capture_enclose_caught = nativeCapture_EncloseCaught,
    .constrain_caught = ftCo_800DB368,
    .add_joint_rotation_y = HSD_JObjAddRotationY,
    .set_joint_translation = HSD_JObjSetTranslate,
    .copy_bone_pose = ftAnim_8006FA58,
    .pose_graph = nativeFighterRuntime_PoseGraph,
    .pose_clip = nativeFighterRuntime_PoseClip,
    .pose_validate = nativeFighterPose_Validate,
    .load_pose_animation = ftAnim_8006F4C8,
    .request_pose_frame = ftAnim_80070710,
    .copy_blend_pose = ftAnim_8006FB88,
    .blend_graph_live = ftAnim_80070010,
    .blend_graph_alternate = ftAnim_80070108,
    .blend_pose_to_live = ftAnim_8006FE9C,
    .copy_pose_to_live = ftAnim_8006FF74,
    .find_costume_pose = ftAnim_8006F994,
    .create_item_at_position = nativeArticle_CreateAtPosition,
    .item_hits_stage = it_8026E058,
    .item_land_on_stage = it_8026E414,
    .install_guard = ftCo_80092450,
    .install_guard_reflector = ftCo_8009370C,
    .consume_guard_release_input = ftCo_80092BCC,
    .drain_guard = ftCo_800925A4,
    .tick_powershield = ftCo_80093BC0,
    .guard_sdi = ftCo_80093240,
    .guard_post_sdi = ftCo_800932DC,
    .enter_guard_hold = ftCo_800928CC,
    .enter_guard_release = ftCo_80092BE8,
    .scale_guard_joint = ftCo_80091D58,
    .guard_reflect_actions = ftCo_GuardReflect_IASA,
    .guard_physics = ftCo_Guard_Phys,
    .guard_hit_physics = ftCo_GuardSetOff_Phys,
    .guard_collision = ftCo_Guard_Coll,
    .guard_hit_collision = ftCo_GuardSetOff_Coll,
    .check_powershield = ftCo_80093694,
    .check_guard_item_throw = ftCo_8009515C,
    .check_guard_platform_drop = ftCo_8009A080,
    .update_aerial_jump = ftCo_JumpAerial_Anim,
};

bool nativeFighterModule_SetParameters(
    NativeFighterModule* module,
    const NativeFighterParameters* parameters)
{
    if (module == NULL || module->library == NULL || parameters == NULL ||
        parameters->count > NATIVE_FIGHTER_PARAMETER_CAPACITY)
        return false;
    typedef int (__cdecl* Setter)(const uint32_t*, unsigned, uint64_t);
    Setter setter = (Setter) GetProcAddress(
        (HMODULE) module->library, "FreakMelee_SetFighterParameters");
    uint32_t values[NATIVE_FIGHTER_PARAMETER_CAPACITY];
    for (unsigned i = 0; i < parameters->count; ++i)
        values[i] = parameters->fields[i].bits;
    bool valid = parameters->count == 0 ? setter == NULL :
        setter != NULL && setter(values, parameters->count,
                                 parameters->schema_hash) != 0;
    if (!valid) return false;
    module->parameter_count = parameters->count;
    module->parameter_schema = parameters->schema_hash;
    memcpy(module->parameter_bits, values, parameters->count * sizeof(values[0]));
    module->parameters_bound = true;
    return true;
}

bool nativeFighterModule_Open(const char* path,
                              const NativeFighterDefinition* base,
                              NativeFighterModule* module)
{
    if (path == NULL || module == NULL || *path == 0 ||
        module->library != NULL) return false;
    module->definition = NULL;
    char absolute[4096];
    DWORD length = GetFullPathNameA(path, sizeof(absolute), absolute, NULL);
    if (length == 0 || length >= sizeof(absolute)) return false;
    HMODULE library = LoadLibraryExA(
        absolute, NULL,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (library == NULL) return false;
    LayoutEntry layout = (LayoutEntry) GetProcAddress(
        library, "FreakMelee_GetFighterModuleLayout");
    if (layout == NULL ||
        layout() != nativeFighterModule_LayoutFingerprint()) {
        FreeLibrary(library);
        return false;
    }
    fighter_host.angle_atan2_float = atan2f;
    DefinitionEntry entry = (DefinitionEntry) GetProcAddress(
        library, "FreakMelee_GetFighterModule");
    const NativeFighterDefinition* definition = entry != NULL
        ? entry(base, &fighter_host) : NULL;
    bool valid = definition != NULL && nativeModelEffect_Validate(definition) &&
                 nativeFighterSoundPolicy_Validate(definition->sound_policy) &&
                 definition->sound_mute_channels <= 0x7F &&
                 definition->code_key != NULL &&
                 strnlen(definition->code_key,
                         NATIVE_FIGHTER_CODE_KEY_BYTES) > 0 &&
                 strnlen(definition->code_key,
                         NATIVE_FIGHTER_CODE_KEY_BYTES) <
                     NATIVE_FIGHTER_CODE_KEY_BYTES &&
                 definition->fighter_root != NULL &&
                 definition->costume_joint != NULL &&
                 definition->costume_matanim != NULL &&
                 definition->motion_states != NULL &&
                 definition->motion_state_count > 0 &&
                 definition->state_bytes <=
                     sizeof(((Fighter*) 0)->native_module_state) &&
                 ((definition->items == NULL) ==
                  (definition->item_count == 0)) &&
                 ((definition->article_logics == NULL) ==
                  (definition->article_logic_count == 0)) &&
                 definition->article_logic_count <= 64;
    if (valid) valid = nativeFighterEvents_Validate(definition);
    if (valid)
        for (uint16_t i = 0; i < definition->article_logic_count; ++i)
            if (definition->article_logics[i].resource_capacity && !nativeArticleResource_ValidateRecipe(
                    definition->article_logics[i].resource_capacity,
                    definition->article_logics[i].resource_reserved_part,
                    definition->joint_count + definition->omission_count)) {
                valid = false;
                break;
            }
    if (valid)
        valid = nativeFighterState_Validate(
            definition->state_fields, definition->state_field_count,
            definition->state_bytes,
            sizeof(((Fighter*) 0)->native_module_state));
    if (valid)
        for (uint16_t i = 0; i < definition->article_logic_count; ++i) {
            const NativeFighterArticleLogic* article =
                &definition->article_logics[i];
            if (!nativeFighterState_Validate(
                    article->state_fields, article->state_field_count,
                    article->local_state_bytes,
                    sizeof(((Item*) 0)->xDD4_itemVar.native_article.state)) ||
                article->logic == NULL || article->logic->states == NULL ||
                article->state_count == 0 ||
                article->signal_count > NATIVE_FIGHTER_ARTICLE_SIGNAL_LIMIT ||
                ((article->signal_count == 0) != (article->signals == NULL)) ||
                article->model_animation_count > NATIVE_FIGHTER_ARTICLE_MODEL_ANIMATION_LIMIT ||
                ((article->model_animation_count == 0) != (article->model_animations == NULL)) ||
                (article->projectile != NULL &&
                 article->projectile->item_state >= article->state_count)) {
                valid = false;
                break;
            }
            for (unsigned s = 0; s < article->signal_count; ++s) {
                if (article->signals[s].callback == NULL) valid = false;
                for (unsigned p = 0; p < s; ++p)
                    if (article->signals[p].name_hash == article->signals[s].name_hash)
                        valid = false;
            }
            for (unsigned a = 0; a < article->model_animation_count; ++a) {
                const NativeFighterArticleModelAnimation* animation = &article->model_animations[a];
                if (animation->name_hash == 0) valid = false;
                for (unsigned c = 0; c < 3; ++c) {
                    const NativeFighterArticleAnimationComponent* component = &animation->components[c];
                    if ((component->attribute_offset != UINT32_MAX &&
                        ((component->attribute_offset & 3) ||
                         component->attribute_offset > FT_CHARACTER_EXT_ATTR_CAPACITY - 4)) ||
                        (component->array_index != UINT32_MAX &&
                         (component->attribute_offset == UINT32_MAX || component->array_index >= UINT32_MAX / 4)))
                        valid = false;
                }
                for (unsigned p = 0; p < a; ++p)
                    if (article->model_animations[p].name_hash == animation->name_hash)
                        valid = false;
            }
            for (uint16_t j = 0; j < i; ++j)
                if (definition->article_logics[j].kind == article->kind)
                    valid = false;
            if (!valid) break;
        }
    if (!valid) {
        FreeLibrary(library);
        return false;
    }
    module->library = library;
    module->definition = definition;
    module->base = base;
    return true;
}

bool nativeFighterModule_OpenCached(const char* root, const NativeContentId* id,
                                    unsigned member_index,
                                    const uint8_t* bytes, size_t size,
                                    const NativeFighterDefinition* base,
                                    NativeFighterModule* module)
{
    if (root == NULL || id == NULL || bytes == NULL || size < 32 ||
        size > 0xFFFFFF || member_index >= NATIVE_FIGHTER_FAMILY_MEMBERS ||
        module == NULL || module->library != NULL) {
        return false;
    }
    char directory[MAX_PATH];
    int length = snprintf(directory, sizeof(directory), "%s\\modules", root);
    if (length <= 0 || length >= sizeof(directory) ||
        (!CreateDirectoryA(directory, NULL) &&
         GetLastError() != ERROR_ALREADY_EXISTS)) return false;
    char hex[65];
    nativeContentStore_IdHex(id, hex);
    char path[MAX_PATH];
    length = snprintf(path, sizeof(path), "%s\\%s-member%u.dll", directory, hex, member_index);
    if (length <= 0 || length >= sizeof(path)) return false;

    HANDLE file = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file != INVALID_HANDLE_VALUE) {
        size_t offset = 0;
        bool written = true;
        while (written && offset < size) {
            DWORD count = 0;
            DWORD wanted = (DWORD) (size - offset);
            written = WriteFile(file, bytes + offset, wanted, &count, NULL) &&
                      count != 0;
            offset += count;
        }
        written = written && FlushFileBuffers(file);
        written = CloseHandle(file) && written;
        if (!written) {
            DeleteFileA(path);
            return false;
        }
    } else if (GetLastError() != ERROR_FILE_EXISTS) {
        return false;
    }

    file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER length_on_disk;
    bool equal = GetFileSizeEx(file, &length_on_disk) &&
                 length_on_disk.QuadPart == (LONGLONG) size;
    uint8_t block[4096];
    size_t offset = 0;
    while (equal && offset < size) {
        DWORD wanted = (DWORD) (size - offset);
        if (wanted > sizeof(block)) wanted = sizeof(block);
        DWORD count = 0;
        equal = ReadFile(file, block, wanted, &count, NULL) &&
                count == wanted &&
                memcmp(block, bytes + offset, wanted) == 0;
        offset += count;
    }
    equal = CloseHandle(file) && equal;
    return equal && nativeFighterModule_Open(path, base, module);
}

void nativeFighterModule_Close(NativeFighterModule* module)
{
    if (module == NULL) return;
    if (module->library != NULL) FreeLibrary((HMODULE) module->library);
    *module = (NativeFighterModule){ 0 };
}

#endif
