#include <melee/ft/native_hitbox_lifetime.h>
#include <melee/ft/types.h>
#include <melee/lb/lbcollision.h>
#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)

// Engine boundaries are controlled here; the production lifetime functions
// run unchanged. The create boundary deliberately replaces victim history.
void ftAction_8007121C(Fighter_GObj* object, CommandInfo* command)
{
    Fighter* fighter = object->user_data;
    unsigned slot = command->u->create_hitbox_0.id;
    nativeHitboxLifetime_Clear(fighter, slot);
    memset(&fighter->x914[slot], 0, sizeof(HitCapsule));
    fighter->x914[slot].state = HitCapsule_Enabled;
    fighter->x914[slot].hit_grabbed_victim_only = command->u->create_hitbox_0.only_hit_grabbed;
    command->u += 5;
}

void ftColl_8007AFC8(Fighter_GObj* object, int slot)
{
    Fighter* fighter = object->user_data;
    fighter->x914[slot].state = HitCapsule_Disabled;
    nativeHitboxLifetime_Clear(fighter, slot);
}

void lbColl_CopyHitCapsule(HitCapsule* source, HitCapsule* destination)
{
    memcpy(destination->victims_1, source->victims_1, sizeof(source->victims_1));
    memcpy(destination->victims_2, source->victims_2, sizeof(source->victims_2));
    destination->x44 = source->x44;
    destination->x45 = source->x45;
}

void lbColl_80008440(HitCapsule* capsule)
{
    memset(capsule->victims_1, 0, sizeof(capsule->victims_1));
    memset(capsule->victims_2, 0, sizeof(capsule->victims_2));
    capsule->x44 = capsule->x45 = 0;
}

static void spawn(Fighter_GObj* object, float timer, uint32_t life)
{
    union CmdUnion words[6] = { 0 };
    words[0].create_hitbox_0.opcode = NATIVE_HITBOX_TIMED_OPCODE;
    words[0].create_hitbox_0.id = 0;
    memcpy(words + 5, &life, sizeof(life));
    CommandInfo command = { .timer = timer, .u = words };
    nativeHitboxLifetime_Spawn(object, &command);
}

static void victim(Fighter* fighter)
{
    fighter->x914[0].victims_1[0] = (HitVictim){ .victim = fighter, .x4 = 17 };
    fighter->x914[0].victims_2[0] = (HitVictim){ .victim = fighter, .x4 = 23 };
    fighter->x914[0].x44 = fighter->x914[0].x45 = 1;
}

int main(void)
{
    static Fighter fighter;
    static Fighter checkpoint;
    Fighter_GObj object = { .user_data = &fighter };
    spawn(&object, 0, 512 | NATIVE_HITBOX_EXPIRE_ALL | NATIVE_HITBOX_EXCLUDE_GRABBED);
    fighter.x2219_b3 = true;
    CHECK(fighter.native_hitbox_lifetimes[0].remaining == 2);
    CHECK(fighter.native_hitbox_lifetimes[0].exclude_grabbed);
    CHECK(!fighter.x914[0].hit_grabbed_victim_only);
    nativeHitboxLifetime_BeginStep(&fighter, 0.5f);
    nativeHitboxLifetime_EndStep(&object);
    CHECK(fighter.x914[0].state != HitCapsule_Disabled);
    CHECK(fighter.native_hitbox_lifetimes[0].remaining == 1.5f);
    // Hitlag does not run an animation step. Remaining time survives saves.
    checkpoint = fighter;
    nativeHitboxLifetime_BeginStep(&fighter, 1.5f);
    nativeHitboxLifetime_EndStep(&object);
    CHECK(fighter.x914[0].state == HitCapsule_Disabled);
    CHECK(!fighter.x2219_b3);
    fighter = checkpoint;
    CHECK(fighter.native_hitbox_lifetimes[0].remaining == 1.5f);
    CHECK(fighter.native_hitbox_lifetimes[0].exclude_grabbed);

    nativeHitboxLifetime_BeginStep(&fighter, 1.5f);
    victim(&fighter);
    spawn(&object, 0, 256);
    nativeHitboxLifetime_EndStep(&object);
    CHECK(fighter.x914[0].state != HitCapsule_Disabled);
    CHECK(!fighter.native_hitbox_lifetimes[0].exclude_grabbed);
    CHECK(fighter.x914[0].victims_1[0].victim == &fighter);
    CHECK(fighter.x914[0].victims_2[0].x4 == 23);
    CHECK(fighter.x914[0].x44 == 1 && fighter.x914[0].x45 == 1);

    nativeHitboxLifetime_BeginStep(&fighter, 1);
    spawn(&object, 0, 256 | NATIVE_HITBOX_REFRESH);
    CHECK(fighter.x914[0].victims_1[0].victim == NULL);
    CHECK(fighter.x914[0].x44 == 0);

    // A fast step crosses the handoff by half a frame. History still carries.
    victim(&fighter);
    nativeHitboxLifetime_BeginStep(&fighter, 1.5f);
    spawn(&object, -0.5f, 512);
    CHECK(fighter.x914[0].victims_1[0].victim == &fighter);
    CHECK(fighter.native_hitbox_lifetimes[0].remaining == 1.5f);
    // A gap crossed in that same fast step starts fresh despite expiry being
    // deferred until after command execution.
    nativeHitboxLifetime_BeginStep(&fighter, 2);
    spawn(&object, -0.25f, 512);
    CHECK(fighter.x914[0].victims_1[0].victim == NULL);
    CHECK(fighter.native_hitbox_lifetimes[0].remaining == 1.75f);

    // A regular create or explicit clear cancels the old clock.
    union CmdUnion words[6] = { 0 };
    CommandInfo command = { .u = words };
    ftAction_8007121C(&object, &command);
    nativeHitboxLifetime_BeginStep(&fighter, 20);
    nativeHitboxLifetime_EndStep(&object);
    CHECK(fighter.x914[0].state != HitCapsule_Disabled);
    spawn(&object, 0, 256);
    ftColl_8007AFC8(&object, 0);
    CHECK(!fighter.native_hitbox_lifetimes[0].armed);

    union CmdUnion manual[NATIVE_HITBOX_EXTENSION_WORDS] = { 0 };
    manual[0].create_hitbox_0.opcode = NATIVE_HITBOX_EXPLICIT_EXTENDED_OPCODE;
    const uint32_t flags = NATIVE_HITBOX_EXCLUDE_GRABBED;
    memcpy(manual + 5, &flags, sizeof(flags));
    CommandInfo explicit_clear = { .u = manual };
    nativeHitboxLifetime_Spawn(&object, &explicit_clear);
    CHECK(explicit_clear.u == manual + NATIVE_HITBOX_EXTENSION_WORDS);
    CHECK(fighter.native_hitbox_lifetimes[0].exclude_grabbed);
    CHECK(!fighter.native_hitbox_lifetimes[0].armed);
    nativeHitboxLifetime_BeginStep(&fighter, 20);
    nativeHitboxLifetime_EndStep(&object);
    CHECK(fighter.x914[0].state != HitCapsule_Disabled);
    ftColl_8007AFC8(&object, 0);
    CHECK(!fighter.native_hitbox_lifetimes[0].exclude_grabbed);

    static Fighter target;
    fighter.facing_dir = -1;
    fighter.cur_pos = (Vec3){ 10, 2, 0 };
    target.cur_pos = (Vec3){ 8, 7, 0 };
    spawn(&object, 0, 256);
    NativeHitboxLifetime* hit = &fighter.native_hitbox_lifetimes[0];
    hit->launch.mode = NativeHitboxLaunch_Target;
    hit->launch.target.forward = 7;
    hit->launch.target.up = 20;
    hit->launch.target.frames = 5;
    float angle = 70;
    nativeHitboxLifetime_ResolveLaunch(&fighter, &target, &fighter.x914[0], 1, &angle);
    CHECK(target.native_contact_launch_active);
    CHECK(target.native_contact_launch.x == -1 && target.native_contact_launch.y == 3);
    CHECK(angle > 71 && angle < 72);

    hit->launch.mode = NativeHitboxLaunch_Follow;
    hit->launch.follow.momentum = 1;
    hit->launch.follow.attraction = 0.25f;
    hit->launch.follow.min_up = -1;
    hit->launch.follow.ground_angle = 80;
    fighter.self_vel = (Vec3){ -2, 3, 0 };
    fighter.x914[0].x4C = (Vec3){ 6, 3, 0 };
    target.native_contact_launch_active = false;
    target.ground_or_air = GA_Air;
    nativeHitboxLifetime_ResolveLaunch(&fighter, &target, &fighter.x914[0], 1, &angle);
    CHECK(target.native_contact_launch_active);
    CHECK(target.native_contact_launch.x == -2.5f && target.native_contact_launch.y == 2);
    fighter.self_vel.y = -3;
    nativeHitboxLifetime_ResolveLaunch(&fighter, &target, &fighter.x914[0], 1, &angle);
    CHECK(target.native_contact_launch.y == -1);

    target.native_contact_launch_active = false;
    target.ground_or_air = GA_Ground;
    nativeHitboxLifetime_ResolveLaunch(&fighter, &target, &fighter.x914[0], 1, &angle);
    CHECK(angle == 80 && !target.native_contact_launch_active);
    hit->launch.follow.override_ground_knockback = true;
    hit->launch.follow.ground_angle = 359.75f;
    hit->launch.follow.ground_growth = 511;
    hit->launch.follow.ground_base = 509;
    uint32_t encoded[5];
    u32 operands[5];
    nativeHitboxLaunch_ToWords(&hit->launch, 0.5f, encoded);
    memcpy(operands, encoded, sizeof(operands));
    NativeHitboxLaunch decoded = nativeHitboxLaunch_FromWords(operands);
    CHECK(nativeHitbox_DamageFraction(operands[0]) == 0.5f);
    CHECK(nativeHitboxLaunch_IsValid(&decoded));
    CHECK(decoded.mode == NativeHitboxLaunch_Follow);
    CHECK(decoded.follow.override_ground_knockback);
    CHECK(decoded.follow.ground_angle == 359.75f);
    CHECK(decoded.follow.ground_growth == 511 && decoded.follow.ground_base == 509);
    spawn(&object, 0, 256);
    CHECK(hit->launch.mode == NativeHitboxLaunch_Default);
    angle = 67;
    nativeHitboxLifetime_ResolveLaunch(&fighter, &target, &fighter.x914[0], 1, &angle);
    CHECK(angle == 67 && !target.native_contact_launch_active);

    puts("Hitbox lifetime: expiry, handoff, refresh, snapshot and contact launch passed.");
    return 0;
}
