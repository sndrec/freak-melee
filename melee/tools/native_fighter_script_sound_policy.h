#ifndef NATIVE_FIGHTER_SCRIPT_SOUND_POLICY_H
#define NATIVE_FIGHTER_SCRIPT_SOUND_POLICY_H
#include "native_fighter_script.h"
#include <melee/ft/native_fighter_sound_policy.h>

typedef struct ScriptSoundPolicy {
    bool declared;
    uint8_t mute_channels;
    NativeFighterSoundIdRule rules[NATIVE_FIGHTER_SOUND_RULE_CAPACITY];
    NativeFighterKoCue ko_cues[NATIVE_FIGHTER_KO_CUE_CAPACITY];
    uint16_t rule_count;
    uint16_t ko_cue_count;
} ScriptSoundPolicy;

bool nativeScriptSoundPolicy_Parse(char* source, size_t size,
    ScriptSoundPolicy*, NativeFighterScriptError*);
void nativeScriptSoundPolicy_EmitDefinition(const ScriptSoundPolicy*, FILE*);
#endif
