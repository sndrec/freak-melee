#include "ftcaptain_clone.h"

#ifdef MELEE_NATIVE
#include <melee/ft/types.h>
#include <melee/ft/ftdat.h>

#include "ftcaptain.h"
#include "ftcaptainspecialn.h"
#include "ftcaptainspecials.h"
#include "ftcaptainspecialhi.h"
#include "ftcaptainspeciallw.h"

// Owned asset/rig base. An untouched clone retains stock behavior; compiling
// captain_wip.fms clears that family and supplies explicit script/common policies.
// Exact semantic bindings from supplied v1.02 PlCo.dat; no omissions.
static const NativeFighterBone ftCaptainClone_Bones[] = {
    { FtPart_TopN, 0 },
    { FtPart_TransN, 1 },
    { FtPart_XRotN, 2 },
    { FtPart_YRotN, 3 },
    { FtPart_HipN, 4 },
    { FtPart_LLegJA, 6 },
    { FtPart_LLegJ, 7 },
    { FtPart_LKneeJ, 8 },
    { FtPart_LFootJA, 9 },
    { FtPart_LFootJ, 10 },
    { FtPart_RLegJA, 12 },
    { FtPart_RLegJ, 13 },
    { FtPart_RKneeJ, 14 },
    { FtPart_RFootJA, 15 },
    { FtPart_RFootJ, 16 },
    { FtPart_BustN, 18 },
    { FtPart_LShoulderN, 19 },
    { FtPart_LShoulderJA, 21 },
    { FtPart_LShoulderJ, 22 },
    { FtPart_LArmJ, 23 },
    { FtPart_LHandN, 24 },
    { FtPart_L1stNa, 25 },
    { FtPart_L1stNb, 26 },
    { FtPart_L2ndNa, 27 },
    { FtPart_L2ndNb, 28 },
    { FtPart_L3rdNa, 29 },
    { FtPart_L3rdNb, 30 },
    { FtPart_L4thNa, 31 },
    { FtPart_L4thNb, 32 },
    { FtPart_LThumbNa, 33 },
    { FtPart_LHandNb, 35 },
    { FtPart_NeckN, 36 },
    { FtPart_HeadN, 38 },
    { FtPart_RShoulderN, 39 },
    { FtPart_RShoulderJA, 42 },
    { FtPart_RShoulderJ, 44 },
    { FtPart_RArmJ, 45 },
    { FtPart_RHandN, 46 },
    { FtPart_R1stNa, 47 },
    { FtPart_R1stNb, 48 },
    { FtPart_R2ndNa, 49 },
    { FtPart_R2ndNb, 50 },
    { FtPart_R3rdNa, 51 },
    { FtPart_R3rdNb, 52 },
    { FtPart_R4thNa, 53 },
    { FtPart_R4thNb, 54 },
    { FtPart_RThumbNa, 55 },
    { FtPart_RThumbNb, 57 },
    { FtPart_RHandNb, 58 },
    { FtPart_ThrowN, 59 },
    { FtPart_TransN2, 61 },
    { (Fighter_Part) (FT_COMMON_PART_COUNT - 1), 62 },
};

static const Fighter_DemoStrings ftCaptainClone_DemoMotionNames = {
    "ftDemoResultMotionFileCaptain",
    "ftDemoIntroMotionFileCaptain",
    "ftDemoEndingMotionFileCaptain",
    "ftDemoViWaitMotionFileCaptain",
};

const NativeFighterDefinition ftCaptainClone_Definition = {
    .demo_motion_names = &ftCaptainClone_DemoMotionNames,
    .code_key = "captain_clone",
    .sound_banks = UINT64_C(1) << 6, // captain.ssm
    .sound_mute_channels = 0x44,
    .source_kind = Ft_Kind_Captain,
    .asset_layout = { 0 },
    .effect_bank = 4,
    .bones = ftCaptainClone_Bones,
    .bone_count = sizeof(ftCaptainClone_Bones) / sizeof(ftCaptainClone_Bones[0]),
    .animation_source_kind = Ft_Kind_Captain,
    .fighter_root = "ftDataCaptain",
    .costume_joint = "PlyCaptain5K_Share_joint",
    .costume_matanim = "",
    .action_count = 318,
    .demo_count = 14,
    .motion_state_count = ftCa_MS_SelfCount,
    .motion_states = ftCa_Init_MotionStateTable,
    .motion_state_first = ftCa_MS_SwordSwing4,
    .special_ground_state = {
        ftCa_MS_SpecialN, ftCa_MS_SpecialSStart,
        ftCa_MS_SpecialHi, ftCa_MS_SpecialLw,
    },
    .special_air_state = {
        ftCa_MS_SpecialAirN, ftCa_MS_SpecialAirSStart,
        ftCa_MS_SpecialAirHi, ftCa_MS_SpecialAirLw,
    },
    .special_transition_flags = {
        [NativeFighterSpecial_Neutral] =
            Ft_MF_KeepGfx | Ft_MF_SkipMatAnim | Ft_MF_SkipRumble |
            Ft_MF_UpdateCmd | Ft_MF_SkipColAnim | Ft_MF_SkipItemVis |
            Ft_MF_Unk19 | Ft_MF_SkipModelPartVis |
            Ft_MF_SkipModelFlags | Ft_MF_Unk27,
        [NativeFighterSpecial_Side] =
            Ft_MF_KeepGfx | Ft_MF_SkipMatAnim | Ft_MF_UpdateCmd |
            Ft_MF_SkipColAnim | Ft_MF_SkipItemVis | Ft_MF_Unk19 |
            Ft_MF_SkipModelPartVis | Ft_MF_SkipModelFlags | Ft_MF_Unk27,
        [NativeFighterSpecial_Up] = ftCa_MF_SpecialHi,
        [NativeFighterSpecial_Down] = ftCa_MF_SpecialLw,
    },
    .joint_count = 63,
    .special_n = ftCa_SpecialN_Enter,
    .special_air_n = ftCa_SpecialAirN_Enter,
    .special_s = ftCa_SpecialS_Enter,
    .special_air_s = ftCa_SpecialAirS_Enter,
    .special_hi = ftCa_SpecialHi_Enter,
    .special_air_hi = ftCa_SpecialAirHi_Enter,
    .special_lw = ftCa_SpecialLw_Enter,
    .special_air_lw = ftCa_SpecialAirLw_Enter,
};

#endif
