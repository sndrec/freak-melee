#ifndef SCRIPT_STATE_ENGINE_FIXTURE_H
#define SCRIPT_STATE_ENGINE_FIXTURE_H
#include <melee/ft/types.h>
void scriptStateFixture_Reset(void);
void scriptStateFixture_RetireOnChange(bool);
extern unsigned script_state_changes,script_state_hooks;
extern unsigned script_collision_calls,script_collision_mode,script_collision_retire;
extern ftCollisionBox script_collision_box;
extern bool script_collision_owned;
#endif
