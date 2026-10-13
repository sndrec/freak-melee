#include "ftrook.h"

#ifdef MELEE_NATIVE

#include <math.h>
#include <string.h>

#include <melee/ft/fighter.h>
#include <melee/ft/ftcharacter_assets.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcamera.h>
#include <melee/ft/ftcliffcommon.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/kinds/ftCommon/ftCo_Attack100.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallSpecial.h>
#include <melee/ft/kinds/ftCommon/ftCo_JumpAerial.h>
#include <melee/ft/kinds/ftCommon/ftCo_Landing.h>
#include <melee/ft/types.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/random.h>

enum {
    Rook_MS_JumpAerialF1 = 341,
    Rook_MS_SpecialN,
    Rook_MS_SpecialAirN,
    Rook_MS_SpecialS,
    Rook_MS_SpecialAirS,
    Rook_MS_SpecialHiLegacy,
    Rook_MS_SpecialAirHiLegacy,
    Rook_MS_SpecialLw,
    Rook_MS_SpecialAirLw,
    Rook_MS_SpecialHiStart,
    Rook_MS_SpecialHiTravel,
    Rook_MS_SpecialHiEnd,
    Rook_MS_Count = 12
};

static void rook_on_load(Fighter_GObj* gobj)
{
    Fighter* fighter = GET_FIGHTER(gobj);
    memcpy(fighter->dat_attrs_backup, fighter->ft_data->ext_attr,
           FT_CHARACTER_EXT_ATTR_CAPACITY);
    fighter->dat_attrs = fighter->dat_attrs_backup;
    fighter->can_multijump = true;
    fighter->x2D0 = fighter->dat_attrs;
    fighter->u.rk.num_crimes = 3;
}

static void rook_on_death(Fighter_GObj* gobj)
{
    ftParts_80074A4C(gobj, 0, 0);
    ftParts_80074A4C(gobj, 1, 0);
    GET_FIGHTER(gobj)->u.rk.num_crimes = 3;
}

static void rook_on_landing(Fighter_GObj* gobj)
{
    GET_FIGHTER(gobj)->u.rk.num_crimes = 3;
}

static void rook_special_n(Fighter_GObj* gobj)
{
    Fighter_ChangeMotionState(gobj, Rook_MS_SpecialN, Ft_MF_None,
                              1.0f, 1.0f, 0.0f, NULL);
}

static void rook_special_air_n(Fighter_GObj* gobj)
{
    Fighter_ChangeMotionState(gobj, Rook_MS_SpecialAirN, Ft_MF_None,
                              1.0f, 1.0f, 0.0f, NULL);
}

// The MexTK neutral-special callbacks are empty. Use the ordinary grounded
// and aerial motion contracts until Rook's intended effect is authored.
static void rook_neutral_ground_anim(Fighter_GObj* gobj)
{
    if (!ftAnim_IsFramesRemaining(gobj)) {
        ft_8008A2BC(gobj);
    }
}

static void rook_neutral_air_anim(Fighter_GObj* gobj)
{
    if (!ftAnim_IsFramesRemaining(gobj)) {
        ftCo_Fall_Enter(gobj);
    }
}

static void rook_neutral_ground_coll(Fighter_GObj* gobj)
{
    if (!ft_800827A0(gobj)) {
        ftCo_Fall_Enter(gobj);
    }
}

static void rook_neutral_air_coll(Fighter_GObj* gobj)
{
    if (ft_CheckGroundAndLedge(gobj, 0)) {
        ft_80082B1C(gobj);
    } else {
        ftCliffCommon_80081298(gobj);
    }
}

static void rook_side_hit(Fighter_GObj* gobj)
{
    Fighter_ChangeMotionState(gobj, Rook_MS_SpecialS, Ft_MF_None,
                              1.0f, 1.0f, 0.0f, NULL);
}

static void rook_special_s(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    Fighter_ChangeMotionState(gobj, Rook_MS_SpecialAirS, Ft_MF_None,
                              1.0f, 1.0f, 0.0f, NULL);
    fp->deal_dmg_cb = rook_side_hit;
    fp->mv.rk.special_s_started_grounded = 1.0f;
    fp->mv.rk.special_s_grab_flip = 0.0f;
    fp->mv.rk.special_s_started_turned = 0.0f;
}

static void rook_special_air_s(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    Fighter_ChangeMotionState(gobj, Rook_MS_SpecialAirS, Ft_MF_None,
                              1.0f, 1.0f, 0.0f, NULL);
    fp->deal_dmg_cb = rook_side_hit;
    fp->mv.rk.special_s_started_grounded = 0.0f;
    fp->mv.rk.special_s_grab_flip = 0.0f;
    fp->mv.rk.special_s_started_turned = 0.0f;
}

static void rook_side_ground_anim(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->cur_anim_frame == 10.0f && fp->mv.rk.special_s_grab_flip == 0.0f) {
        fp->facing_dir = -fp->facing_dir;
    }
    if (!ftAnim_IsFramesRemaining(gobj)) {
        ft_8008A2BC(gobj);
    }
}

static void rook_side_ground_iasa(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->cmd_vars[0] == 2) {
        ftCliffCommon_80081298(gobj);
        ftCo_800CB870(gobj);
    } else if (fp->cmd_vars[0] == 3) {
        ftCo_Fall_Enter(gobj);
    }
}

static void rook_side_ground_phys(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    ft_800851C0(gobj);
    ft_80085134(gobj);
    if (fp->cmd_vars[0] == 2) {
        ftCommon_Fall(fp, 1.0f, 1.0f);
        ftCommon_CalcSelfAccel_DriftSimple_NoFriction(fp, 0.0f, 0.6f, 1.5f);
    }
}

static void rook_side_air_iasa(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->cmd_vars[0] == 1) {
        ftCliffCommon_80081298(gobj);
    } else if (fp->cmd_vars[0] == 3) {
        ftCo_80096900(gobj, 1, 0, false, 1.0f, 12.0f);
    }
}

static void rook_side_air_phys(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    ft_800851C0(gobj);
    ft_80085134(gobj);
    if (fp->mv.rk.special_s_started_grounded == 1.0f &&
        fp->cur_anim_frame < 11.0f)
    {
        fp->self_vel.y = 0.5f;
    }
    if (fp->cmd_vars[0] == 2) {
        ftCommon_CalcSelfAccel_DriftSimple_NoFriction(fp, 0.0f, 0.6f, 1.5f);
    }
}

static void rook_special_lw(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->u.rk.num_crimes <= 0) {
        return;
    }
    --fp->u.rk.num_crimes;
    fp->mv.rk.special_lw_rand_x = HSD_Randf() * 6.0f - 3.0f;
    fp->mv.rk.special_lw_rand_y = HSD_Randf() * 6.0f - 3.0f;
    Fighter_ChangeMotionState(gobj, Rook_MS_SpecialAirLw, Ft_MF_None,
                              1.0f, 1.0f, 0.0f, NULL);
}

static void rook_down_air_phys(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->cmd_vars[0] == 1) {
        fp->self_vel.x = fp->mv.rk.special_lw_rand_x;
        fp->self_vel.y = fp->mv.rk.special_lw_rand_y;
    } else if (fp->cmd_vars[0] == 2) {
        fp->self_vel.x *= 0.6f;
        fp->self_vel.y *= 0.6f;
    } else if (fp->cmd_vars[0] == 3) {
        ftCo_Fall_Enter(gobj);
    }
}

static void rook_down_air_coll(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (ft_CheckGroundAndLedge(gobj, 1)) {
        if (fp->cmd_vars[0] != 1) {
            ftCo_LandingFallSpecial_Enter(gobj, false, 15.0f);
        }
    }
    ftCliffCommon_80081298(gobj);
}

static void rook_special_hi(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->mv.rk.special_hi_frame_direction_changed = -20.0f;
    fp->mv.rk.special_hi_start_direction = 1.0f;
    fp->mv.rk.special_hi_turned = 0.0f;
    Fighter_ChangeMotionState(gobj, Rook_MS_SpecialHiStart, Ft_MF_None,
                              1.0f, 1.0f, 0.0f, NULL);
    fp->self_vel.y = fp->self_vel.x * 0.25f;
    fp->self_vel.x *= 0.25f;
}

static void rook_up_start_anim(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    float frame = fp->cur_anim_frame;
    if (fp->mv.rk.special_hi_turned == 0.0f &&
        fabsf(fp->input.lstick[0].x) > 0.2f &&
        fp->facing_dir * fp->input.lstick[0].x < 0.0f && frame < 9.0f)
    {
        if (frame > 2.0f) {
            fp->mv.rk.special_hi_turned = 1.0f;
        }
        fp->facing_dir = -fp->facing_dir;
        Fighter_ChangeMotionState(gobj, Rook_MS_SpecialHiStart, Ft_MF_None,
                              frame, 1.0f, 0.0f, NULL);
    }
    if (fp->cmd_vars[1] == 1) {
        Fighter_ChangeMotionState(gobj, Rook_MS_SpecialHiTravel, Ft_MF_None,
                              1.0f, 1.0f, 0.0f, NULL);
    }
}

static void rook_up_travel_anim(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->cmd_vars[1] == 2 || !(fp->input.held_buttons[0] & HSD_PAD_B)) {
        Fighter_ChangeMotionState(gobj, Rook_MS_SpecialHiEnd, Ft_MF_None,
                              1.0f, 1.0f, 0.0f, NULL);
    }
}

static void rook_up_travel_phys(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    float frame = fp->cur_anim_frame;
    float ratio = frame / 110.0f;
    float max_x = 1.5f - ratio * 0.5f;
    float drift = 0.08f - ratio * 0.06f;
    float base_y = 0.6f - ratio * 0.4f;
    float add_y = 0.5f - ratio * 0.3f;
    float stick_x = fp->input.lstick[0].x * 2.0f;
    float stick_y = fp->input.lstick[0].y * 2.0f;
    if (stick_x < -1.0f) {
        stick_x = -1.0f;
    }
    if (stick_y < -1.0f) {
        stick_y = -1.0f;
    }
    // Retain MexTK's second scaling step, even though it is asymmetric.
    stick_x *= 2.0f;
    stick_y *= 2.0f;
    if (stick_x > 1.0f) {
        stick_x = 1.0f;
    }
    if (stick_y > 1.0f) {
        stick_y = 1.0f;
    }
    if (frame == 1.0f) {
        fp->self_vel.x = fp->facing_dir * max_x;
        fp->mv.rk.special_hi_start_direction = fp->facing_dir;
    }
    if (signbit(fp->facing_dir) != signbit(fp->self_vel.x)) {
        fp->mv.rk.special_hi_frame_direction_changed = frame;
        fp->facing_dir = -fp->facing_dir;
    }
    float turn =
        (fp->mv.rk.special_hi_frame_direction_changed + 16.0f - frame) / 16.0f;
    if (turn < 0.0f) {
        turn = 0.0f;
    }
    if (turn > 1.0f) {
        turn = 1.0f;
    }
    ftPartSetRotY(fp, 2,
                  3.14159f *
                      (fp->facing_dir == fp->mv.rk.special_hi_start_direction
                           ? turn
                           : 1.0f - turn));
    fp->self_vel.y = base_y + stick_y * add_y;
    fp->self_vel.x += stick_x * drift;
    if (fp->self_vel.x < -max_x) {
        fp->self_vel.x = -max_x;
    }
    if (fp->self_vel.x > max_x) {
        fp->self_vel.x = max_x;
    }
    ftPartSetRotX(fp, 2,
                  fabsf(fp->self_vel.x * 0.5f) - 0.25f -
                      fp->input.lstick[0].y * 0.25f + 0.1f);
}

static void rook_up_end_anim(Fighter_GObj* gobj)
{
    if (GET_FIGHTER(gobj)->cmd_vars[1] == 3 || !ftAnim_IsFramesRemaining(gobj))
    {
        ftCo_80096900(gobj, 1, 0, false, 1.0f, 20.0f);
    }
}

static void rook_up_moving_coll(Fighter_GObj* gobj)
{
    ft_CheckGroundAndLedge(gobj, 0);
    ftCliffCommon_80081298(gobj);
}

static void rook_up_end_coll(Fighter_GObj* gobj)
{
    if (ft_CheckGroundAndLedge(gobj, 0)) {
        ftCo_LandingFallSpecial_Enter(gobj, false, 20.0f);
    }
    ftCliffCommon_80081298(gobj);
}

static MotionState rook_motion_states[Rook_MS_Count] = {
    { 295, 0, FtMoveId_Default << 24, ftCo_JumpAerialF1_Anim,
      ftCo_JumpAerialF1_IASA, ft_80084DB0, ftCo_JumpAerialF1_Coll,
      ftCamera_UpdateCameraBox },
    { 302, 0, FtMoveId_SpecialN << 24, rook_neutral_ground_anim, NULL,
      ft_80084F3C, rook_neutral_ground_coll, ftCamera_UpdateCameraBox },
    { 303, 0, FtMoveId_SpecialN << 24, rook_neutral_air_anim, NULL,
      ft_80084EEC, rook_neutral_air_coll, ftCamera_UpdateCameraBox },
    { 317, 0, FtMoveId_SpecialS << 24, rook_side_ground_anim,
      rook_side_ground_iasa, rook_side_ground_phys, ftCo_JumpAerialF1_Coll,
      ftCamera_UpdateCameraBox },
    { 318, 0, FtMoveId_SpecialS << 24, NULL, rook_side_air_iasa,
      rook_side_air_phys, ftCo_JumpAerialF1_Coll, ftCamera_UpdateCameraBox },
    // The MexTK file copied Jigglypuff callbacks here. Those callbacks jump
    // to states outside Rook's 341-352 table, so use Rook-owned transitions.
    { 319, 0x340013, FtMoveId_SpecialHi << 24, rook_neutral_ground_anim, NULL,
      ft_80084F3C, rook_neutral_ground_coll, ftCamera_UpdateCameraBox },
    { 320, 0x340413, FtMoveId_SpecialHi << 24, rook_neutral_air_anim, NULL,
      ft_80084EEC, rook_neutral_air_coll, ftCamera_UpdateCameraBox },
    { 323, 0, FtMoveId_SpecialLw << 24, rook_neutral_ground_anim, NULL,
      ft_80084F3C, rook_neutral_ground_coll, ftCamera_UpdateCameraBox },
    { 324, 0, FtMoveId_SpecialLw << 24, NULL, NULL, rook_down_air_phys,
      rook_down_air_coll, ftCamera_UpdateCameraBox },
    { 296, 0, FtMoveId_SpecialHi << 24, rook_up_start_anim, NULL, NULL,
      rook_up_moving_coll, ftCamera_UpdateCameraBox },
    { 297, 0, FtMoveId_SpecialHi << 24, rook_up_travel_anim, NULL,
      rook_up_travel_phys, rook_up_moving_coll, ftCamera_UpdateCameraBox },
    { 298, 0, FtMoveId_SpecialHi << 24, rook_up_end_anim, NULL, ft_80084DB0,
      rook_up_end_coll, ftCamera_UpdateCameraBox },
};

// Joint indices follow the preorder of Rook's 50-node costume skeleton.
// Missing standard hand and finger parts deliberately remain invalid.
static const NativeFighterBone rook_bones[] = {
    { FtPart_TopN, 0 },          { FtPart_TransN, 1 },
    { FtPart_XRotN, 2 },         { FtPart_YRotN, 3 },
    { FtPart_HipN, 4 },          { FtPart_WaistN, 5 },
    { FtPart_BustN, 7 },         { FtPart_LShoulderN, 8 },
    { FtPart_LShoulderJA, 9 },   { FtPart_LShoulderJ, 10 },
    { FtPart_LArmJ, 11 },        { FtPart_LHandN, 12 },
    { FtPart_NeckN, 14 },        { FtPart_HeadN, 15 },
    { FtPart_RShoulderN, 16 },   { FtPart_RShoulderJA, 17 },
    { FtPart_RShoulderJ, 18 },   { FtPart_RArmJ, 19 },
    { FtPart_RHandN, 20 },       { FtPart_LLegJA, 36 },
    { FtPart_LLegJ, 37 },        { FtPart_LKneeJ, 38 },
    { FtPart_LFootJA, 39 },      { FtPart_LFootJ, 40 },
    { FtPart_RLegJA, 42 },       { FtPart_RLegJ, 43 },
    { FtPart_RKneeJ, 44 },       { FtPart_RFootJA, 45 },
    { FtPart_RFootJ, 46 },       { FtPart_ThrowN, 48 },
    { FtPart_TransN2, 48 },      { 53, 49 },
};

const NativeFighterDefinition ftRook_Definition = {
    .code_key = "rook",
    .sound_banks = UINT64_C(1) << 25,
    .sound_mute_channels = 0x44,
    .source_kind = Ft_Kind_None,
    .effect_bank = 0xFF,
    .animation_source_kind = Ft_Kind_Purin,
    .fighter_root = "ftDataRook",
    .costume_joint = "PlyRook5K_Share_joint",
    .costume_matanim = "PlyRook5K_Share_matanim_joint",
    .action_count = 327,
    .demo_count = 14,
    .motion_state_count = Rook_MS_Count,
    .motion_states = rook_motion_states,
    .motion_state_first = Rook_MS_JumpAerialF1,
    .special_ground_state = {
        Rook_MS_SpecialN, Rook_MS_SpecialS, Rook_MS_SpecialHiStart,
        Rook_MS_SpecialAirLw,
    },
    .special_air_state = {
        Rook_MS_SpecialAirN, Rook_MS_SpecialAirS, Rook_MS_SpecialHiStart,
        Rook_MS_SpecialAirLw,
    },
    .joint_count = 50,
    .bone_count = sizeof(rook_bones) / sizeof(rook_bones[0]),
    .bones = rook_bones,
    .on_load = rook_on_load,
    .on_death = rook_on_death,
    .on_landing = rook_on_landing,
    .special_n = rook_special_n,
    .special_air_n = rook_special_air_n,
    .special_s = rook_special_s,
    .special_air_s = rook_special_air_s,
    .special_hi = rook_special_hi,
    .special_air_hi = rook_special_hi,
    .special_lw = rook_special_lw,
    .special_air_lw = rook_special_lw,
};

#endif
