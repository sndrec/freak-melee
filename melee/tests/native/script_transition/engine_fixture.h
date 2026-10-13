#ifndef SCRIPT_TRANSITION_ENGINE_FIXTURE_H
#define SCRIPT_TRANSITION_ENGINE_FIXTURE_H
#include <melee/ft/types.h>
typedef struct ScriptTransitionChange {uint32_t motion,flags;float frame,rate;} ScriptTransitionChange;
extern ScriptTransitionChange script_transition_changes[32];
extern unsigned script_state_changes,script_state_hooks;
void scriptStateFixture_Reset(void);
void scriptStateFixture_RetireOnChange(bool);
#endif
