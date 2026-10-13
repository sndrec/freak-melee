#include "ftpikachu_clone.h"
#ifdef MELEE_NATIVE
#include <melee/ft/types.h>
#include "ftpikachu.h"
#include <melee/ft/kinds/ftPikachu/ftpikachuspecialn.h>
#include <melee/ft/kinds/ftPikachu/ftpikachuspecials.h>
#include <melee/ft/kinds/ftPikachu/ftpikachuspecialhi.h>
#include <melee/ft/kinds/ftPikachu/ftpikachuspeciallw.h>

// Independent asset base. The template authors all four specials and articles;
// fighter-wide policies are authored too; broader clone acceptance remains open.
// Exact retail v1.02 PlCo semantic bindings; all four costumes have no omissions.
static const NativeFighterBone ftPikachuClone_Bones[] = {
    { FtPart_TopN, 0 },
    { FtPart_TransN, 1 },
    { FtPart_XRotN, 2 },
    { FtPart_YRotN, 3 },
    { FtPart_HipN, 4 },
    { FtPart_WaistN, 5 },
    { FtPart_LLegJA, 31 },
    { FtPart_LLegJ, 32 },
    { FtPart_LKneeJ, 33 },
    { FtPart_LFootJA, 34 },
    { FtPart_LFootJ, 35 },
    { FtPart_RLegJA, 39 },
    { FtPart_RLegJ, 40 },
    { FtPart_RKneeJ, 41 },
    { FtPart_RFootJA, 42 },
    { FtPart_RFootJ, 43 },
    { FtPart_LShoulderJA, 6 },
    { FtPart_LShoulderJ, 7 },
    { FtPart_LArmJ, 8 },
    { FtPart_LHandN, 9 },
    { FtPart_L1stNa, 10 },
    { FtPart_HeadN, 14 },
    { FtPart_RShoulderJA, 22 },
    { FtPart_RShoulderJ, 23 },
    { FtPart_RArmJ, 24 },
    { FtPart_RHandN, 25 },
    { FtPart_R1stNa, 26 },
    { FtPart_TransN2, 51 },
    { 53, 52 },
};

static const NativeFighterArticleAsset ftPikachuClone_ArticleAssets[] = {
    { It_Kind_Pikachu_Thunder, 0, -1 },
    { It_Kind_Pikachu_TJolt_Ground, 1, -1 },
    { It_Kind_Pikachu_TJolt_Air, 2, -1 },
};

static const Fighter_DemoStrings ftPikachuClone_DemoMotionNames = {
    "ftDemoResultMotionFilePikachu",
    "ftDemoIntroMotionFilePikachu",
    "ftDemoEndingMotionFilePikachu",
    "ftDemoViWaitMotionFilePikachu",
};

const NativeFighterDefinition ftPikachuClone_Definition = {
    .demo_motion_names = &ftPikachuClone_DemoMotionNames,
    .code_key = "pikachu_clone",
    .sound_banks = UINT64_C(1) << 24,
    .sound_mute_channels = 0x44,
    .source_kind = Ft_Kind_Pikachu,
    .asset_layout = { .article_mask = 0x007 },
    .effect_bank = 7,
    .article_asset_count = sizeof(ftPikachuClone_ArticleAssets) / sizeof(ftPikachuClone_ArticleAssets[0]),
    .article_assets = ftPikachuClone_ArticleAssets,
    .bones = ftPikachuClone_Bones,
    .bone_count = sizeof(ftPikachuClone_Bones) / sizeof(ftPikachuClone_Bones[0]),
    .animation_source_kind = Ft_Kind_Pikachu,
    .fighter_root = "ftDataPikachu",
    .costume_joint = "PlyPikachu5K_Share_joint",
    .costume_matanim = "PlyPikachu5K_Share_matanim_joint",
    .action_count = 320, .demo_count = 14, .joint_count = 53,
    .motion_state_count = ftPk_MS_SelfCount,
    .motion_states = ftPk_Init_MotionStateTable,
    .motion_state_first = ftCo_MS_Count,
    .special_ground_state = { ftPk_MS_SpecialN, ftPk_MS_SpecialSStart,
                              ftPk_MS_SpecialHiStart0, ftPk_MS_SpecialLwStart },
    .special_air_state = { ftPk_MS_SpecialAirN, ftPk_MS_SpecialAirSStart,
                           ftPk_MS_SpecialAirHiStart0, ftPk_MS_SpecialAirLwStart },
    .special_n = ftPk_SpecialN_Enter, .special_air_n = ftPk_SpecialAirN_Enter,
    .special_s = ftPk_SpecialS_Enter, .special_air_s = ftPk_SpecialAirS_Enter,
    .special_hi = ftPk_SpecialHi_Enter, .special_air_hi = ftPk_SpecialAirHi_Enter,
    .special_lw = ftPk_SpecialLw_Enter, .special_air_lw = ftPk_SpecialAirLw_Enter,
};
#endif
