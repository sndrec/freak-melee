#include "ftfalco_clone.h"
#ifdef MELEE_NATIVE
#include <melee/ft/types.h>
#include <melee/ft/ftdat.h>
#include "ftfalco.h"
#include <melee/ft/kinds/ftFox/ftfoxspecialn.h>
#include <melee/ft/kinds/ftFox/ftfoxspecials.h>
#include <melee/ft/kinds/ftFox/ftfoxspecialhi.h>
#include <melee/ft/kinds/ftFox/ftfoxspeciallw.h>

// Independent asset base. The shared script owns Reflector, blaster/throw,
// side/up specials, radio and basic events; lifecycle acceptance is ongoing.
// Exact semantic bindings from retail v1.02 PlCo.dat; no omissions.
static const NativeFighterBone ftFalcoClone_Bones[] = {
    { FtPart_TopN, 0 },
    { FtPart_TransN, 1 },
    { FtPart_XRotN, 2 },
    { FtPart_YRotN, 3 },
    { FtPart_HipN, 4 },
    { FtPart_LLegJA, 5 },
    { FtPart_LLegJ, 6 },
    { FtPart_LKneeJ, 7 },
    { FtPart_LFootJA, 8 },
    { FtPart_LFootJ, 9 },
    { FtPart_RLegJA, 11 },
    { FtPart_RLegJ, 12 },
    { FtPart_RKneeJ, 13 },
    { FtPart_RFootJA, 14 },
    { FtPart_RFootJ, 15 },
    { FtPart_BustN, 18 },
    { FtPart_LShoulderN, 19 },
    { FtPart_LShoulderJA, 21 },
    { FtPart_LShoulderJ, 22 },
    { FtPart_LArmJ, 23 },
    { FtPart_LHandN, 24 },
    { FtPart_L1stNa, 26 },
    { FtPart_L1stNb, 27 },
    { FtPart_L2ndNa, 28 },
    { FtPart_L2ndNb, 29 },
    { FtPart_L3rdNa, 30 },
    { FtPart_L3rdNb, 31 },
    { FtPart_L4thNa, 32 },
    { FtPart_L4thNb, 33 },
    { FtPart_LThumbNa, 34 },
    { FtPart_LHandNb, 35 },
    { FtPart_NeckN, 36 },
    { FtPart_HeadN, 38 },
    { FtPart_RShoulderN, 39 },
    { FtPart_RShoulderJA, 47 },
    { FtPart_RShoulderJ, 48 },
    { FtPart_RArmJ, 49 },
    { FtPart_RHandN, 50 },
    { FtPart_R1stNa, 52 },
    { FtPart_R1stNb, 53 },
    { FtPart_R2ndNa, 54 },
    { FtPart_R2ndNb, 55 },
    { FtPart_R3rdNa, 56 },
    { FtPart_R3rdNb, 57 },
    { FtPart_R4thNa, 58 },
    { FtPart_R4thNb, 59 },
    { FtPart_RThumbNa, 60 },
    { FtPart_RThumbNb, 61 },
    { FtPart_RHandNb, 62 },
    { FtPart_ThrowN, 63 },
    { FtPart_TransN2, 65 },
    { 53, 66 },
};

static const NativeFighterArticleAsset ftFalcoClone_ArticleAssets[] = {
    { It_Kind_Falco_Laser, 0, -1 },
    { It_Kind_Falco_Blaster, 1, -1 },
    { It_Kind_Falco_Phantasm, 3, -1 },
};

static const Fighter_DemoStrings ftFalcoClone_DemoMotionNames = {
    "ftDemoResultMotionFileFalco",
    "ftDemoIntroMotionFileFalco",
    "ftDemoEndingMotionFileFalco",
    "ftDemoViWaitMotionFileFalco",
};

const NativeFighterDefinition ftFalcoClone_Definition = {
    .demo_motion_names = &ftFalcoClone_DemoMotionNames,
    .code_key = "falco_clone",
    .sound_banks = UINT64_C(1) << 10, // falco.ssm
    .sound_mute_channels = 0x44,
    .source_kind = Ft_Kind_Falco,
    .asset_layout = { .article_mask = 0x00B },
    .effect_bank = 3,
    .article_asset_count = sizeof(ftFalcoClone_ArticleAssets) / sizeof(ftFalcoClone_ArticleAssets[0]),
    .article_assets = ftFalcoClone_ArticleAssets,
    .bones = ftFalcoClone_Bones,
    .bone_count = sizeof(ftFalcoClone_Bones) / sizeof(ftFalcoClone_Bones[0]),
    .animation_source_kind = Ft_Kind_Falco,
    .fighter_root = "ftDataFalco",
    .costume_joint = "PlyFalco5K_Share_joint",
    .costume_matanim = "PlyFalco5K_Share_matanim_joint",
    .action_count = 327,
    .demo_count = 14,
    .motion_state_count = ftFx_MS_SelfCount,
    .motion_states = ftFc_Init_MotionStateTable,
    .motion_state_first = ftFx_MS_SpecialNStart,
    .special_ground_state = { ftFx_MS_SpecialNStart, ftFx_MS_SpecialSStart,
                              ftFx_MS_SpecialHiHold, ftFx_MS_SpecialLwStart },
    .special_air_state = { ftFx_MS_SpecialAirNStart, ftFx_MS_SpecialAirSStart,
                           ftFx_MS_SpecialHiHoldAir, ftFx_MS_SpecialAirLwStart },
    .special_transition_flags = {
        [NativeFighterSpecial_Down] = ftCommon_GroundAirColl_MF | Ft_MF_KeepGfx,
    },
    .joint_count = 67,
    .special_n = ftFx_SpecialN_Enter,
    .special_air_n = ftFx_SpecialAirN_Enter,
    .special_s = ftFx_SpecialSStart_Enter,
    .special_air_s = ftFx_SpecialAirSStart_Enter,
    .special_hi = ftFx_SpecialHi_Enter,
    .special_air_hi = ftFx_SpecialAirHiStart_Enter,
    .special_lw = ftFx_SpecialLw_Enter,
    .special_air_lw = ftFx_SpecialAirLw_Enter,
};
#endif
