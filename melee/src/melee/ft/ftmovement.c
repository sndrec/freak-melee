#include "ftcommon.h"
#include "native_fighter_runtime.h"
#ifdef MELEE_NATIVE
#include "native_source_package.h"
#endif
#include "ft_081B.h"
#include <melee/mp/mpcoll.h>
#include <sysdolphin/baselib/gobj.h>
#include "fighter.h"
#include "types.h"
#include <sysdolphin/baselib/debug.h>

void ftCommon_CalcGroundAccel_Deaccel(Fighter* fp, float friction)
{
    if (ABS(friction) > ABS(fp->gr_vel)) {
        friction = -fp->gr_vel;
    } else if (fp->gr_vel > 0) {
        friction = -friction;
    }
    fp->xE4_ground_accel_1 = friction;
}

void ftCommon_CalcGroundAccel_DashRun(Fighter* fp, float accel,
                                      float target_vel, float friction)
{
    if (!target_vel) {
        ftCommon_CalcGroundAccel_Deaccel(fp, friction);
        return;
    }

    if (!(fp->gr_vel * accel < 0)) {
        if (accel > 0) {
            if (fp->gr_vel + accel > target_vel) {
                accel = -friction;
                if (fp->gr_vel + accel < target_vel) {
                    accel = target_vel - fp->gr_vel;
                }
                if (fp->gr_vel + accel >
                    fp->co_attrs.ground_max_horizontal_velocity)
                {
                    accel = fp->co_attrs.ground_max_horizontal_velocity -
                            fp->gr_vel;
                }
            }
        } else if (fp->gr_vel + accel < target_vel) {
            accel = friction;
            if (fp->gr_vel + accel > target_vel) {
                accel = target_vel - fp->gr_vel;
            }
            if (fp->gr_vel + accel <
                -fp->co_attrs.ground_max_horizontal_velocity)
            {
                accel =
                    -fp->co_attrs.ground_max_horizontal_velocity - fp->gr_vel;
            }
        }
    }
    fp->xE4_ground_accel_1 = accel;
}

void ftCommon_CalcGroundAccel_AccelToVel(Fighter* fp, float accel,
                                         float target_vel, float friction)
{
    if (!target_vel) {
        accel = -fp->gr_vel;
    } else if (!(fp->gr_vel * accel < 0)) {
        if (accel > 0) {
            if (fp->gr_vel + accel > target_vel) {
                accel = target_vel - fp->gr_vel;
            }
        } else if (fp->gr_vel + accel < target_vel) {
            accel = target_vel - fp->gr_vel;
        }
    }
    fp->xE4_ground_accel_1 = accel;
}

void ftCommon_CalcGroundAccel_AccelToLStickX(Fighter* fp, float threshold,
                                             float accel_max, float target_max)
{
    float lsx = fp->input.lstick[0].x;
    float accel;
    float target_vel;

    if (ABS(lsx) >= threshold) {
        accel = lsx * accel_max;
        target_vel = lsx * target_max;
    } else {
        target_vel = 0;
        accel = 0;
    }
    ftCommon_CalcGroundAccel_AccelToVel(fp, accel, target_vel, lsx);
}

void ftCommon_ClampGroundVel(Fighter* fp, float max)
{
    if (fp->gr_vel < -max) {
        fp->gr_vel = -max;
    } else if (fp->gr_vel > max) {
        fp->gr_vel = +max;
    }
}

void ftCommon_ApplyGroundedKnockbackFriction(Fighter* fp, float friction)
{
    if (fp->xF0_ground_kb_vel < 0) {
        fp->xF0_ground_kb_vel = fp->xF0_ground_kb_vel + friction;
        if (fp->xF0_ground_kb_vel > 0) {
            fp->xF0_ground_kb_vel = 0;
        }
    } else {
        fp->xF0_ground_kb_vel = fp->xF0_ground_kb_vel - friction;
        if (fp->xF0_ground_kb_vel < 0) {
            fp->xF0_ground_kb_vel = 0;
        }
    }
}

void ftCommon_SetGroundedKnockbackIfLanded(Fighter* fp)
{
    Vec3* ground_normal;
    if (fp->ground_or_air == GA_Ground && fp->xF0_ground_kb_vel == 0) {
        ground_normal = &fp->coll_data.floor.normal;
        fp->xF0_ground_kb_vel = fp->x8c_kb_vel.x;
        if (fp->xF0_ground_kb_vel > p_ftCommonData->max_grounded_kb_on_landing)
        {
            fp->xF0_ground_kb_vel = p_ftCommonData->max_grounded_kb_on_landing;
        }
        if (fp->xF0_ground_kb_vel <
            -p_ftCommonData->max_grounded_kb_on_landing)
        {
            fp->xF0_ground_kb_vel =
                -p_ftCommonData->max_grounded_kb_on_landing;
        }
        fp->x8c_kb_vel.x = +ground_normal->y * fp->xF0_ground_kb_vel;
        fp->x8c_kb_vel.y = -ground_normal->x * fp->xF0_ground_kb_vel;
    }
}

float ftCommon_SandbagKnockbackDeaccel(float kb, float decrement)
{
    float result = kb;
    if (kb > 0) {
        result -= decrement;
        if (result < 0) {
            return 0;
        }
    } else if (kb < 0) {
        result += decrement;
        if (result > 0) {
            return 0;
        }
    }
    return result;
}

float ftCommon_SandbagGetKnockbackDeaccelX(Fighter* fp)
{
    HSD_ASSERT(299, fp->kind == Ft_Kind_Sandbag);
    return ((float*) fp->dat_attrs)[0];
}

float ftCommon_SandbagGetKnockbackDeaccelY(Fighter* fp)
{
    HSD_ASSERT(308, fp->kind == Ft_Kind_Sandbag);
    return ((float*) fp->dat_attrs)[1];
}

void ftCommon_ApplyShieldKnockbackFriction(Fighter* fp, float val)
{
    if (fp->xF4_ground_attacker_shield_kb_vel < 0) {
        fp->xF4_ground_attacker_shield_kb_vel += val;
        if (fp->xF4_ground_attacker_shield_kb_vel > 0) {
            fp->xF4_ground_attacker_shield_kb_vel = 0;
        }
    } else {
        fp->xF4_ground_attacker_shield_kb_vel -= val;
        if (fp->xF4_ground_attacker_shield_kb_vel < 0) {
            fp->xF4_ground_attacker_shield_kb_vel = 0;
        }
    }
}

void ftCommon_CalcSelfAccel_Deaccel(Fighter* fp, float friction)
{
    if (ABS(friction) >= ABS(fp->self_vel.x)) {
        friction = -fp->self_vel.x;
    } else if (fp->self_vel.x > 0) {
        friction = -friction;
    }
    fp->x74_self_accel.x = friction;
}

#define SOLUTION 0
/// @todo This surely calls #ftCommon_CalcSelfAccel_Deaccel somehow...
void ftCommon_CalcSelfAccel_DeaccelAir(Fighter* fp)
{
#if SOLUTION == 0
    float result = fp->co_attrs.aerial_friction;
    float lhs = ABS(result);
    float absvel = ABS(fp->self_vel.x);
    if (ABS(result) >= absvel) {
        result = -fp->self_vel.x;
    } else if (fp->self_vel.x > 0) {
        result = -fp->co_attrs.aerial_friction;
    }
    fp->x74_self_accel.x = result;
#elif SOLUTION == 1
    ftCommon_CalcSelfAccel_Deaccel(fp, fp->co_attrs.aerial_friction);
#endif
}
#undef SOLUTION

bool ftCommon_CalcSelfAccel_DeaccelQuickAir(Fighter* fp)
{
    ftCo_DatAttrs* ca = &fp->co_attrs;
    float vel = fp->self_vel.x;
    float _ = ABS(vel);

    if (ABS(vel) > ca->air_drift_max) {
        float accel = p_ftCommonData->aerial_friction_oob;
        if (ABS(accel) >= ABS(vel)) {
            accel = -vel;
        } else if (vel > 0) {
            accel = -p_ftCommonData->aerial_friction_oob;
        }
        fp->x74_self_accel.x = accel;
        return true;
    } else {
        float accel = fp->co_attrs.aerial_friction;
        if (ABS(accel) >= ABS(vel)) {
            accel = -vel;
        } else if (vel > 0) {
            accel = -fp->co_attrs.aerial_friction;
        }
        fp->x74_self_accel.x = accel;
        return false;
    }
}

bool ftCommon_CalcSelfAccel_DeaccelQuick(Fighter* fp, float max_vel)
{
    float vel = fp->self_vel.x;
    float _ = ABS(vel);

    if (ABS(vel) > max_vel) {
        float accel = p_ftCommonData->aerial_friction_oob;
        if (ABS(accel) >= ABS(vel)) {
            accel = -vel;
        } else if (vel > 0) {
            accel = -p_ftCommonData->aerial_friction_oob;
        }
        fp->x74_self_accel.x = accel;
        return true;
    } else {
        float accel = fp->co_attrs.aerial_friction;
        if (ABS(accel) >= ABS(vel)) {
            accel = -vel;
        } else if (vel > 0) {
            accel = -fp->co_attrs.aerial_friction;
        }
        fp->x74_self_accel.x = accel;
        return false;
    }
}

void ftCommon_CalcSelfAccel_AccelToVelClamped(Fighter* fp, float accel,
                                              float target_vel, float friction)
{
    ftCommon_CalcSelfAccel_AccelToVelClampedFrom(fp, fp->self_vel.x, accel,
                                                 target_vel, friction);
}

void ftCommon_CalcSelfAccel_AccelToVelClampedFrom(Fighter* fp, float vel,
                                                  float accel,
                                                  float target_vel,
                                                  float friction)
{
    if (!target_vel) {
        ftCommon_CalcSelfAccel_Deaccel(fp, friction);
        return;
    }

    if (!(vel * accel < 0.0F)) {
        if (accel > 0) {
            if (vel + accel > target_vel) {
                accel = -friction;
                if (vel + accel < target_vel) {
                    accel = target_vel - vel;
                }
                if (vel + accel > fp->co_attrs.air_max_horizontal_velocity) {
                    accel = fp->co_attrs.air_max_horizontal_velocity - vel;
                }
            }
        } else if (vel + accel < target_vel) {
            accel = friction;
            if (vel + accel > target_vel) {
                accel = target_vel - vel;
            }
            if (vel + accel < -fp->co_attrs.air_max_horizontal_velocity) {
                accel = -fp->co_attrs.air_max_horizontal_velocity - vel;
            }
        }
    }
    fp->x74_self_accel.x = accel;
}

void ftCommon_CalcSelfAccel_Drift(Fighter* fp)
{
    ftCommon_CalcSelfAccel_DriftFrom(fp, fp->self_vel.x);
}

void ftCommon_CalcSelfAccel_DriftFrom(Fighter* fp, float vel)
{
    float accel_scaling;
    float accel_flat;
    float lsx = fp->input.lstick[0].x;
    ftCo_DatAttrs* attrs = &fp->co_attrs;
    accel_scaling = lsx * attrs->air_drift_stick_mul;

    if (lsx > 0) {
        accel_flat = +attrs->aerial_drift_base;
    } else {
        accel_flat = -attrs->aerial_drift_base;
    }
    ftCommon_CalcSelfAccel_AccelToVelClampedFrom(
        fp, vel, accel_scaling + accel_flat, lsx * attrs->air_drift_max,
        attrs->aerial_friction);
}

void ftCommon_CalcSelfAccel_AccelToVel(Fighter* fp, float accel,
                                       float target_vel, float unused)
{
    if (!target_vel) {
        accel = -fp->self_vel.x;
    } else if (!(fp->self_vel.x * accel < 0)) {
        if (accel > 0) {
            if (fp->self_vel.x + accel > target_vel) {
                accel = target_vel - fp->self_vel.x;
            }
        } else {
            if (fp->self_vel.x + accel < target_vel) {
                accel = target_vel - fp->self_vel.x;
            }
        }
    }
    fp->x74_self_accel.x = accel;
}

void ftCommon_CalcSelfAccel_DriftSimple(Fighter* fp, float threshold,
                                        float accel_max, float target_max)
{
    float accel, target;

    if (ABS(fp->input.lstick[0].x) >= threshold) {
        accel = fp->input.lstick[0].x * accel_max;
        target = fp->input.lstick[0].x * target_max;
    } else {
        target = 0;
        accel = 0;
    }
    ftCommon_CalcSelfAccel_AccelToVelClamped(fp, accel, target,
                                             fp->co_attrs.aerial_friction);
}

void ftCommon_CalcSelfAccel_DriftSimple_NoFriction(Fighter* fp,
                                                   float threshold,
                                                   float accel_max,
                                                   float target_max)
{
    float accel;
    float target_vel;

    if (ABS(fp->input.lstick[0].x) >= threshold) {
        accel = fp->input.lstick[0].x * accel_max;
        target_vel = fp->input.lstick[0].x * target_max;
    } else {
        target_vel = 0;
        accel = 0;
    }

    ftCommon_CalcSelfAccel_AccelToVel(fp, accel, target_vel, 0.0F);
}

void ftCommon_ClampSelfVelX(Fighter* fp, float max)
{
    float vel = fp->self_vel.x;
    if (vel < -max) {
        fp->self_vel.x = -max;
    } else if (vel > max) {
        fp->self_vel.x = +max;
    }
}

void ftCommon_ClampAirDrift(Fighter* fp)
{
    ftCommon_ClampSelfVelX(fp, fp->co_attrs.air_drift_max);
}

void ftCommon_Fall(Fighter* fp, float gravity, float terminal_vel)
{
    fp->self_vel.y -= gravity;
    if (fp->self_vel.y < -terminal_vel) {
        fp->self_vel.y = -terminal_vel;
    }
}

void ftCommon_FallBasic(Fighter* fp)
{
    ftCommon_Fall(fp, fp->co_attrs.gravity, fp->co_attrs.terminal_velocity);
}

void ftCommon_FallFast(Fighter* fp)
{
    fp->self_vel.y = -fp->co_attrs.fast_fall_velocity;
}

void ftCommon_ClampFallSpeed(Fighter* fp, float val)
{
    if (fp->self_vel.y > val) {
        fp->self_vel.y = val;
    }
}

void ftCommon_Ascend(Fighter* fp, float accel, float max)
{
    fp->self_vel.y += accel;
    if (fp->self_vel.y > max) {
        fp->self_vel.y = max;
    }
}

void ftCommon_SetSelfMovementFromGroundedMovement(HSD_GObj* gobj)
{
    float ground_friction;
    Vec3* ground_normal;
    Fighter* fp = gobj->user_data;

    u8 _[8];

    ground_normal = &fp->coll_data.floor.normal;
    ground_friction = ft_GetGroundFrictionMultiplier(fp);
    if (ground_friction < 1) {
        fp->xE4_ground_accel_1 *= ground_friction;
    }

    // TODO(Aitch): this almost definitely uses above inline, but I couldn't
    // make it work. Let the brain machines match it.
    fp->x74_self_accel.x = +ground_normal->y * fp->xE4_ground_accel_1;
    fp->x74_self_accel.y = -ground_normal->x * fp->xE4_ground_accel_1;
    fp->x74_self_accel.z = 0;
    fp->self_vel.x = +ground_normal->y * fp->gr_vel;
    fp->self_vel.y = -ground_normal->x * fp->gr_vel;
    fp->self_vel.z = 0;
}

void ftCommon_SetSelfMovementFromGroundedMovement_NoFriction(HSD_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    Vec3* ground_normal = &fp->coll_data.floor.normal;
    fp->x74_self_accel.x = +ground_normal->y * fp->xE4_ground_accel_1;
    fp->x74_self_accel.y = -ground_normal->x * fp->xE4_ground_accel_1;
    fp->x74_self_accel.z = 0;
    fp->self_vel.x = +ground_normal->y * fp->gr_vel;
    fp->self_vel.y = -ground_normal->x * fp->gr_vel;
    fp->self_vel.z = 0;
}

float ft_GetGroundFrictionMultiplier(Fighter* fp)
{
#ifdef MELEE_NATIVE
    const NativeSourcePackage* source=nativeFighterRuntime_SourcePackage(fp->kind);
    if(source)return source->assets.facts.ignore_surface_friction?1.0F:mpColl_8004CA6C(&fp->coll_data);
#endif
    const NativeFighterDefinition* definition =
        nativeFighterRuntime_Definition(fp->kind);
    if (definition != NULL)
        return definition->ignore_surface_friction ? 1.0F
            : mpColl_8004CA6C(&fp->coll_data);
    if (fp->kind == Ft_Kind_Popo || fp->kind == Ft_Kind_Nana) {
        return 1.0F;
    }
    return mpColl_8004CA6C(&fp->coll_data);
}

