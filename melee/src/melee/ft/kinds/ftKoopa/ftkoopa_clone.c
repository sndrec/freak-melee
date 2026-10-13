#include "ftkoopa_clone.h"
#ifdef MELEE_NATIVE
#include <melee/ft/types.h>
#include <melee/ft/ftdat.h>
#include "ftkoopa.h"
#include "ftkoopaspecialn.h"
#include "ftkoopaspecials.h"
#include "ftkoopaspecialhi.h"
#include "ftkoopaspeciallw.h"

// Independent asset base. The authored template replaces all special states,
// capture decisions, Flame callbacks and fighter-wide setup/item/CPU policies.
// The uncompiled asset base still uses stock callbacks; selected comparisons
// of the compiled template do not establish whole-clone runtime acceptance.
// Retail PlCo semantic map; all costume graphs have no omitted joints.
static const NativeFighterBone ftKoopaClone_Bones[] = {
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
    { FtPart_RLegJA, 14 },
    { FtPart_RLegJ, 15 },
    { FtPart_RKneeJ, 16 },
    { FtPart_RFootJA, 17 },
    { FtPart_RFootJ, 18 },
    { FtPart_BustN, 29 },
    { FtPart_LShoulderN, 30 },
    { FtPart_LShoulderJA, 31 },
    { FtPart_LShoulderJ, 32 },
    { FtPart_LArmJ, 33 },
    { FtPart_LHandN, 34 },
    { FtPart_L1stNa, 35 },
    { FtPart_L1stNb, 36 },
    { FtPart_L2ndNa, 37 },
    { FtPart_L2ndNb, 38 },
    { FtPart_L3rdNa, 39 },
    { FtPart_L3rdNb, 40 },
    { FtPart_L4thNa, 41 },
    { FtPart_LHandNb, 42 },
    { FtPart_NeckN, 43 },
    { FtPart_HeadN, 47 },
    { FtPart_RShoulderN, 48 },
    { FtPart_RShoulderJA, 58 },
    { FtPart_RShoulderJ, 59 },
    { FtPart_RArmJ, 60 },
    { FtPart_RHandN, 61 },
    { FtPart_R1stNa, 62 },
    { FtPart_R1stNb, 63 },
    { FtPart_R2ndNa, 64 },
    { FtPart_R2ndNb, 65 },
    { FtPart_R3rdNa, 66 },
    { FtPart_R3rdNb, 67 },
    { FtPart_R4thNa, 68 },
    { FtPart_RThumbNb, 69 },
    { FtPart_RHandNb, 70 },
    { FtPart_ThrowN, 71 },
    { FtPart_TransN2, 74 },
    { 53, 75 },
};

static const NativeFighterArticleAsset ftKoopaClone_ArticleAssets[] = {
    { It_Kind_Koopa_Flame, 0, -1 },
};

static const Fighter_DemoStrings ftKoopaClone_DemoMotionNames = {
    "ftDemoResultMotionFileKoopa",
    "ftDemoIntroMotionFileKoopa",
    "ftDemoEndingMotionFileKoopa",
    "ftDemoViWaitMotionFileKoopa",
};

const NativeFighterDefinition ftKoopaClone_Definition = {
    .bone_count = sizeof(ftKoopaClone_Bones) / sizeof(ftKoopaClone_Bones[0]),
    .bones = ftKoopaClone_Bones,
    .demo_motion_names = &ftKoopaClone_DemoMotionNames,
    .code_key = "koopa_clone",
    .sound_banks = UINT64_C(1) << 15, // koopa.ssm, matching stock audio
    .sound_mute_channels = 0x44,
    .source_kind = Ft_Kind_Koopa,
    .asset_layout = { .article_mask = 0x001 },
    .effect_bank = 12,
    .article_asset_count = sizeof(ftKoopaClone_ArticleAssets) / sizeof(ftKoopaClone_ArticleAssets[0]),
    .article_assets = ftKoopaClone_ArticleAssets,
    .animation_source_kind = Ft_Kind_Koopa,
    .fighter_root = "ftDataKoopa",
    .costume_joint = "PlyKoopa5K_Share_joint",
    .costume_matanim = "PlyKoopa5K_Share_matanim_joint",
    .action_count = 316, .demo_count = 14, .joint_count = 76,
    .motion_state_count = ftKp_MS_SelfCount,
    .motion_states = ftKp_Init_MotionStateTable,
    .motion_state_first = ftCo_MS_Count,
    .special_ground_state = { ftKp_MS_SpecialNStart, ftKp_MS_SpecialSStart,
                              ftKp_MS_SpecialHi, ftKp_MS_SpecialLw },
    .special_air_state = { ftKp_MS_SpecialAirNStart, ftKp_MS_SpecialAirSStart,
                           ftKp_MS_SpecialAirHi, ftKp_MS_SpecialAirLw },
    .special_n = ftKp_SpecialN_Enter, .special_air_n = ftKp_SpecialAirN_Enter,
    .special_s = ftKp_SpecialS_Enter, .special_air_s = ftKp_SpecialAirS_Enter,
    .special_hi = ftKp_SpecialHi_Enter, .special_air_hi = ftKp_SpecialAirHi_Enter,
    .special_lw = ftKp_SpecialLw_Enter,
    .special_air_lw = ftKp_SpecialAirLw_Enter,
};
#endif
