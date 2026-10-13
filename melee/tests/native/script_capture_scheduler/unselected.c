// Unselected observations added since this fixture was first written. Reaching
// any of these is a fixture error, not a simulated implementation.
#include <melee/ft/fighter.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/kinds/ftCommon/ftCo_DownBound.h>
#include <melee/pl/player.h>
#include <melee/pl/plbonus.h>
#include <stdlib.h>
__typeof__(Fighter_SmashChargeShakeTable) Fighter_SmashChargeShakeTable;
void ftCo_80097D40(Fighter_GObj* object) { (void)object;abort(); }
void ft_800851C0(Fighter_GObj* object) { (void)object;abort(); }
plActionStats* Player_GetActionStats(int slot) { (void)slot;abort(); }
unsigned int pl_800386D8(plActionStats* stats,ssize_t index)
{ (void)stats;(void)index;abort(); }
