#include "ftemblem_clone.h"
#ifdef MELEE_NATIVE
#include <melee/ft/types.h>
#include "ftemblem.h"
#include <melee/ft/kinds/ftMars/ftmarsspecialn.h>
#include <melee/ft/kinds/ftMars/ftmarsspecials.h>
#include <melee/ft/kinds/ftMars/ftmarsspecialhi.h>
#include <melee/ft/kinds/ftMars/ftmarsspeciallw.h>

// Roy shares the Mars motion family, while owning his assets and parameters.
// The template owns all four specials; common kind exceptions remain under audit.
// Exact semantic bindings from retail v1.02 PlCo.dat; no omissions.
static const NativeFighterBone ftEmblemClone_Bones[] = {
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
    { FtPart_RLegJA, 13 },
    { FtPart_RLegJ, 14 },
    { FtPart_RKneeJ, 15 },
    { FtPart_RFootJA, 16 },
    { FtPart_RFootJ, 17 },
    { FtPart_BustN, 23 },
    { FtPart_LShoulderN, 24 },
    { FtPart_LShoulderJA, 27 },
    { FtPart_LShoulderJ, 29 },
    { FtPart_LArmJ, 30 },
    { FtPart_LHandN, 31 },
    { FtPart_L1stNa, 33 },
    { FtPart_L1stNb, 34 },
    { FtPart_L2ndNa, 35 },
    { FtPart_L2ndNb, 36 },
    { FtPart_L3rdNa, 37 },
    { FtPart_L3rdNb, 38 },
    { FtPart_L4thNa, 39 },
    { FtPart_L4thNb, 40 },
    { FtPart_LThumbNa, 41 },
    { FtPart_LThumbNb, 42 },
    { FtPart_LHandNb, 43 },
    { FtPart_NeckN, 44 },
    { FtPart_HeadN, 61 },
    { FtPart_RShoulderN, 62 },
    { FtPart_RShoulderJA, 69 },
    { FtPart_RShoulderJ, 71 },
    { FtPart_RArmJ, 72 },
    { FtPart_RHandN, 73 },
    { FtPart_R1stNa, 75 },
    { FtPart_R1stNb, 79 },
    { FtPart_R2ndNa, 80 },
    { FtPart_R2ndNb, 81 },
    { FtPart_R3rdNa, 82 },
    { FtPart_R3rdNb, 83 },
    { FtPart_R4thNa, 84 },
    { FtPart_R4thNb, 85 },
    { FtPart_RThumbNa, 86 },
    { FtPart_RHandNb, 87 },
    { FtPart_ThrowN, 88 },
    { FtPart_TransN2, 90 },
    { 53, 91 },
};

static const Fighter_DemoStrings ftEmblemClone_DemoMotionNames = {
    "ftDemoResultMotionFileEmblem",
    "ftDemoIntroMotionFileEmblem",
    "ftDemoEndingMotionFileEmblem",
    "ftDemoViWaitMotionFileEmblem",
};

const NativeFighterDefinition ftEmblemClone_Definition = {
    .demo_motion_names = &ftEmblemClone_DemoMotionNames,
    .code_key = "roy_clone",
    .sound_banks = UINT64_C(1) << 31,
    .sound_mute_channels = 0x44,
    .source_kind = Ft_Kind_Emblem,
    .asset_layout = { 0 },
    .bones = ftEmblemClone_Bones,
    .bone_count = sizeof(ftEmblemClone_Bones) / sizeof(ftEmblemClone_Bones[0]),
    .effect_bank = 49,
    .animation_source_kind = Ft_Kind_Emblem,
    .fighter_root = "ftDataEmblem",
    .costume_joint = "PlyEmblem5K_Share_joint",
    .costume_matanim = "PlyEmblem5K_Share_matanim_joint",
    .action_count = 327, .demo_count = 14, .joint_count = 92,
    .motion_state_count = ftMs_MS_SelfCount,
    .motion_states = ftFe_Init_MotionStateTable,
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
