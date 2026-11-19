#include "fighter.h"
#include <melee/ft/native_fighter_target.h>
#include <melee/ft/native_callback_lifetime.h>
#include "ftcommon_assets.h"

#include <math.h>
#include <string.h>
#include <placeholder.h>
#ifdef MELEE_NATIVE
#include <malloc.h>
#include <melee/ft/native_costume_runtime.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/it/native_article_resource.h>
#include <melee/it/native_item_owner.h>
#include <melee/gm/native_profile_input.h>
#include <melee/gm/native_replay.h>
#define NATIVE_ASSET_ROLE_PLCO_SOURCE UINT64_C(0x504C434F42595445)
#define NATIVE_ASSET_ROLE_PLCO_ARENA UINT64_C(0x504C434F4152454E)
#endif

#include "ft_07C1.h"
#include "ft_07C6.h"
#include "ft_0819.h"
#include "ft_081B.h"
#include "ft_0877.h"
#include "ft_0881.h"
#include "ft_0892.h"
#include "ft_0899.h"
#include "ft_0C31.h"
#include "ft_0C88.h"
#include "ft_0C8C.h"
#include "ft_0D31.h"
#include "ft_0DF0.h"
#include "ftaction.h"
#include "ftafterimage.h"
#include "ftanim.h"
#include "ftcamera.h"
#include "ftchangeparam.h"
#include "ftCo_800C7CA0.h"
#include "ftcolanim.h"
#include "ftcoll.h"
#include "ftcommon.h"
#include "ftdata.h"
#include "ftdevice.h"
#include "ftdrawcommon.h"
#include "ftdynamics.h"
#ifdef MELEE_NATIVE
#include "native_fighter_pool.h"
#endif
#include "ftlib.h"
#include "ftmetal.h"
#include "ftparts.h"
#include "kinds/ftCommon/ftCo_09F4.h"
#include "kinds/ftCommon/ftCo_0A01.h"
#include "kinds/ftCommon/ftCo_0C35.h"
#include "kinds/ftCommon/ftCo_Attack100.h"
#include "kinds/ftCommon/ftCo_Bury.h"
#include "kinds/ftCommon/ftCo_Damage.h"
#include "kinds/ftCommon/ftCo_DamageFall.h"
#include "kinds/ftCommon/ftCo_FallSpecial.h"
#include "kinds/ftCommon/ftCo_HammerWait.h"
#include "kinds/ftCommon/ftCo_ItemThrow.h"
#include "kinds/ftCommon/ftCo_Jump.h"
#include "kinds/ftCommon/ftCo_KinokoGiantEnd.h"
#include "kinds/ftCommon/ftCo_KinokoGiantStart.h"
#include "kinds/ftCommon/ftCo_KinokoSmallEnd.h"
#include "kinds/ftCommon/ftCo_KinokoSmallStart.h"
#include "kinds/ftCommon/ftCo_LandingAir.h"
#include "native_ucf.h"
#include "kinds/ftCommon/ftCo_Rebound.h"
#include "kinds/ftCommon/ftCo_ShieldBreakFly.h"
#include "kinds/ftCommon/ftCo_SpecialS.h"
#include "kinds/ftCrazyHand/ftcrazyhandwait10.h"
#include "kinds/ftKirby/ftkirby.h"
#include "kinds/ftMasterHand/ftmasterhandwait10.h"
#include "kinds/ftPeach/types.h"
#include "types.h"
#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#include <melee/cm/camera.h>
#include <melee/db/db.h>
#include <melee/ef/efasync.h>
#include <melee/ef/eflib.h>
#include <melee/gm/gm_unsplit.h>
#include <melee/gm/native_asset_identity.h>
#include <melee/gr/ground.h>
#include <melee/gr/stage.h>
#include <melee/if/ifmagnify.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_279C.h>
#include <melee/it/item.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lb_00CE.h>
#include <melee/lb/lbanim.h>
#include <melee/lb/lbarchive.h>
#include <melee/lb/lbfile.h>
#include <melee/lb/lbheap.h>
#include <melee/lb/lbshadow.h>
#include <melee/lb/types.h>
#include <melee/mp/mpcoll.h>
#include <melee/mp/mplib.h>
#include <melee/pl/pl_040D.h>
#include <melee/pl/player.h>
#include <melee/pl/plbonuslib.h>
#include <melee/pl/pltrick.h>
#include <melee/sfx/crowdsfx.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/random.h>

/// ==== fighter.c variables ====
/// =============================

const Vec3 Fighter_803B7488 = { 0.0f, 0.0f, 0.0f };
const Vec3 vec3_803B7494 = { 0.0f, 0.0f, 0.0f };

#ifndef MELEE_NATIVE
HSD_ObjAllocData fighter_alloc_data;
HSD_ObjAllocData fighter_dat_attrs_alloc_data;
HSD_ObjAllocData fighter_parts_alloc_data;
HSD_ObjAllocData fighter_dobj_list_alloc_data;
#endif
HSD_ObjAllocData fighter_x2040_alloc_data;
#ifndef MELEE_NATIVE
HSD_ObjAllocData fighter_x59C_alloc_data;
#endif

/// @todo verify that this is really a spawn number counter, then rename this
/// var globally
u32 Fighter_804D64F8 = 0;
#define g_spawnNumCounter Fighter_804D64F8

/// the following seems to be an array, initialized in reverse in
struct Fighter_804D64FC_t* Fighter_804D64FC = NULL;
CrowdConfig* gCrowdConfig = NULL;
HSD_Joint* Fighter_804D6504 = NULL;
u8* Fighter_804D6508 = NULL;
u8* Fighter_804D650C = NULL;
UNK_T Fighter_804D6510 = NULL;
HSD_Joint* Fighter_804D6514 = NULL;
struct Fighter_804D6518_t* Fighter_804D6518 = NULL;
struct Fighter_804D651C_t* Fighter_804D651C = NULL;
struct Fighter_804D6520_t* Fighter_804D6520 = NULL;
struct Fighter_804D6524_t* Fighter_804D6524 = NULL;
struct Fighter_ShakeTable_t* Fighter_SmashChargeShakeTable = NULL;
struct Fighter_ShakeTable_t* Fighter_GrabMashShake = NULL;
struct Fighter_DamageShiftTable* Fighter_804D6530 = NULL;
struct Fighter_RespawnPlatform* Fighter_804D6534 = NULL;
struct Fighter_804D653C_t* Fighter_804D6538 = NULL;
struct Fighter_804D653C_t* Fighter_804D653C = NULL;

float* Fighter_804D6548 = NULL;
float (*Fighter_804D654C)[5] = NULL;
float* Fighter_804D6550 = NULL;


void Fighter_800679B0(void)
{
    s32 i;

    /// @warning don't hardcode the allocation sizes
#ifdef MELEE_NATIVE
    HSD_ASSERTREPORT(0,nativeFighterPool.lifetime!=0 && nativeFighterPool.used==0,
        "Fighter family initialization requires an active empty scene pool.\n");
#else
    HSD_ObjAllocInit(&fighter_alloc_data, sizeof(Fighter), /*align*/ 4);
    HSD_ObjAllocInit(&fighter_dat_attrs_alloc_data, /*size*/ 0x424,
                     /*align*/ 4);
#endif
    ft_800852B0();
    Fighter_LoadCommonData();
    ft_8008549C();
    ftCo_8009F4A4();
    ftCo_800C8064();
    ftCo_800C8F6C(); ///< @todo &fighter_alloc_data+2, +3, +4 are not defined
                     ///< in the fighter.s
    // data section, how does this work?
#ifndef MELEE_NATIVE
    HSD_ObjAllocInit(&fighter_parts_alloc_data,
                     /*size*/ MAX_FT_PARTS * sizeof(struct FighterBone),
                     /*align*/ 4);
    HSD_ObjAllocInit(&fighter_dobj_list_alloc_data,
                     /*size*/ (0x1f0 / 4) * sizeof(HSD_DObj*), /*align*/ 4);
#endif
    HSD_ObjAllocInit(&fighter_x2040_alloc_data,
                     /*size*/ (0x80 / 4) * sizeof(HSD_DObj*), /*align*/ 4);

    g_spawnNumCounter = 1;

    for (i = 0; i < Ft_Kind_RuntimeCapacity; i++) {
        if (ftData_Table_Unk1[i]) {
            ftData_Table_Unk1[i]();
        }
    }
}

void Fighter_FirstInitialize_80067A84(void)
{
    Fighter_800679B0();
#ifndef MELEE_NATIVE
    HSD_ObjAllocInit(&fighter_x59C_alloc_data, 0x8000, 0x20);
#endif
}

void Fighter_LoadCommonData(void)
{
#ifdef MELEE_NATIVE
    static FtCommonAssets assets;
    static bool loaded;
    if (!loaded) {
        size_t length = lbFileGetSize("PlCo.dat");
        void* bytes = _aligned_malloc(OSRoundUp32B(length), 32);
        enum { ARENA_SIZE = 16 * 1024 * 1024 };
        void* arena = _aligned_malloc(ARENA_SIZE, 32);
        HSD_ASSERT(0, bytes != NULL && arena != NULL);
        lbFile_8001668C("PlCo.dat", bytes, &length);
        HSD_ASSERTREPORT(0,
                         ftCommonAssets_Import(bytes, length, arena,
                                               ARENA_SIZE, 32768, &assets),
                         "Invalid native fighter common DAT");
        uint64_t content_hash = nativeAssetIdentity_Hash(bytes, length);
        HSD_ASSERTREPORT(
            0,
            nativeAssetIdentity_Register(
                bytes, OSRoundUp32B(length),
                nativeAssetIdentity_Derive(content_hash,
                                           NATIVE_ASSET_ROLE_PLCO_SOURCE)),
            "Could not register PlCo.dat source identity");
        HSD_ASSERTREPORT(
            0,
            nativeAssetIdentity_Register(
                arena, ARENA_SIZE,
                nativeAssetIdentity_Derive(content_hash,
                                           NATIVE_ASSET_ROLE_PLCO_ARENA)),
            "Could not register PlCo.dat arena identity");
        assets.arena = arena;
        assets.owned_bytes = bytes;
        loaded = true;
    }
    p_ftCommonData = &assets.common;
    Fighter_804D6550 = assets.item_throw_attributes;
    Fighter_804D654C = assets.swing_multipliers;
    Fighter_804D6548 = assets.stale_move_penalties;
    ftPartsTable = assets.parts;
    Fighter_804D6540 = assets.omissions;
    Fighter_804D653C = assets.color_entries;
    Fighter_804D6538 = assets.special_color_entries;
    Fighter_804D6534 = &assets.respawn_platform;
    Fighter_804D6530 = assets.damage_shift_tables;
    Fighter_GrabMashShake = &assets.grab_mash_shake;
    Fighter_SmashChargeShakeTable = &assets.smash_charge_shake;
    Fighter_804D6524 = &assets.scale_modifiers;
    Fighter_804D6520 = &assets.bunny_modifiers;
    Fighter_804D651C = &assets.metal_modifiers;
    Fighter_804D6518 = &assets.zako_modifiers;
    gCrowdConfig = &assets.crowd_config;
    Fighter_804D6514 = assets.entry_platform;
    Fighter_804D6510 = assets.x6510_colors;
    Fighter_804D650C = assets.x650C_colors;
    Fighter_804D6508 = assets.x6508_colors;
    Fighter_804D6504 = assets.metal_model;
    Fighter_804D64FC = &assets.cpu_data.descriptor;
#else
    void** pData;
    lbArchive_LoadSymbols("PlCo.dat", (void**) &pData, "ftLoadCommonData", 0);

    // copy 23 4-byte chunks from pData to p_ftCommonData in reverse order,
    // equivalent to this: for(i=0; i<23; i++)
    //   (&Fighter_804D64FC)[23-1-i] = pData[i];
    // loop unrolling doesn't work (only up to 8 elements)
    p_ftCommonData = pData[0]; // p_ftCommonData
    Fighter_804D6550 = pData[1];
    Fighter_804D654C = pData[2];
    Fighter_804D6548 = pData[3];
    ftPartsTable = pData[4];
    Fighter_804D6540 = pData[5];
    Fighter_804D653C = pData[6];
    Fighter_804D6538 = pData[7];
    Fighter_804D6534 = pData[8];
    Fighter_804D6530 = pData[9];
    Fighter_GrabMashShake = pData[10];
    Fighter_SmashChargeShakeTable = pData[11];
    Fighter_804D6524 = pData[12];
    Fighter_804D6520 = pData[13];
    Fighter_804D651C = pData[14];
    Fighter_804D6518 = pData[15];
    Fighter_804D6514 = pData[16];
    Fighter_804D6510 = pData[17];
    Fighter_804D650C = pData[18];
    Fighter_804D6508 = pData[19];
    Fighter_804D6504 = pData[20];
    gCrowdConfig = pData[21];
    Fighter_804D64FC = pData[22];
#endif
}

void Fighter_UpdateModelScale(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    HSD_JObj* jobj = GET_JOBJ(gobj);
    Vec3 scale;
    float modelScale = ftCommon_GetModelScale(fp);

    if (fp->x34_scale.z != 1.0f) {
        scale.x = fp->x34_scale.z;
    } else {
        scale.x = modelScale;
    }

    scale.y = modelScale;
    scale.z = modelScale;

    HSD_JObjSetScale(jobj, &scale);
}

void Fighter_UnkInitReset_80067C98(Fighter* fp)
{
    Vec3 player_coords;
    float x, y, z;

    fp->x8_spawnNum = Fighter_NewSpawn_80068E40();
#ifdef MELEE_NATIVE
    nativeScriptFighter_NewLife(fp->gobj,(uint32_t)fp->x8_spawnNum);
#endif
    Player_LoadPlayerCoords(fp->player_id, &player_coords);
    fp->facing_dir = Player_GetFacingDirection(fp->player_id);

    player_coords.x = fp->facing_dir * ftCommon_800804EC(fp) + player_coords.x;
    x = player_coords.x;
    fp->cur_pos.x = x;
    fp->prev_pos.x = x;

    y = player_coords.y;
    fp->cur_pos.y = y;
    fp->prev_pos.y = y;

    z = player_coords.z;
    fp->cur_pos.z = z;
    fp->prev_pos.z = z;

    fp->facing_dir1 = fp->facing_dir;
    fp->x34_scale.y = fp->x34_scale.x;

    fp->x2220_b5 = 0;
    fp->x2220_b6 = 0;

    fp->x200C = 0;
    fp->x2010 = 0;
    fp->x2008 = 0;

    fp->x2219_b1 = 0;
    fp->x2219_b2 = 0;
    fp->x2219_b3 = 0;
    fp->x2219_b4 = 0;
    fp->x221A_b5 = 0;
    fp->x221A_b6 = 0;
    fp->x221D_b2 = 0;
    fp->x221E_b7 = 0;
    fp->x2220_b7 = 0;
    fp->x2221_b4 = false;
    fp->x2221_b5 = false;
    fp->x2221_b6 = true;
    fp->x2221_b7 = false;

    fp->x61D = 255;

    fp->pos_delta.z = 0;
    fp->pos_delta.y = 0;
    fp->pos_delta.x = 0;
    fp->cur_anim_frame = 0;
    fp->x898_unk = 0;

    fp->frame_speed_mul = 1;
    fp->x8A0_unk = 1;
    fp->dmg.kb_applied = 0;
    fp->dmg.x18A4_knockbackMagnitude = 0;
    fp->dmg.x18A8 = 0;
    fp->dmg.x18ac_time_since_hit = -1;
    fp->dmg.armor0 = 0;
    fp->dmg.armor1 = 0;
    fp->x1828 = 0;

    fp->x221C_b6 = 0;

    fp->dmg.x18a0 = 0;
    fp->x1968_jumpsUsed = 0;
    fp->x1969_walljumpUsed = 0;
    fp->hitlag_mul = 0;
    fp->x2064_ledgeCooldown = 0;

    fp->dmg.x1830_percent = Player_GetDamage(fp->player_id);

    fp->dmg.x1838_percentTemp = 0;

    fp->dmg.x183C_applied = 0;
    fp->dmg.x18C0 = 0;

    fp->dmg.x18c4_source_ply = 6;
    fp->dmg.x18C8 = -1;
    fp->dmg.x18F0 = 0;
    fp->dmg.x18CC = 0;
    fp->dmg.x18D0 = -10;

    fp->x221F_b5 = 0;
    fp->x2221_b1 = 0;

    fp->dmg.x18F4 = 0;
    fp->dmg.x18F8 = 1;
    fp->dmg.x18fa_model_shift_frames = 0;
    fp->dmg.x18FD = 0;
    fp->dmg.x18FC = 0;
    fp->dmg.x1834 = 0;

    fp->x2222_b2 = 0;

    fp->dmg.x1840 = 0;

    fp->x2219_b5 = 0;
    fp->x2219_b6 = 0;
    fp->x2219_b7 = 0;
    fp->x221A_b0 = 0;
    fp->x221A_b1 = 0;

    fp->dmg.x1954 = 0;
    fp->dmg.x1958 = 0;

    fp->allow_sdi = 0;

    fp->dmg.x195c_hitlag_frames = 0;

    fp->x221A_b3 = 0;
    fp->x1960_vibrateMult = 1;
    fp->x1964 = 0;
    fp->dmg.x189C_unk_num_frames = 0;

    fp->x2220_b3 = 0;
    fp->x2220_b4 = 0;

    fp->dmg.x1914 = 0;
    fp->dmg.int_value = 0;
    fp->dmg.x191C = 0;
    fp->dmg.x1924 = 0;
    fp->dmg.x1928 = 0;

    fp->x2223_b5 = 0;

    fp->dmg.x1950 = 0;
    fp->dmg.x1948 = 0;

    fp->x2223_b4 = 0;

    fp->xF8_playerNudgeVel.y = 0;
    fp->xF8_playerNudgeVel.x = 0;
    fp->x100 = -1;

    fp->x2222_b7 = 0;
    fp->x2223_b0 = 0;
    fp->fall_fast = 0;
    fp->x2219_b0 = 0;

    fp->x20A0_accessory = 0;
    fp->throw_flags = 0;
    fp->cmd_timer = 0;
    fp->item_gobj = 0;
    fp->x1978 = 0;

    fp->x221E_b3 = 1;

    fp->x1984_heldItemSpec = 0;
    fp->x1988 = 0;
    fp->x198C = 0;

    fp->x2221_b0 = 0;

    fp->x1990 = 0;
    fp->throw_invincibility_frames = 0;

    fp->x221D_b6 = 0;
    fp->x221B_b5 = 0;

    fp->victim_gobj = 0;
    fp->x1A5C = 0;
    fp->native_hitlag_link_generation = nativeFighterTarget_Id(fp->x1A5C);
    memset(fp->native_hitbox_lifetimes, 0, sizeof(fp->native_hitbox_lifetimes));

    fp->x221B_b6 = 0;

    fp->target_item_gobj = 0;
    fp->x1A64 = 0;

    fp->x221B_b7 = 0;
    fp->x221C_b0 = 0;

    fp->x1064_thrownHitbox.owner = NULL;
    fp->x221C_u16_y = 0;
    fp->unk_gobj = NULL;
    fp->x221C_b5 = 0;

    fp->x2150 = fp->x2154 = fp->x2158 = fp->x215C = fp->x2160 = fp->x2144 =
        fp->x2148 = fp->x214C = -1;

    fp->x2168 = 0;
    fp->x2164 = 0;
    fp->x208C = 0;
    fp->x2090 = 0;
    fp->x2098 = 0;
    fp->x2092 = 0;
    fp->x2094 = 0;
    fp->shield_health = p_ftCommonData->x260_startShieldHealth;

    fp->x221A_b7 = 0;
    fp->x221B_b0 = 0;
    fp->x221B_b1 = 0;
    fp->x221B_b3 = 0;
    fp->x221B_b4 = 0;
    fp->x221C_b3 = 0;
    fp->x221C_b1 = 0;
    fp->x221C_b2 = 0;

    fp->x19A0_shieldDamageTaken = 0;
    fp->x19A4 = 0;
    fp->lightshield_amount = 0;
    fp->x19A8 = 0;
    fp->shield_unk0 = 0;
    fp->shield_unk1 = 0;
    fp->x19BC_shieldDamageTaken3 = 6;

    fp->x221F_b6 = false;
    fp->reflecting = false;
    fp->x2218_b4 = false;
    fp->ReflectAttr.x1A3C_damageOver = 0;
    fp->ReflectAttr.x1A2C_reflectHitDirection = 0;
    fp->x2218_b6 = false;
    fp->x2218_b7 = false;

    fp->AbsorbAttr.x1A40_absorbHitDirection = 0;

    fp->AbsorbAttr.x1A44_damageTaken = 0;
    fp->AbsorbAttr.x1A48_hitsTaken = 0;

    fp->x68C_transNPos.x = fp->x68C_transNPos.y = fp->x68C_transNPos.z = 0;
    fp->x6A4_transNOffset.x = fp->x6A4_transNOffset.y =
        fp->x6A4_transNOffset.z = 0;
    fp->lstick_angle = 0;

    fp->x6C0.x = fp->x6C0.y = fp->x6C0.z = 0;
    fp->x6D8.x = fp->x6D8.y = fp->x6D8.z = 0;

    fp->x209C = 0;
    fp->x2224_b1 = false;
    fp->death2_cb = NULL;
    fp->x2100 = -1;
    fp->x2101_bits_0to6 = 0;
    fp->accessory2_cb = NULL;
    fp->accessory3_cb = NULL;
    fp->death1_cb = NULL;
    fp->death3_cb = NULL;
    fp->x221E_b4 = true;
    fp->x197C = 0;

    fp->is_metal = false;
    fp->metal_timer = 0;
    fp->metal_health = 0;

    ftCo_800C88A0(fp);

    fp->x2227_b3 = false;
    fp->x2034 = 0;
    fp->x2038 = 0;
    fp->x1980 = 0;

    fp->x2224_b2 = fp->x2224_b3 = false;

    fp->x2224_b4 = false;
    fp->capture_timer = 0;
    fp->x2224_b5 = false;
    fp->x1A53 = 0;
    fp->x1A52 = 0;
    fp->wall_jump_input_timer = 254;
    fp->dmg.x1910 = 0;
    fp->x2225_b0 = false;
    fp->x2225_b2 = true;
    fp->x2225_b4 = false;
    ftCo_800DEEA8(fp->gobj);
    fp->dmg.x18BC = 0;
    fp->dmg.x18B8 = 0;
    fp->x2226_b2 = false;
    fp->x2170 = 0;
    fp->x2225_b6 = fp->x2225_b5;
    fp->dmg.x1908 = -1;
    fp->dmg.x190C = 0;
    fp->x2227_b4 = false;
    fp->smash_attrs.x2138_smashSinceHitbox = -1;
    fp->x213C = -1;
    fp->x2227_b5 = false;
    fp->x2228_b1 = false;
    fp->x2140 = 0.0f;
    fp->x2227_b6 = false;
    fp->x2180 = 6;
    fp->x2229_b4 = true;
}

bool Fighter_UnkProcessDeath_80068354(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);

    Fighter_UnkInitReset_80067C98(fp);
    // Reset starts a new life; callbacks below belong to that new generation.
    const uint32_t callback_generation = (uint32_t)fp->x8_spawnNum;
#ifdef MELEE_NATIVE
    const FighterKind callback_kind = fp->kind;
    const ftData* callback_data = fp->ft_data;
    const NativeFighterDefinition* callback_definition = nativeFighterRuntime_Definition(callback_kind);
#endif
    HSD_JObjSetTranslate(GET_JOBJ(gobj), &fp->cur_pos);

    ftCo_800D105C(gobj);
#ifdef MELEE_NATIVE
    if(nativeScriptFighter_IsAttached(gobj)&&nativeScriptFighter_Status(gobj)!=NativeScriptVM_Ok)return false;
#endif
    if(!nativeCallbackSource_Alive(gobj,callback_generation,NativeCallbackSource_Fighter))return false;
    ftCo_800C09B4(gobj);
    ftCommon_8007E2FC(gobj);
    ft_80088A50(fp);
    ft_800890BC(fp);
    ft_800892D4(fp);
    ft_80081B38(gobj);
    ft_80081938(gobj);

    if (fp->smash_attrs.x2135 == -1) {
        if (ft_80082A68(gobj) && !fp->x2229_b6) {
            ftCommon_8007D6A4(fp);
            if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return false;
#ifdef MELEE_NATIVE
            if (gobj->user_data != fp || fp->kind != callback_kind || fp->ft_data != callback_data ||
                nativeFighterRuntime_Definition(callback_kind) != callback_definition) return false;
#endif
        } else {
            ftCommon_8007D5D4(fp);
        }
    } else {
        ftCommon_8007D5D4(fp);
    }
    ftCamera_80076064(fp);

    HSD_JObjSetTranslate(GET_JOBJ(gobj), &fp->cur_pos);
    Fighter_UnkApplyTransformation_8006C0F0(gobj);
    Fighter_UpdateModelScale(gobj);

    ftCo_800BFFAC(fp);
    ftCo_800C0074(fp);
    ftCo_800C8438(gobj);
    ftCo_800C89A0(gobj);
    ftCo_800C8FC4(gobj);
    ftColl_8007AFF8(gobj);
    ftColl_8007B0C0(gobj, HurtCapsule_Enabled);

#ifdef MELEE_NATIVE
    if (nativeScriptFighter_IsAttached(gobj)) {
        if (nativeScriptFighter_Run(gobj,NativeScriptFighter_Death,NULL,0, NULL,NULL)!=NativeScriptVM_Ok) return false;
        if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return false;
    } else
#endif
    if (ftData_OnDeath[fp->kind]) {
        ftData_OnDeath[fp->kind](gobj);
        if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return false;
#ifdef MELEE_NATIVE
        if (gobj->user_data != fp || fp->kind != callback_kind || fp->ft_data != callback_data ||
            nativeFighterRuntime_Definition(callback_kind) != callback_definition) return false;
#endif
    }

    ftCo_800A101C(fp, Player_GetCpuType(fp->player_id),
                  Player_GetCpuLevel(fp->player_id), 0);
#ifdef MELEE_NATIVE
    if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter) ||
        gobj->user_data != fp || fp->kind != callback_kind || fp->ft_data != callback_data ||
        nativeFighterRuntime_Definition(callback_kind) != callback_definition) return false;
#endif

    efAsync_QueueClear(&fp->x60C);
    ft_8007C17C(gobj);
    ft_8007C630(gobj);
    return true;
}

void Fighter_UnkUpdateCostumeJoint_800686E4(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    HSD_JObj* jobj;

    const UnkCostumeStruct* costume =
        &CostumeListsForeachCharacter[fp->kind]
             .costume_list[fp->x619_costume_id];
#ifdef MELEE_NATIVE
    const UnkCostumeStruct* selected = nativeCostumeRuntime_ForFighter(fp);
    if (selected != NULL) {
        costume = selected;
    }
#endif
    fp->x108_costume_joint = costume->joint;
    ftPartsPObjSetDefaultClass();
    jobj = HSD_JObjLoadJoint(fp->x108_costume_joint);
    ftPartsPObjClearDefaultClass();
    ftParts_80073758(jobj);

    HSD_GObjObject_80390A70(gobj, HSD_GObj_JObjKind, jobj);
}

void Fighter_UnkUpdateVecFromBones_8006876C(Fighter* fp)
{
    Vec3 vec;
    Vec3 vec2;
    HSD_JObj* jobj = fp->parts[ftParts_GetBoneIndex(fp, 2)].joint;

    HSD_JObjGetTranslation(jobj, &vec);

    fp->x1A6C = (vec.y / 8.55f);

    lb_8000B1CC(jobj, 0, &vec);
    lb_8000B1CC(fp->parts[ftParts_GetBoneIndex(fp, 1)].joint, 0, &vec2);
    fp->x1A70.x = vec2.x - vec.x;
    fp->x1A70.y = vec2.y - vec.y;
    fp->x1A70.z = vec2.z - vec.z;
}

void Fighter_ResetInputData_80068854(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);

    fp->input.lstick[0].x = fp->input.lstick[0].y = fp->input.lstick[1].x =
        fp->input.lstick[1].y = 0.0f;
    fp->action_buffer = (NativeActionBuffer){ 0 };
    memset(fp->ucf_stick, 0, sizeof(fp->ucf_stick));
    fp->ucf_shield_drop_frames = 0;

    fp->input.cstick[1].y = 0.0f;
    fp->input.cstick[1].x = 0.0f;
    fp->input.cstick[0].y = 0.0f;
    fp->input.cstick[0].x = 0.0f;

    fp->input.triggers[1] = 0.0f;
    fp->input.triggers[0] = 0.0f;

    fp->input.held_buttons[1] = 0;
    fp->input.released_buttons = 0;
    fp->input.pressed_buttons = 0;
    fp->input.held_buttons[0] = 0;

    fp->trigger_analog_timer = 0xFE;
    fp->x671_timer_lstick_tilt_y = 0xFE;
    fp->x670_timer_lstick_tilt_x = 0xFE;

    fp->x675 = 0xFE;
    fp->x674 = 0xFE;
    fp->x673 = 0xFE;

    fp->x678 = 0xFE;
    fp->x677_y = 0xFE;
    fp->x676_x = 0xFE;

    fp->x67B = 0xFE;
    fp->x67A_y = 0xFE;
    fp->x679_x = 0xFE;

    fp->x68B = 0xFF;
    fp->x68A = 0xFF;
    fp->x689 = 0xFF;
    fp->x688 = 0xFF;
    fp->x687 = 0xFF;
    fp->x686 = 0xFF;
    fp->x685 = 0xFF;
    fp->x684 = 0xFF;
    fp->x683 = 0xFF;

    fp->x680 = 0xFF;

    fp->x682 = 0xFF;
    fp->x681 = 0xFF;

    fp->x67E = 0xFF;
    fp->x67D = 0xFF;
    fp->x67C = 0xFF;
}

static void Fighter_UnkInitLoad_80068914_Inner1(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);

    fp->input.triggers[0] = fp->input.triggers[1] = fp->input.cstick[0].x =
        fp->input.cstick[0].y = fp->input.cstick[1].x = fp->input.cstick[1].y =
            fp->input.lstick[0].x = fp->input.lstick[0].y =
                fp->input.lstick[1].x = fp->input.lstick[1].y = 0.0f;

    fp->input.held_buttons[1] = 0;
    fp->input.released_buttons = 0;
    fp->input.pressed_buttons = 0;
    fp->input.held_buttons[0] = 0;

    fp->x679_x = fp->x67A_y = fp->x67B =

        fp->x676_x = fp->x677_y = fp->x678 =

            fp->x673 = fp->x674 = fp->x675 =

                fp->x670_timer_lstick_tilt_x = fp->x671_timer_lstick_tilt_y =
                    fp->trigger_analog_timer = 0xFE;

    fp->x67C = fp->x67D = fp->x67E = fp->x681 = fp->x682 =
        fp->x680 = fp->x683 = fp->x684 = fp->x685 = fp->x686 = fp->x687 =
            fp->x688 = fp->x689 = fp->x68A = fp->x68B = 0xFF;
}

void Fighter_UnkInitLoad_80068914(Fighter_GObj* gobj,
                                  struct plAllocInfo* argdata)
{
    Fighter* fp = GET_FIGHTER(gobj);
    s32 costume_id;
    fp->kind = argdata->internal_id;
#ifdef MELEE_NATIVE
    fp->behavior_kind = nativeFighterRuntime_BehaviorKind(fp->kind);
    memset(fp->native_module_state, 0, sizeof(fp->native_module_state));
    memset(fp->native_attachments, 0, sizeof(fp->native_attachments));
    fp->native_attachment_dynamic_mask = 0;
#endif
    fp->player_id = argdata->slot;

    fp->x221F_b4 = argdata->b0;

    fp->x34_scale.x = Player_GetModelScale(fp->player_id);
    fp->x61C = argdata->x5;
    fp->x618_player_id = Player_GetPlayerId(fp->player_id);
    fp->x61A_controller_index = Player_GetControllerIndex(fp->player_id);
    fp->is_always_metal = Player_GetFlagsBit5(fp->player_id);
    fp->x2226_b3 = Player_GetFlagsBit6(fp->player_id);
    fp->x2226_b6 = Player_GetFlagsBit7(fp->player_id);
    fp->x2225_b5 = Player_GetMoreFlagsBit1(fp->player_id);
    fp->x2225_b7 = Player_GetMoreFlagsBit2(fp->player_id);
    fp->x2228_b3 = Player_GetMoreFlagsBit6(fp->player_id);
    fp->x2229_b1 = Player_GetFlagsAEBit0(fp->player_id);

    if (fp->x61A_controller_index > 4) {
        HSD_ASSERTREPORT(0x33C, 0, "fighter sub color num over!\n");
    }

    if (fp->x61A_controller_index != 0) {
        GXColor* color =
            &p_ftCommonData
                 ->x6DC_colorsByPlayer[fp->x61A_controller_index - 1];
        fp->x610_color_rgba[0].r = (color->r * color->a) / 0xff;
        fp->x610_color_rgba[0].g = (color->g * color->a) / 0xff;
        fp->x610_color_rgba[0].b = (color->b * color->a) / 0xff;
        fp->x610_color_rgba[0].a = color->a;
    }

    costume_id = Player_GetCostumeId(fp->player_id);
    if (costume_id >= CostumeListsForeachCharacter[fp->kind].numCostumes) {
        costume_id = 0;
    }

    fp->x619_costume_id = costume_id;
    fp->team = Player_GetTeam(fp->player_id);
    fp->gobj = gobj;
    fp->ft_data = gFtDataList[fp->kind];
    ftCo_800D0FA0(gobj);
    fp->x2CC = 0;
    fp->x2D0 = 0;
    fp->x18 = 0x155;
    fp->x1C_actionStateList = ftData_MotionStateList;
    fp->x20_actionStateList = ftData_CharacterStateTables[fp->kind];
    fp->x24 = fp->ft_data->xC;
    fp->x28 = fp->ft_data->x10;

    fp->input.lstick[2].y = 0.0f;
    fp->input.lstick[2].x = 0.0f;
    fp->input.cstick[2].y = 0.0f;
    fp->input.cstick[2].x = 0.0f;
    fp->input.triggers[2] = 0.0f;
    fp->input.held_buttons[2] = 0;

    Fighter_UnkInitLoad_80068914_Inner1(gobj);

    fp->x594_s32 = 0;
    fp->x21FC_flag.u8 = 0;
    fp->x21FC_flag.b7 = 1;

    fp->invisible = false;
    fp->x221E_b1 = 0;
    fp->x221E_b2 = 0;
    fp->x221F_b1 = 0;
    fp->x221F_b2 = 0;

    fp->x209A = 0;
    fp->x221E_b5 = 0;
    fp->x221F_b0 = 0;
    fp->x21EC = 0;

    fp->x221D_b3 = 0;
    fp->x221D_b4 = 0;

    fp->x221F_b3 = 0;

    fp->x2220_b0 = 0;

    fp->x2221_b2 = 0;

    fp->no_normal_motion = false;
    fp->x2229_b6 = 0;
    fp->no_kb = 0;

    fp->x222A_b0 = 0;
    fp->x222A_b1 = 0;

    fp->x2228_b5 = 0;
    fp->used_tether = false;

    fp->x2221_b3 = 0;

    fp->x2222_b0 = false;
    fp->can_multijump = false;
    fp->x2222_b4 = 0;
    fp->x2222_b5 = 0;
    fp->x2222_b6 = 0;

    fp->x2223_b1 = 0;

    fp->x40 = 0.0f;

    fp->can_walljump = false;

    fp->x60C = 0;

    fp->x2225_b3 = 0;
    fp->is_sandbag = 0;

    fp->x2226_b0 = 0;
    fp->x2226_b1 = 0;

    fp->x2227_b0 = 0;
    fp->x2224_b0 = 0;

    fp->smash_attrs.x2135 = -1;
    fp->x2184 = NULL;

    fp->x2229_b3 = 0;
}

/// increments the spawn number, returns the spawn number value before
/// incrementing
u32 Fighter_NewSpawn_80068E40(void)
{
    u32 spawnNum = g_spawnNumCounter++;
    if (g_spawnNumCounter == 0) {
        g_spawnNumCounter = 1;
    }
    return spawnNum;
}

void Fighter_80068E64(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);

    if (stage_info.grkind == Gr_Kind_Flatzone) {
        fp->x34_scale.z = p_ftCommonData->x7E4_scaleZ;
    } else {
        fp->x34_scale.z = 1.0f;
    }
}

static void Fighter_Create_Inline2(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (!fp->no_normal_motion) {
        fp->x2EC = lbAnim_8001E8F8(ftData_80085E50(fp, 0x23));
        if (!fp->is_sandbag) {
            fp->x2DC = lbAnim_8001E8F8(ftData_80085E50(fp, 7));
            fp->x2E0 = lbAnim_8001E8F8(ftData_80085E50(fp, 8));
            fp->x2E4 = lbAnim_8001E8F8(ftData_80085E50(fp, 9));
            fp->x2E8 = lbAnim_8001E8F8(ftData_80085E50(fp, 0x25));
        }
    }
}

Fighter_GObj* Fighter_Create(struct plAllocInfo* input)
{
    Fighter_GObj* gobj;
    Fighter* fp;
    HSD_JObj* jobj;

    gobj = GObj_Create(HSD_GOBJ_CLASS_FIGHTER, 8, 0);
#ifdef MELEE_NATIVE
    if(gobj==NULL)return NULL;
    NativeMatchPoolReference object_reference;
    HSD_ASSERTREPORT(0, nativeMatchPool_Reference(&nativeGObjPool, gobj,
        &object_reference), "Invalid native Fighter constructor GObj.\n");
#endif
    GObj_SetupGXLink(gobj, &ftDrawCommon_80080E18, 5U, 0U);
#ifdef MELEE_NATIVE
    NativeMatchPoolReference reference;
    NativeFighterPoolSlot* own=nativeMatchPool_Allocate(&nativeFighterPool,&reference);
    if(own==NULL){HSD_GObjFree(gobj);return NULL;}
    fp=&own->actor;
    fp->native_constructing = true;
    fp->dat_attrs_backup=own->attributes;
#else
    fp = HSD_ObjAlloc(&fighter_alloc_data);
    // Pool blocks can previously belong to a different practice fighter.
    // Initialize the new instance before its character-specific setup so the
    // match never inherits another scene's velocity, move state, or flags.
    *fp = (Fighter){ 0 };
    fp->dat_attrs_backup = HSD_ObjAlloc(&fighter_dat_attrs_alloc_data);
#endif
    GObj_InitUserData(gobj, 4U, &Fighter_Unload_8006DABC, fp);
    ftData_8008572C(input->internal_id);
    Fighter_UnkInitLoad_80068914(gobj, input);
    const uint32_t callback_generation = (uint32_t)fp->x8_spawnNum;
#ifdef MELEE_NATIVE
    const FighterKind constructor_kind = fp->kind;
    const ftData* constructor_data = fp->ft_data;
    const NativeScriptFighterProgram* constructor_program =
        nativeScriptFighter_Program(constructor_kind);
    NativeFighterLifetime lifetime;
#endif
    if (ftData_UnkBytePerCharacter[fp->kind] != 0xFF) {
        efAsync_LoadSync(ftData_UnkBytePerCharacter[fp->kind]);
    }
    ftData_80085820(fp->kind, fp->x619_costume_id);

    Fighter_UnkUpdateCostumeJoint_800686E4(gobj);

    ftData_80085B10(fp);
    ftParts_80074E58(fp);
    ftParts_SetupParts(gobj);
    ftAnim_80070308(gobj);
    ftCo_800C884C(gobj);

    Fighter_80068E64(gobj);

    ftParts_800749CC(gobj);
    ftAnim_8007077C(gobj);
    ftCo_8009CF84(fp);
    ftAnim_8006FE48(gobj);

    Fighter_UnkUpdateVecFromBones_8006876C(fp);

    ftCo_8009F578(fp);

#ifdef MELEE_NATIVE
    if (!nativeScriptFighter_Attach(gobj)) goto native_create_failed;
    if (nativeMatchPool_Resolve(object_reference) != gobj ||
        nativeMatchPool_Resolve(reference) != fp || gobj->user_data != fp ||
        fp->kind != constructor_kind || fp->ft_data != constructor_data ||
        !nativeFighterLifetime_Capture(fp, &lifetime) ||
        lifetime.source_owned != (constructor_program != NULL) ||
        (constructor_program && lifetime.program != constructor_program))
        goto native_create_failed;
    if (!nativeScriptFighter_IsAttached(gobj)) {
#endif
    if (ftData_OnLoad[fp->kind]) {
#ifdef MELEE_NATIVE
        if (!nativeCallbackDispatch_Enter()) goto native_create_failed;
#endif
        ftData_OnLoad[fp->kind](gobj);
#ifdef MELEE_NATIVE
        nativeCallbackDispatch_Leave();
#endif
#ifdef MELEE_NATIVE
        if (!nativeFighterLifetime_Current(&lifetime)) goto native_create_failed;
#else
        if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return NULL;
#endif
    }
#ifdef MELEE_NATIVE
    }
#endif

    Fighter_Create_Inline2(gobj);

    ftColl_8007B320(gobj);
    fp->x890_cameraBox = Camera_80029020();

    jobj = GET_JOBJ(gobj);
    lbShadow_8000ED54(&fp->x20A4, jobj);
    HSD_GObj_SetupProc(gobj, &Fighter_8006A1BC, 0);
    HSD_GObj_SetupProc(gobj, &Fighter_8006A360, 1);
    HSD_GObj_SetupProc(gobj, &Fighter_8006ABA0, 2);
    HSD_GObj_SetupProc(gobj, &Fighter_Spaghetti_8006AD10, 3);
    HSD_GObj_SetupProc(gobj, &Fighter_procUpdate, 4);
    HSD_GObj_SetupProc(gobj, &Fighter_procMap, 6);
    HSD_GObj_SetupProc(gobj, &Fighter_8006C5F4, 7);
    HSD_GObj_SetupProc(gobj, &Fighter_CallAcessoryCallbacks_8006C624, 8);
    HSD_GObj_SetupProc(gobj, &Fighter_8006C80C, 9);
    HSD_GObj_SetupProc(gobj, &Fighter_UnkProcessGrab_8006CA5C, 0xC);
    HSD_GObj_SetupProc(gobj, &Fighter_8006CB94, 0xD);
    HSD_GObj_SetupProc(gobj, &Fighter_ProcessHit_8006D1EC, 0xE);
    HSD_GObj_SetupProc(gobj, &Fighter_8006D9AC, 0x10);
    HSD_GObj_SetupProc(gobj, &Fighter_UnkCallCameraCallback_8006D9EC, 0x12);
    HSD_GObj_SetupProc(gobj, &Fighter_8006DA4C, 0x16);
    if (!Fighter_UnkProcessDeath_80068354(gobj)) {
#ifdef MELEE_NATIVE
        goto native_create_failed;
#else
        return NULL;
#endif
    }
#ifdef MELEE_NATIVE
    // Reset legitimately publishes a new life, while the allocation, source
    // program and persistent storage remain the constructor's original owners.
    NativeFighterLifetime reset_lifetime;
    if (nativeMatchPool_Resolve(object_reference) != gobj ||
        nativeMatchPool_Resolve(reference) != fp || gobj->user_data != fp ||
        !nativeFighterLifetime_Capture(fp, &reset_lifetime) ||
        reset_lifetime.kind != lifetime.kind ||
        reset_lifetime.fighter_data != lifetime.fighter_data ||
        reset_lifetime.source_owned != lifetime.source_owned ||
        reset_lifetime.program != lifetime.program ||
        reset_lifetime.native_definition != lifetime.native_definition ||
        (lifetime.source_owned &&
         (reset_lifetime.view.storage != lifetime.view.storage ||
          reset_lifetime.program_identity != lifetime.program_identity ||
          reset_lifetime.layout_identity != lifetime.layout_identity ||
          reset_lifetime.parameter_identity != lifetime.parameter_identity)))
        goto native_create_failed;
    lifetime = reset_lifetime;
#endif

    if (fp->kind == Ft_Kind_MasterH) {
        ftMh_MS_341_8014FE10(gobj);
    } else if (fp->kind == Ft_Kind_CrezyH) {
        ftCh_Init_80155FCC(gobj);
    } else if (input->has_transformation) {
        ftCo_800BFD04(gobj);
    } else if (Player_GetFlagsBit3(fp->player_id) != 0) {
        ftCo_800C61B0(gobj);
    } else {
        if (!fp->no_normal_motion) {
            ftCommon_8007D92C(gobj);
        } else {
            HSD_ASSERTREPORT(1065, 0, "ellegal flag fp->no_normal_motion\n");
        }
    }
#ifdef MELEE_NATIVE
    if (!nativeFighterLifetime_Current(&lifetime)) goto native_create_failed;
#endif
    ftLib_800867E8(gobj);
#ifdef MELEE_NATIVE
    fp->native_constructing = false;
#endif
    return gobj;
#ifdef MELEE_NATIVE
native_create_failed:
    // A faulted actor still needs teardown. A retired/reused physical owner
    // must not be touched merely because it occupies the same address.
    if (nativeMatchPool_Resolve(object_reference) == gobj &&
        nativeMatchPool_Resolve(reference) == fp && gobj->user_data == fp &&
        fp->gobj == gobj)
        HSD_GObjFree(gobj);
    return NULL;
#endif
}

void Fighter_ChangeMotionState(Fighter_GObj* gobj, FtMotionId msid,
                               MotionFlags flags, f32 anim_start,
                               f32 anim_speed, f32 anim_blend,
                               Fighter_GObj* arg3)
{
    HSD_JObj* jobj = GET_JOBJ(gobj);
    Fighter* fp = GET_FIGHTER(gobj);
    const uint32_t callback_generation = (uint32_t)fp->x8_spawnNum;
    MotionState* new_motion_state;
    struct Fighter_WaitAnimData* unk_struct_x18;
    s32 bone_index;
    u8(*unk_byte_ptr)[2];
    bool animflags_bool;
    union Struct2070 x2070;

    fp->motion_id = msid;
    fp->aerial_landing_interrupt_frame = 0.0f;
    if (fp->action_buffer.checking) {
        fp->action_buffer.direction.frames = 0;
        fp->action_buffer.movement.frames = 0;
    }
    fp->facing_dir1 = fp->facing_dir;

    HSD_JObjSetTranslate(jobj, &fp->cur_pos);
    efAsync_QueueFlush(gobj, &fp->x60C);

    if ((flags & Ft_MF_SkipHit) == 0) {
        if (fp->x2219_b3 != 0) {
            ftColl_8007AFF8(gobj);
        }
        if (fp->x2219_b4 != 0) {
            ft_8007C114(gobj);
        }
    }

    if ((flags & Ft_MF_SkipThrowException) == 0) {
        fp->x1064_thrownHitbox.owner = NULL;
    }

    if ((flags & Ft_MF_KeepColAnimHitStatus) == 0) {
        if (fp->x1988 != 0) {
            ftColl_8007B62C(gobj, 0);
        }
        if (fp->x221A_b5 != 0) {
            ftColl_8007B0C0(gobj, HurtCapsule_Enabled);
        }
    }

    if (fp->x221A_b6 != 0) {
        ftColl_8007B4E0(gobj);
    }

    if (((flags & Ft_MF_SkipModel) == 0) && (fp->x221D_b2 != 0U)) {
        ftParts_80074A8C(gobj);
    }

    if (((flags & Ft_MF_SkipMatAnim) == 0) && ((fp->x221E_b7) != 0)) {
        ftAnim_80070654(gobj);
    }

    if (!(flags & Ft_MF_SkipParasol)) {
        NativeFighterLifetime held_lifetime;
        if (!nativeFighterLifetime_Capture(fp, &held_lifetime)) return;
        fp->x2221_b4 = false;
        int held_status = ftGetParasolStatus(gobj);
        if (!nativeFighterLifetime_Current(&held_lifetime)) return;
        if (held_status != -1) {
            held_status = ftGetParasolStatus(gobj);
            if (!nativeFighterLifetime_Current(&held_lifetime)) return;
            if (held_status != 6) {
                ftCommon_8007E83C(gobj, 6, 0.0f);
                if (!nativeFighterLifetime_Current(&held_lifetime)) return;
            }
        }
    }

    ftAnim_80070F28(gobj);
    ftAnim_80070E74(gobj);
    ftCommon_8007ECD4(fp, 7);
    ftCommon_8007ECD4(fp, 8);
    ftCommon_8007ECD4(fp, 0x24);

    if ((flags & Ft_MF_SkipRumble) == 0) {
        ftCommon_8007ECD4(fp, 1);
        ftCommon_8007ECD4(fp, 0x19);
    }

    if (((flags & Ft_MF_KeepColAnimPartHitStatus) == 0) &&
        (fp->x2221_b1 != 0U))
    {
        ftColl_8007B6EC(gobj);
        ftColl_8007B760(gobj, p_ftCommonData->x134);
        fp->x2221_b1 = 0;
    }
    ftCommon_8007E2F4(fp, 0);

    if (fp->dmg.x18F4 != 0) {
        fp->dmg.x18F4 = 0;
        fp->x2220_b4 = 0;
    }

    if ((flags & Ft_MF_Unk19) == 0) {
        fp->x2222_b2 = 0;
    }

    if ((flags & Ft_MF_SkipMetalB) == 0) {
        fp->x2223_b4 = 0;
    }

    if ((flags & Ft_MF_Unk27) == 0) {
        fp->x2227_b2 = 0;
    }

    if (((flags & Ft_MF_SkipHitStun) == 0) && (fp->x221C_b6 != 0U)) {
        fp->x221C_b6 = 0;
        fp->x2098 = p_ftCommonData->x4CC;
    }

    fp->x221F_b3 = 0;
    fp->x2219_b1 = 0;
    fp->x2219_b2 = 0;

    fp->dmg.x182c_behavior = 1.0f;
    fp->dmg.kb_applied = 0.0f;
    fp->dmg.x18A8 = 0.0f;
    fp->dmg.armor1 = 0.0f;
    fp->dmg.x18a0 = 0.0f;

    fp->x221A_b7 = 0;
    fp->x221B_b0 = 0;
    fp->x221C_b3 = 0;

    fp->shield_unk0 = 0.0f;
    fp->shield_unk1 = 0.0f;

    fp->x221D_b5 = 0;
    fp->reflecting = false;
    fp->x2218_b6 = 0;
    fp->x221C_b4 = 0;

    fp->x1A6A = 0;

    fp->x221D_b7 = 0;
    fp->invisible = false;
    fp->x221E_b1 = 0;
    fp->x221E_b2 = 0;
    fp->x221F_b1 = 0;
    fp->x221E_b5 = 0;
    fp->x221E_b6 = 0;
    fp->x2220_b3 = 0;
    fp->x2220_b7 = 0;

    fp->x209C = 0;

    fp->x2223_b0 = 0;
    fp->x2222_b3 = 0;
    fp->x2224_b5 = 0;
    fp->x2225_b1 = 0;
    fp->x2225_b4 = 0;

    mpClearFloorSkip(&fp->coll_data);

    ftCo_800DEEA8(gobj);

    fp->smash_attrs.x2138_smashSinceHitbox = -1.0f;
    fp->x2224_b4 = false;

    if ((flags & Ft_MF_SkipItemVis) == 0) {
        fp->x221E_b3 = 1;
    } else if (fp->x221E_b3 == 0U) {
        ftCommon_8007F578(gobj);
    }

    if ((flags & Ft_MF_SkipModelPartVis) == 0) {
        fp->x221E_b4 = 1;
    }

    if ((flags & Ft_MF_SkipModelFlags) == 0) {
        fp->x2225_b2 = 1;
    }

    if ((flags & Ft_MF_KeepFastFall) == 0) {
        fp->fall_fast = 0;
    }

    if ((flags & Ft_MF_SkipColAnim) == 0) {
        ftCo_800C0134(fp);
    }

    if (((flags & Ft_MF_KeepGfx) == 0) && (fp->x2219_b0 != 0U)) {
        ftCommon_8007DB24(gobj);
    }

    if (((flags & Ft_MF_KeepAccessory) == 0) && fp->x20A0_accessory != NULL)
    {
        HSD_JObjRemoveAll(fp->x20A0_accessory);
        fp->x20A0_accessory = NULL;
    }

    if (fp->ground_or_air == GA_Ground) {
        NativeFighterLifetime grounded_lifetime;
        if (!nativeFighterLifetime_Capture(fp, &grounded_lifetime)) return;
        const FighterKind grounded_kind = fp->kind;
        const NativeFighterDefinition* grounded_definition = nativeFighterRuntime_Definition(grounded_kind);
        if (grounded_lifetime.source_owned) {
            NativeScriptValue result;
            if (nativeScriptFighter_Run(gobj, NativeScriptFighter_GroundedMotionSetup,
                    NULL, 0, NULL, &result) != NativeScriptVM_Ok ||
                !nativeFighterLifetime_Current(&grounded_lifetime)) return;
        } else if (grounded_definition != NULL && grounded_definition->on_grounded_motion_setup != NULL) {
            grounded_definition->on_grounded_motion_setup(gobj);
            if (!nativeFighterLifetime_Current(&grounded_lifetime)) return;
        } else if (fp->kind == Ft_Kind_Peach) {
            fp->u.pe.has_float = true;
        }
        fp->x2221_b5 = false;
        fp->x2221_b7 = true;
        fp->x2221_b6 = true;
        fp->x2224_b1 = false;
        fp->x2227_b1 = false;
        fp->x213C = -1;

        if (fp->x2227_b4 != 0U) {
            pl_8003FE1C(fp->player_id, fp->x221F_b4);
            fp->x2227_b4 = false;
        }
        fp->x2227_b5 = false;
        pl_80040330(fp->player_id, fp->x221F_b4, fp->x2140);
        fp->x2140 = 0;
        fp->used_tether = false;
        fp->x2180 = 6;
    }

    if ((msid != 0xE) && (msid != 0xF) && (msid != 0x10) && (msid != 0x11)) {
        fp->hitlag_mul = 0.0f;
    }

    if ((flags & Ft_MF_KeepSfx) == 0) {
        ft_80088884(fp);
        ft_800888E0(fp);
        ft_800887CC(fp);
    }

    if ((flags & Ft_MF_KeepSwordTrail) == 0) {
        fp->x2100 = -1;
    }
    if ((flags & Ft_MF_SkipNametagVis) == 0) {
        fp->x209A = 0;
    }

    fp->x2222_b7 = 0;

    if ((flags & Ft_MF_UnkUpdatePhys) != 0) {
        fp->x100 = 0.0f;
    } else {
        fp->x100 = -1.0f;
    }

    if ((flags & Ft_MF_Unk24) == 0) {
        fp->x221C_u16_y = 0;
    }

    fp->lstick_angle = 0.0f;

    ftPartSetRotX(fp, 0, 0.0F);
    ftPartSetRotY(fp, 0, (M_PI_2 * fp->facing_dir));
    ftPartSetRotZ(fp, 0, 0.0F);

    if (msid >= fp->x18) {
        new_motion_state = &fp->x20_actionStateList[(msid - fp->x18)];
    } else {
        new_motion_state = &fp->x1C_actionStateList[msid];
    }

    if (fp->ground_or_air == GA_Ground) {
        if ((flags & 0x40) == 0) {
            if (new_motion_state->x9_b1 != 0 && fp->dmg.x18C8 == -1) {
                if (p_ftCommonData->x814 > 0) {
                    fp->dmg.x18C8 = p_ftCommonData->x814;
                } else {
                    fp->dmg.x18C8 = 1;
                }
            }
        }
    }

    {
        // load in the union.
        x2070 = fp->x2070;
        ft_800890D0(fp, new_motion_state->move_id);
        ft_800895E0(fp, new_motion_state->x4_flags);
        if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
        fp->x2225_b3 = new_motion_state->x9_b0;

        if (fp->x2226_b4) {
            if (fp->x2070.x2071_b5) {
                ftCo_800C8B2C(fp, 0x7E, 0);
            }
            if (fp->x2070.x2071_b6) {
                ftCo_800C8B2C(fp, 0x7F, 0);
            }
        }

#ifdef MELEE_NATIVE
        if (nativeScriptFighter_CallbackBound(gobj,NativeScriptFighterCallback_StateChange)) {
            if (!nativeScriptFighter_RunCallback(gobj,NativeScriptFighterCallback_StateChange)) return;
            if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
        } else
#endif
        if (fp->x21EC != NULL) {
            fp->x21EC(gobj);
            if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
            fp->x21EC = NULL;
        }

        if (!(flags & Ft_MF_SkipAttackCount)) {
            pl_80037C60(gobj, x2070.x2070_int);
        }

        fp->anim_id = new_motion_state->anim_id;
        fp->frame_speed_mul = anim_speed;
        fp->x8A0_unk = anim_speed;

        fp->cur_anim_frame = (anim_start - fp->frame_speed_mul);
        fp->x898_unk = 0.0f;

        if ((fp->x594_b0) || (fp->x594_b5)) {
            animflags_bool = true;
        } else {
            animflags_bool = false;
        }

        if (fp->anim_id != -1) {
            Vec3 translation;
            Quaternion quat;

#ifdef MELEE_NATIVE
            bone_index = fp->x596_bone;
#else
            bone_index = fp->x596_bits.x7;
#endif

            if ((flags & Ft_MF_FreezeState) != 0) {
                fp->x2223_b0 = 1;
                fp->x104 = 0x14;
                fp->frame_speed_mul = 0.0f;
                anim_speed = 0.0f;
            }

            if (arg3 != NULL) {
                unk_struct_x18 =
                    &((Fighter*) arg3->user_data)->x24[fp->anim_id];
                unk_byte_ptr = &((Fighter*) arg3->user_data)->x28[fp->anim_id];
            } else {
                unk_struct_x18 = &fp->x24[fp->anim_id];
                unk_byte_ptr = &fp->x28[fp->anim_id];
            }
            fp->x594_s32 = unk_struct_x18->x10_animCurrFlags;
            ftCo_8009E7B4(fp, unk_byte_ptr);
            if ((flags & Ft_MF_SkipAnim) == 0) {
                if (arg3 != 0U) {
                    ftData_80085CD8(fp, GET_FIGHTER(arg3), fp->anim_id);
                    ftColl_8007B8CC(fp, arg3);
                } else {
                    ftData_80085CD8(fp, fp, fp->anim_id);
                }
                fp->x3E4_fighterCmdScript.u = unk_struct_x18->xC;
                fp->x3E4_fighterCmdScript.loop_count = 0;

                if (anim_start) {
                    if (fp->x590 != 0U) {
                        ftAnim_8006EBE8(gobj, anim_start - anim_speed,
                                        anim_speed,
                                        (anim_blend == -1.0f) ? 0.0f
                                        : anim_blend ? anim_blend
                                                     : (*unk_byte_ptr)[0]);
                    }
                    ftAnim_8006E9B4(gobj);
                    if (fp->x594_b0 != 0U) {
                        fp->x6B0.x = fp->x6B0.y = fp->x6B0.z = 0.0F;
                        fp->x6A4_transNOffset.x = fp->x6A4_transNOffset.y =
                            fp->x6A4_transNOffset.z = 0.0F;
                        fp->x698 = fp->x68C_transNPos;
                    }

                    if (fp->x594_b5 != 0U) {
                        fp->x6E4.x = fp->x6E4.y = fp->x6E4.z = 0.0F;
                        fp->x6D8.x = fp->x6D8.y = fp->x6D8.z = 0.0F;
                        fp->x6CC = fp->x6C0;
                    }
                    fp->x3E4_fighterCmdScript.timer = -anim_start;
                } else {
                    if (fp->x590 != 0U) {
                        ftAnim_8006EBE8(gobj, anim_start, anim_speed,
                                        (anim_blend == -1.0f) ? 0.0f
                                        : anim_blend ? anim_blend
                                                     : (*unk_byte_ptr)[0]);
                    }
                    fp->x3E4_fighterCmdScript.timer = 0.0f;
                }

                ftAnim_8006E9B4(gobj);
                if ((bone_index != 0) && (*unk_byte_ptr)[0] != 0U) {
                    HSD_JObj* temp_joint = fp->parts[bone_index].x4_jobj2;

                    HSD_JObjGetTranslation(temp_joint, &translation);
                    HSD_JObjSetTranslate(fp->parts[bone_index].joint,
                                         &translation);
                    HSD_JObjGetRotation(temp_joint, &quat);
                    ftParts_JObjSetRotation(fp->parts[bone_index].joint,
                                            &quat);
                }

                if (fp->x594_b0 != 0U) {
                    if (!anim_start) {
                        float c = 0.0f;
                        fp->x6B0.x = fp->x6B0.y = fp->x6B0.z = c;
                        fp->x6A4_transNOffset.x = fp->x6A4_transNOffset.y =
                            fp->x6A4_transNOffset.z = c;
                        fp->x698 = fp->x68C_transNPos;
                    } else if (((flags & Ft_MF_SkipAnimVel) == 0) &&
                               (fp->ground_or_air == GA_Ground))
                    {
                        float temp_vel =
                            fp->x6A4_transNOffset.z * fp->facing_dir;
                        fp->self_vel.x = temp_vel;
                        fp->gr_vel = temp_vel;
                    }
                }

                if (fp->x594_b5 != 0U) {
                    if (!anim_start) {
                        float c = 0.0f;
                        fp->x6E4.x = fp->x6E4.y = fp->x6E4.z = c;
                        fp->x6D8.x = fp->x6D8.y = fp->x6D8.z = c;
                        fp->x6CC = fp->x6C0;
                    } else if (((flags & Ft_MF_SkipAnimVel) == 0) &&
                               (fp->ground_or_air == GA_Ground))
                    {
                        float temp_vel = fp->x6D8.z * fp->facing_dir;
                        fp->self_vel.x = temp_vel;
                        fp->gr_vel = temp_vel;
                    }
                }
                if ((flags & Ft_MF_UpdateCmd) != 0) {
                    ftAction_8007349C(gobj);
                } else if (anim_start) {
                    ftAction_80073354(gobj);
                } else {
                    ftCo_800C0408(gobj);
                    ftAction_80073240(gobj);
                }
            } else {
                fp->anim_id = -1;
            }
        }

        if (fp->anim_id == -1) {
            fp->x594_s32 = 0;
            ftAnim_80070758(jobj);
            ftAnim_80070758(fp->x8AC_animSkeleton);
            fp->x3E4_fighterCmdScript.u = NULL;
            fp->x8A4_animBlendFrames = 0;
            fp->x8A8_anim_frame = 0;
        }

        if (animflags_bool) {
            if (!fp->x594_b0 && !fp->x594_b0) {
                !fp;
                ftCommon_ClampGroundVel(fp, fp->co_attrs.dash_max_velocity);
            }
        }

        fp->anim_cb = new_motion_state->anim_cb;
        fp->input_cb = new_motion_state->input_cb;
        fp->phys_cb = new_motion_state->phys_cb;
        fp->coll_cb = new_motion_state->coll_cb;
        fp->cam_cb = new_motion_state->cam_cb;

        fp->accessory1_cb = NULL;
        fp->accessory4_cb = NULL;
        fp->deal_dmg_cb = NULL;
        fp->shield_hit_cb = NULL;
        fp->reflect_hit_cb = NULL;
        fp->hitlag_cb = NULL;
        fp->x21CC = NULL;
        fp->post_hitlag_cb = NULL;
        fp->pre_hitlag_cb = NULL;
        fp->take_dmg_cb = NULL;
        fp->take_dmg_2_cb = NULL;
        fp->hurtbox_detect_cb = NULL;
        fp->x21F8 = NULL;
        fp->death2_cb = NULL;
#ifdef MELEE_NATIVE
        nativeScriptFighter_ResetMotionCallbacks(gobj);
        nativeScriptFighter_SelectMotion(gobj,(uint32_t)msid);
#endif
    }
}

void Fighter_8006A1BC(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    const uint32_t callback_generation = (uint32_t)fp->x8_spawnNum;

    if (!fp->x221F_b3) {
        if (fp->dmg.x1954 > 0.0f) {
            fp->dmg.x1954 -= 1.0f;
            if (fp->dmg.x1954 <= 0.0f) {
                fp->dmg.x1954 = 0.0f;
                if (!fp->allow_sdi && !fp->x2219_b7) {
                    Fighter_8006D10C(gobj);
                    if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
                }
            }
        }

        if (fp->x221A_b1) {
            Fighter_8006CFE0(gobj);
            if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
            fp->x221A_b1 = 0;
        }

        if (fp->dmg.x195c_hitlag_frames > 0.0f) {
            fp->dmg.x195c_hitlag_frames -= 1.0f;
            if (fp->dmg.x195c_hitlag_frames <= 0.0f) {
                fp->dmg.x195c_hitlag_frames = 0.0f;
                if (fp->x221A_b3) {
                    ftCo_80090718(fp);
                    fp->x221A_b3 = 0;
                }
                if ((!fp->dmg.x1954) && !fp->x2219_b7) {
                    Fighter_8006D10C(gobj);
                    if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
                }
                fp->allow_sdi = 0;
            }
        }
        ftCo_800C37A0(gobj);

        while (fp->x200C != 0) {
            Fighter_SuperMushroomApply(gobj);
            fp->x200C--;
        }

        while (fp->x2010 != 0) {
            Fighter_PoisonMushroomApply(gobj);
            fp->x2010--;
        }

        ft_800819A8(gobj);
        fp->x2219_b6 = fp->x2219_b5;
    }
}

void Fighter_8006A360(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    const uint32_t callback_generation = (uint32_t)fp->x8_spawnNum;

    if (!fp->x221F_b3) {
        fp->pos_delta.x = fp->cur_pos.x - fp->prev_pos.x;
        fp->pos_delta.y = fp->cur_pos.y - fp->prev_pos.y;
        fp->pos_delta.z = fp->cur_pos.z - fp->prev_pos.z;

        fp->prev_pos = fp->cur_pos;

        if (fp->dmg.x18C8 != -1 && fp->dmg.x18C8 > 0) {
            fp->dmg.x18C8--;
            if (fp->dmg.x18C8 == 0) {
                fp->dmg.x18c4_source_ply = 6;
                fp->dmg.x18C8 = -1;
                fp->dmg.x18D0 = -0xA;
            }
        }

        if (fp->x1990) {
            fp->x1990--;
            if (fp->x1990 == 0 && !fp->x2221_b0) {
                fp->x198C = fp->throw_invincibility_frames != 0 ? 1 : 0;

                if (ftCo_800C0694(fp) == 9) {
                    ftCo_800C0200(fp, 9);
                }
            }
        }

        if (fp->throw_invincibility_frames) {
            fp->throw_invincibility_frames--;
            if (fp->throw_invincibility_frames == 0) {
                fp->x198C = (fp->x2221_b0 || fp->x1990 != 0) ? 2 : 0;

                if (ftCo_800C0694(fp) == 9) {
                    ftCo_800C0200(fp, 9);
                }
            }
        }

        if (fp->x221D_b6) {
            if (fp->x2004) {
                fp->x2004--;
                if (fp->x2004 == 0) {
                    fp->x221D_b6 = 0;
                    if (ftCo_800C0694(fp) == 0x6B) {
                        ftCo_800C0200(fp, 0x6B);
                    }
                } else if (fp->x2004 == it_8026B588()) {
                    ft_800880D8(fp);
                }
            }
        }

        if (fp->x2220_b5 || fp->x2220_b6) {
            if (fp->x2008) {
                fp->x2008--;
            }

            if (fp->x2008 == 0) {
                if (fp->x2220_b5) {
                    Fighter_SuperMushroomEnd(gobj);
                } else if (fp->x2220_b6) {
                    Fighter_PoisonMushroomEnd(gobj);
                }
            }
        }

        if (fp->x197C) {
            if (fp->x2014) {
                fp->x2014--;
                if (fp->x2014 == 0) {
                    ftCommon_8007F8E8(gobj);
                    if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
#ifdef MELEE_NATIVE
                    if (nativeScriptFighter_IsAttached(gobj) && nativeScriptFighter_Status(gobj) != NativeScriptVM_Ok) return;
#endif
                    Item_8026A8EC(fp->x197C);
                    if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
                    ftCommon_8007F9B4(gobj);
                }
            }
        }

        if (fp->is_metal && fp->metal_timer != 0) {
            fp->metal_timer--;
            if (fp->metal_timer == 0 || fp->metal_health <= 0) {
                // Exit fighter metal mode
                ftCo_800C8540(gobj);
            }
        }

        if (fp->x2227_b3) {
            if (fp->x2034) {
                fp->x2034--;
                if (!fp->x2034 || fp->x2038 <= 0) {
                    ftCo_800C9034(gobj);
                    return;
                }
            }
        }

        if (fp->x1980) {
            fp->x2018--;
            fp->x201C--;

            if (fp->x201C == 0) {
                Fighter_TakeDamage_8006CC7C(fp,
                                            p_ftCommonData->x6F4_unkDamage);
                fp->x201C = p_ftCommonData->x6F8;
            }

            if (fp->x2018 <= 0) {
                Vec3 vec = Fighter_803B7488;

                ftCommon_8007F8E8(gobj);
                if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
#ifdef MELEE_NATIVE
                if (nativeScriptFighter_IsAttached(gobj) && nativeScriptFighter_Status(gobj) != NativeScriptVM_Ok) return;
#endif
                Item_8026ABD8(fp->x1980, &vec, 0.0f);
                if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
                ftCommon_8007FF74(gobj);
            }
        }

        if (fp->x2226_b4) {
            if (fp->x2030) {
                fp->x2030--;
                if (fp->x2030 == 0) {
                    ftCo_800C8A64(gobj);
                    return;
                }
                if (!fp->x2226_b3 && fp->x2030 == p_ftCommonData->x7D0 &&
                    ftCo_800C8B2C(fp, 0x7D, 0))
                {
                    fp->x2226_b7 = 1;
                }
            }
        }

        if (fp->x2221_b4) {
            if (fp->x2104) {
                fp->x2104--;
                if (fp->x2104 == 0) {
                    fp->x2221_b4 = false;
                    NativeFighterLifetime held_lifetime;
                    if (!nativeFighterLifetime_Capture(fp, &held_lifetime)) return;
                    const FighterKind held_kind = fp->kind;
                    const NativeFighterDefinition* held_definition = nativeFighterRuntime_Definition(held_kind);
                    if (held_lifetime.source_owned) {
                        NativeScriptValue result;
                        if (nativeScriptFighter_Run(gobj, NativeScriptFighter_HeldTimerExpired,
                                NULL, 0, NULL, &result) != NativeScriptVM_Ok ||
                            !nativeFighterLifetime_Current(&held_lifetime)) return;
                        if (!result.as.u64) ftCo_80095744(gobj, 0, NULL);
                    } else if (held_definition != NULL && held_definition->on_held_timer_expired != NULL) {
                        if (!nativeCallbackDispatch_Enter()) return;
                        bool handled = held_definition->on_held_timer_expired(gobj);
                        nativeCallbackDispatch_Leave();
                        if (!nativeFighterLifetime_Current(&held_lifetime)) return;
                        if (!handled) ftCo_80095744(gobj, 0, NULL);
                    } else if (fp->item_gobj &&
                        itGetKind(fp->item_gobj) == It_Kind_Peach_Parasol)
                    {
                        fp->x2221_b5 = true;
                        ftCo_800968C8(gobj);
                    } else {
                        ftCo_80095744(gobj, 0, NULL);
                    }
                    if (!nativeFighterLifetime_Current(&held_lifetime)) return;
                }
            }
        }

        if (!fp->x221F_b4 && Camera_80031144() == 1.0f) {
            if (fp->dmg.x1830_percent < p_ftCommonData->x7B0) {
                if (ifMagnify_802FC998(fp->player_id) &&
                    (Player_GetMoreFlagsBit3(fp->player_id) != 0))
                {
                    fp->dmg.x1910++;
                } else {
                    fp->dmg.x1910 = 0;
                }

                if (fp->dmg.x1910 >= p_ftCommonData->x7AC) {
                    Fighter_TakeDamage_8006CC7C(
                        fp, p_ftCommonData->x7B4_unkDamage);
                    fp->dmg.x1910 = 0;
                }
            }
        }

        if (fp->dmg.x18F0) {
            fp->dmg.x18F0--;
            if (fp->dmg.x1830_percent > 0.0f) {
                fp->dmg.x1830_percent--;
                ft_80088640(fp, 0x7D, 0x7F, 0x40);
                Player_SetHPByIndex(fp->player_id, fp->x221F_b4,
                                    fp->dmg.x1830_percent);
                pl_80040B8C(fp->player_id, fp->x221F_b4, 1);
            }

            if (fp->dmg.x1830_percent <= 0.0f) {
                fp->dmg.x1830_percent = 0.0f;
                fp->dmg.x18F0 = 0;
            }

            if (fp->dmg.x18F0 == 0) {
                if (ftCo_800C0694(fp) == 8) {
                    ftCo_800C0200(fp, 8);
                }
                ftCommon_8007ECD4(fp, 2);
            }
        }

        if (fp->item_gobj) {
            if (itGetKind(fp->item_gobj) != It_Kind_Hammer) {
                !fp;
            } else {
                ftCo_800C511C(gobj);
            }
        }

        if (fp->dmg.x18fa_model_shift_frames) {
            fp->dmg.x18fa_model_shift_frames--;
            fp->dmg.x18FC++;
            if (fp->dmg.x18FC >= fp->dmg.x18FD) {
                fp->dmg.x18FC = 0;
            }
        }

#ifdef MELEE_NATIVE
        if (nativeScriptFighter_IsAttached(gobj)) {
            if (nativeScriptFighter_Run(gobj,NativeScriptFighter_LateUpdate,NULL,0, NULL,NULL)!=NativeScriptVM_Ok) return;
            if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
        } else
#endif
        if (ftData_UnkMotionStates3[fp->kind]) {
            ftData_UnkMotionStates3[fp->kind](gobj);
            if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
        }

        if (fp->x21CC) {
            fp->x21CC(gobj);
            if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
        }

        if (!fp->x2219_b5) {
            if (fp->x209A > 1 && !fp->x221D_b4) {
                fp->x209A--;
            }
            if (fp->x2223_b0) {
                if (fp->x104 == 0x14U) {
                    ftAnim_8006F0FC(gobj, 0.0f);
                } else {
                    fp->frame_speed_mul += fp->x8A0_unk;
                }
                fp->x104--;
                if (fp->x104 == 0) {
                    ftAnim_8006F0FC(gobj, fp->frame_speed_mul);
                    fp->x104 = 0x14U;
                }
            }

            if (fp->smash_attrs.x2138_smashSinceHitbox != -1.0f) {
                fp->smash_attrs.x2138_smashSinceHitbox++;
            }

            if (fp->dmg.x18ac_time_since_hit != -1) {
                fp->dmg.x18ac_time_since_hit++;
            }
            ftAnim_8006EBA4(gobj);
            ftCo_800D71D8(gobj);
            ftColl_800764DC(gobj);

            if (!fp->x221C_b6) {
                pl_800411C4(fp->player_id, fp->x221F_b4);
            }
            ftCo_800DEF38(gobj);

#ifdef MELEE_NATIVE
            if (nativeScriptFighter_MotionBound(gobj,NativeScriptMotion_Animation)) {
                if (!nativeScriptFighter_RunMotion(gobj,NativeScriptMotion_Animation)) return;
                if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
            } else
#endif
            if (fp->anim_cb) {
                fp->anim_cb(gobj);
                if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
            }
        }

        ftCommon_8007E0E4(gobj);
        ftCo_800C0408(gobj);
    }
}

void Fighter_8006ABA0(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (!fp->x221F_b3 && ftCo_800A2040(fp)) {
        ftCo_800B3900(gobj);
    }
}

/// https://decomp.me/scratch/A7CgG
void Fighter_UnkIncrementCounters_8006ABEC(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);

    if (ftCo_Jump_GetInput(gobj)) {
        fp->x68A = fp->x685;
        fp->x685 = 0;
    } else if (fp->x685 < 0xFF) {
        fp->x685++;
    }

    if (ftCo_800D6928(fp)) {
        fp->x68B = fp->x686;
        fp->x686 = 0;
    } else if (fp->x686 < 0xFF) {
        fp->x686++;
    }

    if (ftCo_800D688C(fp)) {
        fp->x687 = 0;
    } else if (fp->x687 < 0xFF) {
        fp->x687++;
    }
    if (ftCo_SpecialS_HasInput(fp)) {
        fp->x688 = 0;
    } else if (fp->x688 < 0xFF) {
        fp->x688++;
    }

    if (ftCo_800D67C4(fp)) {
        fp->x689 = 0;
    } else if (fp->x689 < 0xFF) {
        fp->x689++;
    }
}

/// the stick pairs seen in input structs might make more sense as an array of
/// 2, or a struct of 2 floats.. if it still matches.
#define SET_STICKS(stickXPtr, stickYPtr, x, y)                                \
    do {                                                                      \
        float* stickX = (float*) &stickXPtr;                                  \
        float* stickY = (float*) &stickYPtr;                                  \
        *stickX = x;                                                          \
        *stickY = y;                                                          \
    } while (0)

static void Fighter_Spaghetti_8006AD10_Inner1(Fighter* fp)
{
    s32 temp0_loc_1;
    s32 temp0_loc_0;

    temp0_loc_0 = (fp->input.held_buttons[0] &
                   (fp->input.held_buttons[1] ^ fp->input.held_buttons[0]));
    temp0_loc_1 = (fp->input.held_buttons[1] &
                   (fp->input.held_buttons[1] ^ fp->input.held_buttons[0]));

    if (fp->x2219_b5) {
        fp->input.pressed_buttons |= temp0_loc_0;
        fp->input.released_buttons |= temp0_loc_1;
    } else {
        fp->input.pressed_buttons = temp0_loc_0;
        fp->input.released_buttons = temp0_loc_1;
    }
}

void Fighter_Spaghetti_8006AD10(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    const uint32_t callback_generation=(uint32_t)fp->x8_spawnNum;
    float tempf1;
    float tempf0;

    if (!fp->x221F_b3) {
        if (!fp->x2224_b2) {
            const bool cpu = ftCo_800A2040(fp);
            if (!fp->x221D_b3) {
                SET_STICKS(fp->input.lstick[1].x, fp->input.lstick[1].y,
                           fp->input.lstick[2].x, fp->input.lstick[2].y);
                SET_STICKS(fp->input.cstick[1].x, fp->input.cstick[1].y,
                           fp->input.cstick[2].x, fp->input.cstick[2].y);
                fp->input.triggers[1] = fp->input.triggers[2];
                fp->input.held_buttons[1] = fp->input.held_buttons[2];
                fp->x221D_b3 = 1;
            } else {
                SET_STICKS(fp->input.lstick[1].x, fp->input.lstick[1].y,
                           fp->input.lstick[0].x, fp->input.lstick[0].y);
                SET_STICKS(fp->input.cstick[1].x, fp->input.cstick[1].y,
                           fp->input.cstick[0].x, fp->input.cstick[0].y);
                fp->input.triggers[1] = fp->input.triggers[0];
                fp->input.held_buttons[1] = fp->input.held_buttons[0];
            }

            if (cpu) {
                SET_STICKS(fp->input.lstick[0].x, fp->input.lstick[0].y,
                           ftCo_GetCpuLStickX(fp), ftCo_GetCpuLStickY(fp));
                if (DbLevel < DbLKind_DebugRom &&
                    !gm_IsCurrently1PMode_inline())
                {
                    SET_STICKS(fp->input.cstick[0].x, fp->input.cstick[0].y,
                               ftCo_GetCpuCStickX(fp), ftCo_GetCpuCStickY(fp));
                } else {
                    fp->input.cstick[0].x = 0;
                    fp->input.cstick[0].y = 0;
                }

                tempf0 = ftCo_GetCpuLTrigger(fp);
                tempf1 = ftCo_GetCpuRTrigger(fp);

                fp->input.triggers[0] = (tempf0 > tempf1) ? tempf0 : tempf1;

            } else {
                SET_STICKS(fp->input.lstick[0].x, fp->input.lstick[0].y,
                           HSD_PadGameStatus[fp->x618_player_id].nml_stickX,
                           HSD_PadGameStatus[fp->x618_player_id].nml_stickY);
                if (DbLevel < DbLKind_DebugRom &&
                    gm_IsCurrently1PMode_inline() == 0)
                {
                    SET_STICKS(
                        fp->input.cstick[0].x, fp->input.cstick[0].y,
                        HSD_PadGameStatus[fp->x618_player_id].nml_subStickX,
                        HSD_PadGameStatus[fp->x618_player_id].nml_subStickY);
                } else {
                    fp->input.cstick[0].x = 0;
                    fp->input.cstick[0].y = 0;
                }

                tempf1 = HSD_PadGameStatus[fp->x618_player_id].nml_analogR;
                tempf0 = HSD_PadGameStatus[fp->x618_player_id].nml_analogL;

                fp->input.triggers[0] = (tempf0 > tempf1) ? tempf0 : tempf1;
            }

            if (ABS(fp->input.lstick[0].x) <=
                p_ftCommonData->horizontal_stick_deadzone)
            {
                fp->input.lstick[0].x = 0.0f;
            }

            if (ABS(fp->input.lstick[0].y) <=
                p_ftCommonData->vertical_stick_deadzone)
            {
                fp->input.lstick[0].y = 0.0f;
            }

            if (ABS(fp->input.cstick[0].x) <=
                p_ftCommonData->horizontal_stick_deadzone)
            {
                fp->input.cstick[0].x = 0.0f;
            }

            if (ABS(fp->input.cstick[0].y) <=
                p_ftCommonData->vertical_stick_deadzone)
            {
                fp->input.cstick[0].y = 0.0f;
            }

            if (fp->input.triggers[0] <=
                p_ftCommonData->analog_shoulder_deadzone)
            {
                fp->input.triggers[0] = 0.0f;
            }

            if (cpu) {
                fp->input.held_buttons[0] = ftCo_GetCpuButtons(fp);
            } else {
                fp->input.held_buttons[0] =
                    HSD_PadGameStatus[fp->x618_player_id].button;
#ifdef MELEE_NATIVE
                u32 profile_extension =
                    HSD_PadGameStatus[fp->x618_player_id].extension;
                if (profile_extension & NATIVE_PROFILE_EXT_SHORT_HOP) {
                    fp->input.held_buttons[0] |= HSD_PAD_PROFILE_SHORT_HOP;
                }
                if (profile_extension &
                    NATIVE_PROFILE_EXT_TAP_JUMP_DISABLED)
                {
                    fp->input.held_buttons[0] |=
                        HSD_PAD_PROFILE_TAP_JUMP_DISABLED;
                }
                if (profile_extension & NATIVE_PROFILE_EXT_CSTICK_TILT) {
                    fp->input.held_buttons[0] |= HSD_PAD_PROFILE_CSTICK_TILT;
                }
#endif
            }

            if (gm_8016B0FC()) {
                fp->input.triggers[0] = 0.0f;
                if (cpu) {
                    fp->input.held_buttons[0] &= HSD_PAD_A | HSD_PAD_XY;
                } else {
                    fp->input.held_buttons[0] &= HSD_PAD_A;
                }
            } else {
                if (fp->input.held_buttons[0] & (HSD_PAD_L | HSD_PAD_R)) {
                    fp->input.held_buttons[0] |= HSD_PAD_LR;
                    fp->input.triggers[0] = 1.0f;
                } else if (fp->input.triggers[0]) {
                    fp->input.held_buttons[0] |= HSD_PAD_LR;
                }
                if (!gm_GetDbPauseFlag(0)) {
                    if (fp->input.held_buttons[0] & HSD_PAD_Z) {
                        fp->input.held_buttons[0] |= HSD_PAD_LR | HSD_PAD_A;
                        fp->input.triggers[0] =
                            p_ftCommonData->z_press_analog_value;
                    }
                }
            }

            Fighter_Spaghetti_8006AD10_Inner1(fp);

            // Fighter_ClampSpecificValue
            fp->x676_x++;
            if (fp->x676_x > 0xFE) {
                fp->x676_x = 0xFE;
            }

            if (fp->input.lstick[0].x >=
                p_ftCommonData->horizontal_stick_smash_deadzone)
            {
                if (fp->input.lstick[1].x >=
                    p_ftCommonData->horizontal_stick_smash_deadzone)
                {
                    // Fighter_ClampThreeValues
                    fp->x670_timer_lstick_tilt_x++;
                    if (fp->x670_timer_lstick_tilt_x > 0xFE) {
                        fp->x670_timer_lstick_tilt_x = 0xFE;
                    }
                    fp->x673++;
                    if (fp->x673 > 0xFE) {
                        fp->x673 = 0xFE;
                    }
                    fp->x679_x++;
                    if (fp->x679_x > 0xFE) {
                        fp->x679_x = 0xFE;
                    }
                } else {
                    fp->x676_x = 0;
                    fp->x673 = 0;
                    fp->x670_timer_lstick_tilt_x = 0;
                    fp->x2228_b7 = 1;
                }
            } else if (fp->input.lstick[0].x <=
                       -p_ftCommonData->horizontal_stick_smash_deadzone)
            {
                if (fp->input.lstick[1].x <=
                    -p_ftCommonData->horizontal_stick_smash_deadzone)
                {
                    // Fighter_ClampThreeValues
                    fp->x670_timer_lstick_tilt_x++;
                    if (fp->x670_timer_lstick_tilt_x > 0xFE) {
                        fp->x670_timer_lstick_tilt_x = 0xFE;
                    }
                    fp->x673++;
                    if (fp->x673 > 0xFE) {
                        fp->x673 = 0xFE;
                    }
                    fp->x679_x++;
                    if (fp->x679_x > 0xFE) {
                        fp->x679_x = 0xFE;
                    }
                } else {
                    fp->x676_x = 0;
                    fp->x673 = 0;
                    fp->x670_timer_lstick_tilt_x = 0;
                    fp->x2228_b7 = 0;
                }
            } else {
                fp->x679_x = 0xFEU;
                fp->x673 = 0xFEU;
                fp->x670_timer_lstick_tilt_x = 0xFEU;
            }

            // Fighter_ClampSpecificValue
            fp->x677_y++;
            if (fp->x677_y > 0xFE) {
                fp->x677_y = 0xFE;
            }

            if (fp->input.lstick[0].y >=
                p_ftCommonData->vertical_stick_smash_deadzone)
            {
                if (fp->input.lstick[1].y >=
                    p_ftCommonData->vertical_stick_smash_deadzone)
                {
                    // Fighter_ClampThreeValues
                    fp->x671_timer_lstick_tilt_y++;
                    if (fp->x671_timer_lstick_tilt_y > 0xFE) {
                        fp->x671_timer_lstick_tilt_y = 0xFE;
                    }
                    fp->x674++;
                    if (fp->x674 > 0xFE) {
                        fp->x674 = 0xFE;
                    }
                    fp->x67A_y++;
                    if (fp->x67A_y > 0xFE) {
                        fp->x67A_y = 0xFE;
                    }
                } else {
                    fp->x677_y = 0;
                    fp->x674 = 0;
                    fp->x671_timer_lstick_tilt_y = 0;
                    fp->x2229_b0 = 0;
                }
            } else if (fp->input.lstick[0].y <=
                       -p_ftCommonData->vertical_stick_smash_deadzone)
            {
                if (fp->input.lstick[1].y <=
                    -p_ftCommonData->vertical_stick_smash_deadzone)
                {
                    // Fighter_ClampThreeValues
                    fp->x671_timer_lstick_tilt_y++;
                    if (fp->x671_timer_lstick_tilt_y > 0xFE) {
                        fp->x671_timer_lstick_tilt_y = 0xFE;
                    }
                    fp->x674++;
                    if (fp->x674 > 0xFE) {
                        fp->x674 = 0xFE;
                    }
                    fp->x67A_y++;
                    if (fp->x67A_y > 0xFE) {
                        fp->x67A_y = 0xFE;
                    }
                } else {
                    fp->x677_y = 0;
                    fp->x674 = 0;
                    fp->x671_timer_lstick_tilt_y = 0;
                    fp->x2229_b0 = 1;
                }
            } else {
                fp->x67A_y = 0xFE;
                fp->x674 = 0xFE;
                fp->x671_timer_lstick_tilt_y = 0xFE;
            }

            if (lb_8000D148(fp->input.lstick[1].x, fp->input.lstick[1].y,
                            fp->input.lstick[0].x, fp->input.lstick[0].y, 0.0f,
                            0.0f,
                            p_ftCommonData->horizontal_stick_smash_deadzone))
            {
                fp->x67A_y = 0;
                fp->x679_x = 0;
            }

            // Fighter_ClampSpecificValue
            fp->x678++;
            if (fp->x678 > 0xFE) {
                fp->x678 = 0xFE;
            }

            if (fp->input.triggers[0] >=
                p_ftCommonData->shield_press_threshold)
            {
                if (fp->input.triggers[1] >=
                    p_ftCommonData->shield_press_threshold)
                {
                    // Fighter_ClampThreeValues
                    fp->trigger_analog_timer++;
                    if (fp->trigger_analog_timer > 0xFE) {
                        fp->trigger_analog_timer = 0xFE;
                    }
                    fp->x675++;
                    if (fp->x675 > 0xFE) {
                        fp->x675 = 0xFE;
                    }
                    fp->x67B++;
                    if (fp->x67B > 0xFE) {
                        fp->x67B = 0xFE;
                    }
                } else {
                    fp->x67B = 0;
                    fp->x678 = 0;
                    fp->x675 = 0;
                    fp->trigger_analog_timer = 0;
                }
            } else {
                fp->x67B = 0xFE;
                fp->x675 = 0xFE;
                fp->trigger_analog_timer = 0xFE;
            }

            if (fp->input.pressed_buttons & HSD_PAD_A) {
                fp->x683 = fp->x67C;
                fp->x67C = 0;
            } else if (fp->x67C < 0xFF) {
                fp->x67C++;
            }

            if (fp->input.pressed_buttons & HSD_PAD_B) {
                fp->x67D = 0;
            } else if (fp->x67D < 0xFF) {
                fp->x67D++;
            }

            if (fp->input.pressed_buttons & HSD_PAD_XY) {
                fp->x67E = 0;
            } else if (fp->x67E < 0xFF) {
                fp->x67E++;
            }

            if (fp->input.pressed_buttons & HSD_PAD_DPADUP) {
                fp->x681 = 0;
            } else if (fp->x681 < 0xFF) {
                fp->x681++;
            }

            if (fp->input.pressed_buttons & HSD_PAD_DPADDOWN) {
                fp->x682 = 0;
            } else if (fp->x682 < 0xFF) {
                fp->x682++;
            }

            if (fp->input.pressed_buttons & (HSD_PAD_L | HSD_PAD_R)) {
                fp->x684 = fp->x680;
                fp->x680 = 0;
            } else if (fp->x680 < 0xFF) {
                fp->x680++;
            }

            unsigned buffer_frames = 0;
            if (!cpu && fp->x618_player_id < 4) {
                nativeUcf_Sample(fp);
                if (fp->motion_id >= ftCo_MS_Wait && !gm_8016B0FC()) {
                    buffer_frames = (HSD_PadGameStatus[fp->x618_player_id].extension &
                                     NATIVE_PROFILE_EXT_ACTION_BUFFER_MASK) >>
                                    NATIVE_PROFILE_EXT_ACTION_BUFFER_SHIFT;
                }
            }
            nativeActionBuffer_Record(fp, buffer_frames);
        }

        if (fp->x221D_b4 || fp->x2224_b2 || gm_GetDbPauseFlag(2)) {
            fp->input.lstick[2].x = fp->input.lstick[0].x;
            fp->input.lstick[2].y = fp->input.lstick[0].y;
            fp->input.cstick[2].x = fp->input.cstick[0].x;
            fp->input.cstick[2].y = fp->input.cstick[0].y;
            fp->input.triggers[2] = fp->input.triggers[0];
            fp->input.held_buttons[2] = fp->input.held_buttons[0];
            fp->x221D_b3 = 0;

            Fighter_UnkInitLoad_80068914_Inner1(gobj);
        }

        if (!fp->x2219_b5) {
            if (fp->x1980) {
                ftCommon_8007FFD8(fp, p_ftCommonData->x6FC);
            }
            ftCo_800DF0D0(gobj);
            ftCommon_8008031C(gobj);
            Fighter_UnkIncrementCounters_8006ABEC(gobj);

            NativeActionInput live, offered;
            const bool buffered = nativeActionBuffer_Begin(fp, &live, &offered);
#ifdef MELEE_NATIVE
            if (nativeScriptFighter_MotionBound(gobj,NativeScriptMotion_Interrupt)) {
                const FtMotionId motion = fp->motion_id;
                const bool ran = nativeScriptFighter_RunMotion(
                    gobj, NativeScriptMotion_Interrupt);
                if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
                if (ran && fp->motion_id == motion &&
                    fp->aerial_landing_interrupt_frame > 0.0f)
                {
                    ftCo_LandingAir_IASA(gobj);
                }
            } else
#endif
            if (fp->input_cb) {
                fp->input_cb(gobj);
            }
            if (buffered &&
                nativeCallbackSource_Alive(gobj, callback_generation,
                                           NativeCallbackSource_Fighter))
            {
                nativeActionBuffer_End(fp, &live, &offered);
            }
        }
    }
}

//// https://decomp.me/scratch/oFu1o
#define VEC_CLEAR(vec)                                                        \
    do {                                                                      \
        Vec3* vecLocal = (void*) &vec;                                        \
        float c = 0;                                                          \
        vecLocal->x = vecLocal->y = vecLocal->z = c;                          \
    } while (0)

void Fighter_procUpdate(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    const uint32_t callback_generation = (uint32_t)fp->x8_spawnNum;

    if (fp->x221F_b3) {
        return;
    }

#ifdef MELEE_NATIVE
    if (nativeScriptFighter_IsAttached(gobj)) {
        if (nativeScriptFighter_Run(gobj,NativeScriptFighter_Tick,NULL,0, NULL,NULL)!=NativeScriptVM_Ok) return;
        if (!nativeCallbackSource_Alive(gobj,callback_generation,NativeCallbackSource_Fighter)) return;
    } else if (fp->kind >= Ft_Kind_CustomFirst) {
        const NativeFighterDefinition* definition =
            nativeFighterRuntime_Definition(fp->kind);
        if (definition != NULL && definition->on_tick != NULL) {
            definition->on_tick(gobj);
            if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
        }
    }
#endif

    Fighter_UpdatePosition(gobj);

    if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
    ftCo_800D3158(gobj);
    if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;

    if (fp->x2225_b0) {
        // if position.y crossed
        // (0.25*stage.blastBottom+0.75*stage.cameraBottom) +
        // stage.crowdReactStart from below...
        if (fp->prev_pos.y <= Stage_CalcUnkCamYBounds() &&
            fp->cur_pos.y > Stage_CalcUnkCamYBounds())
        {
            fp->x2225_b0 = 0;
        }
    } else {
        if (!fp->x222A_b1 && !fp->x2228_b5) {
            // if position.y crossed 0.5*(stage.blastBottom+stage.cameraBottom)
            // + stage.crowdReactStart from above...
            if (fp->prev_pos.y >= Stage_CalcUnkCamY() &&
                fp->cur_pos.y < Stage_CalcUnkCamY())
            {
                // plays this sound you always hear when you get close to the
                // bottom blast zone
                ft_PlaySFX(fp, 96, 127, 64);
                fp->x2225_b0 = 1;
            }
        }
    }

    if (fp->dmg.x18A4_knockbackMagnitude && !fp->x221C_b6 &&
        !un_80322258(fp->cur_pos.x))
    {
        fp->dmg.x18A4_knockbackMagnitude = 0.0f;
    }

    ftColl_8007AF28(gobj);

    if (DbLevel >= DbLKind_DebugRom && (fpclassify(fp->cur_pos.x) == FP_NAN ||
                                        fpclassify(fp->cur_pos.y) == FP_NAN ||
                                        fpclassify(fp->cur_pos.z) == FP_NAN))
    {
        HSD_ASSERTREPORT(/*line*/ 2517, 0,
                         "fighter procUpdate pos error.\tpos.x=%f\tpos.y=%f\n",
                         fp->cur_pos.x, fp->cur_pos.y);
    }
}

void Fighter_UnkApplyTransformation_8006C0F0(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);

    if (fp->x34_scale.z != 1.0f) {
        HSD_JObj* jobj = GET_JOBJ(gobj);
        Mtx mtx1;
        Mtx mtx2;
        Vec3 scale;
        Vec3 translation;
        Quaternion rotation;

        HSD_JObjSetupMatrix(jobj);
        HSD_MtxInverse(HSD_JObjGetMtxPtr(jobj), mtx1);
        HSD_JObjGetScale(jobj, &scale);

        scale.x = ftCommon_GetModelScale(fp);

        HSD_JObjGetRotation(jobj, &rotation);
        HSD_JObjGetTranslation(jobj, &translation);

        HSD_MtxSRT(mtx2, &scale, (Vec3*) &rotation, &translation, 0);
        C_MTXConcat(mtx2, mtx1, fp->x44_mtx);
    }
}

static inline float Fighter_GetPosX(Fighter* fp)
{
    return fp->cur_pos.x;
}

static inline float Fighter_GetPosY(Fighter* fp)
{
    return fp->cur_pos.y;
}

void Fighter_procMap(Fighter_GObj* gobj)
{
    Fighter* fp = (Fighter*) HSD_GObjGetUserData(gobj);
    const uint32_t callback_generation = (uint32_t)fp->x8_spawnNum;

    if (!fp->x221F_b3) {
        if (fp->ecb_lock) {
            fp->ecb_lock--;
            if (!fp->ecb_lock) {
                ftCommon_UnlockECB(fp);
            }
        }

        fp->x2223_b5 = 0;

        HSD_JObjSetTranslate(gobj->hsd_obj, &fp->cur_pos);

#ifdef MELEE_NATIVE
        if (nativeScriptFighter_MotionBound(gobj,NativeScriptMotion_Collision)) {
            if (!nativeScriptFighter_RunMotion(gobj,NativeScriptMotion_Collision)) return;
            if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
            ftKb_SpecialN_800F1D24(gobj);
        } else
#endif
        if (fp->coll_cb) {
            fp->coll_cb(gobj);
            if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
            ftKb_SpecialN_800F1D24(gobj);
        }

        if (fp->ground_or_air == GA_Ground) {
            pl_80041280(fp->player_id, fp->x221F_b4);
        }

        if (DbLevel >= DbLKind_DebugRom) {
            if (fpclassify(fp->cur_pos.x) == FP_NAN ||
                fpclassify(fp->cur_pos.y) == FP_NAN ||
                fpclassify(fp->cur_pos.z) == FP_NAN)
            {
                float x = Fighter_GetPosX(fp);
                float y = Fighter_GetPosY(fp);
                HSD_ASSERTREPORT(
                    2590, 0,
                    "fighter procMap pos error.\tpos.x=%f\tpos.y=%f\n", x, y);
            }
        }

        HSD_JObjSetTranslate(gobj->hsd_obj, &fp->cur_pos);
    }
}

void Fighter_8006C5F4(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (!fp->x221F_b3) {
        ft_80089B08(gobj);
    }
}

void Fighter_CallAcessoryCallbacks_8006C624(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    const uint32_t callback_generation = (uint32_t)fp->x8_spawnNum;
#ifdef MELEE_NATIVE
    const bool owned = nativeScriptFighter_IsAttached(gobj);
    NativeFighterLifetime lifetime;
#endif

    u8 _[4];

    if (!fp->x221F_b3) {
#ifdef MELEE_NATIVE
        if (owned && !nativeFighterLifetime_Capture(fp, &lifetime)) return;
#endif
        if (fp->x2219_b5) {
#ifdef MELEE_NATIVE
            if (nativeScriptFighter_CallbackBound(gobj, NativeScriptFighterCallback_AccessoryHitlag)) {
                if (!nativeScriptFighter_RunCallback(gobj, NativeScriptFighterCallback_AccessoryHitlag)) return;
            } else
#endif
            if (fp->accessory3_cb) {
                fp->accessory3_cb(gobj);
            }
            return;
        }

        bool accessory_before = fp->accessory2_cb != NULL;
#ifdef MELEE_NATIVE
        if (nativeScriptFighter_CallbackBound(gobj, NativeScriptFighterCallback_AccessoryBefore)) {
            if (!nativeScriptFighter_RunCallback(gobj, NativeScriptFighterCallback_AccessoryBefore)) return;
            accessory_before = true;
        } else
#endif
        if (fp->accessory2_cb) fp->accessory2_cb(gobj);
        if (accessory_before) {
            if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
#ifdef MELEE_NATIVE
            if (owned && !nativeFighterLifetime_Current(&lifetime)) return;
#endif
            HSD_JObjSetTranslate(gobj->hsd_obj, &fp->cur_pos);
#ifdef MELEE_NATIVE
            if (owned && !nativeFighterLifetime_Current(&lifetime)) return;
#endif
        }

        if (fp->accessory1_cb) {
            fp->accessory1_cb(gobj);
            if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
#ifdef MELEE_NATIVE
            if (owned && !nativeFighterLifetime_Current(&lifetime)) return;
#endif
            HSD_JObjSetTranslate(gobj->hsd_obj, &fp->cur_pos);
        }
    }
}

void Fighter_8006C80C(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    const uint32_t callback_generation = (uint32_t)fp->x8_spawnNum;

    if (!fp->x221F_b3) {
        efAsync_QueueFlush(gobj, &fp->x60C);
        Fighter_UnkApplyTransformation_8006C0F0(gobj);

        if (!fp->x2219_b5) {
#ifdef MELEE_NATIVE
            if (nativeScriptFighter_MotionBound(gobj,NativeScriptMotion_Accessory)) {
                if (!nativeScriptFighter_RunMotion(gobj,NativeScriptMotion_Accessory)) return;
                if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
            } else
#endif
            if (fp->accessory4_cb) {
                fp->accessory4_cb(gobj);
                if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
            }
        }

        ftColl_8007AE80(gobj);
        ft_8007C224(gobj);
        ft_8007C6DC(gobj);

        if (fp->x20A0_accessory) {
            HSD_JObjAnimAll(fp->x20A0_accessory);
        }

        if (fp->ground_or_air == GA_Air &&
            fp->cur_pos.y < Stage_GetCamBoundsBottomOffset())
        {
            if (ifMagnify_802FB6E8(fp->player_id) == 3) {
                Vec3 cam_offset;
                Stage_UnkSetVec3TCam_Offset(&cam_offset);

                if (fp->cur_pos.y + cam_offset.y < fp->x2140) {
                    fp->x2140 = fp->cur_pos.y + cam_offset.y;
                }
            }
        }
    }
}

void Fighter_UnkProcessGrab_8006CA5C(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    const uint32_t callback_generation = (uint32_t)fp->x8_spawnNum;

    if (!fp->x221F_b3 && !gm_8016B1C4()) {
        ftColl_8007BA0C(gobj);
        if (fp->x221E_b6) {
            ftColl_80078A2C(gobj);
            if (fp->victim_gobj) {
                if (!fp->x2225_b1) {
                    ft_PlaySFX(fp, fp->ft_data->x4C_sfx->x30, 0x7F, 0x40);
                }
                ftColl_80078754(gobj, fp->victim_gobj, 0);
                Fighter_GObj* victim = fp->victim_gobj;
                uint32_t victim_generation;
                if (!nativeCallbackSource_Snapshot(victim, NativeCallbackSource_Fighter, &victim_generation)) return;
                if (fp->grab_cb) fp->grab_cb(gobj);
                else {
                    NativeScriptValue argument;
                    if (!nativeScriptFighter_HasHook(gobj,NativeScriptFighter_GrabCatch) ||
                        !nativeScriptReceiver_CaptureExternal(victim,NativeCallbackSource_Fighter,
                            nativeGObjPool.lifetime,&argument,NULL) ||
                        nativeScriptFighter_Run(gobj,NativeScriptFighter_GrabCatch,&argument,1,NULL,NULL)!=NativeScriptVM_Ok) return;
                }
                if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
                if (fp->victim_gobj == victim) {
                    if (!nativeCallbackSource_Alive(victim, victim_generation, NativeCallbackSource_Fighter)) return;
                } else if (!nativeCallbackSource_Snapshot(fp->victim_gobj, NativeCallbackSource_Fighter, &victim_generation)) return;
                if (fp->grabbed_cb) fp->grabbed_cb(fp->victim_gobj, gobj);
                else {
                    // Capturing fighter owns this code and its variables. The
                    // victim is an explicit payload, never a substitute script
                    // actor running another fighter's program/storage.
                    NativeScriptValue argument;
                    if (!nativeScriptReceiver_CaptureExternal(fp->victim_gobj,NativeCallbackSource_Fighter,
                            nativeGObjPool.lifetime,&argument,NULL)) return;
                    if (!nativeScriptFighter_HasHook(gobj,NativeScriptFighter_Grabbed)) return;
                    nativeScriptFighter_Run(gobj,NativeScriptFighter_Grabbed,&argument,1,NULL,NULL);
                }
                return;
            }
            ftColl_8007BC90(gobj);

            if (fp->target_item_gobj) {
                if (!fp->x2225_b1) {
                    ft_PlaySFX(fp, fp->ft_data->x4C_sfx->x30, 0x7F, 0x40);
                }
                it_8027B4A4(gobj, fp->target_item_gobj);
                if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
                if (fp->x2194) {
                    fp->x2194(gobj);
                } else if (nativeScriptFighter_HasHook(gobj,NativeScriptFighter_ItemCatch)) {
                    nativeScriptFighter_Run(gobj,NativeScriptFighter_ItemCatch,NULL,0,NULL,NULL);
                }
            }
        }
    }
}

void Fighter_8006CB94(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    float func_8007BBCC_float_output;

    if (!fp->x221F_b3 && !fp->x2219_b1) {
        ftColl_800765E0();
        ftColl_80078C70(gobj);
        ft_8007C77C(gobj);
        ftColl_8007925C(gobj);
        ftColl_8007BAC0(gobj);
        ft_8007C4BC(gobj);
        ftColl_8007AB48(gobj);
        ftColl_8007AB80(gobj);
        func_8007BBCC_float_output = ftColl_8007BBCC(gobj);
        if (func_8007BBCC_float_output > 0.0f) {
            ftCommon_8007FC7C(gobj, func_8007BBCC_float_output);
        }
    }
}

void Fighter_UnkTakeDamage_8006CC30(Fighter* fp, float arg0)
{
    Fighter_TakeDamage_8006CC7C(fp, arg0);
    ftCommon_8007EA90(fp, arg0);
}

void Fighter_TakeDamage_8006CC7C(Fighter* fp, float damage_amount)
{
    if (!fp->x2226_b4 || fp->x2226_b3) {
        fp->dmg.x1830_percent += damage_amount;
        if (fp->metal_timer != 0) {
            fp->metal_health -= damage_amount;
        }

        if (fp->x2034) {
            fp->x2038 -= damage_amount;
        }

        if (fp->dmg.x1830_percent > 999.0f) {
            fp->dmg.x1830_percent = 999.0f;
        }
        Player_SetHPByIndex(fp->player_id, fp->x221F_b4,
                            fp->dmg.x1830_percent);
        pl_8003EC9C(fp->player_id, fp->x221F_b4, fp->dmg.x1830_percent,
                    damage_amount);
        ftCo_800C8C84(fp->gobj);
    }
}

void Fighter_8006CDA4(Fighter* fp, s32 arg1)
{
    Fighter_GObj* gobj = fp->gobj;
    const uint32_t callback_generation = (uint32_t) fp->x8_spawnNum;
    bool temp_bool;
    bool hold_item_bool = false;
    Vec3 vec;
    PAD_STACK(8);

    if (fp->item_gobj && !itIsHeavy(fp->item_gobj)) {
        hold_item_bool = true;
    }

    temp_bool = !(fp->x2220_b3 || fp->x2220_b4 || ftCo_8008E984(fp));
    vec = vec3_803B7494;

    if (fp->motion_id != 0x145 && (unsigned) fp->motion_id - 0x122 > 1 &&
        fp->dmg.x1860_element != HitElement_Cape && !fp->x2226_b2)
    {
        if ( ///// giant if condition
            hold_item_bool && temp_bool &&
            ((HSD_Randi(p_ftCommonData->x418) < arg1) ||
             (((itGetUseKind(fp->item_gobj) == 3) &&
               it_8026B594(fp->item_gobj)) &&
              !HSD_Randi(p_ftCommonData->x41C))))
        {
            if (fp->x1978) {
                Item_8026ABD8(fp->x1978, &vec, 1.0f);
                if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
            }
            Item_8026ABD8(fp->item_gobj, &vec, 1.0f);
            if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
        }
        if (fp->x197C) {
            if (HSD_Randi(p_ftCommonData->x418) < arg1) {
                ftCommon_8007F8E8(fp->gobj);
                if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
#ifdef MELEE_NATIVE
                if (nativeScriptFighter_IsAttached(gobj) && nativeScriptFighter_Status(gobj) != NativeScriptVM_Ok) return;
#endif
                Item_8026ABD8(fp->x197C, &vec, 1.0f);
                if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
                ftCommon_8007F9B4(fp->gobj);
            }
        }
    }
}

void Fighter_8006CF5C(Fighter* fp, s32 arg1)
{
    if (!fp->x2224_b2) {
        fp->dmg.x18F0 += arg1;
        ftCo_800BFFD0(fp, 8, 0);
        ftCommon_8007EBAC(fp, 2, 0);
    }
}

void Fighter_UnkSetFlag_8006CFBC(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);

    if (fp->x2219_b7) {
        fp->x221A_b1 = 1;
    }
}

void Fighter_8006CFE0(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    const uint32_t callback_generation = (uint32_t)fp->x8_spawnNum;

    if (fp->x2219_b7) {
        if (!fp->allow_sdi) {
            if (!fp->dmg.x1954) {
                Fighter_8006D10C(gobj);
                if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
            }
        }
        fp->x2219_b7 = 0;
    }
}

static inline void setBit(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->x2219_b7 = 1;
}

void Fighter_UnkRecursiveFunc_8006D044(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    const uint32_t callback_generation = (uint32_t)fp->x8_spawnNum;

#ifdef MELEE_NATIVE
    if (nativeScriptFighter_CallbackBound(gobj,NativeScriptFighterCallback_PreHitlag)) {
        if (!nativeScriptFighter_RunCallback(gobj,NativeScriptFighterCallback_PreHitlag)) return;
        if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
    } else
#endif
    if (fp->pre_hitlag_cb) {
        fp->pre_hitlag_cb(gobj);
        if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
    }

    fp->x2219_b5 = 1;

    if (fp->x1A5C && !fp->x2219_b7 &&
        nativeCallbackSource_Alive(fp->x1A5C, fp->native_hitlag_link_generation,
                                   NativeCallbackSource_Fighter)) {
        Fighter_GObj* new_gobj = gobj; ///< @todo What is going on here?
        setBit(new_gobj = fp->x1A5C);

        Fighter_UnkRecursiveFunc_8006D044(new_gobj);
    }
}

static void Fighter_8006D10C_Inline2(Fighter* fp)
{
    Fighter_GObj* gobj = fp->x1A5C;

    if (fp->x1A5C != NULL && !fp->x2219_b7 &&
        nativeCallbackSource_Alive(fp->x1A5C, fp->native_hitlag_link_generation,
                                   NativeCallbackSource_Fighter)) {
        Fighter_8006CFE0(gobj);
    }
}

static void Fighter_8006D10C_Inline1(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    const uint32_t callback_generation = (uint32_t)fp->x8_spawnNum;

    if (fp->x2219_b7) {
        if (!fp->allow_sdi && !fp->dmg.x1954) {
#ifdef MELEE_NATIVE
            if (nativeScriptFighter_CallbackBound(gobj,NativeScriptFighterCallback_PostHitlag)) {
                if (!nativeScriptFighter_RunCallback(gobj,NativeScriptFighterCallback_PostHitlag)) return;
                if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
            } else
#endif
            if (fp->post_hitlag_cb) {
                fp->post_hitlag_cb(gobj);
                if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
            }
            fp->x2219_b5 = 0;
            Fighter_8006D10C_Inline2(fp);
            if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
        }
        fp->x2219_b7 = 0;
    }
}

void Fighter_8006D10C(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    const uint32_t callback_generation = (uint32_t)fp->x8_spawnNum;

#ifdef MELEE_NATIVE
    if (nativeScriptFighter_CallbackBound(gobj,NativeScriptFighterCallback_PostHitlag)) {
        if (!nativeScriptFighter_RunCallback(gobj,NativeScriptFighterCallback_PostHitlag)) return;
        if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
    } else
#endif
    if (fp->post_hitlag_cb) {
        fp->post_hitlag_cb(gobj);
        if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
    }

    fp->x2219_b5 = 0;

    if (fp->x1A5C != NULL && !fp->x2219_b7 &&
        nativeCallbackSource_Alive(fp->x1A5C, fp->native_hitlag_link_generation,
                                   NativeCallbackSource_Fighter)) {
        Fighter_8006D10C_Inline1(fp->x1A5C);
    }
}

void Fighter_ProcessHit_8006D1EC(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    const uint32_t callback_generation = (uint32_t)fp->x8_spawnNum;
    s32 hitlag_damage = 0;
    s32 motion_state_index = fp->motion_id;
    bool bool2 = 0;
    bool bool3 = 0;
    bool bool4 = 0;
    float forceAppliedOnHit;

    if (!fp->x221F_b3) {
        if (!fp->x221A_b7) {
            if (fp->shield_health < p_ftCommonData->x260_startShieldHealth) {
                fp->shield_health += p_ftCommonData->x27C;
                if (fp->shield_health > p_ftCommonData->x260_startShieldHealth)
                {
                    fp->shield_health = p_ftCommonData->x260_startShieldHealth;
                }
            }
        }

        if (fp->x221A_b7) {
            fp->shield_health -=
                (p_ftCommonData->x284 *
                 ((fp->x19A0_shieldDamageTaken) *
                  (1.0f - ((fp->lightshield_amount *
                            (p_ftCommonData->x2E0 - p_ftCommonData->x2DC)) +
                           p_ftCommonData->x2DC)))) +
                p_ftCommonData->x288;
            if (fp->shield_health < 0.0f) {
                bool3 = 1;
                fp->shield_health = p_ftCommonData->x280_unkShieldHealth;
                /// this function is called when shield is broken
                pl_8003E058(fp->x19BC_shieldDamageTaken3, fp->x221F_b6,
                            fp->player_id, fp->x221F_b4);
            }
        }

        if (fp->dmg.x189C_unk_num_frames > 0.0f) {
            fp->dmg.x189C_unk_num_frames--;
            if (fp->dmg.x189C_unk_num_frames <= 0.0f && !fp->dmg.kb_applied) {
                fp->dmg.x189C_unk_num_frames = 0.0f;
                ftColl_8007BE3C(gobj);
            }
        }

        forceAppliedOnHit = fp->dmg.kb_applied;
        if (forceAppliedOnHit) {
            s32 ground_or_air = fp->ground_or_air;
            s32 damage;

            fp->dmg.x189C_unk_num_frames = 0.0f;
            Fighter_UnkTakeDamage_8006CC30(fp, fp->dmg.x1838_percentTemp);
            ftCo_Damage_CalcKnockback(fp);
            ftKb_SpecialN_800F5BA4(fp);

            if (nativeScriptFighter_CallbackBound(gobj, NativeScriptFighterCallback_SecondaryDamage)) {
                if (!nativeScriptFighter_RunCallback(gobj, NativeScriptFighterCallback_SecondaryDamage)) return;
                if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
            } else if (fp->take_dmg_2_cb) {
                fp->take_dmg_2_cb(gobj);
                if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
            }

            if (!fp->no_kb) {
                switch (fp->x1828) {
                case 0:
                    ftCo_8008EC90(gobj);
                    break;
                case 1:
                    ftCommon_8007DB58(gobj);
                    if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
                    ftCo_8008E908(gobj, 0.0f);
                    break;
                case 2:
                    ftCommon_8007DB58(gobj);
                    if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
                    ftCo_8008E9D0(gobj);
                    break;
                case 3:
                    ftCommon_8007DB58(gobj);
                    if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
                    ftCo_8008EB58(gobj);
                    break;
                }

                damage = fp->dmg.x183C_applied;
                bool2 = 1;
                ftCo_80090594(fp, fp->dmg.x1860_element, damage,
                              motion_state_index, ground_or_air,
                              fp->x1960_vibrateMult);
                ftCommon_8007ED50(fp, fp->dmg.x1838_percentTemp);
                hitlag_damage = damage;

            } else {
                switch (fp->kind) {
                case Ft_Kind_MasterH:
                    ftMh_MS_341_8014FE58(gobj);
                    break;
                case Ft_Kind_CrezyH:
                    ftCh_Init_80156014(gobj);
                    break;
                default:
                    HSD_ASSERTREPORT(3085, 0,
                                     "ellegal flag fp->no_reaction_always\n");
                }
                ftCo_8008E9D0(gobj);
            }
        } else if (fp->dmg.x18a0) {
            hitlag_damage = fp->dmg.x1840;
            bool4 = 1;
        } else if (fp->x19A4) {
            if (bool3) {
                ftCo_80098B20(gobj);
                ft_PlaySFX(fp, 0x82, 0x7F, 0x40);
            } else {
#ifdef MELEE_NATIVE
                if (nativeScriptFighter_CallbackBound(gobj,NativeScriptFighterCallback_ShieldHit)) {
                    NativeFighterLifetime shield_source;
                    if (!nativeFighterLifetime_Capture(fp, &shield_source) ||
                        !nativeScriptFighter_RunCallback(gobj,NativeScriptFighterCallback_ShieldHit) ||
                        !nativeFighterLifetime_Current(&shield_source)) return;
                } else
#endif
                if (fp->shield_hit_cb) {
                    fp->shield_hit_cb(gobj);
                    if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
                }
            }
            hitlag_damage = fp->x19A4;
        } else if (fp->dmg.int_value) {
            if ((fp->dmg.x191C) && (!fp->victim_gobj) &&
                (!fp->target_item_gobj))
            {
                ftCommon_8007DB58(gobj);
                if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
                ftCo_80099D9C(gobj);
            }
            hitlag_damage = fp->dmg.int_value;
        } else if (fp->dmg.x1914) {
#ifdef MELEE_NATIVE
            if (nativeScriptFighter_CallbackBound(gobj,NativeScriptFighterCallback_DealDamage)) {
                if (!nativeScriptFighter_RunCallback(gobj,NativeScriptFighterCallback_DealDamage)) return;
                if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
            } else
#endif
            if (fp->deal_dmg_cb) {
                fp->deal_dmg_cb(gobj);
                if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
            }
            hitlag_damage = fp->dmg.x1914;
            if (fp->x2070.x2073 == 0x46U) {
                ftCommon_8007EBAC(fp, 0xE, 0);
            } else {
                ftCommon_8007EE0C(fp, fp->dmg.x1914);
            }
        } else {
            if (fp->dmg.x1924) {
                hitlag_damage = fp->dmg.x1924;
            } else if (fp->ReflectAttr.x1A3C_damageOver) {
                ftCo_80098C9C(gobj);
            } else if (fp->ReflectAttr.x1A2C_reflectHitDirection) {
#ifdef MELEE_NATIVE
                if (nativeScriptFighter_CallbackBound(gobj,NativeScriptFighterCallback_ReflectHit)) {
                    if (!nativeScriptFighter_RunCallback(gobj,NativeScriptFighterCallback_ReflectHit)) return;
                    if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
                } else
#endif
                if (fp->reflect_hit_cb) {
                    fp->reflect_hit_cb(gobj);
                    if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
                }
            } else if (fp->AbsorbAttr.x1A40_absorbHitDirection) {
#ifdef MELEE_NATIVE
                if (nativeScriptFighter_IsAttached(gobj)) {
                    NativeFighterLifetime absorb_source;
                    if (!nativeFighterLifetime_Capture(fp, &absorb_source) ||
                        nativeScriptFighter_Run(gobj, NativeScriptFighter_Absorb,
                                               NULL, 0, NULL, NULL) != NativeScriptVM_Ok ||
                        !nativeFighterLifetime_Current(&absorb_source)) return;
                } else
#endif
                if (ftData_OnAbsorb[fp->kind]) {
                    ftData_OnAbsorb[fp->kind](gobj);
                    if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
                }
            } else if (fp->unk_gobj != NULL) {
#ifdef MELEE_NATIVE
                if (nativeScriptFighter_CallbackBound(gobj,NativeScriptFighterCallback_HurtboxDetect)) {
                    nativeReplay_LogBodyContact(fp);
                    if (!nativeScriptFighter_RunCallback(gobj,NativeScriptFighterCallback_HurtboxDetect)) return;
                    if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
                } else
#endif
                if (fp->hurtbox_detect_cb) {
#ifdef MELEE_NATIVE
                    nativeReplay_LogBodyContact(fp);
#endif
                    fp->hurtbox_detect_cb(gobj);
                    if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
                }
            }
        }

        if (!forceAppliedOnHit && fp->dmg.x1838_percentTemp) {
            Fighter_UnkTakeDamage_8006CC30(fp, fp->dmg.x1838_percentTemp);
            ftKb_SpecialN_800F5C34(fp);
            ftCommon_800804FC(fp);
        }
        ftCo_800C8D00(gobj);

        if (hitlag_damage) {
            fp->dmg.x195c_hitlag_frames = ftCommon_CalcHitlag(
                hitlag_damage, motion_state_index, fp->x1960_vibrateMult);
            if (fp->dmg.x195c_hitlag_frames < fp->x1964) {
                fp->dmg.x195c_hitlag_frames = fp->x1964;
            }
            if (fp->dmg.x195c_hitlag_frames > 0.0f) {
                if (fp->dmg.x195c_hitlag_frames >
                    p_ftCommonData->x194_unkHitLagFrames)
                {
                    fp->dmg.x195c_hitlag_frames =
                        p_ftCommonData->x194_unkHitLagFrames;
                }
                fp->allow_sdi = 1;
                if (bool2) {
                    fp->x221A_b3 = 1;
                }
                if (bool4) {
                    fp->dmg.x189C_unk_num_frames = fp->dmg.x195c_hitlag_frames;
                }
                if (!fp->x2219_b5) {
                    Fighter_UnkRecursiveFunc_8006D044(gobj);
                    if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
                }
            }
        } else {
            ftCo_80090718(fp);
        }

        if (fp->x221A_b0 || fp->dmg.x1958) {
            if (!fp->x2219_b5) {
                Fighter_UnkRecursiveFunc_8006D044(gobj);
                if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
            }

            if (fp->x221A_b0) {
                fp->x2219_b7 = 1;
                fp->x221A_b0 = 0;
            } else {
                if (fp->dmg.x1958 > fp->dmg.x1954) {
                    fp->dmg.x1954 = fp->dmg.x1958;
                }
                fp->dmg.x1958 = 0.0f;
            }
        }

        if (fp->dmg.x1928) {
            float eval =
                (fp->dmg.x1928 * p_ftCommonData->x3E0) + p_ftCommonData->x3E4;
            fp->xF4_ground_attacker_shield_kb_vel =
                (fp->dmg.x192c < 0.0f) ? eval : -eval;
            ftCommon_8007E2A4(gobj);
        }

        fp->dmg.x1838_percentTemp = 0.0f;
        fp->dmg.x183C_applied = 0;
        fp->x1828 = 0;
        fp->dmg.kb_applied = 0.0f;
#ifdef MELEE_NATIVE
        fp->native_contact_launch_active = false;
#endif
        fp->dmg.x18a0 = 0.0f;
        fp->dmg.x1840 = 0;
        fp->dmg.x1914 = 0;
        fp->dmg.int_value = 0;
        fp->dmg.x191C = 0.0f;
        fp->unk_gobj = NULL;
        fp->x221C_b5 = 0;

        fp->dmg.x1924 = 0;
        fp->dmg.x1928 = 0.0f;
        fp->x19A0_shieldDamageTaken = 0;
        fp->x19A4 = 0;
        fp->x19A8 = 0;
        fp->ReflectAttr.x1A3C_damageOver = 0;
        fp->ReflectAttr.x1A2C_reflectHitDirection = 0.0f;
        fp->AbsorbAttr.x1A40_absorbHitDirection = 0.0f;
        fp->AbsorbAttr.x1A44_damageTaken = 0;
        fp->AbsorbAttr.x1A48_hitsTaken = 0;
        fp->x1960_vibrateMult = 1.0f;
        fp->x1964 = 0.0f;
        fp->dmg.x1950 = 0;

        if (!fp->x2219_b6 || fp->dmg.x18F4) {
            ftCo_800C2FD8(gobj);
        }
        ftCo_800A0DA4(fp);
    }
}

void Fighter_8006D9AC(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);

    if (fp->x221F_b3 || fp->x2219_b5) {
        return;
    }

    ftCo_8009E0A8(gobj);
}

void Fighter_UnkCallCameraCallback_8006D9EC(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);

    if (!fp->x221F_b3) {
        ftCommon_8008021C(gobj);
        if (fp->cam_cb) {
            fp->cam_cb(gobj);
        }
    }
}

void Fighter_8006DA4C(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);

    if (!fp->x221F_b3) {
        Player_80032828(fp->player_id, fp->x221F_b4, &fp->cur_pos);
        Player_SetFacingDirectionConditional(fp->player_id, fp->x221F_b4,
                                             fp->facing_dir);
        pl_8003FAA8(fp->player_id, fp->x221F_b4, &fp->cur_pos, &fp->prev_pos);
    }
}

void Fighter_Unload_8006DABC(void* user_data)
{
    /// @remarks This doesn't use #GET_FIGHTER, but since it appears to pass it
    ///          directly it's probably just written directly.
    Fighter* fp = (Fighter*) user_data;
    int kind = fp->kind;

#ifdef MELEE_NATIVE
    Fighter_GObj* object = fp->gobj;
    NativeMatchPoolReference object_reference, owner_reference;
    HSD_ASSERTREPORT(0,
        nativeMatchPool_Reference(&nativeGObjPool, object, &object_reference) &&
        nativeMatchPool_Reference(&nativeFighterPool, fp, &owner_reference) &&
        object->user_data == fp, "Invalid native Fighter teardown owners.\n");
    if(!nativeScriptFighter_IsAttached(fp->gobj)) {
#endif
    if (ftData_OnUserDataRemove[kind]) {
        ftData_OnUserDataRemove[kind](fp->gobj);
#ifdef MELEE_NATIVE
        if (nativeMatchPool_Resolve(object_reference) != object ||
            nativeMatchPool_Resolve(owner_reference) != fp || object->user_data != fp)
            return;
#endif
    }
#ifdef MELEE_NATIVE
    }
    nativeScriptFighter_Detach(fp->gobj);
    if (nativeMatchPool_Resolve(object_reference) != object ||
        nativeMatchPool_Resolve(owner_reference) != fp || object->user_data != fp)
        return;
#endif

#ifdef MELEE_NATIVE
    if (fp->native_constructing)
        nativeArticleResource_CancelConstructingAttachment(object, object_reference,
                                                          owner_reference);
    if (nativeMatchPool_Resolve(object_reference) != object ||
        nativeMatchPool_Resolve(owner_reference) != fp || object->user_data != fp)
        return;
    nativeArticleResource_CancelCreator(fp->gobj, (uint32_t)fp->x8_spawnNum);
    if (nativeMatchPool_Resolve(object_reference) != object ||
        nativeMatchPool_Resolve(owner_reference) != fp || object->user_data != fp)
        return;
#endif
    ftColl_8007B8E8(fp->gobj);
    efAsync_QueueClear(&fp->x60C);
    it_8026B7F8(fp->gobj);
#ifdef MELEE_NATIVE
    if (nativeMatchPool_Resolve(object_reference) != object ||
        nativeMatchPool_Resolve(owner_reference) != fp || object->user_data != fp)
        return;
    if (fp->native_constructing)
        nativeItem_RemoveConstructingOwnerChildren(object, object_reference, owner_reference);
    if (nativeMatchPool_Resolve(object_reference) != object ||
        nativeMatchPool_Resolve(owner_reference) != fp || object->user_data != fp)
        return;
    efLib_DestroyAll(object);
    if (nativeMatchPool_Resolve(object_reference) != object ||
        nativeMatchPool_Resolve(owner_reference) != fp || object->user_data != fp)
        return;
    if (fp->x890_cameraBox != NULL) Camera_800290D4(fp->x890_cameraBox);
    nativeFighterAttachment_Clear(fp->gobj);
#else
    Camera_800290D4(fp->x890_cameraBox);
#endif
    ftCo_UnloadDynamicBones(fp);
    ftColl_800765AC(fp->gobj);
    ft_80088C5C(fp->gobj);
    lbShadow_8000EE8C(&fp->x20A4);

    if (fp->x20A0_accessory) {
        HSD_JObjRemoveAll(fp->x20A0_accessory);
    }

    HSD_JObjRemoveAll(fp->x8AC_animSkeleton);
    HSD_JObjUnref(fp->x2184);
    ftData_800859A8(fp);
    HSD_LObjRemoveAll(fp->x588);
#ifdef MELEE_NATIVE
    Player_80031FB0(fp->player_id, fp->x221F_b4, object);
#else
    Player_80031FB0(fp->player_id, fp->x221F_b4, fp->gobj);
#endif

#ifndef MELEE_NATIVE
    HSD_ObjFree(&fighter_x59C_alloc_data, fp->x59C);
    HSD_ObjFree(&fighter_x59C_alloc_data, fp->x5A0);
#endif
#ifdef MELEE_NATIVE
    HSD_ASSERTREPORT(0,nativeMatchPool_Free(&nativeFighterPool,owner_reference),
        "Invalid native Fighter owner retirement.\n");
#else
    HSD_ObjFree(&fighter_parts_alloc_data, fp->parts);
    HSD_ObjFree(&fighter_dobj_list_alloc_data, fp->dobj_list.data);
    HSD_ObjFree(&fighter_x2040_alloc_data, fp->x203C.data);
    HSD_ObjFree(&fighter_dat_attrs_alloc_data, fp->dat_attrs_backup);
    HSD_ObjFree(&fighter_alloc_data, fp);
#endif
}
