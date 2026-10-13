#include "ftgamewatch_clone.h"
#ifdef MELEE_NATIVE
#include "ftgamewatch.h"
#include "ftgamewatchspecialn.h"
#include "ftgamewatchspecials.h"
#include "ftgamewatchspecialhi.h"
#include "ftgamewatchspeciallw.h"

// Retail assets for normal project import; gamewatch_full.fms replaces the
// fighter-specific callback family in the independent build. PlCo.dat supplies
// 38 semantic mappings; the costume has 53 physical joints, no omissions.
// Slot10 contains outline visibility, not an article.
static const NativeFighterBone ftGameWatchClone_Bones[] = {
    { FtPart_TopN, 0 },
    { FtPart_TransN, 2 },
    { FtPart_XRotN, 3 },
    { FtPart_YRotN, 4 },
    { FtPart_HipN, 5 },
    { FtPart_WaistN, 6 },
    { FtPart_LLegJA, 35 },
    { FtPart_LLegJ, 36 },
    { FtPart_LKneeJ, 37 },
    { FtPart_LFootJA, 38 },
    { FtPart_LFootJ, 39 },
    { FtPart_RLegJA, 43 },
    { FtPart_RLegJ, 44 },
    { FtPart_RKneeJ, 45 },
    { FtPart_RFootJA, 46 },
    { FtPart_RFootJ, 47 },
    { FtPart_LShoulderJA, 9 },
    { FtPart_LShoulderJ, 10 },
    { FtPart_LArmJ, 11 },
    { FtPart_LHandN, 12 },
    { FtPart_L1stNa, 14 },
    { FtPart_L2ndNb, 15 },
    { FtPart_L3rdNa, 16 },
    { FtPart_LThumbNb, 17 },
    { FtPart_LHandNb, 18 },
    { FtPart_HeadN, 19 },
    { FtPart_RShoulderN, 20 },
    { FtPart_RShoulderJA, 24 },
    { FtPart_RShoulderJ, 25 },
    { FtPart_RArmJ, 26 },
    { FtPart_RHandN, 27 },
    { FtPart_R1stNa, 29 },
    { FtPart_R2ndNb, 30 },
    { FtPart_R3rdNa, 31 },
    { FtPart_RThumbNb, 32 },
    { FtPart_RHandNb, 34 },
    { FtPart_TransN2, 51 },
    { 53, 52 },
};

static const NativeFighterArticleAsset ftGameWatchClone_Articles[] = {
    { It_Kind_GameWatch_Greenhouse, 0, -1 },
    { It_Kind_GameWatch_Manhole, 1, -1 },
    { It_Kind_GameWatch_Fire, 2, -1 },
    { It_Kind_GameWatch_Parachute, 3, -1 },
    { It_Kind_GameWatch_Turtle, 4, -1 },
    { It_Kind_GameWatch_Breath, 5, -1 },
    { It_Kind_GameWatch_Judge, 6, -1 },
    { It_Kind_GameWatch_Panic, 7, -1 },
    { It_Kind_GameWatch_Chef, 8, -1 },
    { It_Kind_GameWatch_Rescue, 9, -1 },
};

static const Fighter_DemoStrings ftGameWatchClone_DemoMotionNames = {
    "ftDemoResultMotionFileGamewatch",
    "ftDemoIntroMotionFileGamewatch",
    "ftDemoEndingMotionFileGamewatch",
    "ftDemoViWaitMotionFileGamewatch",
};

const NativeFighterDefinition ftGameWatchClone_Definition = {
    .code_key = "gamewatch_clone",
    .source_kind = Ft_Kind_GameWatch,
    .animation_source_kind = Ft_Kind_GameWatch,
    .sound_banks = UINT64_C(1) << 29,
    .sound_mute_channels = 0x00,
    .effect_bank = 0xFF,
    .asset_layout = { .article_mask = 0x3FF, .visibility_mask = 0x400,
        .item_format = FtCharacterItems_PaperArticles,
        .byte_range_count = 1, .byte_ranges = {{ 4, 20 }} },
    .article_assets = ftGameWatchClone_Articles,
    .article_asset_count = sizeof(ftGameWatchClone_Articles) / sizeof(ftGameWatchClone_Articles[0]),
    .bones = ftGameWatchClone_Bones,
    .bone_count = sizeof(ftGameWatchClone_Bones) / sizeof(ftGameWatchClone_Bones[0]),
    .demo_motion_names = &ftGameWatchClone_DemoMotionNames,
    .fighter_root = "ftDataGamewatch",
    .costume_joint = "PlyGamewatch5K_Share_joint",
    .costume_matanim = "",
    .action_count = 323, .demo_count = 14, .joint_count = 53,
    .motion_state_first = ftCo_MS_Count,
    .motion_state_count = ftGw_MS_SelfCount,
    .motion_states = ftGw_Init_MotionStateTable,
    .special_ground_state = { ftGw_MS_SpecialN, ftGw_MS_SpecialS1,
                              ftGw_MS_SpecialHi, ftGw_MS_SpecialLw },
    .special_air_state = { ftGw_MS_SpecialAirN, ftGw_MS_SpecialAirS1,
                           ftGw_MS_SpecialAirHi, ftGw_MS_SpecialAirLw },
    .special_n = ftGw_SpecialN_Enter,
    .special_air_n = ftGw_SpecialAirN_Enter,
    .special_s = ftGw_SpecialS_Enter,
    .special_air_s = ftGw_SpecialAirS_Enter,
    .special_hi = ftGw_SpecialHi_Enter,
    .special_air_hi = ftGw_SpecialAirHi_Enter,
    .special_lw = ftGw_SpecialLw_Enter,
    .special_air_lw = ftGw_SpecialAirLw_Enter,
};
#endif
