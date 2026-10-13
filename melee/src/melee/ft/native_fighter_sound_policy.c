#include "native_fighter_sound_policy.h"

#include <limits.h>
#include <stddef.h>

bool nativeFighterSoundPolicy_Validate(const NativeFighterSoundPolicy* policy)
{
    if (policy == NULL) return true;
    if (policy->rule_count > NATIVE_FIGHTER_SOUND_RULE_CAPACITY ||
        policy->ko_cue_count > NATIVE_FIGHTER_KO_CUE_CAPACITY ||
        (policy->rules == NULL) != (policy->rule_count == 0) ||
        (policy->ko_cues == NULL) != (policy->ko_cue_count == 0)) return false;
    for (unsigned i = 0; i < policy->rule_count; ++i) {
        const NativeFighterSoundIdRule* rule = &policy->rules[i];
        if (rule->costume_mask == 0 || rule->bank < 0 || rule->bank >= 64 ||
            rule->first < 0 || rule->last < rule->first ||
            (int64_t)rule->first + rule->offset < 0 ||
            (int64_t)rule->last + rule->offset > INT32_MAX) return false;
    }
    uint32_t costumes = 0;
    for (unsigned i = 0; i < policy->ko_cue_count; ++i) {
        const NativeFighterKoCue* cue = &policy->ko_cues[i];
        if (cue->costume_mask == 0 || cue->sound_id < 0 ||
            (cue->costume_mask & costumes) != 0) return false;
        costumes |= cue->costume_mask;
    }
    return true;
}

int32_t nativeFighterSoundPolicy_MapId(const NativeFighterSoundPolicy* policy,
                                      unsigned costume, int32_t bank,
                                      int32_t sound_id)
{
    if (policy == NULL || costume >= 32 ||
        policy->rule_count > NATIVE_FIGHTER_SOUND_RULE_CAPACITY ||
        policy->rules == NULL) return sound_id;
    const uint32_t costume_bit = UINT32_C(1) << costume;
    for (unsigned i = 0; i < policy->rule_count; ++i) {
        const NativeFighterSoundIdRule* rule = &policy->rules[i];
        if ((rule->costume_mask & costume_bit) && rule->bank == bank &&
            sound_id >= rule->first && sound_id <= rule->last) {
            const int64_t mapped = (int64_t)sound_id + rule->offset;
            if (mapped >= 0 && mapped <= INT32_MAX) sound_id = (int32_t)mapped;
        }
    }
    return sound_id;
}

int32_t nativeFighterSoundPolicy_KoCue(const NativeFighterSoundPolicy* policy,
                                      unsigned costume, int32_t fallback)
{
    if (policy == NULL || costume >= 32 ||
        policy->ko_cue_count > NATIVE_FIGHTER_KO_CUE_CAPACITY ||
        policy->ko_cues == NULL) return fallback;
    const uint32_t costume_bit = UINT32_C(1) << costume;
    for (unsigned i = 0; i < policy->ko_cue_count; ++i)
        if (policy->ko_cues[i].costume_mask & costume_bit)
            return policy->ko_cues[i].sound_id;
    return fallback;
}
