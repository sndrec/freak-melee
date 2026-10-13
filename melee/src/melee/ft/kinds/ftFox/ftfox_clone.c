#include "ftfox_clone.h"
#ifdef MELEE_NATIVE
#include <melee/ft/types.h>
#include <melee/ft/ftdat.h>
#include "ftfox.h"
#include <melee/ft/kinds/ftFox/ftfoxspecialn.h>
#include <melee/ft/kinds/ftFox/ftfoxspecials.h>
#include <melee/ft/kinds/ftFox/ftfoxspecialhi.h>
#include <melee/ft/kinds/ftFox/ftfoxspeciallw.h>

// Independent asset base. The shared script owns Reflector, blaster/throw,
// side/up specials, radio and basic events; lifecycle acceptance is ongoing.
// Exact semantic bindings from retail v1.02 PlCo.dat; no omissions.
static const NativeFighterBone ftFoxClone_Bones[] = {
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
    { FtPart_BustN, 21 },
    { FtPart_LShoulderN, 22 },
    { FtPart_LShoulderJA, 23 },
    { FtPart_LShoulderJ, 24 },
    { FtPart_LArmJ, 25 },
    { FtPart_LHandN, 26 },
    { FtPart_L1stNa, 27 },
    { FtPart_L1stNb, 28 },
    { FtPart_L2ndNa, 29 },
    { FtPart_L2ndNb, 30 },
    { FtPart_L3rdNa, 31 },
    { FtPart_L3rdNb, 32 },
    { FtPart_L4thNa, 33 },
    { FtPart_L4thNb, 34 },
    { FtPart_LThumbNa, 35 },
    { FtPart_LHandNb, 37 },
    { FtPart_NeckN, 38 },
    { FtPart_HeadN, 40 },
    { FtPart_RShoulderN, 41 },
    { FtPart_RShoulderJA, 53 },
    { FtPart_RShoulderJ, 54 },
    { FtPart_RArmJ, 55 },
    { FtPart_RHandN, 56 },
    { FtPart_R1stNa, 57 },
    { FtPart_R1stNb, 58 },
    { FtPart_R2ndNa, 59 },
    { FtPart_R2ndNb, 60 },
    { FtPart_R3rdNa, 61 },
    { FtPart_R3rdNb, 62 },
    { FtPart_R4thNa, 63 },
    { FtPart_R4thNb, 64 },
    { FtPart_RThumbNa, 65 },
    { FtPart_RThumbNb, 67 },
    { FtPart_RHandNb, 68 },
    { FtPart_ThrowN, 69 },
    { FtPart_TransN2, 71 },
    { 53, 72 },
};

static const NativeFighterArticleAsset ftFoxClone_ArticleAssets[] = {
    { It_Kind_Fox_Laser, 0, -1 },
    { It_Kind_Fox_Blaster, 1, -1 },
    { It_Kind_Fox_Illusion, 2, -1 },
};

static const Fighter_DemoStrings ftFoxClone_DemoMotionNames = {
    "ftDemoResultMotionFileFox",
    "ftDemoIntroMotionFileFox",
    "ftDemoEndingMotionFileFox",
    "ftDemoViWaitMotionFileFox",
};

const NativeFighterDefinition ftFoxClone_Definition = {
    .demo_motion_names = &ftFoxClone_DemoMotionNames,
    .code_key = "fox_clone",
    .sound_banks = UINT64_C(1) << 11,
    .sound_mute_channels = 0x44,
    .source_kind = Ft_Kind_Fox,
    .asset_layout = { .article_mask = 0x007 },
    .effect_bank = 3,
    .article_asset_count = sizeof(ftFoxClone_ArticleAssets) / sizeof(ftFoxClone_ArticleAssets[0]),
    .article_assets = ftFoxClone_ArticleAssets,
    .bones = ftFoxClone_Bones,
    .bone_count = sizeof(ftFoxClone_Bones) / sizeof(ftFoxClone_Bones[0]),
    .animation_source_kind = Ft_Kind_Fox,
    .fighter_root = "ftDataFox",
    .costume_joint = "PlyFox5K_Share_joint",
    .costume_matanim = "PlyFox5K_Share_matanim_joint",
    .action_count = 327,
    .demo_count = 14,
    .motion_state_count = ftFx_MS_SelfCount,
    .motion_states = ftFx_Init_MotionStateTable,
    .motion_state_first = ftFx_MS_SpecialNStart,
    .special_ground_state = { ftFx_MS_SpecialNStart, ftFx_MS_SpecialSStart,
                              ftFx_MS_SpecialHiHold, ftFx_MS_SpecialLwStart },
    .special_air_state = { ftFx_MS_SpecialAirNStart, ftFx_MS_SpecialAirSStart,
                           ftFx_MS_SpecialHiHoldAir, ftFx_MS_SpecialAirLwStart },
    .special_transition_flags = {
        [NativeFighterSpecial_Down] = ftCommon_GroundAirColl_MF | Ft_MF_KeepGfx,
    },
    .joint_count = 73,
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
