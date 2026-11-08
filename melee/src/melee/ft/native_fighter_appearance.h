#pragma once
#include <melee/ft/forward.h>
#include <stdbool.h>
bool nativeFighterAppearance_IsFiltered(const Fighter* fighter);
void nativeFighterAppearance_Draw(Fighter_GObj* actor,int pass);
