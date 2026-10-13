#include "ftmars_clone.h"
#ifdef MELEE_NATIVE
#include <melee/ft/types.h>
#include "ftmars.h"
#include "ftmarsspecialn.h"
#include "ftmarsspecials.h"
#include "ftmarsspecialhi.h"
#include "ftmarsspeciallw.h"

// Independent asset base. The template scripts own all four specials,
// lifecycle reset and ribbons; common engine kind exceptions remain under audit.
// Exact semantic bindings from retail v1.02 PlCo.dat; no omissions.
static const NativeFighterBone ftMarsClone_Bones[] = {
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
    { FtPart_BustN, 21 },
    { FtPart_LShoulderN, 22 },
    { FtPart_LShoulderJA, 25 },
    { FtPart_LShoulderJ, 27 },
    { FtPart_LArmJ, 28 },
    { FtPart_LHandN, 29 },
    { FtPart_L1stNa, 31 },
    { FtPart_L1stNb, 32 },
    { FtPart_L2ndNa, 33 },
    { FtPart_L2ndNb, 34 },
    { FtPart_L3rdNa, 35 },
    { FtPart_L3rdNb, 36 },
    { FtPart_L4thNa, 37 },
    { FtPart_L4thNb, 38 },
    { FtPart_LThumbNa, 39 },
    { FtPart_LThumbNb, 40 },
    { FtPart_LHandNb, 41 },
    { FtPart_NeckN, 42 },
    { FtPart_HeadN, 59 },
    { FtPart_RShoulderN, 60 },
    { FtPart_RShoulderJA, 67 },
    { FtPart_RShoulderJ, 69 },
    { FtPart_RArmJ, 70 },
    { FtPart_RHandN, 71 },
    { FtPart_R1stNa, 73 },
    { FtPart_R1stNb, 77 },
    { FtPart_R2ndNa, 78 },
    { FtPart_R2ndNb, 79 },
    { FtPart_R3rdNa, 80 },
    { FtPart_R3rdNb, 81 },
    { FtPart_R4thNa, 82 },
    { FtPart_R4thNb, 83 },
    { FtPart_RThumbNa, 84 },
    { FtPart_RHandNb, 85 },
    { FtPart_ThrowN, 86 },
    { FtPart_TransN2, 88 },
    { 53, 89 },
};

static const Fighter_DemoStrings ftMarsClone_DemoMotionNames = {
    "ftDemoResultMotionFileMars",
    "ftDemoIntroMotionFileMars",
    "ftDemoEndingMotionFileMars",
    "ftDemoViWaitMotionFileMars",
};

const NativeFighterDefinition ftMarsClone_Definition = {
    .demo_motion_names = &ftMarsClone_DemoMotionNames,
    .code_key = "marth_clone",
    .sound_banks = UINT64_C(1) << 19,
    .sound_mute_channels = 0x44,
    .source_kind = Ft_Kind_Mars,
    .asset_layout = { 0 },
    .bones = ftMarsClone_Bones,
    .bone_count = sizeof(ftMarsClone_Bones) / sizeof(ftMarsClone_Bones[0]),
    .effect_bank = 16,
    .animation_source_kind = Ft_Kind_Mars,
    .fighter_root = "ftDataMars",
    .costume_joint = "PlyMars5K_Share_joint",
    .costume_matanim = "PlyMars5K_Share_matanim_joint",
    .action_count = 327, .demo_count = 14, .joint_count = 90,
    .motion_state_count = ftMs_MS_SelfCount,
    .motion_states = ftMs_Init_MotionStateTable,
    .motion_state_first = ftMs_MS_SpecialNStart,
    .special_ground_state = { ftMs_MS_SpecialNStart, ftMs_MS_SpecialS1,
                              ftMs_MS_SpecialHi, ftMs_MS_SpecialLw },
    .special_air_state = { ftMs_MS_SpecialAirNStart, ftMs_MS_SpecialAirS1,
                           ftMs_MS_SpecialAirHi, ftMs_MS_SpecialAirLw },
    .special_n = ftMs_SpecialN_Enter, .special_air_n = ftMs_SpecialAirN_Enter,
    .special_s = ftMs_SpecialS_Enter, .special_air_s = ftMs_SpecialAirS_Enter,
    .special_hi = ftMs_SpecialHi_Enter, .special_air_hi = ftMs_SpecialAirHi_Enter,
    .special_lw = ftMs_SpecialLw_Enter, .special_air_lw = ftMs_SpecialAirLw_Enter,
};
#endif
