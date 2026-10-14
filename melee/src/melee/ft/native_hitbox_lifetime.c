#include "native_hitbox_lifetime.h"
#ifdef MELEE_NATIVE
#include <math.h>
#include <string.h>

#include "ftaction.h"
#include "ftcoll.h"
#include "types.h"
#include <melee/lb/lbcollision.h>

void nativeHitboxLifetime_Clear(Fighter* fighter, unsigned slot)
{
    if (slot < 4) {
        fighter->native_hitbox_lifetimes[slot] = (NativeHitboxLifetime){ 0 };
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
    const unsigned slot = command->u->create_hitbox_0.id;
    const bool victim_only = command->u->create_hitbox_0.only_hit_grabbed;
    if (slot >= 4 || (command->u[3].create_hitbox_3.ignore_thrown_fighters &&
                      fighter->x1064_thrownHitbox.owner == NULL))
    {
        command->u += 6;
        return;
    }
    uint32_t word;
    memcpy(&word, command->u + 5, sizeof(word));
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
    union CmdUnion* next = command->u + 6;
    ftAction_8007121C(object, command);
    // A conditional spawn may have deliberately skipped the capsule.
    if (fighter->x914[slot].state != HitCapsule_Disabled) {
        // Retail reads this flag from the following command. Timed commands
        // own it explicitly in their header; the duration is not a command.
        fighter->x914[slot].hit_grabbed_victim_only = victim_only;
        if (handoff && !(word & NATIVE_HITBOX_REFRESH)) {
            lbColl_CopyHitCapsule(&history, &fighter->x914[slot]);
        } else {
            lbColl_80008440(&fighter->x914[slot]);
        }
        fighter->native_hitbox_lifetimes[slot] = (NativeHitboxLifetime){
            .remaining = length + command->timer,
            .armed = 1,
            .expire_all = (word & NATIVE_HITBOX_EXPIRE_ALL) != 0,
        };
    }
    command->u = next;
}

void nativeHitboxLifetime_Skip(Fighter_GObj* object, CommandInfo* command)
{
    (void) object;
    command->u += 6;
}
#endif
