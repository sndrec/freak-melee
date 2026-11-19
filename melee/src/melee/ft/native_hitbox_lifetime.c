#include "native_hitbox_lifetime.h"
#ifdef MELEE_NATIVE
#include <math.h>
#include <string.h>

#include "ftaction.h"
#include "ftcoll.h"
#include "types.h"
#include <dolphin/mtx.h>
#include <melee/lb/lbcollision.h>
#include <melee/lb/native_math_functions.h>

void nativeHitboxLifetime_Clear(Fighter* fighter, unsigned slot)
{
    if (slot < 4) {
        fighter->native_hitbox_lifetimes[slot] = (NativeHitboxLifetime){ 0 };
    }
}

const NativeHitboxLifetime* nativeHitboxLifetime_ForCapsule(
    const Fighter* attacker, const HitCapsule* capsule)
{
    for (unsigned i = 0; i < 4; ++i) {
        if (capsule == &attacker->x914[i]) {
            return &attacker->native_hitbox_lifetimes[i];
        }
    }
    return NULL;
}

void nativeHitboxLifetime_ResolveLaunch(const Fighter* attacker, Fighter* victim,
                                        const HitCapsule* capsule, float facing,
                                        float* angle)
{
    const NativeHitboxLifetime* hit = nativeHitboxLifetime_ForCapsule(attacker, capsule);
    if (hit == NULL) {
        return;
    }
    Vec2 launch;
    if (hit->launch.mode == NativeHitboxLaunch_Target) {
        launch.x = (attacker->cur_pos.x + attacker->facing_dir *
                    hit->launch.target.forward - victim->cur_pos.x) /
                   hit->launch.target.frames;
        launch.y = (attacker->cur_pos.y + hit->launch.target.up -
                    victim->cur_pos.y) / hit->launch.target.frames;
    } else if (hit->launch.mode == NativeHitboxLaunch_Follow) {
        if (victim->ground_or_air != GA_Air) {
            *angle = hit->launch.follow.ground_angle;
            return;
        }
        const float center_x = hit->capsule ? (capsule->x4C.x + capsule->x58.x) * 0.5f
                                           : capsule->x4C.x;
        const float center_y = hit->capsule ? (capsule->x4C.y + capsule->x58.y) * 0.5f
                                           : capsule->x4C.y;
        launch.x = attacker->self_vel.x * hit->launch.follow.momentum +
                   (center_x - victim->cur_pos.x) * hit->launch.follow.attraction;
        launch.y = attacker->self_vel.y * hit->launch.follow.momentum +
                   (center_y - victim->cur_pos.y) * hit->launch.follow.attraction;
        if (launch.y < hit->launch.follow.min_up) {
            launch.y = hit->launch.follow.min_up;
        }
    } else {
        return;
    }
    if (!isfinite(launch.x) || !isfinite(launch.y)) {
        return;
    }
    victim->native_contact_launch = launch;
    victim->native_contact_launch_active = true;
    if (launch.x != 0.0f || launch.y != 0.0f) {
        float degrees = MTXRadToDeg(cr_atan2f(launch.y, -facing * launch.x));
        *angle = degrees < 0.0f ? degrees + 360.0f : degrees;
    }
}

void nativeHitboxLifetime_BeginStep(Fighter* fighter, float frames)
{
    for (unsigned i = 0; i < 4; ++i) {
        if (fighter->native_hitbox_lifetimes[i].armed) {
            fighter->native_hitbox_lifetimes[i].remaining -= frames;
        }
    }
}

void nativeHitboxLifetime_EndStep(Fighter_GObj* object)
{
    Fighter* fighter = object->user_data;
    bool expired_all = false;
    bool any_active = false;
    for (unsigned i = 0; i < 4; ++i) {
        NativeHitboxLifetime* lifetime = &fighter->native_hitbox_lifetimes[i];
        if (lifetime->armed && lifetime->remaining <= 0) {
            expired_all |= lifetime->expire_all != 0;
            ftColl_8007AFC8(object, i);
        }
        any_active |= fighter->x914[i].state != HitCapsule_Disabled;
    }
    if (expired_all && !any_active) {
        fighter->x2219_b3 = false;
    }
}

void nativeHitboxLifetime_Spawn(Fighter_GObj* object, CommandInfo* command)
{
    Fighter* fighter = object->user_data;
    const unsigned opcode = command->u->create_hitbox_0.opcode;
    const bool timed = nativeHitbox_IsTimed(opcode);
    const bool extended = nativeHitbox_IsExtended(opcode);
    const unsigned encoded_slot = command->u->create_hitbox_0.id;
    union CmdUnion* next = command->u + (extended ? nativeHitbox_ExtendedWidth(encoded_slot) : 6);
    const unsigned slot = nativeHitbox_Slot(opcode, encoded_slot);
    if (slot >= 4 || (command->u[3].create_hitbox_3.ignore_thrown_fighters &&
                      fighter->x1064_thrownHitbox.owner == NULL))
    {
        command->u = next;
        return;
    }
    NativeHitboxLaunch launch = { 0 };
    if (extended) {
        u32 operands[5];
        memcpy(operands, command->u + NATIVE_HITBOX_EXTENSION_WORD, sizeof(operands));
        launch = nativeHitboxLaunch_FromWords(operands);
    }
    uint32_t word = 0;
    if (timed || extended) {
        memcpy(&word, command->u + 5, sizeof(word));
    }
    if (!timed) {
        ftAction_8007121C(object, command);
        NativeHitboxLifetime* lifetime = &fighter->native_hitbox_lifetimes[slot];
        lifetime->launch = launch;
        lifetime->exclude_grabbed = (word & NATIVE_HITBOX_EXCLUDE_GRABBED) != 0;
        command->u = next;
        return;
    }
    const float length = (word & NATIVE_HITBOX_DURATION_MASK) / 256.0f;
    const NativeHitboxLifetime previous =
        fighter->native_hitbox_lifetimes[slot];
    const bool handoff =
        previous.armed &&
        fabsf(previous.remaining - command->timer) < 0.00001f &&
        fighter->x914[slot].state != HitCapsule_Disabled;
    HitCapsule history;
    if (handoff && !(word & NATIVE_HITBOX_REFRESH)) {
        lbColl_CopyHitCapsule(&fighter->x914[slot], &history);
    }
    if (!handoff || (word & NATIVE_HITBOX_REFRESH)) {
        fighter->x914[slot].state = HitCapsule_Disabled;
    }
    ftAction_8007121C(object, command);
    // A conditional spawn may have deliberately skipped the capsule.
    if (fighter->x914[slot].state != HitCapsule_Disabled) {
        if (handoff && !(word & NATIVE_HITBOX_REFRESH)) {
            lbColl_CopyHitCapsule(&history, &fighter->x914[slot]);
        } else {
            lbColl_80008440(&fighter->x914[slot]);
        }
        NativeHitboxLifetime* lifetime = &fighter->native_hitbox_lifetimes[slot];
        lifetime->remaining = length + command->timer;
        lifetime->armed = 1;
        lifetime->expire_all = (word & NATIVE_HITBOX_EXPIRE_ALL) != 0;
        lifetime->exclude_grabbed = (word & NATIVE_HITBOX_EXCLUDE_GRABBED) != 0;
        lifetime->launch = launch;
    }
    command->u = next;
}

void nativeHitboxLifetime_Skip(Fighter_GObj* object, CommandInfo* command)
{
    (void) object;
    command->u += nativeHitbox_IsExtended(command->u->create_hitbox_0.opcode)
                      ? nativeHitbox_ExtendedWidth(command->u->create_hitbox_0.id) : 6;
}
#endif
