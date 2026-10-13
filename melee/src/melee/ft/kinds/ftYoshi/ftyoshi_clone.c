#include "ftyoshi_clone.h"
#ifdef MELEE_NATIVE
#include <melee/ft/types.h>
#include <melee/ft/ftdat.h>
#include "ftyoshi.h"
#include "ftyoshispecialhi.h"
#include "ftyoshispecialn.h"
#include "ftyoshispecials.h"
#include "ftyoshispeciallw.h"

// Independent copied assets. The partial authored template replaces all four
// specials, their three articles, shield, common player lifecycle, CPU and
// item/knockback presentation. The raw base retains stock fallback; a compiled
// independent template clears that behavior family and uses its owned metadata.
// Semantic joints copied from the supplied v1.02 PlCo.dat; no missing parts.
static const NativeFighterBone ftYoshiClone_Bones[] = {
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
    { FtPart_BustN, 22 },
    { FtPart_LShoulderN, 23 },
    { FtPart_LShoulderJA, 24 },
    { FtPart_LShoulderJ, 25 },
    { FtPart_LArmJ, 26 },
    { FtPart_LHandN, 27 },
    { FtPart_L1stNa, 28 },
    { FtPart_L2ndNa, 30 },
    { FtPart_L3rdNa, 32 },
    { FtPart_L4thNa, 34 },
    { FtPart_NeckN, 36 },
    { FtPart_RShoulderN, 38 },
    { FtPart_RShoulderJA, 54 },
    { FtPart_RShoulderJ, 55 },
    { FtPart_RArmJ, 56 },
    { FtPart_RHandN, 57 },
    { FtPart_R1stNa, 58 },
    { FtPart_R2ndNa, 60 },
    { FtPart_R3rdNa, 62 },
    { FtPart_R4thNa, 64 },
    { FtPart_RThumbNb, 65 },
    { FtPart_ThrowN, 67 },
    { FtPart_TransN2, 68 },
    { (Fighter_Part) (FT_COMMON_PART_COUNT - 1), 69 },
};

static const NativeFighterArticleAsset ftYoshiClone_ArticleAssets[] = {
    { It_Kind_Yoshi_EggThrow, 0, -1 },
    { It_Kind_Yoshi_Star, 1, -1 },
    { It_Kind_Yoshi_EggLay, 2, -1 },
};

static const Fighter_DemoStrings ftYoshiClone_DemoMotionNames = {
    "ftDemoResultMotionFileYoshi",
    "ftDemoIntroMotionFileYoshi",
    "ftDemoEndingMotionFileYoshi",
    "ftDemoViWaitMotionFileYoshi",
};

const NativeFighterDefinition ftYoshiClone_Definition = {
    .demo_motion_names = &ftYoshiClone_DemoMotionNames,
    .code_key = "yoshi_clone",
    .sound_banks = UINT64_C(1) << 28,
    .sound_mute_channels = 0x44,
    .source_kind = Ft_Kind_Yoshi,
    .asset_layout = { .article_mask = 0x007, .joint_graph_mask = 0x008,
                      .byte_range_count = 1, .byte_ranges = {{ 0x12C, 12 }} },
    .effect_bank = 9,
    .article_asset_count = sizeof(ftYoshiClone_ArticleAssets) / sizeof(ftYoshiClone_ArticleAssets[0]),
    .article_assets = ftYoshiClone_ArticleAssets,
    .bones = ftYoshiClone_Bones,
    .bone_count = sizeof(ftYoshiClone_Bones) / sizeof(ftYoshiClone_Bones[0]),
    .animation_source_kind = Ft_Kind_Yoshi,
    .fighter_root = "ftDataYoshi",
    .costume_joint = "PlyYoshi5K_Share_joint",
    .costume_matanim = "PlyYoshi5K_Share_matanim_joint",
    .action_count = 314, .demo_count = 14, .joint_count = 70,
    .motion_state_count = ftYs_MS_SelfCount,
    .motion_states = ftYs_Init_MotionStateTable,
    .motion_state_first = ftCo_MS_Count,
    .special_ground_state = { ftYs_MS_SpecialN1,
                              ftYs_MS_SpecialAirSStart_1,
                              ftYs_MS_SpecialHi, ftYs_MS_SpecialLw },
    .special_air_state = { ftYs_MS_SpecialAirN1_0,
                           ftYs_MS_SpecialAirSStart_1,
                           ftYs_MS_SpecialAirHi, ftYs_MS_SpecialAirLw },
    .special_n = ftYs_SpecialN_Enter, .special_air_n = ftYs_SpecialAirN_Enter,
    .special_s = ftYs_SpecialS_Enter, .special_air_s = ftYs_SpecialAirS_Enter,
    .special_hi = ftYs_SpecialHi_Enter, .special_air_hi = ftYs_SpecialAirHi_Enter,
    .special_lw = ftYs_SpecialLw_Enter,
    .special_air_lw = ftYs_SpecialAirLw_Enter,
};
#endif
