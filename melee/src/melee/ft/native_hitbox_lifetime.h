#ifndef MELEE_FT_NATIVE_HITBOX_LIFETIME_H
#define MELEE_FT_NATIVE_HITBOX_LIFETIME_H
#include <melee/ft/forward.h>
#ifdef MELEE_NATIVE
#include <math.h>
#include <stdint.h>
#include <string.h>
enum {
    NATIVE_HITBOX_TIMED_OPCODE = 59,
    NATIVE_HITBOX_EXPLICIT_OPCODE = 60,
    NATIVE_HITBOX_TIMED_EXTENDED_OPCODE = 61,
    NATIVE_HITBOX_EXPLICIT_EXTENDED_OPCODE = 62,
    NATIVE_HITBOX_EXTENSION_WORD = 6,
    NATIVE_HITBOX_EXTENSION_WORDS = 11,
    NATIVE_HITBOX_CAPSULE_FLAG = 4,
    NATIVE_HITBOX_CAPSULE_WORDS = NATIVE_HITBOX_EXTENSION_WORDS + 2,
    NATIVE_HITBOX_MAX_WORDS = NATIVE_HITBOX_CAPSULE_WORDS,
    NATIVE_HITBOX_LAUNCH_MASK = 3,
    NATIVE_HITBOX_ENCODED_FOLLOW_GROUND = 3,
    NATIVE_HITBOX_REFRESH = 0x80000000u,
    NATIVE_HITBOX_EXPIRE_ALL = 0x40000000u,
    NATIVE_HITBOX_EXCLUDE_GRABBED = 0x20000000u,
    NATIVE_HITBOX_DURATION_MASK = 0x1fffffffu,
};
typedef enum NativeHitboxLaunchMode {
    NativeHitboxLaunch_Default,
    NativeHitboxLaunch_Target,
    NativeHitboxLaunch_Follow,
} NativeHitboxLaunchMode;

typedef struct NativeHitboxLaunch {
    uint32_t mode;
    union {
        struct { float forward, up, frames, reserved; } target;
        struct {
            float momentum, attraction, min_up, ground_angle;
            uint16_t ground_growth, ground_base;
            bool override_ground_knockback;
        } follow;
    };
} NativeHitboxLaunch;

typedef struct NativeHitboxLifetime {
    float remaining;
    unsigned char armed, expire_all, exclude_grabbed;
    bool capsule;
    NativeHitboxLaunch launch;
    Vec3 end_offset;
    Vec3 previous_start;
} NativeHitboxLifetime;
// Owned hitboxes keep their victim-only flag in the header. Opcode 60 retains
// ordinary clear-command lifetime and group-history behavior; 59 owns duration.
static inline bool nativeHitbox_IsCreate(unsigned opcode)
{ return opcode == 11 || (opcode >= NATIVE_HITBOX_TIMED_OPCODE && opcode <= NATIVE_HITBOX_EXPLICIT_EXTENDED_OPCODE); }
static inline bool nativeHitbox_OwnsFlags(unsigned opcode)
{ return opcode >= NATIVE_HITBOX_TIMED_OPCODE && opcode <= NATIVE_HITBOX_EXPLICIT_EXTENDED_OPCODE; }
static inline bool nativeHitbox_IsTimed(unsigned opcode)
{ return opcode == NATIVE_HITBOX_TIMED_OPCODE || opcode == NATIVE_HITBOX_TIMED_EXTENDED_OPCODE; }
static inline bool nativeHitbox_IsExtended(unsigned opcode)
{ return opcode == NATIVE_HITBOX_TIMED_EXTENDED_OPCODE || opcode == NATIVE_HITBOX_EXPLICIT_EXTENDED_OPCODE; }

// The four fighter attack slots use two bits. Extended commands use the
// third bit to append two words with the second endpoint.
static inline bool nativeHitbox_HasCapsule(unsigned opcode, unsigned encoded_slot)
{
    return nativeHitbox_IsExtended(opcode) &&
           (encoded_slot & NATIVE_HITBOX_CAPSULE_FLAG);
}

static inline unsigned nativeHitbox_Slot(unsigned opcode, unsigned encoded_slot)
{
    return nativeHitbox_IsExtended(opcode) ? encoded_slot & 3 : encoded_slot;
}

static inline unsigned nativeHitbox_ExtendedWidth(unsigned encoded_slot)
{
    return (encoded_slot & NATIVE_HITBOX_CAPSULE_FLAG)
               ? NATIVE_HITBOX_CAPSULE_WORDS : NATIVE_HITBOX_EXTENSION_WORDS;
}

// The header keeps the integer damage. A nonnegative IEEE 754 fraction
// below one fits in the remaining 30 bits above the launch mode.
static inline float nativeHitbox_DamageFraction(u32 extension)
{
    const u32 bits = extension >> 2;
    float fraction;
    memcpy(&fraction, &bits, sizeof(fraction));
    return fraction;
}

static inline NativeHitboxLaunch nativeHitboxLaunch_Defaults(uint32_t mode)
{
    NativeHitboxLaunch launch = { 0 };
    launch.mode = mode;
    if (mode == NativeHitboxLaunch_Target) {
        launch.target.frames = 5;
    } else if (mode == NativeHitboxLaunch_Follow) {
        launch.follow.momentum = 1;
        launch.follow.attraction = 0.2f;
        launch.follow.min_up = -1;
        launch.follow.ground_angle = 80;
    }
    return launch;
}

// The first three follow operands are floats. With separate ground knockback,
// the last word holds a 14-bit angle in 1/32-degree units and two 9-bit values.
static inline NativeHitboxLaunch nativeHitboxLaunch_FromWords(const u32* words)
{
    NativeHitboxLaunch launch = { 0 };
    unsigned encoded = words[0] & NATIVE_HITBOX_LAUNCH_MASK;
    launch.mode = encoded;
    if (launch.mode == NativeHitboxLaunch_Target)
        memcpy(&launch.target, words + 1, sizeof(launch.target));
    else if (encoded == NativeHitboxLaunch_Follow ||
             encoded == NATIVE_HITBOX_ENCODED_FOLLOW_GROUND)
    {
        launch.mode = NativeHitboxLaunch_Follow;
        memcpy(&launch.follow.momentum, words + 1, 3 * sizeof(u32));
        if (encoded == NATIVE_HITBOX_ENCODED_FOLLOW_GROUND) {
            launch.follow.ground_angle = (words[4] & 0x3fffu) / 32.0f;
            launch.follow.ground_growth = (words[4] >> 14) & 0x1ffu;
            launch.follow.ground_base = words[4] >> 23;
            launch.follow.override_ground_knockback = true;
        } else {
            memcpy(&launch.follow.ground_angle, words + 4, sizeof(u32));
        }
    }
    return launch;
}

static inline void nativeHitboxLaunch_ToWords(const NativeHitboxLaunch* launch,
                                             float damage_fraction, uint32_t* words)
{
    u32 fraction;
    memcpy(&fraction, &damage_fraction, sizeof(fraction));
    words[0] = (fraction << 2) | launch->mode;
    if (launch->mode == NativeHitboxLaunch_Target) {
        memcpy(words + 1, &launch->target, sizeof(launch->target));
    } else if (launch->mode == NativeHitboxLaunch_Follow) {
        memcpy(words + 1, &launch->follow.momentum, 3 * sizeof(u32));
        if (launch->follow.override_ground_knockback) {
            words[0] = (fraction << 2) | NATIVE_HITBOX_ENCODED_FOLLOW_GROUND;
            words[4] = (u32)(launch->follow.ground_angle * 32.0f) |
                       ((u32) launch->follow.ground_growth << 14) |
                       ((u32) launch->follow.ground_base << 23);
        } else {
            memcpy(words + 4, &launch->follow.ground_angle, sizeof(u32));
        }
    } else {
        memset(words + 1, 0, 4 * sizeof(u32));
    }
}

static inline bool nativeHitboxLaunch_IsValid(const NativeHitboxLaunch* launch)
{
    switch (launch->mode) {
    case NativeHitboxLaunch_Default:
        return true;
    case NativeHitboxLaunch_Target:
        return isfinite(launch->target.forward) && isfinite(launch->target.up) &&
               isfinite(launch->target.frames) && launch->target.frames >= 1 &&
               launch->target.reserved == 0.0f;
    case NativeHitboxLaunch_Follow:
        return isfinite(launch->follow.momentum) &&
               isfinite(launch->follow.attraction) && launch->follow.attraction >= 0 &&
               isfinite(launch->follow.min_up) &&
               launch->follow.ground_angle >= 0 && launch->follow.ground_angle < 360 &&
               (!launch->follow.override_ground_knockback ||
                (launch->follow.ground_growth <= 511 && launch->follow.ground_base <= 511));
    default:
        return false;
    }
}
void nativeHitboxLifetime_BeginStep(Fighter*, float frames);
void nativeHitboxLifetime_EndStep(Fighter_GObj*);
void nativeHitboxLifetime_Clear(Fighter*, unsigned slot);
void nativeHitboxLifetime_Spawn(Fighter_GObj*, CommandInfo*);
void nativeHitboxLifetime_Skip(Fighter_GObj*, CommandInfo*);
const NativeHitboxLifetime* nativeHitboxLifetime_ForCapsule(const Fighter*,
                                                            const HitCapsule*);
void nativeHitboxLifetime_ResolveLaunch(const Fighter* attacker, Fighter* victim,
                                        const HitCapsule*, float facing, float* angle);
#endif
#endif
