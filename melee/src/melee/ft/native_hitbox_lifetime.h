#ifndef MELEE_FT_NATIVE_HITBOX_LIFETIME_H
#define MELEE_FT_NATIVE_HITBOX_LIFETIME_H
#include <melee/ft/forward.h>
#ifdef MELEE_NATIVE
#include <stdint.h>
enum {
    NATIVE_HITBOX_TIMED_OPCODE = 59,
    NATIVE_HITBOX_EXPLICIT_OPCODE = 60,
    NATIVE_HITBOX_REFRESH = 0x80000000u,
    NATIVE_HITBOX_EXPIRE_ALL = 0x40000000u,
    NATIVE_HITBOX_DURATION_MASK = 0x3fffffffu,
};
typedef struct NativeHitboxLifetime {
    float remaining;
    unsigned char armed, expire_all;
} NativeHitboxLifetime;
// Owned hitboxes keep their victim-only flag in the header. Opcode 60 retains
// ordinary clear-command lifetime and group-history behavior; 59 owns duration.
static inline bool nativeHitbox_IsCreate(unsigned opcode)
{ return opcode == 11 || opcode == NATIVE_HITBOX_TIMED_OPCODE || opcode == NATIVE_HITBOX_EXPLICIT_OPCODE; }
static inline bool nativeHitbox_OwnsFlags(unsigned opcode)
{ return opcode == NATIVE_HITBOX_TIMED_OPCODE || opcode == NATIVE_HITBOX_EXPLICIT_OPCODE; }
void nativeHitboxLifetime_BeginStep(Fighter*, float frames);
void nativeHitboxLifetime_EndStep(Fighter_GObj*);
void nativeHitboxLifetime_Clear(Fighter*, unsigned slot);
void nativeHitboxLifetime_Spawn(Fighter_GObj*, CommandInfo*);
void nativeHitboxLifetime_Skip(Fighter_GObj*, CommandInfo*);
#endif
#endif
