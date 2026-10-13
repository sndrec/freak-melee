#ifndef MELEE_FT_NATIVE_HITBOX_LIFETIME_H
#define MELEE_FT_NATIVE_HITBOX_LIFETIME_H
#include <melee/ft/forward.h>
#ifdef MELEE_NATIVE
#include <stdint.h>
enum {
    NATIVE_HITBOX_TIMED_OPCODE = 59,
    NATIVE_HITBOX_REFRESH = 0x80000000u,
    NATIVE_HITBOX_EXPIRE_ALL = 0x40000000u,
    NATIVE_HITBOX_DURATION_MASK = 0x3fffffffu,
};
typedef struct NativeHitboxLifetime {
    float remaining;
    unsigned char armed, expire_all;
} NativeHitboxLifetime;
void nativeHitboxLifetime_BeginStep(Fighter*, float frames);
void nativeHitboxLifetime_EndStep(Fighter_GObj*);
void nativeHitboxLifetime_Clear(Fighter*, unsigned slot);
void nativeHitboxLifetime_Spawn(Fighter_GObj*, CommandInfo*);
void nativeHitboxLifetime_Skip(Fighter_GObj*, CommandInfo*);
#endif
#endif
