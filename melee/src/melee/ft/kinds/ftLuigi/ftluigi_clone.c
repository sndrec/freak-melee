#include "ftluigi_clone.h"

#ifdef MELEE_NATIVE
#include <melee/ft/types.h>
#include <melee/ft/ftdat.h>

#include "ftluigi.h"
#include "ftluigispecialhi.h"
#include "ftluigispeciallw.h"
#include "ftluigispecialn.h"
#include "ftluigispecials.h"
#include "forward.h"
#include "types.h"
#include <melee/ft/inlines.h>

// The clone's imported Fireball article is resolved through its own ftData.
// Do not replace the stock Luigi article registration during fighter load.
static void clone_on_load(Fighter_GObj* gobj)
{
    Fighter* fighter = GET_FIGHTER(gobj);
    PUSH_ATTRS(fighter, ftLuigiAttributes);
}

// Exact semantic bindings from the supplied v1.02 PlCo.dat; no omissions.
static const NativeFighterBone ftLuigiClone_Bones[] = {
    { FtPart_TopN, 0 },
    { FtPart_TransN, 1 },
    { FtPart_XRotN, 2 },
    { FtPart_YRotN, 3 },
    { FtPart_HipN, 4 },
    { FtPart_WaistN, 5 },
    { FtPart_LLegJA, 47 },
    { FtPart_LLegJ, 48 },
    { FtPart_LKneeJ, 49 },
    { FtPart_LFootJA, 50 },
    { FtPart_LFootJ, 51 },
    { FtPart_RLegJA, 53 },
    { FtPart_RLegJ, 54 },
    { FtPart_RKneeJ, 55 },
    { FtPart_RFootJA, 56 },
    { FtPart_RFootJ, 57 },
    { FtPart_LShoulderJA, 6 },
    { FtPart_LShoulderJ, 7 },
    { FtPart_LArmJ, 8 },
    { FtPart_LHandN, 9 },
    { FtPart_L1stNa, 10 },
    { FtPart_L1stNb, 11 },
    { FtPart_L2ndNa, 12 },
    { FtPart_L2ndNb, 13 },
    { FtPart_L3rdNa, 14 },
    { FtPart_L3rdNb, 15 },
    { FtPart_L4thNa, 16 },
    { FtPart_L4thNb, 17 },
    { FtPart_LThumbNa, 18 },
    { FtPart_LHandNb, 19 },
    { FtPart_NeckN, 20 },
    { FtPart_HeadN, 22 },
    { FtPart_RShoulderN, 23 },
    { FtPart_RShoulderJA, 29 },
    { FtPart_RShoulderJ, 30 },
    { FtPart_RArmJ, 31 },
    { FtPart_RHandN, 32 },
    { FtPart_R1stNa, 33 },
    { FtPart_R1stNb, 34 },
    { FtPart_R2ndNa, 35 },
    { FtPart_R2ndNb, 36 },
    { FtPart_R3rdNa, 37 },
    { FtPart_R3rdNb, 38 },
    { FtPart_R4thNa, 39 },
    { FtPart_R4thNb, 40 },
    { FtPart_RThumbNa, 41 },
    { FtPart_RThumbNb, 42 },
    { FtPart_RHandNb, 43 },
    { FtPart_ThrowN, 44 },
    { FtPart_TransN2, 59 },
    { 53, 60 },
};

static const NativeFighterArticleAsset ftLuigiClone_ArticleAssets[] = {
    { It_Kind_Luigi_Fire, 0, -1 },
};

static const Fighter_DemoStrings ftLuigiClone_DemoMotionNames = {
    "ftDemoResultMotionFileLuigi",
    "ftDemoIntroMotionFileLuigi",
    "ftDemoEndingMotionFileLuigi",
    "ftDemoViWaitMotionFileLuigi",
};

const NativeFighterDefinition ftLuigiClone_Definition = {
    .demo_motion_names = &ftLuigiClone_DemoMotionNames,
    .code_key = "luigi_clone",
    .sound_banks = UINT64_C(1) << 17, // luigi.ssm
    .sound_mute_channels = 0x44,
    .source_kind = Ft_Kind_Luigi,
    .asset_layout = { .article_mask = 0x001 },
    .effect_bank = 18,
    .article_asset_count = sizeof(ftLuigiClone_ArticleAssets) / sizeof(ftLuigiClone_ArticleAssets[0]),
    .article_assets = ftLuigiClone_ArticleAssets,
    .animation_source_kind = Ft_Kind_Luigi,
    .fighter_root = "ftDataLuigi",
    .costume_joint = "PlyLuigi5K_Share_joint",
    .costume_matanim = "PlyLuigi5K_Share_matanim_joint",
    .action_count = 312,
    .demo_count = 16,
    .motion_state_count = ftLg_MS_SelfCount,
    .motion_states = ftLg_Init_MotionStateTable,
    .motion_state_first = ftLg_MS_SpecialN,
    .special_ground_state = {
        ftLg_MS_SpecialN, ftLg_MS_SpecialSStart,
        ftLg_MS_SpecialHi, ftLg_MS_SpecialLw,
    },
    .special_air_state = {
        ftLg_MS_SpecialAirN, ftLg_MS_SpecialAirSStart,
        ftLg_MS_SpecialAirHi, ftLg_MS_SpecialAirLw,
    },
    .special_transition_flags = {
        [NativeFighterSpecial_Neutral] = ftLg_MF_SpecialN_Coll,
        [NativeFighterSpecial_Side] = ftLg_MF_SpecialS,
        [NativeFighterSpecial_Up] = ftLg_MF_SpecialHi,
        // Both stock Cyclone ground/air transitions use this exact flag set.
        [NativeFighterSpecial_Down] = 0x0C4C508A,
    },
    .joint_count = 61,
    .bone_count = sizeof(ftLuigiClone_Bones) / sizeof(ftLuigiClone_Bones[0]),
    .bones = ftLuigiClone_Bones,
    .on_load = clone_on_load,
    .special_n = ftLg_SpecialN_Enter,
    .special_air_n = ftLg_SpecialAirN_Enter,
    .neutral_article = ftLg_SpecialN_FireSpawn,
    .special_s = ftLg_SpecialS_Enter,
    .special_air_s = ftLg_SpecialAirS_Enter,
    .special_hi = ftLg_SpecialHi_Enter,
    .special_air_hi = ftLg_SpecialAirHi_Enter,
    .special_lw = ftLg_SpecialLw_Enter,
    .special_air_lw = ftLg_SpecialAirLw_Enter,
};

#endif
