#include "ftsamus_clone.h"
#ifdef MELEE_NATIVE
#include <melee/ft/types.h>
#include "ftsamus.h"
#include "ftsamusspecialhi.h"
#include "ftsamusspeciallw1.h"
#include "ftsamusspecialn.h"
#include "ftsamusspecials.h"

// Copied retail assets for normal project import. Fresh Workshop drafts use
// samus_full.fms; independent preparation replaces these authoring-reference
// callbacks with the complete script-owned fighter and four combat articles.
// Retail v1.02 PlCo semantic map; all five costumes have 60 physical joints
// and no part insertions. These are copied asset facts, not stock callbacks.
static const NativeFighterBone ftSamusClone_Bones[] = {
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
    { FtPart_RLegJA, 12 },
    { FtPart_RLegJ, 13 },
    { FtPart_RKneeJ, 14 },
    { FtPart_RFootJA, 15 },
    { FtPart_RFootJ, 16 },
    { FtPart_BustN, 19 },
    { FtPart_LShoulderN, 20 },
    { FtPart_LShoulderJA, 21 },
    { FtPart_LShoulderJ, 25 },
    { FtPart_LArmJ, 26 },
    { FtPart_LHandN, 27 },
    { FtPart_L1stNa, 28 },
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
    { FtPart_RShoulderJA, 43 },
    { FtPart_RShoulderJ, 47 },
    { FtPart_RArmJ, 48 },
    { FtPart_RHandN, 49 },
    { FtPart_TransN2, 58 },
    { 53, 59 },
};
static const NativeFighterArticleAsset ftSamusClone_ArticleAssets[] = {
    { It_Kind_Samus_Bomb, 0, -1 },
    { It_Kind_Samus_Charge, 1, -1 },
    { It_Kind_Samus_Missile, 2, -1 },
    { It_Kind_Samus_GBeam, 3, -1 },
};

static const Fighter_DemoStrings ftSamusClone_DemoMotionNames = {
    "ftDemoResultMotionFileSamus",
    "ftDemoIntroMotionFileSamus",
    "ftDemoEndingMotionFileSamus",
    "ftDemoViWaitMotionFileSamus",
};

const NativeFighterDefinition ftSamusClone_Definition = {
    .demo_motion_names = &ftSamusClone_DemoMotionNames,
    .code_key = "samus_clone",
    .sound_banks = UINT64_C(1) << 26,
    .sound_mute_channels = 0x00,
    .source_kind = Ft_Kind_Samus,
    .asset_layout = { .article_mask = 0x00F,
                       .item_format = FtCharacterItems_Grapple },
    .effect_bank = 2,
    .article_asset_count = sizeof(ftSamusClone_ArticleAssets) / sizeof(ftSamusClone_ArticleAssets[0]),
    .article_assets = ftSamusClone_ArticleAssets,
    .animation_source_kind = Ft_Kind_Samus,
    .fighter_root = "ftDataSamus",
    .costume_joint = "PlySamus5K_Share_joint",
    .costume_matanim = "",
    .action_count = 313, .demo_count = 14, .joint_count = 60,
    .bones = ftSamusClone_Bones,
    .bone_count = sizeof(ftSamusClone_Bones) / sizeof(ftSamusClone_Bones[0]),
    .motion_state_count = ftSs_MS_SelfCount,
    .motion_states = ftSs_Init_MotionStateTable,
    .motion_state_first = ftCo_MS_Count,
    .special_ground_state = { ftSs_MS_SpecialNStart, ftSs_MS_SpecialS,
                              ftSs_MS_SpecialHi, ftSs_MS_SpecialLwBomb },
    .special_air_state = { ftSs_MS_SpecialAirNStart, ftSs_MS_SpecialAirS,
                           ftSs_MS_SpecialAirHi, ftSs_MS_SpecialAirLwBomb },
    .special_n = ftSs_SpecialN_Enter, .special_air_n = ftSs_SpecialAirN_Enter,
    .special_s = ftSs_SpecialS_Enter, .special_air_s = ftSs_SpecialAirS_Enter,
    .special_hi = ftSs_SpecialHi_Enter, .special_air_hi = ftSs_SpecialAirHi_Enter,
    .special_lw = ftSs_SpecialLw_Enter,
    .special_air_lw = ftSs_SpecialAirLw_Enter,
};
#endif
