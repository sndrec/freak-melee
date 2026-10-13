#include <melee/ft/kinds/ftCommon/ftCo_HammerWait.h>
static bool damage_tick_case;
static unsigned damage_exit_calls;
// The authored knockback-exit callback is controlled; its retirement effect
// exercises the actual caller's program/storage continuation guard.
void ftCommon_8007F86C(Fighter_GObj* actor)
{CHECK(damage_tick_case);++damage_exit_calls;step(33,actor);}
