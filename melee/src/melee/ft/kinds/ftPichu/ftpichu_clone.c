#include "ftpichu_clone.h"
#ifdef MELEE_NATIVE
#include <melee/ft/types.h>
#include "ftpichu.h"
#include <melee/ft/kinds/ftPikachu/ftpikachuspecialn.h>
#include <melee/ft/kinds/ftPikachu/ftpikachuspecials.h>
#include <melee/ft/kinds/ftPikachu/ftpikachuspecialhi.h>
#include <melee/ft/kinds/ftPikachu/ftpikachuspeciallw.h>

// Independent asset base. The template authors all four specials and articles;
// fighter-wide policies are authored too; broader clone acceptance remains open.
// Exact retail v1.02 PlCo semantic bindings; all four costumes have no omissions.
static const NativeFighterBone ftPichuClone_Bones[] = {
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
    { FtPart_RLegJA, 37 },
    { FtPart_RLegJ, 38 },
    { FtPart_RKneeJ, 39 },
    { FtPart_RFootJA, 40 },
    { FtPart_RFootJ, 41 },
    { FtPart_LShoulderJA, 7 },
    { FtPart_LShoulderJ, 8 },
    { FtPart_LArmJ, 9 },
    { FtPart_LHandN, 10 },
    { FtPart_L1stNa, 11 },
    { FtPart_HeadN, 15 },
    { FtPart_RShoulderJA, 23 },
    { FtPart_RShoulderJ, 24 },
    { FtPart_RArmJ, 25 },
    { FtPart_RHandN, 26 },
    { FtPart_R1stNa, 27 },
    { FtPart_TransN2, 44 },
    { 53, 45 },
};

static const NativeFighterArticleAsset ftPichuClone_ArticleAssets[] = {
    { It_Kind_Pichu_Thunder, 0, -1 },
    { It_Kind_Pichu_TJolt_Ground, 1, -1 },
    { It_Kind_Pichu_TJolt_Air, 2, -1 },
};

static const Fighter_DemoStrings ftPichuClone_DemoMotionNames = {
    "ftDemoResultMotionFilePichu",
    "ftDemoIntroMotionFilePichu",
    "ftDemoEndingMotionFilePichu",
    "ftDemoViWaitMotionFilePichu",
};

const NativeFighterDefinition ftPichuClone_Definition = {
    .demo_motion_names = &ftPichuClone_DemoMotionNames,
    .code_key = "pichu_clone",
    .sound_banks = UINT64_C(1) << 23,
    .sound_mute_channels = 0x44,
    .source_kind = Ft_Kind_Pichu,
    .asset_layout = { .article_mask = 0x007 },
    .effect_bank = 7,
    .article_asset_count = sizeof(ftPichuClone_ArticleAssets) / sizeof(ftPichuClone_ArticleAssets[0]),
    .article_assets = ftPichuClone_ArticleAssets,
    .bones = ftPichuClone_Bones,
    .bone_count = sizeof(ftPichuClone_Bones) / sizeof(ftPichuClone_Bones[0]),
    .animation_source_kind = Ft_Kind_Pichu,
    .fighter_root = "ftDataPichu",
    .costume_joint = "PlyPichu5K_Share_joint",
    .costume_matanim = "PlyPichu5K_Share_matanim_joint",
    .action_count = 320, .demo_count = 14, .joint_count = 46,
    .motion_state_count = ftPk_MS_SelfCount,
    .motion_states = ftPc_Init_MotionStateTable,
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
