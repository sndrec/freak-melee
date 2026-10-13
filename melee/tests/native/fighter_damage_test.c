#include <melee/ft/native_fighter_damage.h>
#include <melee/ft/types.h>
#include <melee/ft/fighter.h>
#include <melee/pl/player.h>
#include <melee/pl/plbonuslib.h>
#include <sysdolphin/baselib/gobj.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

static Fighter fighter;
static unsigned calls;
static float self_damage;
static int displayed, healed;
void Player_SetHPByIndex(s32 player, s32 partner, s32 value)
{
    assert(calls++ == 0 && player == 2 && partner);
    assert(value == (int)fighter.dmg.x1830_percent);
    displayed = value;
}
void pl_80040B8C(int player, int partner, int value)
{
    assert(calls++ == 1 && player == 2 && partner);
    healed = value;
}
void Fighter_TakeDamage_8006CC7C(Fighter* target, float amount)
{
    assert(target == &fighter);
    self_damage = amount;
}
int main(void)
{
    HSD_GObj object = { .user_data = &fighter };
    fighter.player_id = 2;
    fighter.x221F_b4 = true;
    const float values[] = { 0, 0.125f, 1, 2.75f, 17, 50.25f, 999 };
    unsigned comparisons = 0;
    for (unsigned p = 0; p < sizeof(values)/sizeof(values[0]); ++p)
        for (unsigned h = 0; h < sizeof(values)/sizeof(values[0]); ++h) {
            // Original OnAbsorb arithmetic, including float subtraction before
            // the clamp and integer conversion only at the bookkeeping call.
            float expected = values[p], amount = values[h];
            expected -= amount;
            if (expected < 0) { amount = expected + amount; expected = 0; }
            fighter.dmg.x1830_percent = values[p];
            calls = 0;
            nativeFighterDamage_Heal(&object, values[h]);
            assert(calls == 2 && displayed == (int)expected &&
                   !memcmp(&expected, &fighter.dmg.x1830_percent, sizeof(float)));
            assert(healed == (int)amount);
            ++comparisons;
        }
    self_damage = 0;
    nativeFighterDamage_Self(&object, 0);
    nativeFighterDamage_Self(&object, -1);
    assert(self_damage == 0);
    nativeFighterDamage_Self(&object, 2);
    assert(self_damage == 2);
    printf("%u healing arithmetic and ordered-bookkeeping comparisons pass\n", comparisons);
    return 0;
}
