#include "ftganon_clone.h"

#ifdef MELEE_NATIVE
#include <melee/ft/types.h>
#include <melee/ft/ftdat.h>

#include "ftganon.h"
#include <melee/ft/kinds/ftCaptain/forward.h>
#include <melee/ft/kinds/ftCaptain/ftcaptainspecialn.h>
#include <melee/ft/kinds/ftCaptain/ftcaptainspecials.h>
#include <melee/ft/kinds/ftCaptain/ftcaptainspecialhi.h>
#include <melee/ft/kinds/ftCaptain/ftcaptainspeciallw.h>

// Owned asset/rig base. An untouched clone retains stock Captain-family behavior;
// compiling ganon_wip.fms clears that family and supplies explicit policies.
// Exact semantic bindings from supplied v1.02 PlCo.dat; no omissions.
static const NativeFighterBone ftGanonClone_Bones[] = {
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
    { FtPart_LShoulderJA, 20 },
    { FtPart_LShoulderJ, 21 },
    { FtPart_LArmJ, 22 },
    { FtPart_LHandN, 23 },
    { FtPart_L1stNa, 25 },
    { FtPart_L1stNb, 26 },
    { FtPart_L2ndNa, 27 },
    { FtPart_L2ndNb, 28 },
    { FtPart_L3rdNa, 29 },
    { FtPart_L3rdNb, 30 },
    { FtPart_L4thNa, 31 },
    { FtPart_L4thNb, 32 },
    { FtPart_LThumbNa, 33 },
    { FtPart_LHandNb, 34 },
    { FtPart_NeckN, 35 },
    { FtPart_RShoulderJA, 64 },
    { FtPart_RShoulderJ, 65 },
    { FtPart_RArmJ, 66 },
    { FtPart_RHandN, 67 },
    { FtPart_R1stNa, 69 },
    { FtPart_R1stNb, 70 },
    { FtPart_R2ndNa, 71 },
    { FtPart_R2ndNb, 72 },
    { FtPart_R3rdNa, 73 },
    { FtPart_R3rdNb, 74 },
    { FtPart_R4thNa, 75 },
    { FtPart_R4thNb, 76 },
    { FtPart_RThumbNa, 77 },
    { FtPart_RThumbNb, 78 },
    { FtPart_RHandNb, 79 },
    { FtPart_ThrowN, 80 },
    { FtPart_TransN2, 82 },
    { (Fighter_Part) (FT_COMMON_PART_COUNT - 1), 83 },
};

static const Fighter_DemoStrings ftGanonClone_DemoMotionNames = {
    "ftDemoResultMotionFileGanon",
    "ftDemoIntroMotionFileGanon",
    "ftDemoEndingMotionFileGanon",
    "ftDemoViWaitMotionFileGanon",
};

const NativeFighterDefinition ftGanonClone_Definition = {
    .demo_motion_names = &ftGanonClone_DemoMotionNames,
    .code_key = "ganon_clone",
    .sound_banks = UINT64_C(1) << 30,
    .sound_mute_channels = 0x44,
    .source_kind = Ft_Kind_Ganon,
    .asset_layout = { 0 },
    .effect_bank = 19,
    .bones = ftGanonClone_Bones,
    .bone_count = sizeof(ftGanonClone_Bones) / sizeof(ftGanonClone_Bones[0]),
    .animation_source_kind = Ft_Kind_Ganon,
    .fighter_root = "ftDataGanon",
    .costume_joint = "PlyGanon5K_Share_joint",
    .costume_matanim = "",
    .action_count = 318,
    .demo_count = 14,
    .motion_state_count = ftCa_MS_SelfCount,
    .motion_states = ftGn_Init_MotionStateTable,
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
    .joint_count = 84,
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
