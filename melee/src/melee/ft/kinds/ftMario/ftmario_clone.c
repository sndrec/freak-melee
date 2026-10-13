#include "ftmario_clone.h"

#ifdef MELEE_NATIVE
#include <melee/ft/types.h>

#include "ftmario.h"
#include "ftmariospecialn.h"
#include "ftmariospecials.h"
#include "ftmariospecialhi.h"
#include "ftmariospeciallw.h"
#include "forward.h"

// Untouched packages retain the stock callback fallback. Authored independent
// modules use these owned asset facts and semantic joint mappings instead.
static const NativeFighterBone ftMarioClone_Bones[] = {
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
    { FtPart_RLegJA, 53 },
    { FtPart_RLegJ, 54 },
    { FtPart_RKneeJ, 55 },
    { FtPart_RFootJA, 56 },
    { FtPart_RFootJ, 57 },
    { FtPart_LShoulderJA, 6 },
    { FtPart_LShoulderJ, 7 },
    { FtPart_LArmJ, 8 },
    { FtPart_LHandN, 9 },
    { FtPart_L1stNa, 10 },
    { FtPart_L1stNb, 11 },
    { FtPart_L2ndNa, 12 },
    { FtPart_L2ndNb, 13 },
    { FtPart_L3rdNa, 14 },
    { FtPart_L3rdNb, 15 },
    { FtPart_L4thNa, 16 },
    { FtPart_L4thNb, 17 },
    { FtPart_LThumbNa, 18 },
    { FtPart_LHandNb, 19 },
    { FtPart_NeckN, 20 },
    { FtPart_HeadN, 22 },
    { FtPart_RShoulderN, 23 },
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
    { FtPart_TransN2, 59 },
    { 53, 60 },
};

static const NativeFighterArticleAsset ftMarioClone_ArticleAssets[] = {
    { It_Kind_Mario_Fire, 0, -1 },
    { It_Kind_Mario_Cape, 2, 20 },
};

static const Fighter_DemoStrings ftMarioClone_DemoMotionNames = {
    "ftDemoResultMotionFileMario",
    "ftDemoIntroMotionFileMario",
    "ftDemoEndingMotionFileMario",
    "ftDemoViWaitMotionFileMario",
};

const NativeFighterDefinition ftMarioClone_Definition = {
    .demo_motion_names = &ftMarioClone_DemoMotionNames,
    .code_key = "mario_clone",
    .sound_banks = UINT64_C(1) << 18, // mario.ssm
    .sound_mute_channels = 0x44,
    .source_kind = Ft_Kind_Mario,
    .asset_layout = { .article_mask = 0x005 },
    .effect_bank = 1,
    .article_asset_count = sizeof(ftMarioClone_ArticleAssets) / sizeof(ftMarioClone_ArticleAssets[0]),
    .article_assets = ftMarioClone_ArticleAssets,
    .animation_source_kind = Ft_Kind_Mario,
    .fighter_root = "ftDataMario",
    .costume_joint = "PlyMario5K_Share_joint",
    .costume_matanim = "PlyMario5K_Share_matanim_joint",
    .action_count = 303,
    .demo_count = 16,
    .motion_state_count = ftMr_MS_SelfCount,
    .motion_states = ftMr_Init_MotionStateTable,
    .motion_state_first = ftMr_MS_AppealSR,
    .special_ground_state = {
        ftMr_MS_SpecialN, ftMr_MS_SpecialS,
        ftMr_MS_SpecialHi, ftMr_MS_SpecialLw,
    },
    .special_air_state = {
        ftMr_MS_SpecialAirN, ftMr_MS_SpecialAirS,
        ftMr_MS_SpecialAirHi, ftMr_MS_SpecialAirLw,
    },
    .special_transition_flags = {
        [NativeFighterSpecial_Neutral] = Ft_MF_SkipColAnim | Ft_MF_UpdateCmd,
        [NativeFighterSpecial_Side] =
            Ft_MF_KeepColAnimHitStatus | Ft_MF_SkipHit |
            Ft_MF_SkipMatAnim | Ft_MF_UpdateCmd | Ft_MF_SkipColAnim |
            Ft_MF_SkipItemVis | Ft_MF_Unk19 | Ft_MF_SkipModelPartVis |
            Ft_MF_SkipModelFlags | Ft_MF_Unk27,
        [NativeFighterSpecial_Down] =
            Ft_MF_KeepGfx | Ft_MF_SkipHit | Ft_MF_SkipMatAnim |
            Ft_MF_UpdateCmd | Ft_MF_SkipColAnim | Ft_MF_SkipItemVis |
            Ft_MF_Unk19 | Ft_MF_SkipModelPartVis | Ft_MF_SkipModelFlags |
            Ft_MF_Unk27,
    },
    .joint_count = 61,
    .bone_count = sizeof(ftMarioClone_Bones) / sizeof(ftMarioClone_Bones[0]),
    .bones = ftMarioClone_Bones,
    .special_n = ftMr_SpecialN_Enter,
    .special_air_n = ftMr_SpecialAirN_Enter,
    .neutral_article = ftMr_SpecialN_ItemFireSpawn,
    .special_s = ftMr_SpecialS_Enter,
    .special_air_s = ftMr_SpecialAirS_Enter,
    .special_hi = ftMr_SpecialHi_Enter,
    .special_air_hi = ftMr_SpecialAirHi_Enter,
    .special_lw = ftMr_SpecialLw_Enter,
    .special_air_lw = ftMr_SpecialAirLw_Enter,
};

#endif
