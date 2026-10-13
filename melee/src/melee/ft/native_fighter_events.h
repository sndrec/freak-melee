#ifndef MELEE_FT_NATIVE_FIGHTER_EVENTS_H
#define MELEE_FT_NATIVE_FIGHTER_EVENTS_H

#include "native_fighter_runtime.h"

bool nativeFighterEvents_Validate(const NativeFighterDefinition*);
bool nativeFighterEvent_Call(uint32_t target, uint32_t name_hash);
bool nativeFighterEvent_CallBool(uint32_t target, uint32_t name_hash);
int32_t nativeFighterEvent_CallInt(uint32_t target, uint32_t name_hash);
float nativeFighterEvent_CallFloat(uint32_t target, uint32_t name_hash);

#endif
