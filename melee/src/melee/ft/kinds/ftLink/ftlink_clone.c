#include "ftlink_clone.h"
#ifdef MELEE_NATIVE
#include <melee/ft/types.h>
#include "ftlink.h"
#include "ftlinkspecialhi.h"
#include "ftlinkspeciallw.h"
#include "ftlinkspecialn.h"
#include "ftlinkspecials.h"

// Copied immutable asset metadata and raw stock fallback motions.
// Fresh Workshop clones compile the independent Link-family template.
static const NativeFighterBone ftLinkClone_Bones[] = {
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
    { FtPart_L1stNa, 23 },
    { FtPart_L1stNb, 27 },
    { FtPart_L2ndNa, 28 },
    { FtPart_L2ndNb, 29 },
    { FtPart_L3rdNa, 30 },
    { FtPart_L3rdNb, 31 },
    { FtPart_L4thNa, 32 },
    { FtPart_L4thNb, 33 },
    { FtPart_LThumbNa, 34 },
    { FtPart_LThumbNb, 35 },
    { FtPart_LHandNb, 36 },
    { FtPart_NeckN, 37 },
    { FtPart_HeadN, 39 },
    { FtPart_RShoulderN, 40 },
    { FtPart_RShoulderJA, 51 },
    { FtPart_RShoulderJ, 52 },
    { FtPart_RArmJ, 53 },
    { FtPart_RHandN, 54 },
    { FtPart_R1stNa, 55 },
    { FtPart_R1stNb, 56 },
    { FtPart_R2ndNa, 57 },
    { FtPart_R2ndNb, 58 },
    { FtPart_R3rdNa, 59 },
    { FtPart_R3rdNb, 60 },
    { FtPart_R4thNa, 61 },
    { FtPart_R4thNb, 62 },
    { FtPart_RThumbNa, 63 },
    { FtPart_RThumbNb, 64 },
    { FtPart_RHandNb, 65 },
    { FtPart_ThrowN, 66 },
    { FtPart_TransN2, 74 },
    { 53, 75 },
};

static const NativeFighterPartOmission ftLinkClone_Omissions[] = {
    { 68, 67, 0, 255 },
};

static const NativeFighterArticleAsset ftLinkClone_ArticleAssets[] = {
    { It_Kind_Link_Bomb, 0, 72 },
    { It_Kind_Link_Boomerang, 1, 44 },
    { It_Kind_Link_HShot, 2, 188 },
    { It_Kind_Link_Arrow, 3, 12 },
    { It_Kind_Link_Bow, 4, 16 },
};
static const Fighter_DemoStrings ftLinkClone_DemoMotionNames = {
    "ftDemoResultMotionFileLink",
    "ftDemoIntroMotionFileLink",
    "ftDemoEndingMotionFileLink",
    "ftDemoViWaitMotionFileLink",
};

const NativeFighterDefinition ftLinkClone_Definition = {
    .demo_motion_names = &ftLinkClone_DemoMotionNames,
    .code_key = "link_clone",
    .sound_banks = UINT64_C(1) << 16,
    .sound_mute_channels = 0x44,
    .source_kind = Ft_Kind_Link,
    .asset_layout = { .article_mask = 0x01F, .joint_graph_mask = 0x040,
                      .item_format = FtCharacterItems_LinkWeapons },
    .effect_bank = 6,
    .article_asset_count = sizeof(ftLinkClone_ArticleAssets) / sizeof(ftLinkClone_ArticleAssets[0]),
    .article_assets = ftLinkClone_ArticleAssets,
    .bones = ftLinkClone_Bones,
    .bone_count = sizeof(ftLinkClone_Bones) / sizeof(ftLinkClone_Bones[0]),
    .omissions = ftLinkClone_Omissions,
    .omission_count = sizeof(ftLinkClone_Omissions) / sizeof(ftLinkClone_Omissions[0]),
    .animation_source_kind = Ft_Kind_Link,
    .fighter_root = "ftDataLink",
    .costume_joint = "PlyLink5K_Share_joint",
    .costume_matanim = "PlyLink5K_Share_matanim_joint",
    .action_count = 314, .demo_count = 14, .joint_count = 75,
    .motion_state_count = ftLk_MS_SelfCount,
    .motion_states = ftLk_Init_MotionStateTable,
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
