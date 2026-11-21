#ifdef MELEE_NATIVE
#include "native_hitbox_lifetime.h"
#endif
#include "ftaction.h"
#include "ftaction_codec.h"
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_runtime.h>

#include <Runtime/platform.h>

#include <melee/lb/forward.h>

#include <placeholder.h>

#include "fighter.h"
#include "forward.h"
#include "ft_081B.h"
#include "ft_0877.h"
#include "ft_0881.h"
#include "ft_0899.h"
#include "ft_0C88.h"
#include "ft_0DF0.h"
#include "ftanim.h"
#include "ftcolanim.h"
#include "ftcoll.h"
#include "ftcommon.h"
#include "ftdynamics.h"
#include "ftparts.h"
#include "inlines.h"
#include "kinds/ftCommon/ftCo_09F7.h"
#include "types.h"
#include <dolphin/mtx.h>
#include <melee/lb/inlines.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/lb/lbcommand.h>
#include <melee/lb/types.h>
#ifdef MELEE_NATIVE
#include <melee/ft/ftaction_data.h>
#endif
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/random.h>

/* 07121C */ static void ftAction_8007121C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 0715EC */ static void ftAction_800715EC(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 07162C */ static void ftAction_8007162C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 07168C */ static void ftAction_8007168C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 07169C */ static void ftAction_8007169C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 0716F8 */ static void ftAction_800716F8(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071708 */ static void ftAction_80071708(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071774 */ static void ftAction_80071774(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071784 */ static void ftAction_80071784(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 0717C8 */ static void ftAction_800717C8(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 0717D8 */ static void ftAction_800717D8(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071810 */ static void ftAction_80071810(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071820 */ static void ftAction_80071820(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 0718A4 */ static void ftAction_800718A4(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071908 */ static void ftAction_80071908(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 07192C */ static void ftAction_8007192C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071950 */ static void ftAction_80071950(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071974 */ static void ftAction_80071974(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071998 */ static void ftAction_80071998(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071A14 */ static void ftAction_80071A14(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071A58 */ static void ftAction_80071A58(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071A9C */ static void ftAction_80071A9C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071AE8 */ static void ftAction_80071AE8(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071B28 */ static void ftAction_80071B28(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071CCC */ static void ftAction_80071CCC(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071D30 */ static void ftAction_80071D30(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071D40 */ static void ftAction_80071D40(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071D94 */ static void ftAction_80071D94(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071DCC */ static void ftAction_80071DCC(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071E04 */ static void ftAction_80071E04(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071F0C */ static void ftAction_80071F0C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071F34 */ static void ftAction_80071F34(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071F78 */ static void ftAction_80071F78(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071FA0 */ static void ftAction_80071FA0(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071FC8 */ static void ftAction_80071FC8(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 0722C8 */ static void ftAction_800722C8(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072320 */ static void ftAction_80072320(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 0726C0 */ static void ftAction_800726C0(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 0726F4 */ static void ftAction_800726F4(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 0727C8 */ static void ftAction_800727C8(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 07283C */ static void ftAction_8007283C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072894 */ static void ftAction_80072894(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 0728F8 */ static void ftAction_800728F8(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 07296C */ static void ftAction_8007296C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 07297C */ static void ftAction_8007297C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 0729C4 */ static void ftAction_800729C4(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 0729D4 */ static void ftAction_800729D4(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072A4C */ static void ftAction_80072A4C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072A5C */ static void ftAction_80072A5C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072AAC */ static void ftAction_80072AAC(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072ABC */ static void ftAction_80072ABC(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072B04 */ static void ftAction_80072B04(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072B14 */ static void ftAction_80072B14(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072B3C */ static void ftAction_80072B3C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072B84 */ static void ftAction_80072B84(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072B94 */ static void ftAction_80072B94(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072BE4 */ static void ftAction_80072BE4(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072BF4 */ static void ftAction_80072BF4(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072C5C */ static void ftAction_80072C5C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072C6C */ static void ftAction_80072C6C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072CB0 */ static void ftAction_80072CB0(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072CD8 */ static void ftAction_80072CD8(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072E24 */ static void ftAction_80072E24(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072E4C */ static void ftAction_80072E4C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072FE0 */ static void ftAction_80072FE0(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 073008 */ static void ftAction_80073008(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 07309C */ static void ftAction_8007309C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 073118 */ static void ftAction_80073118(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 07320C */ static void ftAction_8007320C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);

static FtCmd ftAction_803C06E8[] = {
    ftAction_80071028, ftAction_8007121C, ftAction_8007162C, ftAction_8007169C,
    ftAction_80071708, ftAction_80071784, ftAction_800717D8, ftAction_80071B50,
    ftAction_80071CCC, ftAction_80071820, ftAction_800718A4, ftAction_80071908,
    ftAction_8007192C, ftAction_80071950, ftAction_80071974, ftAction_80071998,
    ftAction_80071A14, ftAction_80071A58, ftAction_80071A9C, ftAction_80071AE8,
    ftAction_80071B28, ftAction_80071D40, ftAction_80071D94, ftAction_80071DCC,
    ftAction_80071E04, ftAction_80071F34, ftAction_80071F78, ftAction_80071FA0,
    ftAction_80071FC8, ftAction_80072320, ftAction_800726F4, ftAction_800727C8,
    ftAction_80072894, ftAction_800728F8, ftAction_8007297C, ftAction_800729D4,
    ftAction_80072A5C, ftAction_80072ABC, ftAction_80072B14, ftAction_80072B3C,
    ftAction_80072B94, ftAction_80072BF4, ftAction_80072C6C, ftAction_80072CB0,
    ftAction_80072CD8, ftAction_80072E4C, ftAction_80073008, ftAction_800730B8,
    ftAction_80073118,
#ifdef MELEE_NATIVE
    nativeHitboxLifetime_Spawn,
    ftAction_8007121C,
    nativeHitboxLifetime_Spawn,
    nativeHitboxLifetime_Spawn,
    ftAction_80071E04,
#endif
};

static FtCmd ftAction_803C07AC[ARRAY_SIZE(ftAction_803C06E8)] = {
    ftAction_800711DC, ftAction_800715EC, ftAction_8007168C, ftAction_800716F8,
    ftAction_80071774, ftAction_800717C8, ftAction_80071810, ftAction_80071CA4,
    ftAction_80071D30, ftAction_80071820, ftAction_800718A4, ftAction_80071908,
    ftAction_8007192C, ftAction_80071950, ftAction_80071974, ftAction_80071998,
    ftAction_80071A14, ftAction_80071A58, ftAction_80071A9C, ftAction_80071AE8,
    ftAction_80071B28, ftAction_80071D40, ftAction_80071D94, ftAction_80071DCC,
    ftAction_80071F0C, ftAction_80071F34, ftAction_80071F78, ftAction_80071FA0,
    ftAction_800722C8, ftAction_800726C0, ftAction_800726F4, ftAction_8007283C,
    ftAction_80072894, ftAction_8007296C, ftAction_800729C4, ftAction_80072A4C,
    ftAction_80072AAC, ftAction_80072B04, ftAction_80072B14, ftAction_80072B84,
    ftAction_80072BE4, ftAction_80072C5C, ftAction_80072C6C, ftAction_80072CB0,
    ftAction_80072E24, ftAction_80072FE0, ftAction_8007309C, ftAction_80073108,
    ftAction_8007320C,
#ifdef MELEE_NATIVE
    nativeHitboxLifetime_Skip,
    ftAction_800715EC,
    nativeHitboxLifetime_Skip,
    nativeHitboxLifetime_Skip,
    ftAction_80071F0C,
#endif
};

static u8 ftAction_803C0870[ARRAY_SIZE(ftAction_803C06E8)] = {
    05, 05, 01, 01, 01, 01, 01, 03, 01, 01, 01, 01, 01, 01, 01, 01, 01,
    01, 01, 01, 01, 01, 01, 01, 03, 01, 01, 01, 07, 04, 01, 01, 01, 01,
    01, 01, 01, 01, 01, 01, 01, 01, 01, 01, 03, 03, 02, 01, 04
#ifdef MELEE_NATIVE
    , 06, 05, NATIVE_HITBOX_EXTENSION_WORDS, NATIVE_HITBOX_EXTENSION_WORDS,
    FT_ACTION_THROW_FRACTIONAL_WORDS
#endif
};

/*
SubactionEvent 10 GFXSpawn

https://github.com/Ploaj/HSDLib/blob/939cc10be5a9f76fdcf3ca8ff4a54e35e545fe4a/HSDRawViewer/Scripts/command_fighter.yml#L35

Bone ID
Use Common Bone IDs
Destroy on State Change
Unk1
GFX ID
Unk2
Offset Z
Offset Y
Offset X
Range Z
Range Y
Range X
*/

/// @brief Spawns GFX
void ftAction_80071028(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    float unk;
    Vec3 offset;
    Vec3 range;
    int bone;
    int use_common_bone_id;
    int destroy_on_state_change;
    u32 gfx_id;
    {
        if (!fp->invisible) {
            if (cmd->u->spawn_gfx_0.useUnkBone) {
                bone = fp->ft_data->x8->x12;
            } else {
                bone = cmd->u->spawn_gfx_0.boneId;
            }
            use_common_bone_id = cmd->u->spawn_gfx_0.useCommonBoneIDs;
            destroy_on_state_change = cmd->u->spawn_gfx_0.destroyOnStateChange;

            NEXT_CMD(cmd);
            gfx_id = cmd->u->spawn_gfx_1.gfxID;
            unk = cmd->u->spawn_gfx_1.unkFloat;

            NEXT_CMD(cmd);
            /// @todo i believe they are actually read in reverse order, maybe
            // ftCo_8009F834 also reads them in reverse.
            // double check this in-game eventually...
            offset.x = 0.003906f * cmd->u->spawn_gfx_2.offsetZ;
            offset.y = 0.003906f * cmd->u->spawn_gfx_2.offsetY;

            NEXT_CMD(cmd);
            offset.z = 0.003906f * cmd->u->spawn_gfx_3.offsetX;
            range.x = 0.003906f * cmd->u->spawn_gfx_3.rangeZ;

            NEXT_CMD(cmd);
            range.y = 0.003906f * cmd->u->spawn_gfx_4.rangeY;
            range.z = 0.003906f * cmd->u->spawn_gfx_4.rangeX;

            NEXT_CMD(cmd);
            ftCo_8009F834(gobj, gfx_id, bone, use_common_bone_id,
                          destroy_on_state_change, &offset, &range, unk);
        } else {
            ftAction_800711DC(gobj, cmd);
        }
    }
}

/// @brief Skips GFX Spawn
void ftAction_800711DC(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 5);
}

/// @brief Spawns Hitboxes
void ftAction_8007121C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp;
    HitCapsule* hitbox;
    u32 hit_group;
    u32 idx;
    unsigned slot = cmd->u->create_hitbox_0.id;
    PAD_STACK(8);
#ifdef MELEE_NATIVE
    const bool owned_flag = nativeHitbox_OwnsFlags(cmd->u->create_hitbox_0.opcode);
    const bool victim_only = cmd->u->create_hitbox_0.only_hit_grabbed;
    const bool capsule = nativeHitbox_HasCapsule(cmd->u->create_hitbox_0.opcode, slot);
    slot = nativeHitbox_Slot(cmd->u->create_hitbox_0.opcode, slot);
#endif

    fp = GET_FIGHTER(gobj);
    // The retail byte-0xF flag is decoded into this field on native builds.
    if (cmd->u[3].create_hitbox_3.ignore_thrown_fighters &&
        fp->x1064_thrownHitbox.owner == NULL)
    {
        ftAction_800715EC(gobj, cmd);
    } else {
#ifdef MELEE_NATIVE
        nativeHitboxLifetime_Clear(fp, slot);
        if (capsule) {
            u32 words[2];
            memcpy(words, cmd->u + NATIVE_HITBOX_EXTENSION_WORDS, sizeof(words));
            NativeHitboxLifetime* lifetime = &fp->native_hitbox_lifetimes[slot];
            lifetime->capsule = true;
            lifetime->end_offset = (Vec3) {
                (s16) words[1] * FT_ACTION_HITBOX_DISTANCE_SCALE,
                (s16) (words[0] >> 16) * FT_ACTION_HITBOX_DISTANCE_SCALE,
                (s16) words[0] * FT_ACTION_HITBOX_DISTANCE_SCALE,
            };
        }
#endif
        hit_group = cmd->u->create_hitbox_0.hit_group;
        if (((hitbox = &fp->x914[slot])->state ==
             HitCapsule_Disabled) ||
            (hitbox->x4 != hit_group))
        {
            hitbox->x4 = hit_group;
            hitbox->state = HitCapsule_Enabled;
            fp->x2219_b3 = 1;
            ftColl_800768A0(fp, hitbox);
        }
        idx = cmd->u->create_hitbox_0.bone;
        if (cmd->u->create_hitbox_0.use_common_bone_ids) {
            hitbox->jobj = fp->parts[ftParts_GetBoneIndex(
                                         fp, cmd->u->create_hitbox_0.bone)]
                               .joint;
        } else {
            hitbox->jobj = fp->parts[idx].joint;
        }
#ifdef MELEE_NATIVE
        // A scripted item can bind and release its reserved joint during play.
        if (hitbox->jobj == NULL) {
            hitbox->state = HitCapsule_Disabled;
            nativeHitboxLifetime_Clear(fp, slot);
            ftAction_800715EC(gobj, cmd);
            ftCommon_80080484(fp);
            return;
        }
        if (nativeHitbox_IsExtended(cmd->u->create_hitbox_0.opcode)) {
            u32 extension;
            memcpy(&extension, cmd->u + NATIVE_HITBOX_EXTENSION_WORD, sizeof(extension));
            ftColl_SetHitboxDamage(hitbox, cmd->u->create_hitbox_0.damage +
                                          nativeHitbox_DamageFraction(extension), gobj);
        } else
#endif
        {
            ftColl_8007ABD0(hitbox, cmd->u->create_hitbox_0.damage, gobj);
        }
        NEXT_CMD(cmd);
        hitbox->scale = FT_ACTION_HITBOX_DISTANCE_SCALE * cmd->u->create_hitbox_1.size;
        hitbox->b_offset.x = FT_ACTION_HITBOX_DISTANCE_SCALE * cmd->u->create_hitbox_1.z_offset;
        NEXT_CMD(cmd);
        hitbox->b_offset.y = FT_ACTION_HITBOX_DISTANCE_SCALE * cmd->u->create_hitbox_2.y_offset;
        hitbox->b_offset.z = FT_ACTION_HITBOX_DISTANCE_SCALE * cmd->u->create_hitbox_2.x_offset;
        NEXT_CMD(cmd);
        ftColl_8007AC9C(hitbox, cmd->u->create_hitbox_3.angle, gobj);
        hitbox->x24 = cmd->u->create_hitbox_3.knockback_growth;
        hitbox->x28 = cmd->u->create_hitbox_3.weight_set_knockback;
        hitbox->x43_b0 = cmd->u->create_hitbox_3.item_hit_interaction;
        hitbox->x43_b1 = cmd->u->create_hitbox_3.ignore_fighter_scale;
        hitbox->x40_b0 = cmd->u->create_hitbox_3.clank;
        hitbox->x40_b1 = cmd->u->create_hitbox_3.rebound;
        NEXT_CMD(cmd);
        hitbox->x2C = cmd->u->create_hitbox_4.base_knockback;
        hitbox->element = cmd->u->create_hitbox_4.element;
        hitbox->x34 = cmd->u->create_hitbox_4.shield_damage;
        hitbox->sfx_severity = cmd->u->create_hitbox_4.hit_sfx_severity;
        hitbox->sfx_kind = cmd->u->create_hitbox_4.hit_sfx_kind;
        hitbox->x40_b2 = cmd->u->create_hitbox_4.hit_aerial;
        hitbox->x40_b3 = cmd->u->create_hitbox_4.hit_grounded;
        NEXT_CMD(cmd);
        hitbox->x42_b5 = 1;
        hitbox->x42_b7 = 1;
        hitbox->x41_b4 = 0;
        hitbox->x41_b6 = 0;
        hitbox->x41_b5 = 0;
        hitbox->x42_b0 = 0;
        hitbox->x42_b4 = 0;
        hitbox->x41_b7 = 0;
#ifdef MELEE_NATIVE
        hitbox->hit_grabbed_victim_only =
            owned_flag ? victim_only : ftActionData_NextHitboxFlag(cmd->u);
#else
        hitbox->hit_grabbed_victim_only = cmd->u->create_hitbox_5.x1_b4;
#endif
        hitbox->x42_b1 = 1;
        hitbox->x42_b2 = 0;
        hitbox->x43_b2 = 0;
        if ((HSD_GObj_CurrentInvokedProc != NULL) &&
            (HSD_GObj_CurrentInvokedProc->s_link > 9))
        {
            ftColl_8007AD18(fp, slot);
        }
    }
    ftCommon_80080484(fp);
}

/// @brief Skips Hitbox Spawn
void ftAction_800715EC(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 5);
}

/*
- code: 12
  name: Adjust Hitbox Damage
  parameters:
  - name: Hitbox ID
    bitCount: 3
  - name: Damage
    bitCount: 23
*/
/// @brief Adjust Hitbox Damage
void ftAction_8007162C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    HitCapsule* hit = &GET_FIGHTER(gobj)->x914[cmd->u->set_hitbox_damage.idx];
    ftColl_8007ABD0(hit, cmd->u->set_hitbox_damage.value, gobj);
    NEXT_CMD(cmd);
}

/// @remarks after each CommandInfo function, there seems to be
/// a function that has the ability to skip it (ftAction_800715EC's usage)
/// unsure if theyre all actually called
void ftAction_8007168C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 1);
}

/// @brief Adjusts Hitbox Scale
void ftAction_8007169C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    HitCapsule* hit = &fp->x914[cmd->u->set_hitbox_scale.idx];
    hit->scale = FT_ACTION_HITBOX_DISTANCE_SCALE * cmd->u->set_hitbox_scale.value;
    NEXT_CMD(cmd);
}

void ftAction_800716F8(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 1);
}

/// @brief Sets Hitbox flags
void ftAction_80071708(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    HitCapsule* hit = &fp->x914[cmd->u->set_hitbox_x42_b57.idx];
    switch (cmd->u->set_hitbox_x42_b57.type) {
    case 0:
        hit->x42_b5 = cmd->u->set_hitbox_x42_b57.value;
        break;
    case 1:
        hit->x42_b7 = cmd->u->set_hitbox_x42_b57.value;
        break;
    }
    NEXT_CMD(cmd);
}

void ftAction_80071774(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 1);
}

void ftAction_80071784(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftColl_8007AFC8(gobj, cmd->u->set_throw_flags.hit_idx);
    NEXT_CMD(cmd);
}

void ftAction_800717C8(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 1);
}

void ftAction_800717D8(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftColl_8007AFF8(gobj);
    NEXT_CMD(cmd);
}

void ftAction_80071810(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 1);
}

/// Set one of #Fighter::cmd_vars
/// @todo Heavily suggests that #Fighter::cmd_vars is not an array.
void ftAction_80071820(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);

    switch (cmd->u->set_cmd_var.idx) {
    case 0:
        fp->cmd_vars[0] = cmd->u->set_cmd_var.value;
        break;
    case 1:
        fp->cmd_vars[1] = cmd->u->set_cmd_var.value;
        break;
    case 2:
        fp->cmd_vars[2] = cmd->u->set_cmd_var.value;
        break;
    case 3:
        fp->cmd_vars[3] = cmd->u->set_cmd_var.value;
        break;
    }
    NEXT_CMD(cmd);
}

void ftAction_800718A4(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    switch (cmd->u->set_throw_flags.hit_idx) {
    case 0:
        fp->throw_flags_b3 = true;
        fp->cmd_timer = cmd->timer;
        break;
    case 1:
        fp->throw_flags_b4 = true;
        break;
    }
    NEXT_CMD(cmd);
}

void ftAction_80071908(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->throw_flags_b1 = true;
    NEXT_CMD(cmd);
}

void ftAction_8007192C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->throw_flags_b2 = true;
    NEXT_CMD(cmd);
}

/// Allow interrupt
void ftAction_80071950(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->allow_interrupt = true;
    NEXT_CMD(cmd);
}

void ftAction_80071974(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->throw_flags_b0 = true;
    NEXT_CMD(cmd);
}

void ftAction_80071998(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    switch (cmd->u->set_airborne_state.state) {
    case 0:
        ftCommon_8007D7FC(fp);
        break;
    case 1:
        ftCommon_8007D5D4(fp);
        break;
    case 2:
        ftCommon_8007D60C(fp);
        break;
    }
    NEXT_CMD(cmd);
}

void ftAction_80071A14(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftColl_8007B62C(gobj, cmd->u->set_airborne_state.state);
    NEXT_CMD(cmd);
}

void ftAction_80071A58(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftColl_8007B0C0(gobj, cmd->u->set_airborne_state.state);
    NEXT_CMD(cmd);
}

/// @brief Sets Hurt Capsule State
void ftAction_80071A9C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftColl_8007B128(gobj, cmd->u->set_hurt_state.bone_idx,
                    cmd->u->set_hurt_state.state);
    NEXT_CMD(cmd);
}

/// @brief Sets Jab Combo
void ftAction_80071AE8(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);

    if (!cmd->u->set_jab_combo.disabled || fp->x197C != NULL) {
        fp->x2218_b1 = true;
    }
    NEXT_CMD(cmd);
}

void ftAction_80071B28(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->x2218_b2 = cmd->u->set_jab_rapid.state;
    NEXT_CMD(cmd);
}

void ftAction_80071B50(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp;
    u32 sfx;
    u8 vol;
    u8 pan;
    s32 behavior;

    fp = GET_FIGHTER(gobj);
    behavior = cmd->u->sound_effect_0.behavior;
    NEXT_CMD(cmd);

    switch (behavior) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        sfx = cmd->u->sound_effect_1.sfx_id;
        NEXT_CMD(cmd);
        vol = cmd->u->sound_effect_2.volume;
        pan = cmd->u->sound_effect_2.panning;

#ifdef MELEE_NATIVE
        if (sfx - FT_ACTION_CLIP_ID_BASE < FT_ACTION_CLIP_COUNT) {
            const int id = nativeFighterRuntime_SoundClip(fp->kind,
                sfx - FT_ACTION_CLIP_ID_BASE, behavior);
            if (id < 0) {
                break;
            }
            sfx = id;
        }
#endif
        switch (behavior) {
        case 0:
            ft_PlaySFX(fp, sfx, vol, pan);
            break;
        case 1:
            ft_80088478(fp, sfx, vol, pan);
            break;
        case 2:
            ft_800881D8(fp, sfx, vol, pan);
            break;
        case 3:
            ft_80088510(fp, sfx, vol, pan);
            break;
        case 4:
            ft_800885A8(fp, sfx, vol, pan);
            break;
        case 5:
            ft_80088640(fp, sfx, vol, pan);
            break;
        case 6:
            ft_80088328(fp, sfx, vol, pan);
            break;
        }
        break;

    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        NEXT_CMD(cmd);
        switch (behavior) {
        case 10:
            ft_80088828(fp);
            break;
        case 11:
            ft_80088770(fp);
            break;
        case 12:
            ft_80088884(fp);
            break;
        case 13:
            ft_800888E0(fp);
            break;
        case 14:
            ft_8008893C(fp);
            break;
        case 15:
            ft_800887CC(fp);
            break;
        }
        break;

    default:
        break;
    }

    NEXT_CMD(cmd);
}

void ftAction_80071CA4(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 3);
}

void ftAction_80071CCC(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    FtSFX* sfx = fp->ft_data->x4C_sfx;
    FtSFXArr* sfx_smash = sfx->smash;
    if (sfx_smash == NULL) {
        NEXT_CMD(cmd);
    } else {
        ft_800889F4(fp, sfx_smash);
        NEXT_CMD(cmd);
    }
}

void ftAction_80071D30(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 1);
}

void ftAction_80071D40(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftParts_80074B0C(gobj, cmd->u->set_dobj_flags.idx,
                     cmd->u->set_dobj_flags.value);
    NEXT_CMD(cmd);
}

void ftAction_80071D94(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftParts_80074A8C(gobj);
    NEXT_CMD(cmd);
}

void ftAction_80071DCC(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftParts_80074ACC(gobj);
    NEXT_CMD(cmd);
}

void ftAction_80071E04(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    HitCapsule* hit = &fp->xDF4[cmd->u->set_throw_hitbox_0.idx];
#ifdef MELEE_NATIVE
    const bool fractional = cmd->u->set_throw_hitbox_0.opcode ==
                            FT_ACTION_THROW_FRACTIONAL_OPCODE;
    if (fractional) {
        float damage;
        memcpy(&damage, cmd->u + FT_ACTION_THROW_WORDS, sizeof(damage));
        ftColl_SetHitboxDamage(hit, damage, gobj);
    } else
#endif
    {
        ftColl_8007ABD0(hit, cmd->u->set_throw_hitbox_0.damage, gobj);
    }
    NEXT_CMD(cmd);

    ftColl_8007AC9C(hit, cmd->u->set_throw_hitbox_1.unk0, gobj);
    hit->x24 = cmd->u->set_throw_hitbox_1.hit_x24;
    hit->x28 = cmd->u->set_throw_hitbox_1.hit_x28;
    NEXT_CMD(cmd);

    hit->x2C = cmd->u->set_throw_hitbox_2.hit_x2C;
    hit->element = cmd->u->set_throw_hitbox_2.element;
    hit->sfx_severity = cmd->u->set_throw_hitbox_2.sfx_severity;
    hit->sfx_kind = cmd->u->set_throw_hitbox_2.sfx_kind;
    NEXT_CMD(cmd);
#ifdef MELEE_NATIVE
    if (fractional) NEXT_CMD(cmd);
#endif
}

void ftAction_80071F0C(Fighter_GObj* gobj, CommandInfo* cmd)
{
#ifdef MELEE_NATIVE
    if (cmd->u->set_throw_hitbox_0.opcode == FT_ACTION_THROW_FRACTIONAL_OPCODE) {
        SKIP_CMD(cmd, FT_ACTION_THROW_FRACTIONAL_WORDS);
        return;
    }
#endif
    SKIP_CMD(cmd, 3);
}

void ftAction_80071F34(Fighter_GObj* gobj, CommandInfo* cmd)
{
    PAD_STACK(8);
    ftCommon_8007F5CC(gobj, cmd->u->unk27.value);
    NEXT_CMD(cmd);
}

void ftAction_80071F78(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->x221E_b4 = cmd->u->set_article_vis.value;
    NEXT_CMD(cmd);
}

void ftAction_80071FA0(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->x221E_b5 = cmd->u->set_fighter_vis.value;
    NEXT_CMD(cmd);
}

void ftAction_80071FC8(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp;
    s32 rand;
    u8 volume;
    u8 panning;
    u32 behavior;
    s32 sfx_id;

    fp = GET_FIGHTER(gobj);
    volume = cmd->u->pseudo_random_sfx_0.volume;
    panning = cmd->u->pseudo_random_sfx_0.panning;
    behavior = cmd->u->pseudo_random_sfx_0.behavior;
    rand = cmd->u->pseudo_random_sfx_0.random_range;
    NEXT_CMD(cmd);
    rand = HSD_Randi(rand);
    switch (rand) {
    case 0:
        sfx_id = cmd->u->pseudo_random_sfx_1.sfx_id;
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        break;
    case 1:
        NEXT_CMD(cmd);
        sfx_id = cmd->u->pseudo_random_sfx_1.sfx_id;
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        break;
    case 2:
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        sfx_id = cmd->u->pseudo_random_sfx_1.sfx_id;
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        break;
    case 3:
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        sfx_id = cmd->u->pseudo_random_sfx_1.sfx_id;
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        break;
    case 4:
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        sfx_id = cmd->u->pseudo_random_sfx_1.sfx_id;
        NEXT_CMD(cmd);
        break;
    case 5:
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        sfx_id = cmd->u->pseudo_random_sfx_1.sfx_id;
        break;
    }
    NEXT_CMD(cmd);
    switch (behavior) {
    case 0:
        ft_PlaySFX(fp, sfx_id, volume, panning);
        return;
    case 1:
        ft_80088478(fp, sfx_id, volume, panning);
        return;
    case 2:
        ft_800881D8(fp, sfx_id, volume, panning);
        return;
    case 3:
        ft_80088510(fp, sfx_id, volume, panning);
        return;
    case 4:
        ft_800885A8(fp, sfx_id, volume, panning);
        return;
    case 5:
        ft_80088640(fp, sfx_id, volume, panning);
        return;
    case 6:
        ft_80088328(fp, sfx_id, volume, panning);
    default:
        return;
    }
}

void ftAction_800722C8(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 7);
}

void ftAction_80072320(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp;
    u8 pitch_select;
    s32 spC;
    s32 sp8;
    s32 sfx;
    u32 behavior;
    u32 sfx_base;
    f32 direction;
    s32 sfx_param0;
    s32 sfx_param1;
    s32 sfx_param2;

    fp = GET_FIGHTER(gobj);
    pitch_select = cmd->u->stage_sfx_0.pitch_select;
    behavior = cmd->u->stage_sfx_0.sfx_base;
    sfx_base = cmd->u->stage_sfx_0.x2_b0_7;

    switch (pitch_select) {
    case 0:
        direction = 0.0f;
        break;
    case 1:
        direction = 1.0f;
        break;
    case 2:
        direction = -1.0f;
        break;
    case 3:
        direction = fp->facing_dir;
        break;
    }

    NEXT_CMD(cmd);
    sfx = ft_80087D0C(fp, cmd->u->stage_sfx_1.sfx_id);
    NEXT_CMD(cmd);
    sfx_param2 = cmd->u->stage_sfx_2.x2_b0_15;
    NEXT_CMD(cmd);
    sfx_param0 = cmd->u->stage_sfx_3.x2_b0_7;
    sfx_param1 = cmd->u->stage_sfx_3.x3_b0_7;
    NEXT_CMD(cmd);

    if (sfx_base <= 6 && fp->x2225_b6 &&
        (nativeFighterRuntime_SoundMuteChannels(fp->kind) & (1u << sfx_base))) return;

    switch (sfx_base) {
    case 0:
        sp8 = 0;
        spC = -1;
        fp->x2160 = lbAudioAx_800264E4(
            lbAudioAx_800263E8(direction, gobj, behavior, sfx, 127, 127,
                               sfx_param0, sfx_param1, sfx_param2, sp8, spC));
        break;

    case 1:
        sp8 = fp->player_id + fp->x221F_b4;
        fp->x214C = lbAudioAx_800264E4(lbAudioAx_800263E8(
            direction, gobj, behavior, sfx, 127, 127, sfx_param0, sfx_param1,
            sfx_param2, sp8 + 0x36, -1));
        break;

    case 2:
        sp8 = fp->player_id + fp->x221F_b4;
        fp->x2144 = lbAudioAx_800264E4(lbAudioAx_800263E8(
            direction, gobj, behavior, sfx, 127, 127, sfx_param0,
            sfx_param1, sfx_param2, sp8 + 0x1E, -1));
        break;

    case 3:
        sp8 = fp->player_id + fp->x221F_b4;
        fp->x2150 = lbAudioAx_800264E4(lbAudioAx_800263E8(
            direction, gobj, behavior, sfx, 127, 127, sfx_param0, sfx_param1,
            sfx_param2, sp8 + 0x42, -1));
        break;

    case 4:
        sp8 = fp->player_id + fp->x221F_b4;
        fp->x2154 = lbAudioAx_800264E4(lbAudioAx_800263E8(
            direction, gobj, behavior, sfx, 127, 127, sfx_param0, sfx_param1,
            sfx_param2, sp8 + 0x4E, -1));
        break;

    case 5:
        sp8 = fp->player_id + fp->x221F_b4;
        fp->x2158 = lbAudioAx_800264E4(lbAudioAx_800263E8(
            direction, gobj, behavior, sfx, 127, 127, sfx_param0, sfx_param1,
            sfx_param2, sp8 + 0x5A, -1));
        break;

    case 6:
        sp8 = fp->player_id + fp->x221F_b4;
        fp->x2148 = lbAudioAx_800264E4(lbAudioAx_800263E8(
            direction, gobj, behavior, sfx, 127, 127, sfx_param0,
            sfx_param1, sfx_param2, sp8 + 0x2A, -1));
        break;

    }
}

void ftAction_800726C0(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 4);
}

void ftAction_800726F4(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftAnim_800704F0(gobj, cmd->u->set_tex_anim.idx,
                    cmd->u->set_tex_anim.frame);
    if (cmd->u->set_tex_anim.b) {
        ftAnim_800704F0(gobj, cmd->u->set_tex_anim.idx2,
                        cmd->u->set_tex_anim.frame);
    }
    NEXT_CMD(cmd);
}

void ftAction_800727C8(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftAnim_ApplyPartAnim(gobj, cmd->u->part_anim.unk1, cmd->u->part_anim.unk2,
                         cmd->u->part_anim.unk3);
    NEXT_CMD(cmd);
}

void ftAction_8007283C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftAnim_ApplyPartAnim(gobj, cmd->u->part_anim.unk1, cmd->u->part_anim.unk2,
                         0.0f);
    NEXT_CMD(cmd);
}

void ftAction_80072894(Fighter_GObj* gobj, CommandInfo* cmd)
{
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(gobj, NativeCallbackSource_Fighter, &generation)) return;
    Fighter* fighter = GET_FIGHTER(gobj);
    FighterKind kind = fighter->kind;
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(kind);
    const void* instruction = cmd->u;
    ftCommon_8007E83C(gobj, cmd->u->unk9.unk1, cmd->u->unk9.unk2);
    if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter) ||
        fighter->kind != kind || nativeFighterRuntime_Definition(kind) != definition ||
        cmd->u != instruction) return;
    NEXT_CMD(cmd);
}

void ftAction_800728F8(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);

    if (cmd->u->unk10.unk1) {
        ftCommon_8007EC30(cmd->u->unk10.unk2, cmd->u->unk10.unk3);
    } else {
        ftCommon_8007EBAC(fp, cmd->u->unk10.unk2, cmd->u->unk10.unk3);
    }
    NEXT_CMD(cmd);
}

void ftAction_8007296C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    NEXT_CMD(cmd);
}

void ftAction_8007297C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    ftCommon_8007ECD4(fp, cmd->u->unk11.unk1);
    NEXT_CMD(cmd);
}

void ftAction_800729C4(Fighter_GObj* gobj, CommandInfo* cmd)
{
    NEXT_CMD(cmd);
}

void ftAction_800729D4(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    int unk2 = cmd->u->unk12.unk2;
    int unk3 = cmd->u->unk12.unk3;
    switch (cmd->u->unk12.unk1) {
    case 0:
        ftCommon_8007EEC8(fp, unk2, unk3);
        break;
    case 1:
        ftCommon_8007EF5C(fp, unk2);
        break;
    }
    NEXT_CMD(cmd);
}

void ftAction_80072A4C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    NEXT_CMD(cmd);
}

void ftAction_80072A5C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    ftCo_800BFFD0(fp, cmd->u->unk13.unk1, cmd->u->unk13.unk2);
    NEXT_CMD(cmd);
}

void ftAction_80072AAC(Fighter_GObj* gobj, CommandInfo* cmd)
{
    NEXT_CMD(cmd);
}

void ftAction_80072ABC(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftCo_800C0200(GET_FIGHTER(gobj), cmd->u->unk14.unk1);
    NEXT_CMD(cmd);
}

void ftAction_80072B04(Fighter_GObj* gobj, CommandInfo* cmd)
{
    NEXT_CMD(cmd);
}

void ftAction_80072B14(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->x221E_b1 = cmd->u->unk15.unk1;
    NEXT_CMD(cmd);
}

void ftAction_80072B3C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->x2100 = cmd->u->unk16.unk4;
    fp->x2101_bits_8 = cmd->u->unk16.unk3;
    NEXT_CMD(cmd);
}

void ftAction_80072B84(Fighter_GObj* gobj, CommandInfo* cmd)
{
    NEXT_CMD(cmd);
}

void ftAction_80072B94(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    ftCo_8009E318(gobj, cmd->u->unk17.unk1, fp->cur_anim_frame);
    NEXT_CMD(cmd);
}

void ftAction_80072BE4(Fighter_GObj* gobj, CommandInfo* cmd)
{
    NEXT_CMD(cmd);
}

void ftAction_80072BF4(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    Fighter_TakeDamage_8006CC7C(fp, cmd->u->unk18.damage_amount);
    NEXT_CMD(cmd);
}

void ftAction_80072C5C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    NEXT_CMD(cmd);
}

void ftAction_80072C6C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ft_8008A1B8(gobj, cmd->u->unk19.unk1);
    NEXT_CMD(cmd);
}

void ftAction_80072CB0(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->x2225_b2 = cmd->u->unk20.unk1;
    NEXT_CMD(cmd);
}

void ftAction_80072CD8(Fighter_GObj* gobj, CommandInfo* cmd)
{
    int sp64;
    int sp60;
    int gfx_id;
    CommandInfo _cmd;
    u32 cmd_words[3];
    Vec3 offset;
    Vec3 range;
    u32 part;
    Fighter* fp;
    u32 unused;

    fp = gobj->user_data;
    sp60 = 1;

    if (ft_80084BFC(gobj, &sp64, &sp60, &gfx_id) != false) {
        if (sp64 != -1) {
            _cmd.u = (union CmdUnion*) cmd_words;
            cmd_words[0] = *(u32*) cmd->u;
            cmd_words[1] = sp64;
            cmd_words[2] = *(u32*) ((u8*) cmd->u + 8);
            ftAction_80071B50(gobj, &_cmd);
        }

        if (gfx_id != -1) {
            offset.z = 0.0f;
            range.z = 0.0f;
            offset.y = 0.0f;
            range.y = 0.0f;
            offset.x = 0.0f;
            range.x = 0.0f;

            if (!cmd->u->footstep_fx_0.use_alt_bone) {
                part = fp->ft_data->x8->x13;
            } else {
                part = fp->ft_data->x8->x14;
            }
            ftCo_8009F834(gobj, gfx_id, part, 0, 0, &offset, &range, 0.0f);
        }
    }

    if (sp60 != 0) {
        ftAction_80071B50(gobj, cmd);
        return;
    }

    ++cmd->u;
    ++cmd->u;
    ++cmd->u;
}

void ftAction_80072E24(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 3);
}

void ftAction_80072E4C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    int sp64;
    int sp60;
    int gfx_id;
    CommandInfo _cmd;
    u32 cmd_words[3];
    Vec3 offset;
    Vec3 range;
    Fighter* fp;
    s32 cmd_flag;
    u32 unused;

    fp = gobj->user_data;
    sp60 = 1;
    gfx_id = -1;
    cmd_flag = cmd->u->unk_fx_0.x1_b0_7;
    cmd_flag &= 1;
    if ((ft_80084C38(gobj, &sp64, &sp60, &gfx_id) != false) &&
        (cmd_flag == 0) && (sp64 != -1))
    {
        _cmd.u = (union CmdUnion*) cmd_words;
#ifdef MELEE_NATIVE
        cmd_words[0] = 0;
        _cmd.u->sound_effect_0.behavior =
            (cmd->u->unk_fx_0.x0_b6_7 << 6) |
            (cmd->u->unk_fx_0.x1_b0_7 >> 2);
#else
        cmd_words[0] = *(u32*) cmd->u;
#endif
        cmd_words[1] = sp64;
        cmd_words[2] = ((u32*) cmd->u)[2];
        ftAction_80071B50(gobj, &_cmd);
    }

    if (gfx_id == -1) {
#ifdef MELEE_NATIVE
        gfx_id = (cmd->u->unk_fx_0.x2_b0_7 << 8) |
                 cmd->u->unk_fx_0.x3_b0_7;
#else
        gfx_id = ((u16*) cmd->u)[1];
#endif
    }
    offset.z = 0.0f;
    range.z = 0.0f;
    offset.y = 0.0f;
    range.y = 0.0f;
    offset.x = 0.0f;
    range.x = 0.0f;
    ftCo_8009F834(gobj, gfx_id, FtPart_TopN, 0, 0, &offset, &range, 0.0f);
    if (sp60 != 0) {
        ft_PlaySFX(fp, 0x46, 0x7FU, 0x40U);
        if (cmd_flag == 0) {
#ifdef MELEE_NATIVE
            _cmd.u = (union CmdUnion*) cmd_words;
            cmd_words[0] = 0;
            _cmd.u->sound_effect_0.behavior =
                (cmd->u->unk_fx_0.x0_b6_7 << 6) |
                (cmd->u->unk_fx_0.x1_b0_7 >> 2);
            cmd_words[1] = ((u32*) cmd->u)[1];
            cmd_words[2] = ((u32*) cmd->u)[2];
            ftAction_80071B50(gobj, &_cmd);
            cmd->u += _cmd.u - (union CmdUnion*) cmd_words;
#else
            ftAction_80071B50(gobj, cmd);
#endif
        }
    }
    if ((sp60 == 0) || (cmd_flag != 0)) {
        ++cmd->u;
        ++cmd->u;
        ++cmd->u;
    }
    ftCommon_8007EBAC(fp, 0x16U, 0U);
}

void ftAction_80072FE0(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 3);
}

void ftAction_80073008(Fighter_GObj* gobj, CommandInfo* cmd)
{
    f32 charge_frames;
    f32 charge_rate;
    f32 color_anim;
    f32 dmg_mult;

    charge_rate = cmd->u->smash_charge_0.charge_rate;
    charge_frames = cmd->u->smash_charge_0.charge_frames;
    dmg_mult = 0.003906f * charge_rate;
    NEXT_CMD(cmd);
    color_anim = cmd->u->smash_charge_1.color_anim;
    NEXT_CMD(cmd);
    ftCo_800DEE84(gobj, (s32) color_anim, charge_frames, dmg_mult);
}

void ftAction_8007309C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 2);
}

void ftAction_800730B8(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    ftCo_800C8B60(fp, cmd->u->unk21.unk1, cmd->u->unk21.unk2);
    NEXT_CMD(cmd);
}

void ftAction_80073108(Fighter_GObj* gobj, CommandInfo* cmd)
{
    NEXT_CMD(cmd);
}

void ftAction_80073118(Fighter_GObj* gobj, CommandInfo* cmd)
{
    u8 idx;
    s32 timer;
    f32 x;
    f32 y;
    f32 mag;
    f32 decay_amt;
    f32 angle;

    idx = cmd->u->wind_fx_0.bone;
    NEXT_CMD(cmd);

    x = 0.003906f * cmd->u->wind_fx_1.timer;
    y = 0.003906f * cmd->u->wind_fx_1.x;
    NEXT_CMD(cmd);

    mag = 0.003906f * cmd->u->wind_fx_2.y;
    decay_amt = 0.003906f * cmd->u->wind_fx_2.mag;
    NEXT_CMD(cmd);

    timer = cmd->u->wind_fx_3.angle;
    angle = 0.003906f * cmd->u->wind_fx_3.decay;
    NEXT_CMD(cmd);

    ftCo_8009E714(gobj, idx, timer, x, y, mag, decay_amt, angle);
}

void ftAction_8007320C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 4);
}

void ftAction_80073240(Fighter_GObj* fighter_gobj)
{
    Fighter* fp = fighter_gobj->user_data;
    CommandInfo* ftCommand = &fp->x3E4_fighterCmdScript;
    u32 eventCode;
#ifdef MELEE_NATIVE
    unsigned remaining = LB_COMMAND_UPDATE_LIMIT;
#endif
    fp->x3E4_fighterCmdScript.frame_count = fp->cur_anim_frame + fp->x898_unk;
    if (fp->x3E4_fighterCmdScript.u != NULL) {
        if (ftCommand->timer != F32_MAX) {
            ftCommand->timer -= fp->frame_speed_mul;
        }
        do {
            if (ftCommand->u == NULL) {
                break;
            }
            if (F32_MAX == ftCommand->timer) {
                if (ftCommand->frame_count >= fp->frame_speed_mul) {
                    break;
                }
                ftCommand->timer = -ftCommand->frame_count;
            } else if (ftCommand->timer > 0.0f) {
                break;
            }
#ifdef MELEE_NATIVE
            if (remaining-- == 0) break;
#endif
            eventCode =
                gmScriptEventCast(ftCommand->u, gmScriptEventDefault)->opcode;
            if (Command_Execute(ftCommand, eventCode) == false) {
                eventCode -= 0xA;
                ftAction_803C06E8[eventCode](fighter_gobj, ftCommand);
            }
        } while (F32_MAX != ftCommand->timer);
    }
}

void ftAction_80073354(Fighter_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    CommandInfo* cmd = (&fp->x3E4_fighterCmdScript);
    u32 eventCode;
#ifdef MELEE_NATIVE
    unsigned remaining = LB_COMMAND_UPDATE_LIMIT;
#endif

    fp->x3E4_fighterCmdScript.frame_count = fp->cur_anim_frame + fp->x898_unk;
    fp->throw_flags = 0;
    if (fp->x3E4_fighterCmdScript.u != NULL) {
        if (cmd->timer != F32_MAX) {
            cmd->timer -= fp->frame_speed_mul;
        }
        do {
            if (cmd->u == NULL) {
                break;
            }
            if (cmd->timer == F32_MAX) {
                if (cmd->frame_count >= fp->frame_speed_mul) {
                    break;
                }
                cmd->timer = -cmd->frame_count;
            } else if (cmd->timer > 0.0f) {
                break;
            }
#ifdef MELEE_NATIVE
            if (remaining-- == 0) break;
#endif
            {
                float timer = cmd->timer;
                eventCode =
                    gmScriptEventCast(cmd->u, gmScriptEventDefault)->opcode;
                if (Command_Execute(cmd, eventCode) == false) {
                    eventCode -= 0xA;
                    ftAction_803C07AC[eventCode](gobj, cmd);
                }
                if (cmd->timer != timer && cmd->timer <= 0.0f) {
                    fp->throw_flags = 0;
                }
            }
        } while (cmd->timer != F32_MAX);
    }
}

void ftAction_8007349C(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    CommandInfo* cmd = (&fp->x3E4_fighterCmdScript);
    PAD_STACK(8);
#ifdef MELEE_NATIVE
    unsigned remaining = LB_COMMAND_UPDATE_LIMIT;
#endif
    fp->x3E4_fighterCmdScript.frame_count = fp->cur_anim_frame + fp->x898_unk;
    if (fp->x3E4_fighterCmdScript.u == NULL) {
        return;
    }
    if (cmd->timer != F32_MAX) {
        cmd->timer -= fp->frame_speed_mul;
    }
    do {
        if (cmd->u == NULL) {
            break;
        }
        if (cmd->timer == F32_MAX) {
            if (cmd->frame_count >= fp->frame_speed_mul) {
                break;
            }
            cmd->timer = -cmd->frame_count;
        } else if (cmd->timer > 0.0f) {
            break;
        }
#ifdef MELEE_NATIVE
        if (remaining-- == 0) break;
#endif
        {
            u32 id = cmd->u->Command_09.id;
            if (!Command_Execute(cmd, id)) {
#ifdef MELEE_NATIVE
                if (nativeHitbox_IsExtended(id)) {
                    cmd->u += nativeHitbox_ExtendedWidth(cmd->u->create_hitbox_0.id);
                } else
#endif
                {
                    cmd->u += ftAction_803C0870[id - 10];
                }
            }
        }
    } while (cmd->timer != F32_MAX);
}
