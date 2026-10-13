#include "ftclink_clone.h"
#ifdef MELEE_NATIVE
#include <melee/ft/types.h>
#include "ftclink.h"
#include <melee/ft/kinds/ftLink/ftlinkspecialhi.h>
#include <melee/ft/kinds/ftLink/ftlinkspeciallw.h>
#include <melee/ft/kinds/ftLink/ftlinkspecialn.h>
#include <melee/ft/kinds/ftLink/ftlinkspecials.h>

// Copied immutable asset metadata and raw stock fallback motions.
// Fresh Workshop clones compile the independent Young Link template.
static const NativeFighterBone ftCLinkClone_Bones[] = {
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
    { FtPart_BustN, 17 },
    { FtPart_LShoulderN, 18 },
    { FtPart_LShoulderJA, 19 },
    { FtPart_LShoulderJ, 20 },
    { FtPart_LArmJ, 21 },
    { FtPart_LHandN, 22 },
    { FtPart_L1stNa, 25 },
    { FtPart_L1stNb, 29 },
    { FtPart_L2ndNa, 30 },
    { FtPart_L2ndNb, 31 },
    { FtPart_L3rdNa, 32 },
    { FtPart_L3rdNb, 33 },
    { FtPart_L4thNa, 34 },
    { FtPart_L4thNb, 35 },
    { FtPart_LThumbNa, 36 },
    { FtPart_LThumbNb, 37 },
    { FtPart_LHandNb, 38 },
    { FtPart_NeckN, 39 },
    { FtPart_HeadN, 41 },
    { FtPart_RShoulderN, 42 },
    { FtPart_RShoulderJA, 53 },
    { FtPart_RShoulderJ, 54 },
    { FtPart_RArmJ, 55 },
    { FtPart_RHandN, 56 },
    { FtPart_R1stNa, 59 },
    { FtPart_R1stNb, 60 },
    { FtPart_R2ndNa, 61 },
    { FtPart_R2ndNb, 62 },
    { FtPart_R3rdNa, 63 },
    { FtPart_R3rdNb, 64 },
    { FtPart_R4thNa, 65 },
    { FtPart_R4thNb, 66 },
    { FtPart_RThumbNa, 67 },
    { FtPart_RThumbNb, 68 },
    { FtPart_RHandNb, 69 },
    { FtPart_ThrowN, 70 },
    { FtPart_TransN2, 78 },
    { 53, 79 },
};

static const NativeFighterPartOmission ftCLinkClone_Omissions[] = {
    { 72, 71, 0, 255 },
};

static const NativeFighterArticleAsset ftCLinkClone_ArticleAssets[] = {
    { It_Kind_CLink_Bomb, 0, 72 },
    { It_Kind_CLink_Boomerang, 1, 44 },
    { It_Kind_CLink_HShot, 2, 188 },
    { It_Kind_CLink_Arrow, 3, 12 },
    { It_Kind_CLink_Bow, 4, 16 },
    { It_Kind_CLink_Milk, 5, -1 },
};
static const Fighter_DemoStrings ftCLinkClone_DemoMotionNames = {
    "ftDemoResultMotionFileClink",
    "ftDemoIntroMotionFileClink",
    "ftDemoEndingMotionFileClink",
    "ftDemoViWaitMotionFileClink",
};

const NativeFighterDefinition ftCLinkClone_Definition = {
    .demo_motion_names = &ftCLinkClone_DemoMotionNames,
    .code_key = "clink_clone",
    .sound_banks = UINT64_C(1) << 7,
    .sound_mute_channels = 0x44,
    .source_kind = Ft_Kind_CLink,
    .asset_layout = { .article_mask = 0x03F, .joint_graph_mask = 0x040,
                       .item_format = FtCharacterItems_LinkWeapons },
    .effect_bank = 6,
    .article_asset_count = sizeof(ftCLinkClone_ArticleAssets) / sizeof(ftCLinkClone_ArticleAssets[0]),
    .article_assets = ftCLinkClone_ArticleAssets,
    .bones = ftCLinkClone_Bones,
    .bone_count = sizeof(ftCLinkClone_Bones) / sizeof(ftCLinkClone_Bones[0]),
    .omissions = ftCLinkClone_Omissions,
    .omission_count = sizeof(ftCLinkClone_Omissions) / sizeof(ftCLinkClone_Omissions[0]),
    .animation_source_kind = Ft_Kind_CLink,
    .fighter_root = "ftDataClink",
    .costume_joint = "PlyClink5K_Share_joint",
    .costume_matanim = "PlyClink5K_Share_matanim_joint",
    .action_count = 314, .demo_count = 14, .joint_count = 79,
    .motion_state_count = ftLk_MS_SelfCount,
    .motion_states = ftCl_Init_MotionStateTable,
    .motion_state_first = ftCo_MS_Count,
    .special_ground_state = { ftLk_MS_SpecialNStart, ftLk_MS_SpecialS1,
                              ftLk_MS_SpecialHi, ftLk_MS_SpecialLw },
    .special_air_state = { ftLk_MS_SpecialAirNStart, ftLk_MS_SpecialAirS1,
                           ftLk_MS_SpecialAirHi, ftLk_MS_SpecialAirLw },
    .special_n = ftLk_SpecialN_Enter, .special_air_n = ftLk_SpecialAirN_Enter,
    .special_s = ftLk_SpecialS_Enter, .special_air_s = ftLk_SpecialAirS_Enter,
    .special_hi = ftLk_SpecialHi_Enter,
    .special_air_hi = ftLk_SpecialAirHi_Enter,
    .special_lw = ftLk_SpecialLw_Enter,
    .special_air_lw = ftLk_SpecialAirLw_Enter,
};
#endif
