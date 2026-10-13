#ifndef NATIVE_FIGHTER_SOUND_POLICY_H
#define NATIVE_FIGHTER_SOUND_POLICY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    NATIVE_FIGHTER_SOUND_RULE_CAPACITY = 16,
    NATIVE_FIGHTER_KO_CUE_CAPACITY = 8,
};

// Rules apply in declaration order to the canonical sound ID. The bank stays
// fixed during the pass, just as it does in the common sound resolver.
typedef struct NativeFighterSoundIdRule {
    uint32_t costume_mask;
    int32_t bank;
    int32_t first;
    int32_t last;
    int32_t offset;
} NativeFighterSoundIdRule;

typedef struct NativeFighterKoCue {
    uint32_t costume_mask;
    int32_t sound_id;
} NativeFighterKoCue;

typedef struct NativeFighterSoundPolicy {
    const NativeFighterSoundIdRule* rules;
    const NativeFighterKoCue* ko_cues;
    uint16_t rule_count;
    uint16_t ko_cue_count;
} NativeFighterSoundPolicy;

bool nativeFighterSoundPolicy_Validate(const NativeFighterSoundPolicy*);
int32_t nativeFighterSoundPolicy_MapId(const NativeFighterSoundPolicy*,
                                      unsigned costume, int32_t bank,
                                      int32_t sound_id);
int32_t nativeFighterSoundPolicy_KoCue(const NativeFighterSoundPolicy*,
                                      unsigned costume, int32_t fallback);

#ifdef __cplusplus
}
#endif

#endif
