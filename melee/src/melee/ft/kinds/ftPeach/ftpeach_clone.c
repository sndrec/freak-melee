#include "ftpeach_clone.h"
#ifdef MELEE_NATIVE
#include "ftpeach.h"
#include "ftpeachspecialhi.h"
#include "ftpeachspeciallw.h"
#include "ftpeachspecialn.h"
#include "ftpeachspecials.h"

// Copied retail assets for normal project import. Fresh Workshop drafts use
// peach_full.fms; its independent build replaces every fighter-specific policy.
// These 53 semantic bindings are from PlCo.dat; all five costumes have 114
// physical joints and no engine-inserted omission joints.
static const NativeFighterBone ftPeachClone_Bones[] = {
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
    { FtPart_LShoulderJA, 69 },
    { FtPart_LShoulderJ, 70 },
    { FtPart_LArmJ, 71 },
    { FtPart_LHandN, 72 },
    { FtPart_L1stNa, 74 },
    { FtPart_L1stNb, 75 },
    { FtPart_L2ndNa, 76 },
    { FtPart_L2ndNb, 77 },
    { FtPart_L3rdNa, 78 },
    { FtPart_L3rdNb, 79 },
    { FtPart_L4thNa, 80 },
    { FtPart_L4thNb, 81 },
    { FtPart_LThumbNa, 82 },
    { FtPart_LThumbNb, 83 },
    { FtPart_LHandNb, 84 },
    { FtPart_NeckN, 85 },
    { FtPart_HeadN, 86 },
    { FtPart_RShoulderN, 87 },
    { FtPart_RShoulderJA, 95 },
    { FtPart_RShoulderJ, 96 },
    { FtPart_RArmJ, 97 },
    { FtPart_RHandN, 98 },
    { FtPart_R1stNa, 100 },
    { FtPart_R1stNb, 101 },
    { FtPart_R2ndNa, 102 },
    { FtPart_R2ndNb, 103 },
    { FtPart_R3rdNa, 104 },
    { FtPart_R3rdNb, 105 },
    { FtPart_R4thNa, 106 },
    { FtPart_R4thNb, 107 },
    { FtPart_RThumbNa, 108 },
    { FtPart_RThumbNb, 109 },
    { FtPart_RHandNb, 110 },
    { FtPart_ThrowN, 111 },
    { FtPart_TransN2, 112 },
    { 53, 113 }, // Unnamed final PlCo semantic binding.
};

static const NativeFighterArticleAsset ftPeachClone_Articles[] = {
    { It_Kind_Peach_Explode, 0, -1 },
    { It_Kind_Peach_Turnip, 1, -1 },
    { It_Kind_Peach_Parasol, 2, -1 },
    { It_Kind_Peach_Toad, 3, -1 },
    { It_Kind_Peach_ToadSpore, 4, -1 },
};

static const Fighter_DemoStrings ftPeachClone_DemoMotionNames = {
    "ftDemoResultMotionFilePeach",
    "ftDemoIntroMotionFilePeach",
    "ftDemoEndingMotionFilePeach",
    "ftDemoViWaitMotionFilePeach",
};

const NativeFighterDefinition ftPeachClone_Definition = {
    .code_key = "peach_clone",
    .source_kind = Ft_Kind_Peach,
    .animation_source_kind = Ft_Kind_Peach,
    .sound_banks = UINT64_C(1) << 22,
    .sound_mute_channels = 0x44,
    .effect_bank = 15,
    .asset_layout = { .article_mask = 0x01F },
    .article_assets = ftPeachClone_Articles,
    .article_asset_count = sizeof(ftPeachClone_Articles) / sizeof(ftPeachClone_Articles[0]),
    .bones = ftPeachClone_Bones,
    .bone_count = sizeof(ftPeachClone_Bones) / sizeof(ftPeachClone_Bones[0]),
    .demo_motion_names = &ftPeachClone_DemoMotionNames,
    .fighter_root = "ftDataPeach",
    .costume_joint = "PlyPeach5K_Share_joint",
    .costume_matanim = "PlyPeach5K_Share_matanim_joint",
    .action_count = 318, .demo_count = 14, .joint_count = 114,
    .motion_state_first = ftCo_MS_Count,
    .motion_state_count = ftPe_MS_SelfCount,
    .motion_states = ftPe_Init_MotionStateTable,
    .special_ground_state = { ftPe_MS_SpecialN, ftPe_MS_SpecialSStart,
                              ftPe_MS_SpecialHiStart, ftPe_MS_SpecialLw },
    .special_air_state = { ftPe_MS_SpecialAirN, ftPe_MS_SpecialAirSStart,
                           ftPe_MS_SpecialAirHiStart, ftPe_MS_SpecialAirLw },
    .special_n = ftPe_SpecialN_Enter,
    .special_air_n = ftPe_SpecialAirN_Enter,
    .special_s = ftPe_SpecialS_Enter,
    .special_air_s = ftPe_SpecialAirS_Enter,
    .special_hi = ftPe_SpecialHi_Enter,
    .special_air_hi = ftPe_SpecialAirHi_Enter,
    .special_lw = ftPe_SpecialLw_Enter,
    .special_air_lw = ftPe_SpecialAirLw_Enter,
};
#endif
