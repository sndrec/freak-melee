#include "ftnana_clone.h"
#ifdef MELEE_NATIVE
#include <melee/ft/kinds/ftPopo/ftpopospecialn.h>
#include <melee/ft/kinds/ftPopo/ftpopospeciallw.h>
#include "ftnana.h"
#include <melee/ft/types.h>

// Copied retail base metadata. This unscripted base retains stock inheritance;
// independent authored modules must replace all fighter decisions before activation.
// USA v1.02 PlCo.dat: 40 semantic mappings, 49 runtime parts, zero omissions.
static const NativeFighterBone ftNanaClone_Bones[] = {
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

static const NativeFighterArticleAsset ftNanaClone_Articles[] = {
    { It_Kind_IceClimber_Ice, 0, -1 },
    { It_Kind_IceClimber_Blizzard, 1, -1 },
    { It_Kind_IceClimber_GumStrings, 2, -1 },
};

static const Fighter_DemoStrings ftNanaClone_DemoMotionNames = {
    "ftDemoResultMotionFileNana",
    "ftDemoIntroMotionFileNana",
    "ftDemoEndingMotionFileNana",
    "ftDemoViWaitMotionFilePopo"
};

const NativeFighterDefinition ftNanaClone_Definition = {
    .code_key = "nana_clone",
    .source_kind = Ft_Kind_Nana, .animation_source_kind = Ft_Kind_Nana,
    .sound_banks = UINT64_C(1) << 13, .sound_mute_channels = 0x44,
    .effect_bank = 14,
    .asset_layout = { .article_mask = 0x007,
                      .item_format = FtCharacterItems_ClimberString },
    .article_assets = ftNanaClone_Articles,
    .article_asset_count = sizeof(ftNanaClone_Articles) / sizeof(ftNanaClone_Articles[0]),
    .bones = ftNanaClone_Bones,
    .bone_count = sizeof(ftNanaClone_Bones) / sizeof(ftNanaClone_Bones[0]),
    .demo_motion_names = &ftNanaClone_DemoMotionNames,
    .fighter_root = "ftDataNana",
    .costume_joint = "PlyNana5K_Share_joint",
    .costume_matanim = "PlyNana5K_Share_matanim_joint",
    .action_count = 321, .demo_count = 14, .joint_count = 49,
    .motion_state_first = ftCo_MS_Count,
    .motion_state_count = ftPp_MS_SelfCount,
    .motion_states = ftNn_Init_MotionStateTable,
    // Nana has no direct Squall or Belay special-entry hooks in the stock tables.
    .special_ground_state = { ftPp_MS_SpecialN, 0, 0, ftPp_MS_SpecialLw },
    .special_air_state = { ftPp_MS_SpecialAirN, 0, 0, ftPp_MS_SpecialAirLw },
    .special_n = ftPp_SpecialN_Enter, .special_air_n = ftPp_SpecialAirN_Enter,
    .special_lw = ftPp_SpecialLw_Enter, .special_air_lw = ftPp_SpecialAirLw_Enter,
};
#endif
