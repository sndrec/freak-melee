#include "ftzelda_clone.h"
#ifdef MELEE_NATIVE
#include <melee/ft/kinds/ftZelda/ftzelda.h>
#include <melee/ft/kinds/ftZelda/ftzeldaspecialn.h>
#include <melee/ft/kinds/ftZelda/ftzeldaspecials.h>
#include <melee/ft/kinds/ftZelda/ftzeldaspecialhi.h>
#include <melee/ft/kinds/ftZelda/ftzeldaspeciallw.h>
#include <melee/ft/types.h>

// Copied retail asset metadata. A raw, unscripted clone retains stock behavior;
// independent generated modules replace every fighter-specific callback.
static const NativeFighterBone ftZeldaClone_Bones[] = {
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
    { FtPart_BustN, 66 },
    { FtPart_LShoulderN, 67 },
    { FtPart_LShoulderJA, 68 },
    { FtPart_LShoulderJ, 71 },
    { FtPart_LArmJ, 72 },
    { FtPart_LHandN, 73 },
    { FtPart_L1stNa, 76 },
    { FtPart_L1stNb, 77 },
    { FtPart_L2ndNa, 78 },
    { FtPart_L2ndNb, 79 },
    { FtPart_L3rdNa, 80 },
    { FtPart_L3rdNb, 81 },
    { FtPart_L4thNa, 82 },
    { FtPart_L4thNb, 83 },
    { FtPart_LThumbNa, 84 },
    { FtPart_LThumbNb, 85 },
    { FtPart_LHandNb, 86 },
    { FtPart_NeckN, 87 },
    { FtPart_HeadN, 88 },
    { FtPart_RShoulderN, 89 },
    { FtPart_RShoulderJA, 96 },
    { FtPart_RShoulderJ, 99 },
    { FtPart_RArmJ, 100 },
    { FtPart_RHandN, 101 },
    { FtPart_R1stNa, 104 },
    { FtPart_R1stNb, 105 },
    { FtPart_R2ndNa, 106 },
    { FtPart_R2ndNb, 107 },
    { FtPart_R3rdNa, 108 },
    { FtPart_R3rdNb, 109 },
    { FtPart_R4thNa, 110 },
    { FtPart_R4thNb, 111 },
    { FtPart_RThumbNa, 112 },
    { FtPart_RThumbNb, 113 },
    { FtPart_RHandNb, 114 },
    { FtPart_ThrowN, 115 },
    { FtPart_TransN2, 116 },
    { 53, 117 },
};

static const NativeFighterArticleAsset ftZeldaClone_Articles[] = {
    { It_Kind_Zelda_DinFire, 0, -1 },
    { It_Kind_Zelda_DinFire_Explode, 1, -1 },
};

static const Fighter_DemoStrings ftZeldaClone_DemoMotionNames = {
    "ftDemoResultMotionFileZelda",
    "ftDemoIntroMotionFileZelda",
    "ftDemoEndingMotionFileZelda",
    "ftDemoViWaitMotionFileZelda",
};

const NativeFighterDefinition ftZeldaClone_Definition = {
    .code_key = "zelda_clone", .source_kind = Ft_Kind_Zelda,
    .animation_source_kind = Ft_Kind_Zelda,
    .sound_banks = UINT64_C(1) << 27, .sound_mute_channels = 0x44, .effect_bank = 17,
    .asset_layout = { .article_mask = 0x003 },
    .article_assets = ftZeldaClone_Articles, .article_asset_count = 2,
    .bones = ftZeldaClone_Bones, .bone_count = 53,
    .demo_motion_names = &ftZeldaClone_DemoMotionNames,
    .fighter_root = "ftDataZelda", .costume_joint = "PlyZelda5K_Share_joint",
    .costume_matanim = "PlyZelda5K_Share_matanim_joint",
    .action_count = 311, .demo_count = 14, .joint_count = 118,
    .motion_state_first = ftCo_MS_Count, .motion_state_count = ftZd_MS_SelfCount,
    .motion_states = ftZd_Init_MotionStateTable,
    .special_ground_state = { ftZd_MS_SpecialN, ftZd_MS_SpecialSStart, ftZd_MS_SpecialHiStart_0, ftZd_MS_SpecialLw },
    .special_air_state = { ftZd_MS_SpecialAirN, ftZd_MS_SpecialAirSStart, ftZd_MS_SpecialAirHiStart_0, ftZd_MS_SpecialAirLw },
    .special_n = ftZd_SpecialN_Enter,
    .special_air_n = ftZd_SpecialAirN_Enter,
    .special_s = ftZd_SpecialS_Enter,
    .special_air_s = ftZd_SpecialAirS_Enter,
    .special_hi = ftZd_SpecialHi_Enter,
    .special_air_hi = ftZd_SpecialAirHi_Enter,
    .special_lw = ftZd_SpecialLw_Enter,
    .special_air_lw = ftZd_SpecialAirLw_Enter,
};
#endif
