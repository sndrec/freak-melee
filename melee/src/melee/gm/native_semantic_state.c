#include "native_semantic_state.h"

#ifdef MELEE_NATIVE

#include <melee/cm/types.h>
#include <melee/ft/types.h>
#include <melee/gm/types.h>
#include <melee/gr/types.h>
#include <melee/it/types.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/lb/types.h>
#include <melee/mn/types.h>
#include <melee/pl/player.h>
#include <dolphin/gx/GXTexture.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/tobj.h>

#include <math.h>
#include <string.h>

enum {
    SEMANTIC_JOINT_LIMIT = 4096,
    SEMANTIC_JOINT_DEPTH_LIMIT = 256,
    SEMANTIC_CAMERA_SUBJECT_LIMIT = 64,
};

static uint32_t float_bits(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static uint64_t add_float(uint64_t hash, float value)
{
    return nativeSemanticState_AddU32(hash, float_bits(value));
}

static uint64_t add_vec2(uint64_t hash, const Vec2* value)
{
    hash = add_float(hash, value->x);
    return add_float(hash, value->y);
}

static uint64_t add_vec3(uint64_t hash, const Vec3* value)
{
    hash = add_float(hash, value->x);
    hash = add_float(hash, value->y);
    return add_float(hash, value->z);
}

// Parity observes the state actually read by each backend. Interpreted articles
// do not populate the stock item union; comparing that dead scratch storage
// would either report allocator residue or require behavior to write fake data.
// This is a read-only observation of the accepted template's declaration, not
// a script API or an alias from custom state back into the stock union.
static uint32_t item_script_int(const Item* item, const char* name)
{
    uint32_t generation;
    NativeScriptReceiverView view;
    if (!item->entity ||
        !nativeCallbackSource_Snapshot(item->entity, NativeCallbackSource_Item,
                                      &generation) ||
        !nativeScriptItem_View(item->entity, generation, &view) ||
        !view.program || !view.storage) {
        return UINT32_MAX;
    }
    const NativeScriptProgramIndex* program = view.program;
    for (uint32_t i = 0; i < program->value_count; ++i) {
        const NativeScriptValueDeclaration* field = &program->values[i];
        if (field->owner != view.storage->owner || field->parameter ||
            field->array || field->type != NativeScriptType_I32 ||
            !nativeScriptSource_SpanEquals(program->tree, field->name, name)) {
            continue;
        }
        NativeScriptValue value;
        NativeScriptDiagnostic diagnostic;
        if (nativeScriptStorage_Read(view.storage,
                                     NATIVE_SCRIPT_STATE_SEMANTIC | i,
                                     NULL, 0, &value, &diagnostic)) {
            return (uint32_t) value.as.i64;
        }
        return UINT32_MAX;
    }
    // Missing/retired/mistyped source state must not fall back to stale native
    // scratch. Chef's stock domain is 0..4, so this remains a visible mismatch.
    return UINT32_MAX;
}

static uint64_t add_camera_transform(uint64_t hash,
                                     const CameraTransformState* transform)
{
    hash = add_vec3(hash, &transform->interest);
    hash = add_vec3(hash, &transform->target_interest);
    hash = add_vec3(hash, &transform->position);
    hash = add_vec3(hash, &transform->target_position);
    hash = add_float(hash, transform->fov);
    return add_float(hash, transform->target_fov);
}

static uint64_t add_camera_subject(uint64_t hash, const CmSubject* subject)
{
    hash = nativeSemanticState_AddU32(hash, (uint32_t) subject->state);
    hash = nativeSemanticState_AddU32(hash, subject->on_ledge);
    hash = nativeSemanticState_AddU32(hash, subject->force_inactive);
    hash = nativeSemanticState_AddU32(hash, subject->was_framed);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) subject->state_timer);
    hash = add_vec3(hash, &subject->pos);
    hash = add_vec3(hash, &subject->bone_pos);
    hash = add_float(hash, subject->facing_dir);
    hash = add_vec2(hash, &subject->ext.h);
    hash = add_vec3(hash, &subject->ext.v);
    hash = add_vec2(hash, &subject->target_ext.h);
    hash = add_vec3(hash, &subject->target_ext.v);
    hash = add_vec3(hash, &subject->x54);
    return add_vec3(hash, &subject->x60);
}

static uint64_t add_ft_ecb(uint64_t hash, const ftECB* ecb)
{
    hash = add_vec2(hash, &ecb->top);
    hash = add_vec2(hash, &ecb->bottom);
    hash = add_vec2(hash, &ecb->right);
    return add_vec2(hash, &ecb->left);
}

static uint64_t add_ft_collision_box(uint64_t hash,
                                     const ftCollisionBox* box)
{
    hash = add_float(hash, box->top);
    hash = add_float(hash, box->bottom);
    hash = add_vec2(hash, &box->left);
    return add_vec2(hash, &box->right);
}

static uint64_t add_surface(uint64_t hash, const SurfaceData* surface)
{
    hash = nativeSemanticState_AddU32(hash, (uint32_t) surface->index);
    hash = nativeSemanticState_AddU32(hash, surface->flags);
    return add_vec3(hash, &surface->normal);
}

static uint64_t add_collision(uint64_t hash, const CollData* collision)
{
    hash = add_vec3(hash, &collision->cur_pos);
    hash = add_vec3(hash, &collision->prev_pos);
    hash = add_vec3(hash, &collision->last_pos);
    hash = add_vec3(hash, &collision->x28_vec);
    hash = nativeSemanticState_AddU32(hash, collision->x34_flags.b0);
    hash = nativeSemanticState_AddU32(hash, collision->x34_flags.b1234);
    hash = nativeSemanticState_AddU32(hash, collision->x34_flags.b5);
    hash = nativeSemanticState_AddU32(hash, collision->x34_flags.b6);
    hash = nativeSemanticState_AddU32(hash, collision->x34_flags.b7);
    hash = nativeSemanticState_AddU32(hash, collision->x35_flags.b0);
    hash = nativeSemanticState_AddU32(hash, collision->x35_flags.b1234);
    hash = nativeSemanticState_AddU32(hash, collision->x35_flags.b5);
    hash = nativeSemanticState_AddU32(hash, collision->x35_flags.b6);
    hash = nativeSemanticState_AddU32(hash, collision->x35_flags.b7);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) collision->facing_dir);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) collision->floor_skip);
    hash = nativeSemanticState_AddU32(hash,
                                      (uint32_t) collision->ledge_id_right);
    hash = nativeSemanticState_AddU32(hash,
                                      (uint32_t) collision->ledge_id_left);
    hash = nativeSemanticState_AddU32(hash,
                                      (uint32_t) collision->joint_id_skip);
    hash = nativeSemanticState_AddU32(hash,
                                      (uint32_t) collision->joint_id_only);
    hash = add_ft_ecb(hash, &collision->x64_ecb);
    hash = add_ft_ecb(hash, &collision->desired_ecb);
    hash = add_ft_ecb(hash, &collision->ecb);
    hash = add_ft_ecb(hash, &collision->prev_ecb);
    hash = nativeSemanticState_AddU32(hash, collision->x130_flags);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) collision->env_flags);
    hash = nativeSemanticState_AddU32(hash,
                                      (uint32_t) collision->prev_env_flags);
    // mpColl uses contact as a query output, then consumes it within that
    // collision call. The next call writes it before reading it. At an idle
    // tick boundary this can still contain a previous heap occupant's result.
    // The owned record snapshot retains it; it is not semantic carry state.
    hash = add_surface(hash, &collision->floor);
    hash = add_surface(hash, &collision->left_facing_wall);
    hash = add_surface(hash, &collision->right_facing_wall);
    return add_surface(hash, &collision->ceiling);
}

static uint64_t add_hit_capsule(uint64_t hash, const HitCapsule* hit)
{
    hash = nativeSemanticState_AddU32(hash, (uint32_t) hit->state);
    hash = nativeSemanticState_AddU32(hash, hit->x4);
    hash = nativeSemanticState_AddU32(hash, hit->unk_count);
    hash = add_float(hash, hit->damage);
    hash = add_vec3(hash, &hit->b_offset);
    hash = add_float(hash, hit->scale);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) hit->kb_angle);
    hash = nativeSemanticState_AddU32(hash, hit->x24);
    hash = nativeSemanticState_AddU32(hash, hit->x28);
    hash = nativeSemanticState_AddU32(hash, hit->x2C);
    hash = nativeSemanticState_AddU32(hash, hit->element);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) hit->x34);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) hit->sfx_severity);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) hit->sfx_kind);
    hash = nativeSemanticState_AddU32(hash, hit->x40_b0);
    hash = nativeSemanticState_AddU32(hash, hit->x40_b1);
    hash = nativeSemanticState_AddU32(hash, hit->x40_b2);
    hash = nativeSemanticState_AddU32(hash, hit->x40_b3);
    hash = nativeSemanticState_AddU32(hash, hit->x40_b4);
    hash = nativeSemanticState_AddU32(hash, hit->x41_b4);
    hash = nativeSemanticState_AddU32(hash, hit->x41_b5);
    hash = nativeSemanticState_AddU32(hash, hit->x41_b6);
    hash = nativeSemanticState_AddU32(hash, hit->x41_b7);
    hash = nativeSemanticState_AddU32(hash, hit->x42_b0);
    hash = nativeSemanticState_AddU32(hash, hit->x42_b1);
    hash = nativeSemanticState_AddU32(hash, hit->x42_b2);
    hash = nativeSemanticState_AddU32(hash, hit->x42_b3);
    hash = nativeSemanticState_AddU32(hash, hit->x42_b4);
    hash = nativeSemanticState_AddU32(hash, hit->x42_b5);
    hash = nativeSemanticState_AddU32(hash, hit->x42_b6);
    hash = nativeSemanticState_AddU32(hash, hit->x42_b7);
    hash = nativeSemanticState_AddU32(hash, hit->x43_b0);
    hash = nativeSemanticState_AddU32(hash, hit->x43_b1);
    hash = nativeSemanticState_AddU32(hash, hit->x43_b2);
    hash = nativeSemanticState_AddU32(hash, hit->x43_b3);
    hash = nativeSemanticState_AddU32(hash, hit->x43_b4);
    hash = nativeSemanticState_AddU32(hash, hit->x43_b5);
    hash = nativeSemanticState_AddU32(hash, hit->x43_b6);
    hash = nativeSemanticState_AddU32(hash, hit->x43_b7);
    hash = nativeSemanticState_AddU32(hash, hit->x44);
    hash = nativeSemanticState_AddU32(hash, hit->x45);
    hash = add_vec3(hash, &hit->x4C);
    hash = add_vec3(hash, &hit->x58);
    // lbColl writes these intersection outputs before ftColl/itColl consumes
    // them on a successful query. A newly enabled hitbox need not initialize
    // the previous result. Keep actual current/previous capsule positions above.
    for (uint32_t i = 0; i < 12; ++i) {
        // lbColl_80008440 clears victim pointers, not counters. Admission and
        // lbColl_80008A5C read a counter only for a nonempty victim entry.
        hash = nativeSemanticState_AddU32(hash,
            hit->victims_1[i].victim ? hit->victims_1[i].x4 : 0);
        hash = nativeSemanticState_AddU32(hash,
            hit->victims_2[i].victim ? hit->victims_2[i].x4 : 0);
    }
    return hash;
}

static uint64_t add_hurt_capsule(uint64_t hash,
                                 const FighterHurtCapsule* hurt)
{
    hash = nativeSemanticState_AddU32(hash,
                                      (uint32_t) hurt->capsule.state);
    hash = add_vec3(hash, &hurt->capsule.a_offset);
    hash = add_vec3(hash, &hurt->capsule.b_offset);
    hash = add_float(hash, hurt->capsule.scale);
    hash = nativeSemanticState_AddU32(hash, hurt->capsule.skip_update_pos);
    hash = add_vec3(hash, &hurt->capsule.a_pos);
    hash = add_vec3(hash, &hurt->capsule.b_pos);
    hash = nativeSemanticState_AddU32(hash,
                                      (uint32_t) hurt->capsule.bone_idx);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) hurt->height);
    return nativeSemanticState_AddU32(hash, hurt->is_grabbable);
}

static uint64_t add_hitbox_lifetime(uint64_t hash,
                                    const NativeHitboxLifetime* hit)
{
    hash = add_float(hash, hit->remaining);
    hash = nativeSemanticState_AddU32(hash, hit->armed);
    hash = nativeSemanticState_AddU32(hash, hit->expire_all);
    hash = nativeSemanticState_AddU32(hash, hit->exclude_grabbed);
    hash = nativeSemanticState_AddU32(hash, hit->capsule);
    if (hit->capsule) {
        hash = add_vec3(hash, &hit->end_offset);
        hash = add_vec3(hash, &hit->previous_start);
    }
    hash = nativeSemanticState_AddU32(hash, hit->launch.mode);
    if (hit->launch.mode == NativeHitboxLaunch_Target) {
        hash = add_float(hash, hit->launch.target.forward);
        hash = add_float(hash, hit->launch.target.up);
        hash = add_float(hash, hit->launch.target.frames);
    } else if (hit->launch.mode == NativeHitboxLaunch_Follow) {
        hash = add_float(hash, hit->launch.follow.momentum);
        hash = add_float(hash, hit->launch.follow.attraction);
        hash = add_float(hash, hit->launch.follow.min_up);
        hash = add_float(hash, hit->launch.follow.ground_angle);
        hash = nativeSemanticState_AddU32(hash, hit->launch.follow.override_ground_knockback);
        if (hit->launch.follow.override_ground_knockback) {
            hash = nativeSemanticState_AddU32(hash, hit->launch.follow.ground_growth);
            hash = nativeSemanticState_AddU32(hash, hit->launch.follow.ground_base);
        }
    }
    return hash;
}

uint64_t nativeSemanticState_FighterHitboxDigest(const Fighter* fighter)
{
    uint64_t hash = nativeSemanticState_Begin();
    for (uint32_t i = 0; i < 4; ++i) {
        hash = add_hit_capsule(hash, &fighter->x914[i]);
        hash = add_hitbox_lifetime(hash, &fighter->native_hitbox_lifetimes[i]);
    }
    for (uint32_t i = 0; i < 2; ++i) {
        hash = add_hit_capsule(hash, &fighter->xDF4[i]);
    }
    return add_hit_capsule(hash, &fighter->x1064_thrownHitbox);
}

uint64_t nativeSemanticState_FighterHurtboxDigest(const Fighter* fighter)
{
    uint64_t hash = nativeSemanticState_AddU32(
        nativeSemanticState_Begin(), fighter->hurt_capsules_len);
    uint32_t count = fighter->hurt_capsules_len;
    if (count > 15) count = 15;
    for (uint32_t i = 0; i < count; ++i) {
        hash = add_hurt_capsule(hash, &fighter->hurt_capsules[i]);
    }
    return hash;
}

static uint64_t add_fighter_common_motion_state(uint64_t hash,
                                                const Fighter* fighter)
{
    switch (fighter->motion_id) {
    case ftCo_MS_WalkSlow:
    case ftCo_MS_WalkMiddle:
    case ftCo_MS_WalkFast:
        hash = add_float(hash, fighter->mv.co.walk.x0);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.walk.msid);
        hash = add_float(hash, fighter->mv.co.walk.slow_anim_frame);
        hash = add_float(hash, fighter->mv.co.walk.middle_anim_frame);
        hash = add_float(hash, fighter->mv.co.walk.fast_anim_frame);
        hash = add_float(hash, fighter->mv.co.walk.slow_anim_rate);
        hash = add_float(hash, fighter->mv.co.walk.middle_anim_rate);
        hash = add_float(hash, fighter->mv.co.walk.fast_anim_rate);
        return add_float(hash, fighter->mv.co.walk.accel_mul);
    case ftCo_MS_Turn:
        hash = nativeSemanticState_AddU32(hash,
                                          fighter->mv.co.turn.has_turned);
        hash = add_float(hash, fighter->mv.co.turn.facing_after);
        hash = add_float(hash, fighter->mv.co.turn.x8);
        hash = add_float(hash, fighter->mv.co.turn.frames_to_turn);
        hash = nativeSemanticState_AddU32(hash, fighter->mv.co.turn.buffered_dash);
        hash = nativeSemanticState_AddU32(hash,
                                          fighter->mv.co.turn.just_turned);
        return nativeSemanticState_AddU32(hash,
                                          fighter->mv.co.turn.x1C);
    case ftCo_MS_TurnRun:
        hash = add_float(hash, fighter->mv.co.turnrun.accel_mul);
        return nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.turnrun.x14);
    case ftCo_MS_Dash:
        hash = add_float(hash, fighter->mv.co.dash.x0);
        return nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.dash.x4);
    case ftCo_MS_Run:
    case ftCo_MS_RunDirect:
        hash = add_float(hash, fighter->mv.co.run.x0);
        return add_float(hash, fighter->mv.co.run.x4);
    case ftCo_MS_RunBrake:
        hash = nativeSemanticState_AddU32(hash,
                                          fighter->mv.co.runbrake.x0);
        return add_float(hash, fighter->mv.co.runbrake.frames);
    case ftCo_MS_KneeBend:
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.kneebend.is_short_hop);
        return nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.kneebend.jump_input);
    case ftCo_MS_JumpF:
    case ftCo_MS_JumpB:
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.jump.x0);
        hash = nativeSemanticState_AddU32(hash, fighter->mv.co.jump.x4);
        return add_float(hash, fighter->mv.co.jump.jump_mul);
    case ftCo_MS_JumpAerialF:
    case ftCo_MS_JumpAerialB:
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.jumpaerial.x0);
        return add_float(hash, fighter->mv.co.jumpaerial.init_h_vel);
    case ftCo_MS_Fall:
    case ftCo_MS_FallF:
    case ftCo_MS_FallB:
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.fall.smid);
        return add_float(hash, fighter->mv.co.fall.x4);
    case ftCo_MS_FallAerial:
    case ftCo_MS_FallAerialF:
    case ftCo_MS_FallAerialB:
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.fallaerial.smid);
        return add_float(hash, fighter->mv.co.fallaerial.x4);
    case ftCo_MS_FallSpecial:
    case ftCo_MS_FallSpecialF:
    case ftCo_MS_FallSpecialB:
        hash = nativeSemanticState_AddU32(hash,
                                          fighter->mv.co.fallspecial.x0);
        hash = add_float(hash, fighter->mv.co.fallspecial.x4);
        hash = add_float(hash, fighter->mv.co.fallspecial.mobility);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.fallspecial.xC);
        hash = nativeSemanticState_AddU32(hash,
                                          fighter->mv.co.fallspecial.x10);
        hash = add_float(hash, fighter->mv.co.fallspecial.landing_lag);
        return nativeSemanticState_AddU32(
            hash, fighter->mv.co.fallspecial.allow_interrupt);
    case ftCo_MS_Squat:
    case ftCo_MS_SquatWait:
    case ftCo_MS_SquatRv:
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.squat.x0);
        return add_float(hash, fighter->mv.co.squat.x4);
    case ftCo_MS_Landing:
    case ftCo_MS_LandingFallSpecial:
        return nativeSemanticState_AddU32(
            hash, fighter->mv.co.landing.allow_interrupt);
    case ftCo_MS_Attack11:
    case ftCo_MS_Attack12:
    case ftCo_MS_Attack13:
        return nativeSemanticState_AddU32(hash,
                                          fighter->mv.co.attack1.x0);
    case ftCo_MS_Attack100Start:
    case ftCo_MS_Attack100Loop:
        hash = nativeSemanticState_AddU32(hash,
                                          fighter->mv.co.attack100.x0);
        return nativeSemanticState_AddU32(hash,
                                          fighter->mv.co.attack100.x4);
    case ftCo_MS_AttackDash:
        return nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.attackdash.x0);
    case ftCo_MS_AttackLw3:
        return nativeSemanticState_AddU32(hash,
                                          fighter->mv.co.attacklw3.x0);
    case ftCo_MS_DamageHi1:
    case ftCo_MS_DamageHi2:
    case ftCo_MS_DamageHi3:
    case ftCo_MS_DamageN1:
    case ftCo_MS_DamageN2:
    case ftCo_MS_DamageN3:
    case ftCo_MS_DamageLw1:
    case ftCo_MS_DamageLw2:
    case ftCo_MS_DamageLw3:
    case ftCo_MS_DamageAir1:
    case ftCo_MS_DamageAir2:
    case ftCo_MS_DamageAir3:
    case ftCo_MS_DamageFlyHi:
    case ftCo_MS_DamageFlyN:
    case ftCo_MS_DamageFlyLw:
    case ftCo_MS_DamageFlyTop:
    case ftCo_MS_DamageFlyRoll:
        hash = add_float(hash, fighter->mv.co.damage.x0);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.damage.x4);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.damage.x8);
        hash = add_float(hash, fighter->mv.co.damage.x14);
        hash = nativeSemanticState_AddU32(hash,
                                          fighter->mv.co.damage.x18);
        hash = nativeSemanticState_AddU32(hash,
                                          fighter->mv.co.damage.x19);
        hash = nativeSemanticState_AddU32(hash,
                                          fighter->mv.co.damage.x1A);
        return nativeSemanticState_AddU32(hash,
                                          fighter->mv.co.damage.x1B);
    case ftCo_MS_LightGet:
    case ftCo_MS_HeavyGet:
        return nativeSemanticState_AddU32(hash,
                                          fighter->mv.co.itemget.x0);
    case ftCo_MS_LightThrowF:
    case ftCo_MS_LightThrowB:
    case ftCo_MS_LightThrowHi:
    case ftCo_MS_LightThrowLw:
    case ftCo_MS_LightThrowDash:
    case ftCo_MS_LightThrowDrop:
    case ftCo_MS_LightThrowAirF:
    case ftCo_MS_LightThrowAirB:
    case ftCo_MS_LightThrowAirHi:
    case ftCo_MS_LightThrowAirLw:
    case ftCo_MS_HeavyThrowF:
    case ftCo_MS_HeavyThrowB:
    case ftCo_MS_HeavyThrowHi:
    case ftCo_MS_HeavyThrowLw:
    case ftCo_MS_LightThrowF4:
    case ftCo_MS_LightThrowB4:
    case ftCo_MS_LightThrowHi4:
    case ftCo_MS_LightThrowLw4:
    case ftCo_MS_LightThrowAirF4:
    case ftCo_MS_LightThrowAirB4:
    case ftCo_MS_LightThrowAirHi4:
    case ftCo_MS_LightThrowAirLw4:
    case ftCo_MS_HeavyThrowF4:
    case ftCo_MS_HeavyThrowB4:
    case ftCo_MS_HeavyThrowHi4:
    case ftCo_MS_HeavyThrowLw4:
        hash = add_float(hash, fighter->mv.co.itemthrow.facing_dir);
        hash = add_float(hash, fighter->mv.co.itemthrow4.anim_spd);
        return add_vec3(hash, &fighter->mv.co.itemthrow4.x8);
    case ftCo_MS_ItemParasolOpen:
        return nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.parasol_open.prev_msid);
    case ftCo_MS_LiftWait:
    case ftCo_MS_LiftWalk1:
    case ftCo_MS_LiftWalk2:
    case ftCo_MS_LiftTurn:
        hash = nativeSemanticState_AddU32(hash,
                                          fighter->mv.co.lift.x0);
        hash = add_float(hash, fighter->mv.co.lift.x4);
        return nativeSemanticState_AddU32(hash,
                                          fighter->mv.co.lift.x8);
    case ftCo_MS_GuardOn:
    case ftCo_MS_Guard:
    case ftCo_MS_GuardOff:
    case ftCo_MS_GuardSetOff:
    case ftCo_MS_GuardReflect:
        hash = add_float(hash, fighter->mv.co.guard.x0);
        hash = add_float(hash, fighter->mv.co.guard.x4);
        hash = add_float(hash, fighter->mv.co.guard.x8);
        hash = nativeSemanticState_AddU32(hash,
                                          fighter->mv.co.guard.xC);
        hash = add_float(hash, fighter->mv.co.guard.x10);
        hash = add_float(hash, fighter->mv.co.guard.x14);
        hash = add_float(hash, fighter->mv.co.guard.x18);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.guard.x1C);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.guard.x20);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.guard.x24);
        return add_float(hash, fighter->mv.co.guard.x2C);
    case ftCo_MS_Catch:
    case ftCo_MS_CatchDash:
        hash = add_float(hash, fighter->mv.co.catch.x0);
        return nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.guard.x24);
    case ftCo_MS_ThrowF:
    case ftCo_MS_ThrowB:
    case ftCo_MS_ThrowHi:
    case ftCo_MS_ThrowLw:
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.throw.x4);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.throw.x8);
        return add_vec3(hash, &fighter->mv.co.throw.xC);
    case ftCo_MS_CaptureWaitHi:
    case ftCo_MS_CaptureDamageHi:
    case ftCo_MS_CaptureWaitLw:
    case ftCo_MS_CaptureDamageLw:
        hash = add_float(hash, fighter->mv.co.capturewait.x0);
        hash = add_float(hash, fighter->mv.co.capturewait.x4);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.capturewait.x8);
        return nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.capturewait.xC);
    case ftCo_MS_ThrownF:
    case ftCo_MS_ThrownB:
    case ftCo_MS_ThrownHi:
    case ftCo_MS_ThrownLw:
    case ftCo_MS_ThrownlwWomen:
    case ftCo_MS_ThrownFF:
    case ftCo_MS_ThrownFB:
    case ftCo_MS_ThrownFHi:
    case ftCo_MS_ThrownFLw:
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.thrown.unk_bool);
        hash = add_float(hash, fighter->mv.co.thrown.anim_timer);
        return nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.thrown.xC);
    case ftCo_MS_ShoulderedWait:
    case ftCo_MS_ShoulderedWalkSlow:
    case ftCo_MS_ShoulderedWalkMiddle:
    case ftCo_MS_ShoulderedWalkFast:
    case ftCo_MS_ShoulderedTurn:
        hash = add_float(hash, fighter->mv.co.shouldered.x0);
        return nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.shouldered.x4);
    case ftCo_MS_YoshiEgg:
    case ftCo_MS_KirbyYoshiEgg:
    {
        const NativeCaptureEnclosureState* state = &fighter->capture_enclosure;
        const NativeCaptureEnclosureSettings* settings = &state->settings;
        hash = add_vec3(hash, &settings->launch_velocity);
        hash = add_vec3(hash, &settings->escape_velocity);
        hash = add_float(hash, settings->damage_behavior);
        hash = add_float(hash, settings->shrink_fraction);
        hash = add_float(hash, settings->shrink_frames);
        hash = add_float(hash, settings->lifetime);
        hash = add_float(hash, settings->damage_time_scale);
        hash = add_float(hash, settings->timer_decay);
        hash = add_float(hash, settings->mash_reduction);
        hash = add_float(hash, settings->mash_pause_frames);
        hash = add_float(hash, settings->mash_animation_rate);
        hash = nativeSemanticState_AddU32(hash, (uint32_t) settings->model_slot);
        hash = nativeSemanticState_AddU32(hash, settings->escape_invincibility_frames);
        hash = nativeSemanticState_AddU32(hash, state->restore_model_on_escape);
        hash = nativeSemanticState_AddU32(hash, state->subtract_damage_form);
        hash = nativeSemanticState_AddU32(
            hash, state->mashed);
        hash = add_float(hash, state->mash_pause_remaining);
        hash = add_float(hash, state->shrink_remaining);
        hash = add_vec3(hash, &state->root_scale);
        return add_vec3(hash, &state->accessory_scale);
    }
    case ftCo_MS_CaptureKoopa:
    case ftCo_MS_CaptureDamageKoopa:
    case ftCo_MS_CaptureWaitKoopa:
    case ftCo_MS_ThrownKoopaF:
    case ftCo_MS_ThrownKoopaB:
    case ftCo_MS_CaptureKoopaAir:
    case ftCo_MS_CaptureDamageKoopaAir:
    case ftCo_MS_CaptureWaitKoopaAir:
    case ftCo_MS_ThrownKoopaAirF:
    case ftCo_MS_ThrownKoopaAirB:
    case ftCo_MS_CaptureMewtwo:
    case ftCo_MS_CaptureMewtwoAir:
    case ftCo_MS_ThrownMewtwo:
    case ftCo_MS_ThrownMewtwoAir:
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.capturekoopa.x0);
        hash = add_float(hash, fighter->mv.co.capturekoopa.x8);
        hash = add_float(hash, fighter->mv.co.capturekoopa.xC);
        return add_float(hash, fighter->mv.co.capturekoopa.x10);
    case ftCo_MS_CaptureKirby:
    case ftCo_MS_CaptureWaitKirby:
        hash = add_vec2(hash, &fighter->mv.co.capturekirby.pos_offset);
        hash = add_vec2(hash, &fighter->mv.co.capturekirby.x8);
        hash = add_vec2(hash, &fighter->mv.co.capturekirby.x10);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.capturekirby.x18);
        return add_vec3(hash, &fighter->mv.co.capturekirby.scale);
    case ftCo_MS_ThrownKirbyStar:
    case ftCo_MS_ThrownCopyStar:
    case ftCo_MS_ThrownKirby:
        hash = add_float(hash, fighter->mv.co.thrownkirby.x4);
        hash = add_float(hash, fighter->mv.co.thrownkirby.x8);
        hash = add_float(hash, fighter->mv.co.thrownkirby.xC);
        hash = add_float(hash, fighter->mv.co.thrownkirby.x10);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.thrownkirby.x14);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.thrownkirby.x18);
        hash = add_vec3(hash, &fighter->mv.co.thrownkirby.scale);
        return add_ft_collision_box(
            hash, &fighter->mv.co.thrownkirby.coll_box);
    case ftCo_MS_Bury:
    case ftCo_MS_BuryWait:
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.bury.x0);
        hash = add_ft_collision_box(hash, &fighter->mv.co.bury.coll_box);
        hash = add_float(hash, fighter->mv.co.bury.x1C);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.bury.x20);
        return add_vec3(hash, &fighter->mv.co.bury.translate);
    case ftCo_MS_CaptureJump:
    case ftCo_MS_BuryJump:
        return add_float(hash, fighter->mv.co.buryjump.x0);
    case ftCo_MS_WarpStarJump:
    case ftCo_MS_WarpStarFall:
        hash = add_vec3(hash, &fighter->mv.co.warpstar.cur_pos);
        hash = add_vec3(hash, &fighter->mv.co.warpstar.self_vel);
        hash = add_float(hash, fighter->mv.co.warpstar.facing_dir);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.warpstar.x1C);
        return add_ft_collision_box(hash, &fighter->mv.co.warpstar.ecb);
    case ftCo_MS_HammerKneeBend:
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.hammerkneebend.x0);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.hammerkneebend.x4);
        return add_float(hash, fighter->mv.co.hammerkneebend.x8);
    case ftCo_MS_HammerLanding:
        return add_float(hash, fighter->mv.co.hammerlanding.x4);
    case ftCo_MS_KinokoGiantStart:
    case ftCo_MS_KinokoGiantStartAir:
    case ftCo_MS_KinokoGiantEnd:
    case ftCo_MS_KinokoGiantEndAir:
    case ftCo_MS_KinokoSmallStart:
    case ftCo_MS_KinokoSmallStartAir:
    case ftCo_MS_KinokoSmallEnd:
    case ftCo_MS_KinokoSmallEndAir:
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.mushroom.clear_velocity);
        hash = add_float(hash, fighter->mv.co.mushroom.start_scale);
        hash = add_float(hash, fighter->mv.co.mushroom.target_scale);
        hash = add_float(hash, fighter->mv.co.mushroom.ground_accel_1);
        hash = add_float(hash, fighter->mv.co.mushroom.ground_accel_2);
        hash = add_float(hash, fighter->mv.co.mushroom.ground_velocity);
        hash = add_float(hash, fighter->mv.co.mushroom.ground_knockback);
        hash = add_float(hash, fighter->mv.co.mushroom.ground_shield_knockback);
        hash = add_vec3(hash, &fighter->mv.co.mushroom.self_accel);
        hash = add_vec3(hash, &fighter->mv.co.mushroom.self_velocity);
        hash = add_vec3(hash, &fighter->mv.co.mushroom.knockback);
        hash = add_vec3(hash, &fighter->mv.co.mushroom.shield_knockback);
        return add_vec3(hash, &fighter->mv.co.mushroom.other_velocity);
    case ftCo_MS_Entry:
    case ftCo_MS_EntryStart:
    case ftCo_MS_EntryEnd:
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.entry.timer);
        hash = nativeSemanticState_AddU32(hash, fighter->mv.co.entry.authored);
        hash = add_float(hash, fighter->mv.co.entry.x4);
        hash = add_vec3(hash, &fighter->mv.co.entry.x8);
        hash = add_vec3(hash, &fighter->mv.co.entry.x14);
        hash = add_float(hash, fighter->mv.co.entry.x20);
        hash = add_float(hash, fighter->mv.co.entry.x24);
        hash = add_float(hash, fighter->mv.co.entry.x28);
        return add_ft_collision_box(hash, &fighter->mv.co.entry.x2C);
    case ftCo_MS_DamageIce:
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.damageice.wall_hit_dir);
        hash = add_float(hash, fighter->mv.co.damageice.rot_speed);
        return add_ft_collision_box(hash, &fighter->mv.co.damageice.ice_coll);
    case ftCo_MS_DamageIceJump:
        return add_float(hash, fighter->mv.co.damageicejump.escape_timer);
    case ftCo_MS_CaptureLikelike:
        return nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.capturelikelike.x4);
    case ftCo_MS_Barrel:
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.barrel.x0);
        return nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.barrel.x4);
    case ftCo_MS_DownBoundU:
    case ftCo_MS_DownBoundD:
    case ftCo_MS_DownSpotU:
    case ftCo_MS_DownSpotD:
        return nativeSemanticState_AddU32(hash,
                                          fighter->mv.co.downspot.x4);
    case ftCo_MS_DownWaitU:
    case ftCo_MS_DownWaitD:
        return add_float(hash, fighter->mv.co.downwait.x0);
    case ftCo_MS_DownDamageU:
    case ftCo_MS_DownDamageD:
        return add_float(hash, fighter->mv.co.downdamage.x0);
    case ftCo_MS_PassiveWall:
    case ftCo_MS_PassiveWallJump:
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.passivewall.timer);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.passivewall.x4);
        hash = nativeSemanticState_AddU32(hash,
                                          fighter->mv.co.passivewall.x8);
        return nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.passivewall.vel_y_exponent);
    case ftCo_MS_EscapeF:
    case ftCo_MS_EscapeB:
    case ftCo_MS_EscapeN:
        hash = nativeSemanticState_AddU32(hash,
                                          fighter->mv.co.escape.x0);
        return nativeSemanticState_AddU32(hash,
                                          fighter->mv.co.escape.x4);
    case ftCo_MS_EscapeAir:
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.escapeair.timer);
        return add_vec3(hash, &fighter->mv.co.escapeair.self_vel);
    case ftCo_MS_ReboundStop:
    case ftCo_MS_Rebound:
        hash = add_float(hash, fighter->mv.co.rebound.x0);
        return add_float(hash, fighter->mv.co.rebound.anim_speed);
    case ftCo_MS_Pass:
        hash = nativeSemanticState_AddU32(hash,
                                          fighter->mv.co.pass.x0);
        return add_float(hash, fighter->mv.co.pass.x4);
    case ftCo_MS_CliffCatch:
    case ftCo_MS_CliffWait:
    case ftCo_MS_CliffClimbSlow:
    case ftCo_MS_CliffClimbQuick:
    case ftCo_MS_CliffAttackSlow:
    case ftCo_MS_CliffAttackQuick:
    case ftCo_MS_CliffEscapeSlow:
    case ftCo_MS_CliffEscapeQuick:
    case ftCo_MS_CliffJumpSlow1:
    case ftCo_MS_CliffJumpQuick1:
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->mv.co.cliff.ledge_id);
        hash = add_float(hash, fighter->mv.co.cliff.x4);
        return nativeSemanticState_AddU32(hash,
                                          fighter->mv.co.cliff.x8);
    case ftCo_MS_CliffJumpSlow2:
    case ftCo_MS_CliffJumpQuick2:
        return nativeSemanticState_AddU32(
            hash, fighter->mv.co.cliffjump.x0);
    default:
        return hash;
    }
}

static uint64_t add_fighter_kind_state(uint64_t hash, const Fighter* fighter)
{
    FighterKind kind = fighter->kind >= Ft_Kind_CustomFirst
                           ? fighter->behavior_kind
                           : fighter->kind;
    switch (kind) {
    case Ft_Kind_Captain:
        hash = nativeSemanticState_AddU32(
            hash, fighter->u.ca.during_specials_start);
        return nativeSemanticState_AddU32(hash,
                                          fighter->u.ca.during_specials);
    case Ft_Kind_Ganon:
        hash = nativeSemanticState_AddU32(
            hash, fighter->u.gn.during_specials_start);
        return nativeSemanticState_AddU32(hash,
                                          fighter->u.gn.during_specials);
    case Ft_Kind_Donkey:
        hash = nativeSemanticState_AddU32(hash,
                                          (uint32_t) fighter->u.dk.x222C);
        return nativeSemanticState_AddU32(hash,
                                          (uint32_t) fighter->u.dk.x2230);
    case Ft_Kind_GameWatch:
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->u.gw.x222C_judgeVar1);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->u.gw.x2230_judgeVar2);
        hash = nativeSemanticState_AddU32(hash, fighter->u.gw.x2234);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->u.gw.x2238_panicCharge);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->u.gw.x223C_panicDamage);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->u.gw.x2240_chefVar1);
        return nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->u.gw.x2244_chefVar2);
    case Ft_Kind_Kirby:
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->u.kb.hat.x4);
        hash = nativeSemanticState_AddU32(hash, fighter->u.kb.hat.x8_b0);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->u.kb.hat.kind);
        hash = nativeSemanticState_AddU32(hash,
                                          (uint32_t) fighter->u.kb.x60);
        hash = nativeSemanticState_AddU32(hash, fighter->u.kb.x64);
        hash = nativeSemanticState_AddU32(hash,
                                          (uint32_t) fighter->u.kb.x68);
        hash = nativeSemanticState_AddU32(hash,
                                          (uint32_t) fighter->u.kb.x6C);
        hash = nativeSemanticState_AddU32(hash,
                                          (uint32_t) fighter->u.kb.x80);
        hash = add_float(hash, fighter->u.kb.x84);
        hash = add_float(hash, fighter->u.kb.x88);
        hash = add_vec3(hash, &fighter->u.kb.x8C);
        hash = nativeSemanticState_AddU32(hash,
                                          (uint32_t) fighter->u.kb.x9C);
        hash = nativeSemanticState_AddU32(hash,
                                          (uint32_t) fighter->u.kb.xA8);
        hash = nativeSemanticState_AddU32(hash,
                                          (uint32_t) fighter->u.kb.xB4);
        hash = nativeSemanticState_AddU32(hash,
                                          (uint32_t) fighter->u.kb.xBC);
        hash = nativeSemanticState_AddU32(hash, fighter->u.kb.xC4);
        hash = add_float(hash, fighter->u.kb.xC8);
        hash = nativeSemanticState_AddU32(hash, fighter->u.kb.xCC);
        hash = nativeSemanticState_AddU32(hash,
                                          (uint32_t) fighter->u.kb.xD4);
        hash = nativeSemanticState_AddU32(hash,
                                          (uint32_t) fighter->u.kb.xD8);
        hash = nativeSemanticState_AddU32(hash,
                                          (uint32_t) fighter->u.kb.xE0);
        hash = nativeSemanticState_AddU32(hash,
                                          (uint32_t) fighter->u.kb.xE4);
        hash = add_float(hash, fighter->u.kb.xE8);
        hash = add_float(hash, fighter->u.kb.xEC);
        hash = add_float(hash, fighter->u.kb.xF0);
        return nativeSemanticState_AddU32(hash, fighter->u.kb.xF4_b0);
    case Ft_Kind_Koopa:
        hash = add_float(hash, fighter->u.kp.x222C);
        return add_float(hash, fighter->u.kp.x2230);
    case Ft_Kind_GKoops:
        hash = add_float(hash, fighter->u.gk.x222C);
        return add_float(hash, fighter->u.gk.x2230);
    case Ft_Kind_Link:
    case Ft_Kind_CLink:
        hash = nativeSemanticState_AddU32(hash,
                                          fighter->u.lk.used_boomerang);
        hash = nativeSemanticState_AddU32(hash, fighter->u.lk.x4);
        return nativeSemanticState_AddU32(hash, fighter->u.lk.x1C);
    case Ft_Kind_Luigi:
        hash = nativeSemanticState_AddU32(
            hash, fighter->u.lg.x222C_cycloneCharge);
        hash = nativeSemanticState_AddU32(hash, fighter->u.lg.x2230);
        return nativeSemanticState_AddU32(hash, fighter->u.lg.x2234);
    case Ft_Kind_Mario:
    case Ft_Kind_DrMario:
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->u.mr.x222C_vitaminCurr);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->u.mr.x2230_vitaminPrev);
        hash = nativeSemanticState_AddU32(
            hash, fighter->u.mr.x2234_tornadoCharge);
        return nativeSemanticState_AddU32(
            hash, fighter->u.mr.x2238_isCapeBoost);
    case Ft_Kind_Mars:
    case Ft_Kind_Emblem:
        return nativeSemanticState_AddU32(hash, fighter->u.ms.x222C);
    case Ft_Kind_MasterH:
        hash = nativeSemanticState_AddU32(hash, fighter->u.mh.x2230);
        hash = nativeSemanticState_AddU32(hash, fighter->u.mh.x2234);
        hash = add_float(hash, fighter->u.mh.x2238);
        hash = add_float(hash, fighter->u.mh.x223C);
        hash = add_vec3(hash, &fighter->u.mh.x2240_pos);
        hash = nativeSemanticState_AddU32(hash, fighter->u.mh.x224C);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->u.mh.x2250);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->u.mh.x2254);
        return nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->u.mh.x2258);
    case Ft_Kind_CrezyH:
        hash = nativeSemanticState_AddU32(hash, fighter->u.ch.x2230);
        hash = nativeSemanticState_AddU32(hash, fighter->u.ch.x2234);
        hash = add_float(hash, fighter->u.ch.x2238);
        hash = add_float(hash, fighter->u.ch.x223C);
        hash = add_vec3(hash, &fighter->u.ch.x2240_pos);
        hash = nativeSemanticState_AddU32(hash, fighter->u.ch.x224C);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->u.ch.x2250);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->u.ch.x2254);
        return nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->u.ch.x2258);
    case Ft_Kind_Mewtwo:
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->u.mt.x2234_shadowBallCharge);
        return nativeSemanticState_AddU32(
            hash, fighter->u.mt.x223C_isConfusionBoost);
    case Ft_Kind_Ness:
        hash = add_vec3(hash, &fighter->u.ns.yoyo_hitbox_pos);
        hash = add_float(hash, fighter->u.ns.x223C);
        return nativeSemanticState_AddU32(hash,
                                          fighter->u.ns.pkthunder_gfx);
    case Ft_Kind_Peach:
        hash = nativeSemanticState_AddU32(hash, fighter->u.pe.has_float);
        hash = add_float(hash, fighter->u.pe.x4);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->u.pe.attacks4_motion_id);
        return nativeSemanticState_AddU32(
            hash, fighter->u.pe.specialairn_used);
    case Ft_Kind_Popo:
    case Ft_Kind_Nana:
        hash = nativeSemanticState_AddU32(hash, fighter->u.pp.x2230_b0);
        hash = nativeSemanticState_AddU32(hash, fighter->u.pp.x2234);
        hash = nativeSemanticState_AddU32(hash, fighter->u.pp.x223C);
        hash = add_vec3(hash, &fighter->u.pp.x2240);
        hash = nativeSemanticState_AddU32(hash, fighter->u.pp.x224C);
        return add_float(hash, fighter->u.pp.x2250);
    case Ft_Kind_Purin:
        hash = nativeSemanticState_AddU32(hash, fighter->u.pr.x222C);
        return add_vec3(hash, &fighter->u.pr.x2230);
    case Ft_Kind_Samus:
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) fighter->u.ss.x2230);
        hash = nativeSemanticState_AddU32(hash, fighter->u.ss.x2234);
        hash = nativeSemanticState_AddU32(hash, fighter->u.ss.x2238);
        hash = nativeSemanticState_AddU32(hash, fighter->u.ss.x2240);
        hash = nativeSemanticState_AddU32(hash, fighter->u.ss.x2241);
        hash = nativeSemanticState_AddU32(hash, fighter->u.ss.x2242);
        hash = nativeSemanticState_AddU32(hash, fighter->u.ss.x2243);
        hash = nativeSemanticState_AddU32(hash, fighter->u.ss.x2244);
        return nativeSemanticState_AddU32(hash, fighter->u.ss.x2248);
    case Ft_Kind_Seak:
        hash = nativeSemanticState_AddU32(hash,
                                          (uint32_t) fighter->u.sk.x0);
        for (uint32_t i = 0; i < 4; ++i) {
            hash = add_vec3(hash, &fighter->u.sk.xC[i]);
            hash = add_vec3(hash, &fighter->u.sk.x3C[i]);
        }
        return add_vec3(hash, &fighter->u.sk.lstick_delta);
    case Ft_Kind_Yoshi:
        return add_vec3(hash, &fighter->u.ys.x222C);
    default:
        return hash;
    }
}

uint64_t nativeSemanticState_Begin(void)
{
    return UINT64_C(14695981039346656037);
}

uint64_t nativeSemanticState_AddU32(uint64_t hash, uint32_t value)
{
    return (hash ^ value) * UINT64_C(1099511628211);
}

uint64_t nativeSemanticState_AddController(uint64_t hash,
                                           const VsSceneController* controller)
{
    hash = nativeSemanticState_AddU32(hash, controller->unk_0);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) controller->pauser);
    hash = nativeSemanticState_AddU32(hash, controller->pause_timer);
    hash = nativeSemanticState_AddU32(hash, controller->unpause_timer);
    hash = nativeSemanticState_AddU32(hash, controller->terminate_match);
    hash = nativeSemanticState_AddU32(hash, controller->match_result);
    hash = nativeSemanticState_AddU32(hash, controller->match_over);
    hash = nativeSemanticState_AddU32(hash, controller->frame_count);
    hash = nativeSemanticState_AddU32(hash, controller->timer_seconds);
    hash = nativeSemanticState_AddU32(hash, controller->unk_2C);
    hash = nativeSemanticState_AddU32(hash, controller->unk_2E);
    for (uint32_t i = 0; i < GM_MAX_PLAYERS; ++i) {
        const struct VsSceneFighter* fighter = &controller->fighters[i];
        hash = nativeSemanticState_AddU32(hash, fighter->x0);
        hash = nativeSemanticState_AddU32(hash, fighter->x1);
        hash = nativeSemanticState_AddU32(hash, fighter->slot_type);
        hash = nativeSemanticState_AddU32(hash,
                                          (uint32_t) fighter->spawn_point);
        hash = nativeSemanticState_AddU32(hash, fighter->x4_b0);
        hash = nativeSemanticState_AddU32(hash, fighter->x4_b1);
        hash = nativeSemanticState_AddU32(hash, fighter->x4_b2);
        hash = nativeSemanticState_AddU32(hash, fighter->x4_b3);
        hash = nativeSemanticState_AddU32(hash, fighter->x4_b4);
        hash = nativeSemanticState_AddU32(hash, fighter->x4_b5);
        hash = nativeSemanticState_AddU32(hash, fighter->x4_b6);
        hash = nativeSemanticState_AddU32(hash, fighter->x4_b7);
        hash = nativeSemanticState_AddU32(hash, fighter->x5);
        hash = nativeSemanticState_AddU32(hash, fighter->x6);
        hash = nativeSemanticState_AddU32(hash, fighter->x8);
        hash = nativeSemanticState_AddU32(hash, fighter->x9);
        hash = nativeSemanticState_AddU32(hash, fighter->respawn_timer);
        hash = nativeSemanticState_AddU32(hash, fighter->xB);
        hash = nativeSemanticState_AddU32(hash, fighter->xC);
    }
    return hash;
}

uint64_t nativeSemanticState_AddPlayer(uint64_t hash,
                                       const StaticPlayer* player)
{
    hash = nativeSemanticState_AddU32(hash, (uint32_t) player->player_state);
    hash = nativeSemanticState_AddU32(hash,
                                      (uint32_t) player->player_character);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) player->slot_type);
    hash = nativeSemanticState_AddU32(hash, player->transformed[0]);
    hash = nativeSemanticState_AddU32(hash, player->transformed[1]);
    hash = add_float(hash, player->facing_direction);
    hash = nativeSemanticState_AddU32(hash, player->costume_id);
    hash = nativeSemanticState_AddU32(hash,
                                      (uint32_t) player->controller_index);
    hash = nativeSemanticState_AddU32(hash, player->team);
    hash = nativeSemanticState_AddU32(hash, player->player_id);
    hash = nativeSemanticState_AddU32(hash, player->cpu_level);
    hash = nativeSemanticState_AddU32(hash, player->cpu_type);
    hash = nativeSemanticState_AddU32(hash, player->handicap);
    hash = add_float(hash, player->attack_ratio);
    hash = add_float(hash, player->defense_ratio);
    hash = add_float(hash, player->model_scale);
    for (uint32_t i = 0; i < 4; ++i) {
        hash = add_vec3(hash, &player->player_poses.byIndex[i]);
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) player->staminas.byIndex[i]);
    }
    hash = nativeSemanticState_AddU32(hash, (uint32_t) player->falls[0]);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) player->falls[1]);
    for (uint32_t i = 0; i < GM_MAX_PLAYERS; ++i) {
        hash = nativeSemanticState_AddU32(hash, player->kos_by_player[i]);
    }
    hash = nativeSemanticState_AddU32(hash, player->match_frame_count);
    hash = nativeSemanticState_AddU32(hash, player->suicide_count);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) player->stocks);
    hash = nativeSemanticState_AddU32(hash,
                                      (uint32_t) player->current_coins);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) player->total_coins);
    hash = nativeSemanticState_AddU32(
        hash, (uint32_t) player->stale_moves.current_index);
    for (uint32_t i = 0; i < 10; ++i) {
        hash = nativeSemanticState_AddU32(
            hash, player->stale_moves.StaleMoves[i].move_id);
        hash = nativeSemanticState_AddU32(
            hash, player->stale_moves.StaleMoves[i].attack_instance);
    }
    return hash;
}

uint64_t nativeSemanticState_AddCamera(uint64_t hash, const Camera* camera,
                                       const CmSubject* subject_tail)
{
    hash = nativeSemanticState_AddU32(hash, camera != NULL);
    if (camera == NULL) {
        return hash;
    }
    hash = nativeSemanticState_AddU32(hash, (uint32_t) camera->mode);
    hash = nativeSemanticState_AddU32(
        hash, (uint32_t) camera->background_color.r << 24 |
                  (uint32_t) camera->background_color.g << 16 |
                  (uint32_t) camera->background_color.b << 8 |
                  camera->background_color.a);
    hash = add_float(hash, camera->nearz);
    hash = add_float(hash, camera->farz);
    hash = add_camera_transform(hash, &camera->transform);
    hash = add_camera_transform(hash, &camera->transform_copy);
    hash = add_vec2(hash, &camera->translation);
    for (uint32_t i = 0; i < QuakeKind_Count; ++i) {
        hash = nativeSemanticState_AddU32(
            hash, (uint32_t) camera->quake_frames_left[i]);
    }
    hash = add_vec2(hash, &camera->quake_offset);
    hash = add_float(hash, camera->quake_scale);
    for (uint32_t bank = 0; bank < 2; ++bank) {
        for (uint32_t i = 0; i < 16; ++i) {
            hash = add_vec3(hash, &camera->quakes[bank][i].epicenter);
            hash = nativeSemanticState_AddU32(
                hash, (uint32_t) camera->quakes[bank][i].kind);
        }
    }
    hash = add_float(hash, camera->x2B0);
    hash = add_float(hash, camera->x2B4);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) camera->x2B8);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) camera->x2BA);
    hash = add_float(hash, camera->x2BC);
    hash = add_float(hash, camera->x2C0);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) camera->x2C4);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) camera->x2C5);
    hash = add_float(hash, camera->pitch_offset);
    hash = add_float(hash, camera->yaw_offset);
    hash = add_float(hash, camera->x2D0.y_max);
    hash = add_float(hash, camera->x2D0.y_min);
    hash = add_float(hash, camera->x2D0.x_min);
    hash = add_float(hash, camera->x2D0.x_max);
    hash = add_float(hash, camera->x2D0.z_max);
    hash = add_float(hash, camera->x2D0.z_min);
    hash = add_float(hash, camera->x2D0.angle_up);
    hash = add_float(hash, camera->x2D0.angle_down);
    hash = add_float(hash, camera->x2D0.angle_right);
    hash = add_float(hash, camera->x2D0.angle_left);
    hash = add_float(hash, camera->x2D0.unk28);
    hash = add_float(hash, camera->x2D0.unk2C);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) camera->x304);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) camera->x305);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) camera->x306);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) camera->x307);
    hash = add_vec3(hash, &camera->x308);
    hash = add_vec3(hash, &camera->x314);
    hash = add_vec3(hash, &camera->pause_eye_offset);
    hash = add_float(hash, camera->x32C);
    hash = add_float(hash, camera->pause_eye_distance);
    hash = add_vec3(hash, &camera->pause_up);
    hash = nativeSemanticState_AddU32(hash, camera->x340);
    hash = nativeSemanticState_AddU32(hash, camera->x341_b0);
    hash = nativeSemanticState_AddU32(hash, camera->x341_b1_b2);
    hash = nativeSemanticState_AddU32(hash, camera->x341_b3_b4);
    hash = nativeSemanticState_AddU32(hash, camera->x341_b5_b6);
    hash = nativeSemanticState_AddU32(hash, camera->x341_b7);
    hash = add_vec3(hash, &camera->x350);
    hash = add_vec3(hash, &camera->x368);
    hash = add_float(hash, camera->x374);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) camera->x378.s32);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) camera->x37C.s32);
    for (uint32_t i = 0; i < sizeof(camera->x380); ++i) {
        hash = nativeSemanticState_AddU32(hash, camera->x380[i]);
    }
    hash = nativeSemanticState_AddU32(hash, camera->x398_b0);
    hash = nativeSemanticState_AddU32(hash, camera->x398_b1);
    hash = nativeSemanticState_AddU32(hash, camera->x398_b2);
    hash = nativeSemanticState_AddU32(hash, camera->x398_b3);
    hash = nativeSemanticState_AddU32(hash, camera->x398_b4);
    hash = nativeSemanticState_AddU32(hash, camera->x398_b5);
    hash = nativeSemanticState_AddU32(hash, camera->x398_b6_b7);
    hash = nativeSemanticState_AddU32(hash, camera->x399_b0_b1);
    hash = nativeSemanticState_AddU32(hash, camera->x399_b2);
    hash = nativeSemanticState_AddU32(hash, camera->x399_b3);
    hash = nativeSemanticState_AddU32(hash, camera->x399_b4);
    hash = nativeSemanticState_AddU32(hash, camera->x399_b5);
    hash = nativeSemanticState_AddU32(hash, camera->x399_b6);
    hash = nativeSemanticState_AddU32(hash, camera->x399_b7);
    hash = nativeSemanticState_AddU32(hash, camera->x39A_b0);
    hash = nativeSemanticState_AddU32(hash, camera->x39A_b1);
    hash = nativeSemanticState_AddU32(hash, camera->x39A_b2);
    hash = nativeSemanticState_AddU32(hash, camera->x39A_b3);
    hash = nativeSemanticState_AddU32(hash, camera->x39A_b4);
    hash = nativeSemanticState_AddU32(hash, camera->x39A_b5);
    hash = nativeSemanticState_AddU32(hash, camera->x39A_b6);
    hash = nativeSemanticState_AddU32(hash, camera->x39A_b7);

    uint32_t subject_count = 0;
    for (const CmSubject* subject = subject_tail; subject != NULL;
         subject = subject->prev)
    {
        if (subject_count == SEMANTIC_CAMERA_SUBJECT_LIMIT) {
            return nativeSemanticState_AddU32(hash, UINT32_MAX);
        }
        hash = add_camera_subject(hash, subject);
        ++subject_count;
    }
    return nativeSemanticState_AddU32(hash, subject_count);
}

uint64_t nativeSemanticState_AddFighter(uint64_t hash, const Fighter* fighter,
                                        uint32_t part_count,
                                        NativeSemanticFighterTrace* trace)
{
    hash = nativeSemanticState_AddU32(hash, (uint32_t) fighter->kind);
    hash = nativeSemanticState_AddU32(hash, fighter->player_id);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) fighter->motion_id);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) fighter->anim_id);
    hash = add_float(hash, fighter->facing_dir);
    hash = add_vec3(hash, &fighter->x34_scale);
    hash = add_vec3(hash, &fighter->x74_self_accel);
    hash = add_vec3(hash, &fighter->self_vel);
    hash = add_vec3(hash, &fighter->x8c_kb_vel);
    hash = add_vec3(hash, &fighter->x98_atk_shield_kb);
    hash = add_vec3(hash, &fighter->xA4_unk_vel);
    hash = add_vec3(hash, &fighter->cur_pos);
    hash = add_vec3(hash, &fighter->prev_pos);
    hash = add_vec3(hash, &fighter->pos_delta);
    hash = nativeSemanticState_AddU32(hash,
                                      (uint32_t) fighter->ground_or_air);
    hash = add_float(hash, fighter->xE4_ground_accel_1);
    hash = add_float(hash, fighter->xE8_ground_accel_2);
    hash = add_float(hash, fighter->gr_vel);
    hash = add_float(hash, fighter->xF0_ground_kb_vel);
    hash = add_float(hash, fighter->xF4_ground_attacker_shield_kb_vel);
    if (trace != NULL) trace->movement = hash;
    for (uint32_t i = 0; i < 3; ++i) {
        hash = add_vec2(hash, &fighter->input.lstick[i]);
        hash = add_vec2(hash, &fighter->input.cstick[i]);
        hash = add_float(hash, fighter->input.triggers[i]);
        hash = nativeSemanticState_AddU32(hash,
                                          fighter->input.held_buttons[i]);
        hash = nativeSemanticState_AddU32(hash, (uint8_t) fighter->ucf_stick[i].x);
        hash = nativeSemanticState_AddU32(hash, (uint8_t) fighter->ucf_stick[i].y);
    }
    hash = nativeSemanticState_AddU32(hash, fighter->input.pressed_buttons);
    hash = nativeSemanticState_AddU32(hash, fighter->input.released_buttons);
    hash = nativeSemanticState_AddU32(hash, fighter->ucf_shield_drop_frames);
    const NativeActionBuffer* pending = &fighter->action_buffer;
    const NativeActionStick* sticks[] = { &pending->movement, &pending->direction };
    for (unsigned i = 0; i < 2; ++i) {
        hash = nativeSemanticState_AddU32(hash, sticks[i]->frames);
        if (sticks[i]->frames) {
            hash = add_vec2(hash, &sticks[i]->position);
            hash = nativeSemanticState_AddU32(hash, sticks[i]->tilt_x);
            hash = nativeSemanticState_AddU32(hash, sticks[i]->tilt_y);
        }
    }
    if (pending->direction.frames) {
        hash = add_vec2(hash, &pending->cstick);
        hash = nativeSemanticState_AddU32(hash, pending->cstick_command);
        hash = add_float(hash, pending->trigger);
        hash = nativeSemanticState_AddU32(hash, pending->pressed);
        hash = nativeSemanticState_AddU32(hash, pending->held);
    }
    if (trace != NULL) trace->input = hash;
    hash = add_collision(hash, &fighter->coll_data);
    if (trace != NULL) trace->collision = hash;
    hash = add_float(hash, fighter->cur_anim_frame);
    hash = add_float(hash, fighter->aerial_landing_interrupt_frame);
    hash = nativeSemanticState_AddU32(hash, fighter->native_quiet_ko_pending |
                                     (fighter->native_quiet_ko_active << 1));
    hash = add_float(hash, fighter->x898_unk);
    hash = add_float(hash, fighter->frame_speed_mul);
    hash = add_float(hash, fighter->x8A0_unk);
    hash = add_float(hash, fighter->x8A4_animBlendFrames);
    hash = add_float(hash, fighter->x8A8_anim_frame);
    hash = add_float(hash, fighter->x3E4_fighterCmdScript.timer);
    hash = add_float(hash, fighter->x3E4_fighterCmdScript.frame_count);
    hash = nativeSemanticState_AddU32(hash, fighter->x594_b1_loop);
    if (trace != NULL) trace->animation = hash;
    for (uint32_t i = 0; i < 4; ++i) {
        hash = add_hit_capsule(hash, &fighter->x914[i]);
        hash = add_hitbox_lifetime(hash, &fighter->native_hitbox_lifetimes[i]);
    }
    for (uint32_t i = 0; i < 2; ++i) {
        hash = add_hit_capsule(hash, &fighter->xDF4[i]);
    }
    hash = add_hit_capsule(hash, &fighter->x1064_thrownHitbox);
    hash = nativeSemanticState_AddU32(hash, fighter->hurt_capsules_len);
    uint32_t hurt_count = fighter->hurt_capsules_len;
    if (hurt_count > 15) {
        hurt_count = 15;
    }
    for (uint32_t i = 0; i < hurt_count; ++i) {
        hash = add_hurt_capsule(hash, &fighter->hurt_capsules[i]);
    }
    if (trace != NULL) trace->hitboxes = hash;
    hash = add_float(hash, fighter->dmg.x1830_percent);
    hash = nativeSemanticState_AddU32(hash,
                                      (uint32_t) fighter->dmg.x183C_applied);
    hash = nativeSemanticState_AddU32(hash,
                                      (uint32_t) fighter->dmg.x1848_kb_angle);
    hash = add_float(hash, fighter->dmg.kb_applied);
    hash = nativeSemanticState_AddU32(hash, fighter->native_contact_launch_active);
    if (fighter->native_contact_launch_active) {
        hash = add_float(hash, fighter->native_contact_launch.x);
        hash = add_float(hash, fighter->native_contact_launch.y);
    }
    hash = add_float(hash, fighter->dmg.x18A4_knockbackMagnitude);
    hash = nativeSemanticState_AddU32(
        hash, (uint32_t) fighter->dmg.x18ac_time_since_hit);
    hash = add_float(hash, fighter->dmg.x195c_hitlag_frames);
    hash = nativeSemanticState_AddU32(hash, fighter->x1968_jumpsUsed);
    hash = nativeSemanticState_AddU32(hash, fighter->x1969_walljumpUsed);
    hash = add_float(hash, fighter->hitlag_mul);
    hash = add_float(hash, fighter->shield_health);
    hash = add_float(hash, fighter->lightshield_amount);
    hash = add_float(hash, fighter->grab_timer);
    hash = nativeSemanticState_AddU32(hash,
                                      (uint32_t) fighter->capture_timer);
    hash = nativeSemanticState_AddU32(
        hash, (uint32_t) fighter->x2064_ledgeCooldown);
    hash = nativeSemanticState_AddU32(hash,
                                      (uint32_t) fighter->x2068_attackID);
    hash = nativeSemanticState_AddU32(hash, fighter->x206C_attack_instance);
    for (uint32_t i = 0; i < 4; ++i) {
        hash = nativeSemanticState_AddU32(hash, fighter->cmd_vars[i]);
    }
    hash = nativeSemanticState_AddU32(hash, fighter->throw_flags);
    hash = add_float(hash, fighter->cmd_timer);
    hash = nativeSemanticState_AddU32(hash, fighter->allow_interrupt);
    hash = nativeSemanticState_AddU32(hash, fighter->allow_sdi);
    hash = nativeSemanticState_AddU32(hash, fighter->fall_fast);
    hash = nativeSemanticState_AddU32(hash, fighter->no_normal_motion);
    hash = nativeSemanticState_AddU32(hash, fighter->no_kb);
    hash = nativeSemanticState_AddU32(hash,
                                      (uint32_t) fighter->bury_grkind);
    hash = nativeSemanticState_AddU32(hash, fighter->bury_timer_1);
    hash = nativeSemanticState_AddU32(hash, fighter->bury_timer_2);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) fighter->x2330.x);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) fighter->x2330.y);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) fighter->x2338.x);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) fighter->x2338.y);
    if (trace != NULL) trace->damage = hash;
    hash = add_fighter_kind_state(hash, fighter);
    if (fighter->kind >= Ft_Kind_CustomFirst) {
        hash = nativeSemanticState_AddU32(hash,
                                          (uint32_t) fighter->behavior_kind);
        // Fighter allocation clears this fixed per-instance storage before
        // module callbacks run. Hash the whole region so an authored variable
        // cannot drift invisibly between replay or rollback peers.
        for (uint32_t i = 0; i < sizeof(fighter->native_module_state);
             i += 4) {
            const u8* state = &fighter->native_module_state[i];
            uint32_t word = (uint32_t) state[0] |
                            ((uint32_t) state[1] << 8) |
                            ((uint32_t) state[2] << 16) |
                            ((uint32_t) state[3] << 24);
            hash = nativeSemanticState_AddU32(hash, word);
        }
    }
    if (trace != NULL) trace->script = hash;
    // Authored costume graphs live outside the fighter's primary joint tree.
    // Include their tracked controls and actual pose/materials in desync checks.
    hash = nativeSemanticState_AddU32(hash, fighter->native_attachment_dynamic_mask);
    for (unsigned slot = 0; slot < NATIVE_FIGHTER_ATTACHMENTS; ++slot) {
        const NativeFighterAttachment* attachment = &fighter->native_attachments[slot];
        hash = nativeSemanticState_AddU32(hash, attachment->joint != NULL);
        if (attachment->joint == NULL) continue;
        hash = nativeSemanticState_AddU32(hash, attachment->bone);
        hash = nativeSemanticState_AddU32(hash, attachment->visible);
        hash = nativeSemanticState_AddU32(hash, attachment->has_visibility);
        hash = nativeSemanticState_AddU32(hash, attachment->dynamic_mask);
        for (unsigned group = 0; group < 5; ++group)
            hash = nativeSemanticState_AddU32(hash, attachment->visibility.cleared[group]);
        hash = nativeSemanticState_AddJointTree(hash, attachment->joint);
        hash = nativeSemanticState_AddJointVisuals(hash, attachment->joint);
    }
    hash = add_fighter_common_motion_state(hash, fighter);
    if (trace != NULL) trace->attachments = hash;
    hash = nativeSemanticState_AddU32(hash, part_count);
    if (part_count > 256) {
        part_count = 0;
    }
    for (uint32_t i = 0; i < part_count; ++i) {
        const HSD_JObj* joint = fighter->parts != NULL
                                    ? fighter->parts[i].joint
                                    : NULL;
        hash = nativeSemanticState_AddU32(hash, joint != NULL);
        if (joint == NULL) {
            continue;
        }
        uint32_t quaternion = (joint->flags & JOBJ_USE_QUATERNION) != 0;
        hash = nativeSemanticState_AddU32(hash, quaternion);
        hash = add_float(hash, joint->rotate.x);
        hash = add_float(hash, joint->rotate.y);
        hash = add_float(hash, joint->rotate.z);
        if (quaternion) {
            hash = add_float(hash, joint->rotate.w);
        }
        hash = add_vec3(hash, &joint->translate);
        hash = add_vec3(hash, &joint->scale);
    }
    if (trace != NULL) trace->pose = hash;
    return hash;
}

uint64_t nativeSemanticState_AddItem(uint64_t hash, const Item* item,
                                     NativeSemanticItemTrace* trace)
{
    hash = nativeSemanticState_AddU32(hash, (uint32_t) item->spawn_kind);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) item->kind);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) item->hold_kind);
    hash = nativeSemanticState_AddU32(hash, item->x20_team_id);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) item->msid);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) item->anim_id);
    hash = add_float(hash, item->facing_dir);
    hash = add_float(hash, item->init_facing_dir);
    hash = add_float(hash, item->spin_spd);
    hash = add_float(hash, item->scl);
    if (trace != NULL) {
        trace->identity = hash;
    }
    hash = add_vec3(hash, &item->x40_vel);
    hash = add_vec3(hash, &item->pos);
    hash = add_vec3(hash, &item->x58_vec_unk);
    hash = add_vec3(hash, &item->x64_vec_unk2);
    hash = add_vec3(hash, &item->x70_nudge);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) item->ground_or_air);
    hash = add_collision(hash, &item->x378_itemColl);
    if (trace != NULL) {
        trace->motion = hash;
    }
    // it_802799E4 replaces frame_count from currentAnimFrame before every
    // command read. Without a live command, timer is also unused scratch.
    hash = nativeSemanticState_AddU32(hash, item->x524_cmd.u != NULL);
    hash = add_float(hash, item->x524_cmd.u ? item->x524_cmd.timer : 0.0f);
    hash = add_float(hash, item->x5CC_currentAnimFrame);
    hash = add_float(hash, item->x5D0_animFrameSpeed);
    if (trace != NULL) {
        trace->animation = hash;
    }
    for (uint32_t i = 0; i < 4; ++i) {
        const HitCapsule* hit = &item->x5D4_hitboxes[i].hit;
        if (hit->state == HitCapsule_Disabled) {
            // Item collision ignores disabled slots. Their payload can retain
            // data from a prior item allocated out of the same pool.
            hash = nativeSemanticState_AddU32(hash, HitCapsule_Disabled);
        } else {
            hash = add_hit_capsule(hash, hit);
            hash = nativeSemanticState_AddU32(
                hash, item->x5D4_hitboxes[i].x138);
        }
        if (trace != NULL) {
            trace->hitbox_state[i] = item->x5D4_hitboxes[i].hit.state;
            trace->hitbox[i] = hash;
        }
    }
    if (trace != NULL) {
        trace->hitboxes = hash;
    }
    hash = nativeSemanticState_AddU32(hash, item->xAC8_hurtboxNum);
    uint32_t item_hurt_count = item->xAC8_hurtboxNum;
    if (item_hurt_count > 2) {
        item_hurt_count = 2;
    }
    if (trace != NULL) {
        trace->hurtbox_count = item->xAC8_hurtboxNum;
    }
    for (uint32_t i = 0; i < item_hurt_count; ++i) {
        const HurtCapsule* hurt = &item->xACC_itemHurtbox[i];
        hash = nativeSemanticState_AddU32(hash, (uint32_t) hurt->state);
        hash = add_vec3(hash, &hurt->a_offset);
        hash = add_vec3(hash, &hurt->b_offset);
        hash = add_float(hash, hurt->scale);
        if (trace != NULL) {
            trace->hurtbox_state[i] = hurt->state;
            trace->hurtbox_shape[i] = hash;
            trace->hurtbox_skip[i] = hurt->skip_update_pos;
        }
        hash = nativeSemanticState_AddU32(hash, hurt->skip_update_pos);
        // Collision updates the cached world positions before reading them
        // when skip_update_pos is clear. New item hurtboxes leave these
        // positions untouched until that first collision query.
        if (hurt->skip_update_pos) {
            hash = add_vec3(hash, &hurt->a_pos);
            hash = add_vec3(hash, &hurt->b_pos);
        }
        if (trace != NULL) {
            trace->hurtbox_position[i] = hash;
            trace->hurtbox[i] = hash;
        }
        // The item path assigns bone directly and never reads bone_idx;
        // unlike fighter hurtboxes, this field is not initialized for items.
    }
    if (trace != NULL) {
        trace->hurtboxes = hash;
    }
    hash = nativeSemanticState_AddU32(hash,
                                      (uint32_t) item->xC34_damageDealt);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) item->xC9C);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) item->xCA0);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) item->xCA8);
    // Collision writes the damage angle together with knockback. Before a
    // damaging hit, xCAC_angle can retain bytes from an earlier pooled item.
    if (item->xCC8_knockback != 0.0f) {
        hash = nativeSemanticState_AddU32(hash,
                                          (uint32_t) item->xCAC_angle);
    }
    hash = add_float(hash, item->xCBC_hitlagFrames);
    hash = add_float(hash, item->xCC8_knockback);
    hash = add_float(hash, item->xD3C_spinSpeed);
    hash = add_float(hash, item->xD40);
    hash = add_float(hash, item->xD44_lifeTimer);
    hash = add_float(hash, item->xD48_halfLifeTimer);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) item->xD4C);
    hash = nativeSemanticState_AddU32(hash, item->xD50_landNum);
    hash = nativeSemanticState_AddU32(hash, item->xD54_throwNum);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) item->destroy_type);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) item->xD88_attackID);
    hash = nativeSemanticState_AddU32(hash, item->xD8C_attack_instance);
    if (trace != NULL) {
        const uint32_t scalar[] = {
            (uint32_t) item->xC34_damageDealt,
            (uint32_t) item->xC9C,
            (uint32_t) item->xCA0,
            (uint32_t) item->xCA8,
            item->xCC8_knockback != 0.0f
                ? (uint32_t) item->xCAC_angle : 0,
            float_bits(item->xCBC_hitlagFrames),
            float_bits(item->xCC8_knockback),
            float_bits(item->xD3C_spinSpeed),
            float_bits(item->xD40),
            float_bits(item->xD44_lifeTimer),
            float_bits(item->xD48_halfLifeTimer),
            (uint32_t) item->xD4C,
            item->xD50_landNum,
            item->xD54_throwNum,
            (uint32_t) item->destroy_type,
            (uint32_t) item->xD88_attackID,
            item->xD8C_attack_instance,
        };
        _Static_assert(sizeof(scalar) / sizeof(scalar[0]) ==
                           NATIVE_SEMANTIC_ITEM_SCALAR_COUNT,
                       "item scalar trace must match digest fields");
        memcpy(trace->scalar, scalar, sizeof(scalar));
        trace->damage = hash;
    }
    // Super Scope and stock Food never read these script scratch words. Food's
    // four states have animation -1 and no DAT command stream; its callbacks
    // use the foods union instead. New items can retain an old heap value here.
    // Authored callbacks can use the generic command properties for any kind,
    // so their command words must remain observed even for these stock kinds.
    if ((item->kind != It_Kind_S_Scope && item->kind != It_Kind_Foods) ||
        nativeScriptItem_IsAttached(item->entity)) {
        hash = nativeSemanticState_AddU32(hash, item->xDAC_itcmd_var0);
        hash = nativeSemanticState_AddU32(hash, item->xDB0_itcmd_var1);
        hash = nativeSemanticState_AddU32(hash, item->xDB4_itcmd_var2);
        hash = nativeSemanticState_AddU32(hash, item->xDB8_itcmd_var3);
    }
    hash = nativeSemanticState_AddU32(hash, item->xDBC_itcmd_var4_word);
    hash = nativeSemanticState_AddU32(hash, item->xDC0);
    hash = nativeSemanticState_AddU32(hash, item->xDC4);
    uint32_t command_flags;
    memcpy(&command_flags, &item->xDC8_word, sizeof(command_flags));
    hash = nativeSemanticState_AddU32(hash, command_flags);
    if (item->kind == It_Kind_GameWatch_Chef ||
        item->kind == It_Kind_Kirby_GameWatchChef) {
        hash = nativeSemanticState_AddU32(
            hash, nativeScriptItem_IsAttached(item->entity)
                      ? item_script_int(item, "variant")
                      : (uint32_t) item->xDD4_itemVar.gamewatchchef.variant);
    }
    if (item->kind == It_Kind_NativeHoldable) {
        hash = nativeSemanticState_AddU32(
            hash, item->xDD4_itemVar.native_holdable.owner_kind);
        hash = nativeSemanticState_AddU32(
            hash, item->xDD4_itemVar.native_holdable.definition_index);
        hash = nativeSemanticState_AddU32(
            hash, item->xDD4_itemVar.native_holdable.age);
        hash = nativeSemanticState_AddU32(
            hash, item->xDD4_itemVar.native_holdable.collision_flags);
        for (unsigned i = 0; i < 8; ++i)
            hash = nativeSemanticState_AddU32(
                hash, float_bits(item->xDD4_itemVar.native_holdable.state[i]));
    }
    if (trace != NULL) {
        trace->commands = hash;
    }
    return hash;
}

uint64_t nativeSemanticState_AddGround(uint64_t hash, const Ground* ground)
{
    // Ground_GetStageGObj does not initialize x0. It can retain an old heap
    // address even when every stage field used by the simulation agrees.
    hash = nativeSemanticState_AddU32(hash, ground->x10_flags.b0);
    hash = nativeSemanticState_AddU32(hash, ground->x10_flags.b1);
    hash = nativeSemanticState_AddU32(hash, ground->x10_flags.b2);
    hash = nativeSemanticState_AddU32(hash, ground->x10_flags.b3);
    hash = nativeSemanticState_AddU32(hash, ground->x10_flags.b4);
    hash = nativeSemanticState_AddU32(hash, ground->x10_flags.b5);
    hash = nativeSemanticState_AddU32(hash, ground->x10_flags.b6);
    hash = nativeSemanticState_AddU32(hash, ground->x10_flags.b7);
    hash = nativeSemanticState_AddU32(hash, ground->x11_flags.b012);
    hash = nativeSemanticState_AddU32(hash, ground->x11_flags.b3);
    hash = nativeSemanticState_AddU32(hash, ground->x11_flags.b4);
    hash = nativeSemanticState_AddU32(hash, ground->x11_flags.b5);
    hash = nativeSemanticState_AddU32(hash, ground->x11_flags.b6);
    hash = nativeSemanticState_AddU32(hash, ground->x11_flags.b7);
    hash = nativeSemanticState_AddU32(hash, (uint32_t) ground->map_id);
    for (uint32_t i = 0; i < 8; ++i) {
        hash = nativeSemanticState_AddU32(hash, (uint32_t) ground->x20[i]);
    }
    // grMaterial reads xC0 only while its alpha-constant flag is active.
    // Otherwise this field can contain untouched allocation bytes.
    if (ground->x10_flags.b6) {
        hash = add_float(hash, ground->xC0);
    }
    return hash;
}

static uint64_t add_joint_chain(uint64_t hash, const HSD_JObj* joint,
                                uint32_t depth, uint32_t* count)
{
    if (depth > SEMANTIC_JOINT_DEPTH_LIMIT) {
        return nativeSemanticState_AddU32(hash, UINT32_C(0xDEAD0001));
    }
    for (; joint != NULL; joint = joint->next) {
        if (++*count > SEMANTIC_JOINT_LIMIT) {
            return nativeSemanticState_AddU32(hash, UINT32_C(0xDEAD0002));
        }
        hash = nativeSemanticState_AddU32(hash, UINT32_C(0x4A4F424A));
        hash = nativeSemanticState_AddU32(hash,
                                          joint->flags & ~JOBJ_MTX_DIRTY);
        hash = add_float(hash, joint->rotate.x);
        hash = add_float(hash, joint->rotate.y);
        hash = add_float(hash, joint->rotate.z);
        if ((joint->flags & JOBJ_USE_QUATERNION) != 0) {
            hash = add_float(hash, joint->rotate.w);
        }
        hash = add_vec3(hash, &joint->scale);
        hash = add_vec3(hash, &joint->translate);
        hash = nativeSemanticState_AddU32(hash, joint->aobj != NULL);
        if (joint->aobj != NULL) {
            hash = nativeSemanticState_AddU32(hash, joint->aobj->flags);
            hash = add_float(hash, joint->aobj->curr_frame);
            hash = add_float(hash, joint->aobj->rewind_frame);
            hash = add_float(hash, joint->aobj->end_frame);
            hash = add_float(hash, joint->aobj->framerate);
        }
        hash = add_joint_chain(hash, joint->child, depth + 1, count);
        hash = nativeSemanticState_AddU32(hash, UINT32_C(0x454E444A));
    }
    return hash;
}

uint64_t nativeSemanticState_AddJointTree(uint64_t hash,
                                          const HSD_JObj* root)
{
    uint32_t count = 0;
    hash = add_joint_chain(hash, root, 0, &count);
    return nativeSemanticState_AddU32(hash, count);
}

static uint64_t add_visual_bytes(uint64_t hash, const void* data, size_t bytes)
{
    hash = nativeSemanticState_AddU32(hash, (uint32_t) bytes);
    if (data == NULL) return nativeSemanticState_AddU32(hash, 0);
    const uint8_t* cursor = data;
    for (size_t index = 0; index < bytes; ++index)
        hash = (hash ^ cursor[index]) * UINT64_C(1099511628211);
    return hash;
}

static uint64_t add_joint_visuals(uint64_t hash, const HSD_JObj* joint,
                                  uint32_t depth, uint32_t* count)
{
    if (depth > SEMANTIC_JOINT_DEPTH_LIMIT)
        return nativeSemanticState_AddU32(hash, UINT32_C(0xDEAD0011));
    for (; joint != NULL; joint = joint->next) {
        if (++*count > SEMANTIC_JOINT_LIMIT)
            return nativeSemanticState_AddU32(hash, UINT32_C(0xDEAD0012));
        if (union_type_dobj(joint)) {
            unsigned draw_count = 0;
            for (const HSD_DObj* draw = joint->u.dobj; draw != NULL;
                 draw = draw->next) {
                if (++draw_count > 128) break;
                hash = nativeSemanticState_AddU32(hash, draw->flags);
                const HSD_MObj* material = draw->mobj;
                hash = nativeSemanticState_AddU32(hash, material != NULL);
                if (material == NULL) continue;
                hash = nativeSemanticState_AddU32(hash, material->rendermode);
                hash = nativeSemanticState_AddU32(hash,
                                                   material->mat != NULL);
                if (material->mat != NULL) {
                    uint32_t diffuse;
                    memcpy(&diffuse, &material->mat->diffuse, sizeof(diffuse));
                    hash = nativeSemanticState_AddU32(hash, diffuse);
                    hash = add_float(hash, material->mat->alpha);
                }
                unsigned texture_count = 0;
                for (const HSD_TObj* texture = material->tobj;
                     texture != NULL; texture = texture->next) {
                    if (++texture_count > 16) break;
                    hash = nativeSemanticState_AddU32(hash, texture->flags);
                    hash = nativeSemanticState_AddU32(hash, texture->tlut_no);
                    hash = add_float(hash, texture->blending);
                    const HSD_ImageDesc* image = texture->imagedesc;
                    hash = nativeSemanticState_AddU32(hash, image != NULL);
                    if (image != NULL) {
                        hash = nativeSemanticState_AddU32(hash, image->width);
                        hash = nativeSemanticState_AddU32(hash, image->height);
                        hash = nativeSemanticState_AddU32(hash, image->format);
                        uint32_t bytes = GXGetTexBufferSize(
                            image->width, image->height, image->format,
                            image->mipmap != 0, (uint8_t) ceilf(image->maxLOD));
                        hash = add_visual_bytes(hash, image->image_ptr, bytes);
                    }
                    const HSD_Tlut* palette = texture->tlut_no != (uint8_t) -1 &&
                                              texture->tluttbl != NULL
                        ? texture->tluttbl[texture->tlut_no] : texture->tlut;
                    hash = nativeSemanticState_AddU32(hash, palette != NULL);
                    if (palette != NULL) {
                        hash = nativeSemanticState_AddU32(hash, palette->fmt);
                        hash = add_visual_bytes(hash, palette->lut,
                                                (size_t) palette->n_entries * 2);
                    }
                }
                hash = nativeSemanticState_AddU32(hash, texture_count);
            }
            hash = nativeSemanticState_AddU32(hash, draw_count);
        }
        hash = add_joint_visuals(hash, joint->child, depth + 1, count);
    }
    return hash;
}

uint64_t nativeSemanticState_AddJointVisuals(uint64_t hash,
                                             const HSD_JObj* root)
{
    uint32_t count = 0;
    hash = add_joint_visuals(hash, root, 0, &count);
    return nativeSemanticState_AddU32(hash, count);
}

static void collect_objects(const HSD_GObj* head, const HSD_GObj** objects,
                            uint32_t capacity, uint32_t* count,
                            bool* overflowed)
{
    *count = 0;
    for (const HSD_GObj* object = head; object != NULL; object = object->next) {
        if (*count == capacity) {
            *overflowed = true;
            return;
        }
        objects[(*count)++] = object;
    }
}

void nativeSemanticState_BuildObjectMap(NativeSemanticObjectMap* map,
                                        const HSD_GObj* fighter_head,
                                        const HSD_GObj* item_head,
                                        const HSD_GObj* ground_head)
{
    memset(map, 0, sizeof(*map));
    collect_objects(fighter_head, map->fighters,
                    NATIVE_SEMANTIC_FIGHTER_CAPACITY, &map->fighter_count,
                    &map->overflowed);
    collect_objects(item_head, map->items, NATIVE_SEMANTIC_ITEM_CAPACITY,
                    &map->item_count, &map->overflowed);
    collect_objects(ground_head, map->grounds, NATIVE_SEMANTIC_GROUND_CAPACITY,
                    &map->ground_count, &map->overflowed);
}

static uint32_t find_object(const HSD_GObj* const* objects, uint32_t count,
                            const HSD_GObj* object, uint32_t category)
{
    for (uint32_t i = 0; i < count; ++i) {
        if (objects[i] == object) {
            return (category << 24) | (i + 1);
        }
    }
    return UINT32_MAX;
}

uint32_t nativeSemanticState_ObjectId(const NativeSemanticObjectMap* map,
                                      const HSD_GObj* object)
{
    if (object == NULL) {
        return 0;
    }
    uint32_t id = find_object(map->fighters, map->fighter_count, object, 1);
    if (id != UINT32_MAX) {
        return id;
    }
    id = find_object(map->items, map->item_count, object, 2);
    if (id != UINT32_MAX) {
        return id;
    }
    return find_object(map->grounds, map->ground_count, object, 3);
}

uint64_t nativeSemanticState_AddObjectMap(
    uint64_t hash, const NativeSemanticObjectMap* map)
{
    hash = nativeSemanticState_AddU32(hash, map->fighter_count);
    hash = nativeSemanticState_AddU32(hash, map->item_count);
    hash = nativeSemanticState_AddU32(hash, map->ground_count);
    return nativeSemanticState_AddU32(hash, map->overflowed);
}

static uint64_t add_object(uint64_t hash, const NativeSemanticObjectMap* map,
                           const HSD_GObj* object)
{
    return nativeSemanticState_AddU32(
        hash, nativeSemanticState_ObjectId(map, object));
}

static uint64_t add_hit_relations(uint64_t hash, const HitCapsule* hit,
                                  const NativeSemanticObjectMap* map)
{
    // Disabled capsules are not queried or aged; item initialization disables
    // their slots without clearing a prior occupant's victim payload.
    hash = nativeSemanticState_AddU32(hash, (uint32_t)hit->state);
    if(hit->state==HitCapsule_Disabled)return hash;
    for (uint32_t i = 0; i < 12; ++i) {
        hash = add_object(hash, map, hit->victims_1[i].victim);
        hash = add_object(hash, map, hit->victims_2[i].victim);
    }
    return add_object(hash, map, hit->owner);
}

static uint64_t add_fighter_kind_relations(
    uint64_t hash, const Fighter* fighter, const NativeSemanticObjectMap* map)
{
    FighterKind kind = fighter->kind >= Ft_Kind_CustomFirst
                           ? fighter->behavior_kind
                           : fighter->kind;
    switch (kind) {
    case Ft_Kind_Fox:
        return add_object(hash, map, fighter->u.fx.x222C_blasterGObj);
    case Ft_Kind_Falco:
        return add_object(hash, map, fighter->u.fc.x222C_blasterGObj);
    case Ft_Kind_GameWatch:
        hash = add_object(hash, map, fighter->u.gw.x2248_manholeGObj);
        hash = add_object(hash, map, fighter->u.gw.x224C_greenhouseGObj);
        hash = add_object(hash, map, fighter->u.gw.x2250_manholeGObj2);
        hash = add_object(hash, map, fighter->u.gw.x2254_fireGObj);
        hash = add_object(hash, map, fighter->u.gw.x2258_parachuteGObj);
        hash = add_object(hash, map, fighter->u.gw.x225C_turtleGObj);
        hash = add_object(hash, map, fighter->u.gw.x2260_sparkyGObj);
        hash = add_object(hash, map, fighter->u.gw.x2264_judgementGObj);
        hash = add_object(hash, map, fighter->u.gw.x2268_panicGObj);
        return add_object(hash, map, fighter->u.gw.x226C_rescueGObj);
    case Ft_Kind_Kirby:
        hash = add_object(hash, map, fighter->u.kb.hat.x0);
        hash = add_object(hash, map, fighter->u.kb.x74);
        hash = add_object(hash, map, fighter->u.kb.x78);
        hash = add_object(hash, map, fighter->u.kb.ns_flash_gobj);
        hash = add_object(hash, map, fighter->u.kb.x98);
        hash = add_object(hash, map, fighter->u.kb.xA0);
        hash = add_object(hash, map, fighter->u.kb.xB0);
        hash = add_object(hash, map, fighter->u.kb.xB8);
        hash = add_object(hash, map, fighter->u.kb.xC0);
        hash = add_object(hash, map, fighter->u.kb.xD0);
        return add_object(hash, map, fighter->u.kb.xDC);
    case Ft_Kind_Link:
    case Ft_Kind_CLink:
        hash = add_object(hash, map, fighter->u.lk.boomerang_gobj);
        hash = add_object(hash, map, fighter->u.lk.xC);
        hash = add_object(hash, map, fighter->u.lk.arrow_gobj);
        hash = add_object(hash, map, fighter->u.lk.x14);
        return add_object(hash, map, fighter->u.lk.x18);
    case Ft_Kind_Mario:
    case Ft_Kind_DrMario:
        hash = add_object(hash, map, fighter->u.mr.x223C_capeGObj);
        return add_object(hash, map, fighter->u.mr.x2240);
    case Ft_Kind_MasterH:
        return add_object(hash, map, fighter->u.mh.x222C);
    case Ft_Kind_CrezyH:
        return add_object(hash, map, fighter->u.ch.x222C);
    case Ft_Kind_Mewtwo:
        hash = add_object(hash, map, fighter->u.mt.x222C_disableGObj);
        hash = add_object(hash, map, fighter->u.mt.x2230_shadowHeldGObj);
        return add_object(hash, map, fighter->u.mt.x2238_shadowBallGObj);
    case Ft_Kind_Ness:
        hash = add_object(hash, map, fighter->u.ns.yoyo_gobj);
        hash = add_object(hash, map, fighter->u.ns.pkflash_gobj);
        hash = add_object(hash, map, fighter->u.ns.pkthunder_gobj);
        return add_object(hash, map, fighter->u.ns.bat_gobj);
    case Ft_Kind_Peach:
        hash = add_object(hash, map, fighter->u.pe.parasol_gobj_0);
        hash = add_object(hash, map, fighter->u.pe.parasol_gobj_1);
        hash = add_object(hash, map, fighter->u.pe.toad_gobj);
        return add_object(hash, map, fighter->u.pe.veg_gobj);
    case Ft_Kind_Popo:
    case Ft_Kind_Nana:
        hash = add_object(hash, map, fighter->u.pp.x222C);
        return add_object(hash, map, fighter->u.pp.x2238);
    case Ft_Kind_Samus:
        hash = add_object(hash, map, fighter->u.ss.x222C);
        return add_object(hash, map, fighter->u.ss.x223C);
    case Ft_Kind_Seak:
        hash = add_object(hash, map, fighter->u.sk.x4);
        return add_object(hash, map, fighter->u.sk.x8);
    case Ft_Kind_Yoshi:
        return add_object(hash, map, fighter->u.ys.x2238);
    case Ft_Kind_Zelda:
        return add_object(hash, map, fighter->u.zd.x222C);
    default:
        return hash;
    }
}

static uint64_t add_fighter_motion_relations(
    uint64_t hash, const Fighter* fighter, const NativeSemanticObjectMap* map)
{
    switch (fighter->motion_id) {
    case ftCo_MS_ThrownF:
    case ftCo_MS_ThrownB:
    case ftCo_MS_ThrownHi:
    case ftCo_MS_ThrownLw:
    case ftCo_MS_ThrownlwWomen:
    case ftCo_MS_ThrownFF:
    case ftCo_MS_ThrownFB:
    case ftCo_MS_ThrownFHi:
    case ftCo_MS_ThrownFLw:
        return add_object(hash, map, fighter->mv.co.fighterthrow.victim);
    case ftCo_MS_YoshiEgg:
    case ftCo_MS_KirbyYoshiEgg:
        return add_object(hash, map, fighter->capture_enclosure.holder);
    case ftCo_MS_ThrownKirbyStar:
    case ftCo_MS_ThrownCopyStar:
    case ftCo_MS_ThrownKirby:
        return add_object(hash, map,
                          fighter->mv.co.thrownkirby.thrower_gobj);
    case ftCo_MS_CaptureLeadead:
        return add_object(hash, map, fighter->mv.co.captureleadead.x0);
    case ftCo_MS_CaptureLikelike:
        return add_object(hash, map, fighter->mv.co.capturelikelike.x0);
    case ftCo_MS_Barrel:
        return add_object(hash, map, fighter->mv.co.barrel.x8);
    default:
        return hash;
    }
}

uint64_t nativeSemanticState_AddFighterRelations(
    uint64_t hash, const Fighter* fighter, const NativeSemanticObjectMap* map)
{
    hash = add_object(hash, map, fighter->gobj);
    hash = add_object(hash, map, fighter->dmg.x1868_source);
    hash = add_object(hash, map, fighter->dmg.x1894);
    hash = add_object(hash, map, fighter->item_gobj);
    hash = add_object(hash, map, fighter->x1978);
    hash = add_object(hash, map, fighter->x1980);
    hash = add_object(hash, map, fighter->x1984_heldItemSpec);
    hash = add_object(hash, map, fighter->victim_gobj);
    hash = add_object(hash, map, fighter->x1A5C);
    hash = add_object(hash, map, fighter->target_item_gobj);
    hash = add_object(hash, map, fighter->x2094);
    hash = add_object(hash, map, fighter->unk_gobj);
    for (uint32_t i = 0; i < 4; ++i) {
        hash = add_hit_relations(hash, &fighter->x914[i], map);
    }
    for (uint32_t i = 0; i < 2; ++i) {
        hash = add_hit_relations(hash, &fighter->xDF4[i], map);
    }
    hash = add_hit_relations(hash, &fighter->x1064_thrownHitbox, map);
    hash = add_fighter_motion_relations(hash, fighter, map);
    return add_fighter_kind_relations(hash, fighter, map);
}

uint64_t nativeSemanticState_AddItemRelations(
    uint64_t hash, const Item* item, const NativeSemanticObjectMap* map)
{
    hash = add_object(hash, map, item->entity);
    hash = add_object(hash, map, item->owner);
    hash = add_object(hash, map, item->x51C);
    hash = add_object(hash, map, item->xC64_reflectGObj);
    hash = add_object(hash, map, item->xC90_absorbGObj);
    hash = add_object(hash, map, item->xCEC_fighterGObj);
    hash = add_object(hash, map, item->xCF0_itemGObj);
    hash = add_object(hash, map, item->xCF4_fighterGObjUnk);
    hash = add_object(hash, map, item->toucher);
    hash = add_object(hash, map, item->xCFC);
    hash = add_object(hash, map, item->grab_victim);
    hash = add_object(hash, map, item->atk_victim);
    for (uint32_t i = 0; i < 4; ++i) {
        hash = add_hit_relations(hash, &item->x5D4_hitboxes[i].hit, map);
    }
    if (item->kind == It_Kind_GameWatch_Rescue) {
        hash = add_object(hash, map,
                          nativeScriptItem_IsAttached(item->entity)
                              ? nativeScriptItem_AttachmentOwner(item->entity)
                              : item->xDD4_itemVar.gamewatchrescue.fighter_owner);
    }
    return hash;
}

#endif
