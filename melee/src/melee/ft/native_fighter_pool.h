#ifndef MELEE_FT_NATIVE_FIGHTER_POOL_H
#define MELEE_FT_NATIVE_FIGHTER_POOL_H
#ifdef MELEE_NATIVE
#include "types.h"
#include "ftparts.h"
#include "ftcharacter_assets.h"
#include <melee/gm/native_match_pool.h>

enum { NATIVE_FIGHTER_POOL_CAPACITY=16, NATIVE_FIGHTER_PRIMARY_DOBJS=124,
       NATIVE_FIGHTER_SECONDARY_DOBJS=32 };
typedef struct NativeFighterPoolSlot {
    _Alignas(32) Fighter actor;
    _Alignas(8) uint8_t attributes[FT_CHARACTER_EXT_ATTR_CAPACITY];
    FighterBone parts[MAX_FT_PARTS];
    HSD_DObj* primary_dobjs[NATIVE_FIGHTER_PRIMARY_DOBJS];
    HSD_DObj* secondary_dobjs[NATIVE_FIGHTER_SECONDARY_DOBJS];
} NativeFighterPoolSlot;

extern NativeMatchPool nativeFighterPool;
bool nativeFighterPool_CanBeginScene(uint64_t epoch);
bool nativeFighterPool_BeginScene(uint64_t epoch);
bool nativeFighterPool_RetireScene(void);
// A substantive owner lookup: only an occupied actor at its stable slot base
// is returned. Satellite pointers are assigned at their actual source phases.
NativeFighterPoolSlot* nativeFighterPool_Slot(Fighter*);
bool nativeFighterPool_Save(void* snapshot,size_t capacity,size_t* written);
bool nativeFighterPool_ValidateSnapshot(const void* snapshot,size_t bytes);
bool nativeFighterPool_Load(const void* snapshot,size_t bytes);
#endif
#endif
