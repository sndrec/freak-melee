#ifndef SCRIPT_SETUP_ENGINE_FIXTURE_H
#define SCRIPT_SETUP_ENGINE_FIXTURE_H
#include <melee/ft/native_script_fighter.h>
void scriptSetupFixture_Reset(void);
void scriptSetupFixture_Held(bool heavy,int kind);
void scriptSetupFixture_RetireAt(unsigned stage);
extern unsigned script_setup_count,script_setup_log[16][3];
extern uint32_t script_setup_callbacks[NativeScriptFighterCallback_Count];
#endif
