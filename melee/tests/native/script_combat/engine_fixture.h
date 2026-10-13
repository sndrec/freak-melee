#ifndef SCRIPT_COMBAT_ENGINE_FIXTURE_H
#define SCRIPT_COMBAT_ENGINE_FIXTURE_H
#include <melee/ft/types.h>
extern unsigned script_combat_free_count,script_combat_walk_count,script_combat_color;
extern int script_combat_hp,script_combat_heal_credit;
extern float script_combat_damage_credit,script_combat_damage_percent;
void scriptCombatFixture_Reset(void);
void scriptCombatFixture_Retire(HSD_GObj* source,unsigned stage,bool replace);
s32 scriptCombatFixture_random_int(s32 upper);
s32 scriptCombatFixture_random_int_excluding(s32 upper,s32 first,s32 second);
#endif
