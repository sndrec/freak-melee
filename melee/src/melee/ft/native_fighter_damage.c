#include "native_fighter_damage.h"
#include "fighter.h"
#include "types.h"
#include <melee/pl/player.h>
#include <melee/pl/plbonuslib.h>
#include <sysdolphin/baselib/gobj.h>

void nativeFighterDamage_Self(Fighter_GObj* object, float amount)
{
    if (amount > 0.0f) Fighter_TakeDamage_8006CC7C(object->user_data, amount);
}

void nativeFighterDamage_Heal(Fighter_GObj* object, float amount)
{
    Fighter* fighter = object->user_data;
    float actual = amount;
    fighter->dmg.x1830_percent -= actual;
    if (fighter->dmg.x1830_percent < 0) {
        actual = fighter->dmg.x1830_percent + actual;
        fighter->dmg.x1830_percent = 0;
    }
    Player_SetHPByIndex(fighter->player_id, fighter->x221F_b4,
                       fighter->dmg.x1830_percent);
    pl_80040B8C(fighter->player_id, fighter->x221F_b4, actual);
}
