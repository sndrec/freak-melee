#include "ftpurin_clone.h"

#ifdef MELEE_NATIVE
#include <melee/ft/types.h>
#include "ftpurin.h"
#include "ftpurinspecialn.h"
#include "ftpurinspecials.h"
#include "ftpurinspecialhi.h"
#include "ftpurinspeciallw.h"

static const NativeFighterBone ftPurinClone_Bones[] = {
    { FtPart_TopN, 0 },
    { FtPart_TransN, 1 },
    { FtPart_XRotN, 2 },
    { FtPart_YRotN, 3 },
    { FtPart_HipN, 4 },
    { FtPart_WaistN, 5 },
    { FtPart_LLegJA, 36 },
    { FtPart_LLegJ, 37 },
    { FtPart_LKneeJ, 38 },
    { FtPart_LFootJA, 39 },
    { FtPart_LFootJ, 40 },
    { FtPart_RLegJA, 42 },
    { FtPart_RLegJ, 43 },
    { FtPart_RKneeJ, 44 },
    { FtPart_RFootJA, 45 },
    { FtPart_RFootJ, 46 },
    { FtPart_LShoulderJA, 27 },
    { FtPart_LShoulderJ, 28 },
    { FtPart_LArmJ, 29 },
    { FtPart_LHandN, 30 },
    { FtPart_RShoulderJA, 31 },
    { FtPart_RShoulderJ, 32 },
    { FtPart_RArmJ, 33 },
    { FtPart_RHandN, 34 },
    { FtPart_TransN2, 48 },
    { 53, 49 },
};

// Independent asset base. purin_wip.fms replaces all four versus specials;
// Its costume and fighter lifecycle decisions are also script-owned.
static const Fighter_DemoStrings ftPurinClone_DemoMotionNames = {
    "ftDemoResultMotionFilePurin",
    "ftDemoIntroMotionFilePurin",
    "ftDemoEndingMotionFilePurin",
    "ftDemoViWaitMotionFilePurin",
};

const NativeFighterDefinition ftPurinClone_Definition = {
    .demo_motion_names = &ftPurinClone_DemoMotionNames,
    .code_key = "purin_clone",
    .sound_banks = UINT64_C(1) << 25,
    .sound_mute_channels = 0x44,
    .source_kind = Ft_Kind_Purin,
    .asset_layout = { .item_format = FtCharacterItems_ModelParts },
    .effect_bank = 11,
    .animation_source_kind = Ft_Kind_Purin,
    .fighter_root = "ftDataPurin",
    .costume_joint = "PlyPurin5K_Share_joint",
    .costume_matanim = "PlyPurin5K_Share_matanim_joint",
    .action_count = 327,
    .demo_count = 14,
    .motion_state_count = ftPr_MS_SelfCount,
    .motion_states = ftPr_Init_MotionStateTable,
    .motion_state_first = ftPr_MS_JumpAerialF1,
    .special_ground_state = {
        ftPr_MS_SpecialNStartR, ftPr_MS_SpecialS,
        ftPr_MS_SpecialHiL, ftPr_MS_SpecialLwL,
    },
    .special_air_state = {
        ftPr_MS_SpecialAirNStartR, ftPr_MS_SpecialAirS,
        ftPr_MS_SpecialAirHiL, ftPr_MS_SpecialAirLwL,
    },
    .special_transition_flags = {
        [NativeFighterSpecial_Side] =
            ftCommon_GroundAirColl_MF | Ft_MF_KeepGfx | Ft_MF_SkipHit,
        [NativeFighterSpecial_Up] = 0x0C4C508A,
        [NativeFighterSpecial_Down] = 0x0C4C508E,
    },
    .joint_count = 50,
    .bone_count = sizeof(ftPurinClone_Bones) / sizeof(ftPurinClone_Bones[0]),
    .bones = ftPurinClone_Bones,
    .special_n = ftPr_SpecialN_Enter,
    .special_air_n = ftPr_SpecialAirN_Enter,
    .special_s = ftPr_SpecialS_Enter,
    .special_air_s = ftPr_SpecialAirS_Enter,
    .special_hi = ftPr_SpecialHi_Enter,
    .special_air_hi = ftPr_SpecialAirHi_Enter,
    .special_lw = ftPr_SpecialLw_Enter,
    .special_air_lw = ftPr_SpecialAirLw_Enter,
};
#endif
