#include "ftmewtwo_clone.h"
#ifdef MELEE_NATIVE
#include <melee/ft/types.h>
#include <melee/ft/ftdat.h>
#include "ftmewtwo.h"
#include "ftmewtwospecialhi.h"
#include "ftmewtwospeciallw.h"
#include "ftmewtwospecialn.h"
#include "ftmewtwospecials.h"

// Independent copied assets. mewtwo_wip.fms owns all four specials and both
// combat articles, forward throw, CPU charge decisions and character policies.
// Runtime coverage for the new common/lifecycle batch is recorded in the ledger.
// Semantic joints copied from the supplied v1.02 PlCo.dat; no missing parts.
static const NativeFighterBone ftMewtwoClone_Bones[] = {
    { FtPart_TopN, 0 },
    { FtPart_TransN, 2 },
    { FtPart_XRotN, 3 },
    { FtPart_YRotN, 4 },
    { FtPart_HipN, 5 },
    { FtPart_LLegJA, 7 },
    { FtPart_LLegJ, 8 },
    { FtPart_LKneeJ, 9 },
    { FtPart_LFootJA, 10 },
    { FtPart_LFootJ, 11 },
    { FtPart_RLegJA, 13 },
    { FtPart_RLegJ, 14 },
    { FtPart_RKneeJ, 15 },
    { FtPart_RFootJA, 16 },
    { FtPart_RFootJ, 17 },
    { FtPart_BustN, 27 },
    { FtPart_LShoulderN, 28 },
    { FtPart_LShoulderJA, 29 },
    { FtPart_LShoulderJ, 30 },
    { FtPart_LArmJ, 31 },
    { FtPart_LHandN, 32 },
    { FtPart_L1stNa, 35 },
    { FtPart_L1stNb, 36 },
    { FtPart_L2ndNa, 37 },
    { FtPart_L2ndNb, 38 },
    { FtPart_L3rdNa, 39 },
    { FtPart_L3rdNb, 40 },
    { FtPart_L4thNa, 41 },
    { FtPart_HeadN, 42 },
    { FtPart_RShoulderN, 43 },
    { FtPart_RShoulderJA, 52 },
    { FtPart_RShoulderJ, 53 },
    { FtPart_RArmJ, 54 },
    { FtPart_RHandN, 55 },
    { FtPart_R1stNa, 58 },
    { FtPart_R1stNb, 59 },
    { FtPart_R2ndNa, 60 },
    { FtPart_R2ndNb, 61 },
    { FtPart_R3rdNa, 62 },
    { FtPart_R3rdNb, 63 },
    { FtPart_R4thNa, 64 },
    { FtPart_RThumbNb, 65 },
    { FtPart_TransN2, 66 },
    { (Fighter_Part)(FT_COMMON_PART_COUNT-1), 67 },
};

static const NativeFighterArticleAsset ftMewtwoClone_ArticleAssets[] = {
    { It_Kind_Mewtwo_Disable, 0, -1 },
    { It_Kind_Mewtwo_ShadowBall, 1, -1 },
};

static const Fighter_DemoStrings ftMewtwoClone_DemoMotionNames = {
    "ftDemoResultMotionFileMewtwo",
    "ftDemoIntroMotionFileMewtwo",
    "ftDemoEndingMotionFileMewtwo",
    "ftDemoViWaitMotionFileMewtwo",
};

const NativeFighterDefinition ftMewtwoClone_Definition = {
    .demo_motion_names = &ftMewtwoClone_DemoMotionNames,
    .code_key = "mewtwo_clone",
    .sound_banks = UINT64_C(1) << 20,
    .sound_mute_channels = 0x44,
    .source_kind = Ft_Kind_Mewtwo,
    .asset_layout = { .article_mask = 0x003 },
    .effect_bank = 13,
    .article_asset_count = sizeof(ftMewtwoClone_ArticleAssets) / sizeof(ftMewtwoClone_ArticleAssets[0]),
    .article_assets = ftMewtwoClone_ArticleAssets,
    .bones = ftMewtwoClone_Bones,
    .bone_count = sizeof(ftMewtwoClone_Bones) / sizeof(ftMewtwoClone_Bones[0]),
    .animation_source_kind = Ft_Kind_Mewtwo,
    .fighter_root = "ftDataMewtwo",
    .costume_joint = "PlyMewtwo5K_Share_joint",
    .costume_matanim = "PlyMewtwo5K_Share_matanim_joint",
    .action_count = 314, .demo_count = 14, .joint_count = 68,
    .motion_state_count = ftMt_MS_SelfCount,
    .motion_states = ftMt_Init_MotionStateTable,
    .motion_state_first = ftCo_MS_Count,
    .special_ground_state = { ftMt_MS_SpecialNStart, ftMt_MS_SpecialS,
                              ftMt_MS_SpecialHiStart, ftMt_MS_SpecialLw },
    .special_air_state = { ftMt_MS_SpecialAirNStart, ftMt_MS_SpecialAirS,
                           ftMt_MS_SpecialAirHiStart, ftMt_MS_SpecialAirLw },
    .special_n = ftMt_SpecialN_Enter,
    .special_air_n = ftMt_SpecialAirN_Enter,
    .special_s = ftMt_SpecialS_Enter,
    .special_air_s = ftMt_SpecialAirS_Enter,
    .special_hi = ftMt_SpecialHiStart_Enter,
    .special_air_hi = ftMt_SpecialAirHiStart_Enter,
    .special_lw = ftMt_SpecialLw_Enter,
    .special_air_lw = ftMt_SpecialAirLw_Enter,
};
#endif
