#include "ftness_clone.h"
#ifdef MELEE_NATIVE
#include <melee/ft/types.h>
#include "ftness.h"
#include "ftnessspecialhi.h"
#include "ftnessspeciallw.h"
#include "ftnessspecialn.h"
#include "ftnessspecials.h"

// Copied assets for the authored Ness template. This raw catalog base retains
// stock fallback; the compiled independent template owns gameplay callbacks.
// Source comparisons and gameplay acceptance are in docs/ness_script_translation.md.
// Exact v1.02 PlCo semantic bindings; Nr/Ye/Bu/Gr each contain 63 parts.
static const NativeFighterBone ftNessClone_Bones[] = {
    { FtPart_TopN, 0 },
    { FtPart_TransN, 1 },
    { FtPart_XRotN, 2 },
    { FtPart_YRotN, 3 },
    { FtPart_HipN, 4 },
    { FtPart_WaistN, 5 },
    { FtPart_LLegJA, 47 },
    { FtPart_LLegJ, 48 },
    { FtPart_LKneeJ, 49 },
    { FtPart_LFootJA, 50 },
    { FtPart_LFootJ, 51 },
    { FtPart_RLegJA, 54 },
    { FtPart_RLegJ, 55 },
    { FtPart_RKneeJ, 56 },
    { FtPart_RFootJA, 57 },
    { FtPart_RFootJ, 58 },
    { FtPart_LShoulderJA, 7 },
    { FtPart_LShoulderJ, 8 },
    { FtPart_LArmJ, 9 },
    { FtPart_LHandN, 10 },
    { FtPart_L1stNa, 11 },
    { FtPart_L1stNb, 12 },
    { FtPart_L2ndNa, 13 },
    { FtPart_L2ndNb, 14 },
    { FtPart_L3rdNa, 15 },
    { FtPart_L3rdNb, 16 },
    { FtPart_L4thNa, 17 },
    { FtPart_L4thNb, 18 },
    { FtPart_LThumbNa, 19 },
    { FtPart_LHandNb, 20 },
    { FtPart_NeckN, 21 },
    { FtPart_HeadN, 23 },
    { FtPart_RShoulderN, 24 },
    { FtPart_RShoulderJA, 29 },
    { FtPart_RShoulderJ, 30 },
    { FtPart_RArmJ, 31 },
    { FtPart_RHandN, 32 },
    { FtPart_R1stNa, 33 },
    { FtPart_R1stNb, 34 },
    { FtPart_R2ndNa, 35 },
    { FtPart_R2ndNb, 36 },
    { FtPart_R3rdNa, 37 },
    { FtPart_R3rdNb, 38 },
    { FtPart_R4thNa, 39 },
    { FtPart_R4thNb, 40 },
    { FtPart_RThumbNa, 41 },
    { FtPart_RThumbNb, 42 },
    { FtPart_RHandNb, 43 },
    { FtPart_ThrowN, 44 },
    { FtPart_TransN2, 61 },
    { 53, 62 },
};

static const NativeFighterArticleAsset ftNessClone_ArticleAssets[] = {
    { It_Kind_Ness_PKFire, 0, -1 },
    { It_Kind_Ness_PKFire_Flame, 1, -1 },
    { It_Kind_Ness_PKFlush, 2, -1 },
    { It_Kind_Ness_PKThunder, 3, -1 },
    { It_Kind_Ness_PKThunder1, 4, -1 },
    { It_Kind_Ness_PKThunder2, 5, -1 },
    { It_Kind_Ness_PKThunder3, 6, -1 },
    { It_Kind_Ness_PKThunder4, 7, -1 },
    { It_Kind_Ness_PKFlush_Explode, 8, -1 },
    { It_Kind_Ness_Bat, 9, -1 },
    { It_Kind_Ness_Yoyo, 10, -1 },
};

static const Fighter_DemoStrings ftNessClone_DemoMotionNames = {
    "ftDemoResultMotionFileNess",
    "ftDemoIntroMotionFileNess",
    "ftDemoEndingMotionFileNess",
    "ftDemoViWaitMotionFileNess",
};

const NativeFighterDefinition ftNessClone_Definition = {
    .demo_motion_names = &ftNessClone_DemoMotionNames,
    .code_key = "ness_clone",
    .sound_banks = UINT64_C(1) << 21,
    .sound_mute_channels = 0x44,
    .source_kind = Ft_Kind_Ness,
    .asset_layout = { .article_mask = 0x7FF,
                      .item_format = FtCharacterItems_Yoyo },
    .effect_bank = 10,
    .article_asset_count = sizeof(ftNessClone_ArticleAssets) / sizeof(ftNessClone_ArticleAssets[0]),
    .article_assets = ftNessClone_ArticleAssets,
    .bones = ftNessClone_Bones,
    .bone_count = sizeof(ftNessClone_Bones) / sizeof(ftNessClone_Bones[0]),
    .animation_source_kind = Ft_Kind_Ness,
    .fighter_root = "ftDataNess",
    .costume_joint = "PlyNess5K_Share_joint",
    .costume_matanim = "PlyNess5K_Share_matanim_joint",
    .action_count = 326, .demo_count = 14, .joint_count = 63,
    .motion_state_count = ftNs_MS_SelfCount,
    .motion_states = ftNs_Init_MotionStateTable,
    .motion_state_first = ftCo_MS_Count,
    .special_ground_state = { ftNs_MS_SpecialNStart, ftNs_MS_SpecialS,
                              ftNs_MS_SpecialHiStart, ftNs_MS_SpecialLwStart },
    .special_air_state = { ftNs_MS_SpecialAirNStart, ftNs_MS_SpecialAirS,
                           ftNs_MS_SpecialAirHiStart, ftNs_MS_SpecialAirLwStart },
    .special_n = ftNs_SpecialNStart_Enter,
    .special_air_n = ftNs_SpecialAirNStart_Enter,
    .special_s = ftNs_SpecialS_Enter,
    .special_air_s = ftNs_SpecialAirS_Enter,
    .special_hi = ftNs_SpecialHiStart_Enter,
    .special_air_hi = ftNs_SpecialAirHiStart_Enter,
    .special_lw = ftNs_SpecialLwStart_Enter,
    .special_air_lw = ftNs_SpecialAirLwStart_Enter,
};
#endif
