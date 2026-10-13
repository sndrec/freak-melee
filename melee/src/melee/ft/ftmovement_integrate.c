#include "ftcommon.h"
#include "fighter.h"
#include "ft_081B.h"
#include "ftanim.h"
#include "types.h"
#include <dolphin/mtx.h>
#include <math.h>

void ftCommon_IntegrateMovement(Fighter* fp)
{
    Vec3* p_kb_vel;
    Vec3* pAtkShieldKB;
    Vec3 selfVel;
    float kb_vel_x, kb_vel_y, atkShieldKB_X;

    p_kb_vel = &fp->x8c_kb_vel;
    if ((kb_vel_x = p_kb_vel->x) != 0 || p_kb_vel->y != 0) {
        if (fp->ground_or_air == GA_Air) {
            kb_vel_x = p_kb_vel->x;
            kb_vel_y = p_kb_vel->y;

            if (fp->is_sandbag) {
                p_kb_vel->x = ftCommon_SandbagKnockbackDeaccel(
                    p_kb_vel->x, ftCommon_SandbagGetKnockbackDeaccelX(fp));
                p_kb_vel->y = ftCommon_SandbagKnockbackDeaccel(
                    p_kb_vel->y, ftCommon_SandbagGetKnockbackDeaccelY(fp));
            } else {
                float kb_angle = atan2f(kb_vel_y, kb_vel_x);

                if (sqrtf(kb_vel_x * kb_vel_x + kb_vel_y * kb_vel_y) <
                    p_ftCommonData->x204_knockbackFrameDecay)
                {
                    p_kb_vel->x = p_kb_vel->y = 0;
                } else {
                    p_kb_vel->x -=
                        p_ftCommonData->x204_knockbackFrameDecay *
                        cosf(kb_angle);
                    p_kb_vel->y -=
                        p_ftCommonData->x204_knockbackFrameDecay *
                        sinf(kb_angle);
                }
            }

            fp->xF0_ground_kb_vel = 0;
        } else {
            Vec3* pNormal = &fp->coll_data.floor.normal;
            struct ftCo_DatAttrs* pAttr;

            if (fp->xF0_ground_kb_vel == 0) {
                fp->xF0_ground_kb_vel = kb_vel_x;
            }

            pAttr = &fp->co_attrs;
            ftCommon_ApplyGroundedKnockbackFriction(
                fp,
                /*effective friction - ground multiplier is
                   usually 1. last factor was 1 when I looked*/
                /*effective friction - ground multiplier is
                   usually 1. last factor was 1 when I looked*/
                ft_GetGroundFrictionMultiplier(fp) *
                    pAttr->ground_friction * p_ftCommonData->x200);

            // set knockback velocity to ground_kb_vel * surfaceTangent
            p_kb_vel->x = pNormal->y * fp->xF0_ground_kb_vel;
            p_kb_vel->y = -pNormal->x * fp->xF0_ground_kb_vel;
        }
    }
    // Now handle the attacker's shield knockback in a similar way
    pAtkShieldKB = &fp->x98_atk_shield_kb;
    if ((atkShieldKB_X = pAtkShieldKB->x) != 0 || pAtkShieldKB->y != 0) {
        if (fp->ground_or_air == GA_Air) {
            float kb_x = pAtkShieldKB->x;
            float kb_y = pAtkShieldKB->y;
            float atkShieldKBAngle = atan2f(kb_y, kb_x);

            if (sqrtf(kb_x * kb_x + kb_y * kb_y) <
                p_ftCommonData->x3E8_shieldKnockbackFrameDecay)
            {
                /// @bug IN THE MELEE CODE THAT CAUSES THE INVISIBLE
                /// CEILING GLITCH The next line should be 'pAtkShieldKB->y
                /// = 0', but instead it is:
                pAtkShieldKB->x = p_kb_vel->y = 0;
            } else {
                // again, the better implementation would be:
                // *pAtkShieldKB *= (atkShieldKB_len -
                // p_stc_ftcommon->x3e8_shield_kb_frameDecay)/atkShieldKB_len
                // float atkShieldKBAngle = atan2_80022C30(pAtkShieldKB->y,
                // pAtkShieldKB->x);
                pAtkShieldKB->x -=
                    p_ftCommonData->x3E8_shieldKnockbackFrameDecay *
                    cosf(atkShieldKBAngle);
                pAtkShieldKB->y -=
                    p_ftCommonData->x3E8_shieldKnockbackFrameDecay *
                    sinf(atkShieldKBAngle);
            }
            fp->xF4_ground_attacker_shield_kb_vel = 0;
        } else {
            Vec3* pNormal =
                &fp->coll_data.floor
                     .normal; // ground_normal offset inside fp is 0x844,
                              // surface normal points out of the surface.
            struct ftCo_DatAttrs* pAttr;

            if (fp->xF4_ground_attacker_shield_kb_vel == 0) {
                fp->xF4_ground_attacker_shield_kb_vel = atkShieldKB_X;
            }

            pAttr = &fp->co_attrs;

            ftCommon_ApplyShieldKnockbackFriction(
                fp,
                /* effectiveFriction - the last constant variable differs
                   from the one for the knockback friction above*/
                ft_GetGroundFrictionMultiplier(fp) *
                    pAttr->ground_friction *
                    p_ftCommonData->x3EC_shieldGroundFrictionMultiplier);

            /* effectiveFriction - the last constant variable differs from
             * the one for the knockback friction above*/
            pAtkShieldKB->x =
                pNormal->y * fp->xF4_ground_attacker_shield_kb_vel;
            pAtkShieldKB->y =
                -pNormal->x * fp->xF4_ground_attacker_shield_kb_vel;
        }
    }

    fp->gr_vel += fp->xE4_ground_accel_1 + fp->xE8_ground_accel_2;
    fp->xE4_ground_accel_1 = fp->xE8_ground_accel_2 = 0;

    C_VECAdd(&fp->self_vel, &fp->x74_self_accel, &fp->self_vel);
    fp->x74_self_accel.x = fp->x74_self_accel.y = fp->x74_self_accel.z = 0;

    // copy selfVel into a stack storage variable
    selfVel = fp->self_vel; ///< @todo these double_lower_32bit variables
                            ///< are probably integer
    // counters that get decremented each frame, but I was not able to
    // trigger the following condition. The double value construction then
    // is only used as an interpolation tool between selfVel and some
    // UnkVel2.
    if (fp->dmg.x1948 != 0) {
        // The compiler casts an u32 integer 'val' to a double type using
        // double v = *(double*)&(0x43300000_00000000 | val ^ 0x80000000) -
        // *(double*)&43300000_80000000 which is all that happens in the
        // lengthy assembly generated by this
        float C1 = 1.0f;
        float C2 = C1 - (float) fp->dmg.x194C / (float) fp->dmg.x1948;

        selfVel.x =
            C2 * (fp->self_vel.x - fp->xA4_unk_vel.x) + fp->xA4_unk_vel.x;
        selfVel.y =
            C2 * (fp->self_vel.y - fp->xA4_unk_vel.y) + fp->xA4_unk_vel.y;

        fp->dmg.x194C--;
        if (fp->dmg.x194C == 0) {
            fp->dmg.x1948 = 0;
        }
    }

    // add some horizontal+depth offset to the position? Why is there no
    // vertical component?
    fp->cur_pos.x += fp->xF8_playerNudgeVel.x;
    fp->cur_pos.y += 0;
    fp->cur_pos.z += fp->xF8_playerNudgeVel.y;

    if (fp->x2222_b6 && !fp->x2222_b7) {
        s32 bit;

        // fp->xD4_unk_vel += selfVel
        C_VECAdd(&fp->xD4_unk_vel, &selfVel, &fp->xD4_unk_vel);

        fp->xD4_unk_vel.x += p_kb_vel->x;
        fp->xD4_unk_vel.y += p_kb_vel->y;
        fp->xD4_unk_vel.z += 0.0f;

        if (fp->throw_flags_b2) {
            fp->throw_flags_b2 = 0;
            bit = 1;
        } else {
            bit = 0;
        }

        /// @todo @c incompatible-pointer-types because bad headers
        if (bit || ftAnim_80070FD0(fp) || fp->x594_b7) {
            // fp->xB0_position += fp->xD4_unk_vel
            C_VECAdd(&fp->cur_pos, &fp->xD4_unk_vel, &fp->cur_pos);
            /// @todo We set this velocity to 0 after applying it.
            ///       Is this SDI or ASDI?
            fp->xD4_unk_vel.x = fp->xD4_unk_vel.y = fp->xD4_unk_vel.z = 0;
        }
        // fp->xB0_position += *pAtkShieldKB
        C_VECAdd(&fp->cur_pos, pAtkShieldKB, &fp->cur_pos);
    } else {
        // fp@r31.position@0xB0.xyz += selfVel + pAtkShieldKB
        C_VECAdd(&fp->cur_pos, &selfVel, &fp->cur_pos);
        fp->cur_pos.x += p_kb_vel->x;
        fp->cur_pos.y += p_kb_vel->y;
        fp->cur_pos.z += 0;

        C_VECAdd(&fp->cur_pos, pAtkShieldKB, &fp->cur_pos);
    }
}
