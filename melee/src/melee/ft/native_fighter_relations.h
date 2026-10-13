#ifndef MELEE_FT_NATIVE_FIGHTER_RELATIONS_H
#define MELEE_FT_NATIVE_FIGHTER_RELATIONS_H
#include <melee/ft/forward.h>
#include <stdbool.h>
#include <stdint.h>

// References are complete creation IDs, never retained authored GObj pointers.
bool nativeFighterRelations_Link(Fighter_GObj*, uint32_t target);
uint32_t nativeFighterRelations_LinkedId(Fighter_GObj*);
bool nativeFighterRelations_CopyCollision(Fighter_GObj* destination, uint32_t source);
#endif
