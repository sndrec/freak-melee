#include "ftsheik_clone.h"
#ifdef MELEE_NATIVE
#include "ftseak.h"
#include "ftseakspecialn.h"
#include "ftseakspecials.h"
#include "ftseakspecialhi.h"
#include "ftseakspeciallw.h"
#include <melee/ft/types.h>

// Copied-asset base. Raw catalog fallback remains stock; an independent
// authored module replaces all decisions before source_kind=None.
// USA v1.02 PlCo.dat has 52 mappings, 58 runtime parts and zero omissions.
static const NativeFighterBone ftSheikClone_Bones[] = {
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
    { FtPart_L1stNb, 24 },
    { FtPart_L2ndNa, 25 },
    { FtPart_L2ndNb, 26 },
    { FtPart_L3rdNa, 27 },
    { FtPart_L3rdNb, 28 },
    { FtPart_L4thNa, 29 },
    { FtPart_L4thNb, 30 },
    { FtPart_LThumbNa, 31 },
    { FtPart_LHandNb, 32 },
    { FtPart_NeckN, 33 },
    { FtPart_HeadN, 35 },
    { FtPart_RShoulderN, 36 },
    { FtPart_RShoulderJA, 39 },
    { FtPart_RShoulderJ, 40 },
    { FtPart_RArmJ, 41 },
    { FtPart_RHandN, 42 },
    { FtPart_R1stNa, 43 },
    { FtPart_R1stNb, 44 },
    { FtPart_R2ndNa, 45 },
    { FtPart_R2ndNb, 46 },
    { FtPart_R3rdNa, 47 },
    { FtPart_R3rdNb, 48 },
    { FtPart_R4thNa, 49 },
    { FtPart_R4thNb, 50 },
    { FtPart_RThumbNa, 51 },
    { FtPart_RThumbNb, 52 },
    { FtPart_RHandNb, 53 },
    { FtPart_ThrowN, 54 },
    { FtPart_TransN2, 56 },
    { 53, 57 },
};

static const NativeFighterArticleAsset ftSheikClone_Articles[] = {
    { It_Kind_Seak_NeedleThrow, 0, -1 },
    { It_Kind_Seak_NeedleHeld, 1, -1 },
    { It_Kind_Seak_Vanish, 2, -1 },
    { It_Kind_Seak_Chain, 3, -1 },
};

static const int32_t ftSheikClone_PoseBones[] = {
    1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,
    20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,
    40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,
};
static const NativeFighterPoseGraph ftSheikClone_Poses[] = {
    // Sorted name hashes, exactly the declarations in the Chain component.
    { UINT32_C(305010971), 4, "0", 57, ftSheikClone_PoseBones },
    { UINT32_C(1363947600), 5, "0", 57, ftSheikClone_PoseBones },
};

static const Fighter_DemoStrings ftSheikClone_DemoMotionNames = {
    "ftDemoResultMotionFileSeak",
    "ftDemoIntroMotionFileSeak",
    "ftDemoEndingMotionFileSeak",
    "ftDemoViWaitMotionFileSeak",
};

const NativeFighterDefinition ftSheikClone_Definition = {
    .code_key = "sheik_clone",
    .source_kind = Ft_Kind_Seak,
    .animation_source_kind = Ft_Kind_Seak,
    .sound_banks = UINT64_C(1) << 27,
    .sound_mute_channels = 0x44,
    .effect_bank = 17,
    .asset_layout = { .article_mask = 0x00F, .joint_graph_mask = 0x030,
                      .item_format = FtCharacterItems_Chain },
    .article_assets = ftSheikClone_Articles,
    .article_asset_count = sizeof(ftSheikClone_Articles) / sizeof(ftSheikClone_Articles[0]),
    .bones = ftSheikClone_Bones,
    .bone_count = sizeof(ftSheikClone_Bones) / sizeof(ftSheikClone_Bones[0]),
    .pose_graphs = ftSheikClone_Poses,
    .pose_graph_count = sizeof(ftSheikClone_Poses) / sizeof(ftSheikClone_Poses[0]),
    .demo_motion_names = &ftSheikClone_DemoMotionNames,
    .fighter_root = "ftDataSeak",
    .costume_joint = "PlySeak5K_Share_joint",
    .costume_matanim = "PlySeak5K_Share_matanim_joint",
    .action_count = 317, .demo_count = 14, .joint_count = 58,
    .motion_state_first = ftCo_MS_Count,
    .motion_state_count = ftSk_MS_SelfCount,
    .motion_states = ftSk_Init_MotionStateTable,
    .special_ground_state = { ftSk_MS_SpecialNStart, ftSk_MS_SpecialSStart,
                              ftSk_MS_SpecialHiStart_0, ftSk_MS_SpecialLw },
    .special_air_state = { ftSk_MS_SpecialAirNStart, ftSk_MS_SpecialAirSStart,
                           ftSk_MS_SpecialAirHiStart_0, ftSk_MS_SpecialAirLw },
    .special_n = ftSk_SpecialN_Enter,
    .special_air_n = ftSk_SpecialAirN_Enter,
    .special_s = ftSk_SpecialS_Enter,
    .special_air_s = ftSk_SpecialAirS_Enter,
    .special_hi = ftSk_SpecialHi_Enter,
    .special_air_hi = ftSk_SpecialAirHi_Enter,
    .special_lw = ftSk_SpecialLw_Enter,
    .special_air_lw = ftSk_SpecialAirLw_Enter,
};
#endif
