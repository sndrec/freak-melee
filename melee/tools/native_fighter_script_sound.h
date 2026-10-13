#ifndef NATIVE_FIGHTER_SCRIPT_SOUND_H
#define NATIVE_FIGHTER_SCRIPT_SOUND_H

#include "native_fighter_script_code.h"

// 0: another statement, 1: emitted, -1: malformed or invalid sound.channel.
int nativeScriptSound_Action(const ScriptCodeBlock*, const char*, char*, size_t);

#endif
