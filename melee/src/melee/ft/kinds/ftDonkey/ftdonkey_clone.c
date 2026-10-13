#include "ftdonkey_clone.h"
#ifdef MELEE_NATIVE
#include <melee/ft/types.h>
#include <melee/ft/ftdat.h>
#include "ftdonkey.h"
#include "ftdonkeyspecialn.h"
#include "ftdonkeyspecials.h"
#include "ftdonkeyspecialhi.h"
#include "ftdonkeyspeciallw.h"

// Independent asset base for the authored template. Inherited setup and
// shared-kind policies still require an ownership audit.
// Retail PlCo semantic map; all costume graphs have no omitted joints.
static const NativeFighterBone ftDonkeyClone_Bones[] = {
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
    { FtPart_RLegJA, 15 },
    { FtPart_RLegJ, 16 },
    { FtPart_RKneeJ, 17 },
    { FtPart_RFootJA, 18 },
    { FtPart_RFootJ, 19 },
    { FtPart_BustN, 25 },
    { FtPart_LShoulderN, 26 },
    { FtPart_LShoulderJA, 27 },
    { FtPart_LShoulderJ, 28 },
    { FtPart_LArmJ, 29 },
    { FtPart_LHandN, 30 },
    { FtPart_L1stNa, 31 },
    { FtPart_L1stNb, 32 },
    { FtPart_L2ndNa, 33 },
    { FtPart_L2ndNb, 34 },
    { FtPart_L3rdNa, 35 },
    { FtPart_L3rdNb, 36 },
    { FtPart_L4thNa, 37 },
    { FtPart_L4thNb, 38 },
    { FtPart_LThumbNa, 39 },
    { FtPart_LHandNb, 40 },
    { FtPart_NeckN, 41 },
    { FtPart_HeadN, 43 },
    { FtPart_RShoulderN, 44 },
    { FtPart_RShoulderJA, 49 },
    { FtPart_RShoulderJ, 50 },
    { FtPart_RArmJ, 51 },
    { FtPart_RHandN, 52 },
    { FtPart_R1stNa, 53 },
    { FtPart_R1stNb, 54 },
    { FtPart_R2ndNa, 55 },
    { FtPart_R2ndNb, 56 },
    { FtPart_R3rdNa, 57 },
    { FtPart_R3rdNb, 58 },
    { FtPart_R4thNa, 59 },
    { FtPart_R4thNb, 60 },
    { FtPart_RThumbNa, 61 },
    { FtPart_RThumbNb, 62 },
    { FtPart_RHandNb, 63 },
    { FtPart_ThrowN, 64 },
    { FtPart_TransN2, 73 },
    { 53, 74 },
};

static const Fighter_DemoStrings ftDonkeyClone_DemoMotionNames = {
    "ftDemoResultMotionFileDonkey",
    "ftDemoIntroMotionFileDonkey",
    "ftDemoEndingMotionFileDonkey",
    "ftDemoViWaitMotionFileDonkey",
};

const NativeFighterDefinition ftDonkeyClone_Definition = {
    .bone_count = sizeof(ftDonkeyClone_Bones) / sizeof(ftDonkeyClone_Bones[0]),
    .bones = ftDonkeyClone_Bones,
    .demo_motion_names = &ftDonkeyClone_DemoMotionNames,
    .code_key = "donkey_clone",
    .sound_banks = UINT64_C(1) << 8,
    .sound_mute_channels = 0x44,
    .source_kind = Ft_Kind_Donkey,
    .asset_layout = { 0 },
    .effect_bank = 8,
    .animation_source_kind = Ft_Kind_Donkey,
    .fighter_root = "ftDataDonkey",
    .costume_joint = "PlyDonkey5K_Share_joint",
    .costume_matanim = "PlyDonkey5K_Share_matanim_joint",
    .action_count = 337, .demo_count = 14, .joint_count = 75,
    .motion_state_count = ftDk_MS_SelfCount,
    .motion_states = ftDk_Init_MotionStateTable,
    .motion_state_first = ftDk_MS_HeavyWait,
    .special_ground_state = { ftDk_MS_SpecialNStart, ftDk_MS_SpecialS,
                              ftDk_MS_SpecialHi, ftDk_MS_SpecialLwStart },
    .special_air_state = { ftDk_MS_SpecialAirNStart, ftDk_MS_SpecialAirS,
                           ftDk_MS_SpecialAirHi, ftDk_MS_SpecialLwEnd1 },
    .special_n = ftDk_SpecialN_Enter, .special_air_n = ftDk_SpecialAirN_Enter,
    .special_s = ftDk_SpecialS_Enter, .special_air_s = ftDk_SpecialAirS_Enter,
    .special_hi = ftDk_SpecialHi_Enter, .special_air_hi = ftDk_SpecialAirHi_Enter,
    .special_lw = ftDk_SpecialLw_Enter,
};
#endif
