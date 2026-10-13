#include "ftpopo_clone.h"
#ifdef MELEE_NATIVE
#include <melee/ft/kinds/ftPopo/ftpopospecialn.h>
#include <melee/ft/kinds/ftPopo/ftpopospeciallw.h>
#include "ftpopo.h"
#include "ftpopospecials.h"
#include "ftpopospecialhi.h"
#include <melee/ft/types.h>

// Copied retail base metadata. This unscripted base retains stock inheritance;
// independent authored modules must replace all fighter decisions before activation.
// USA v1.02 PlCo.dat: 40 semantic mappings, 49 runtime parts, zero omissions.
static const NativeFighterBone ftPopoClone_Bones[] = {
    { FtPart_TopN, 0 },
    { FtPart_TransN, 1 },
    { FtPart_XRotN, 2 },
    { FtPart_YRotN, 3 },
    { FtPart_HipN, 4 },
    { FtPart_WaistN, 5 },
    { FtPart_LLegJA, 35 },
    { FtPart_LLegJ, 36 },
    { FtPart_LKneeJ, 37 },
    { FtPart_LFootJA, 38 },
    { FtPart_LFootJ, 39 },
    { FtPart_RLegJA, 41 },
    { FtPart_RLegJ, 42 },
    { FtPart_RKneeJ, 43 },
    { FtPart_RFootJA, 44 },
    { FtPart_RFootJ, 45 },
    { FtPart_LShoulderJA, 7 },
    { FtPart_LShoulderJ, 8 },
    { FtPart_LArmJ, 9 },
    { FtPart_LHandN, 10 },
    { FtPart_L1stNa, 11 },
    { FtPart_L3rdNb, 12 },
    { FtPart_L4thNa, 13 },
    { FtPart_LThumbNb, 14 },
    { FtPart_LHandNb, 16 },
    { FtPart_NeckN, 17 },
    { FtPart_HeadN, 18 },
    { FtPart_RShoulderN, 19 },
    { FtPart_RShoulderJA, 22 },
    { FtPart_RShoulderJ, 23 },
    { FtPart_RArmJ, 24 },
    { FtPart_RHandN, 25 },
    { FtPart_R1stNa, 26 },
    { FtPart_R3rdNb, 27 },
    { FtPart_R4thNa, 28 },
    { FtPart_RThumbNb, 29 },
    { FtPart_RHandNb, 30 },
    { FtPart_ThrowN, 31 },
    { FtPart_TransN2, 47 },
    { 53, 48 },
};

static const NativeFighterArticleAsset ftPopoClone_Articles[] = {
    { It_Kind_IceClimber_Ice, 0, -1 },
    { It_Kind_IceClimber_Blizzard, 1, -1 },
    { It_Kind_IceClimber_GumStrings, 2, -1 },
};

static const Fighter_DemoStrings ftPopoClone_DemoMotionNames = {
    "ftDemoResultMotionFilePopo",
    "ftDemoIntroMotionFilePopo",
    "ftDemoEndingMotionFilePopo",
    "ftDemoViWaitMotionFilePopo"
};

const NativeFighterDefinition ftPopoClone_Definition = {
    .code_key = "popo_clone",
    .source_kind = Ft_Kind_Popo, .animation_source_kind = Ft_Kind_Popo,
    .sound_banks = UINT64_C(1) << 13, .sound_mute_channels = 0x44,
    .effect_bank = 14,
    .asset_layout = { .article_mask = 0x007,
                      .item_format = FtCharacterItems_ClimberString },
    .article_assets = ftPopoClone_Articles,
    .article_asset_count = sizeof(ftPopoClone_Articles) / sizeof(ftPopoClone_Articles[0]),
    .bones = ftPopoClone_Bones,
    .bone_count = sizeof(ftPopoClone_Bones) / sizeof(ftPopoClone_Bones[0]),
    .demo_motion_names = &ftPopoClone_DemoMotionNames,
    .fighter_root = "ftDataPopo",
    .costume_joint = "PlyPopo5K_Share_joint",
    .costume_matanim = "PlyPopo5K_Share_matanim_joint",
    .action_count = 321, .demo_count = 14, .joint_count = 49,
    .motion_state_first = ftCo_MS_Count,
    .motion_state_count = ftPp_MS_SelfCount,
    .motion_states = ftPp_Init_MotionStateTable,
    .special_ground_state = { ftPp_MS_SpecialN, ftPp_MS_SpecialS1,
                              ftPp_MS_SpecialHiStart_0, ftPp_MS_SpecialLw },
    .special_air_state = { ftPp_MS_SpecialAirN, ftPp_MS_SpecialAirS1,
                           ftPp_MS_SpecialAirHiStart_0, ftPp_MS_SpecialAirLw },
    .special_n = ftPp_SpecialN_Enter, .special_air_n = ftPp_SpecialAirN_Enter,
    .special_s = ftPp_SpecialS_Enter, .special_air_s = ftPp_SpecialAirS_Enter,
    .special_hi = ftPp_SpecialHi_Enter, .special_air_hi = ftPp_SpecialAirHi_Enter,
    .special_lw = ftPp_SpecialLw_Enter, .special_air_lw = ftPp_SpecialAirLw_Enter,
};
#endif
