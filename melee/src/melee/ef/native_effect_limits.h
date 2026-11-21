#ifndef MELEE_EF_NATIVE_EFFECT_LIMITS_H
#define MELEE_EF_NATIVE_EFFECT_LIMITS_H
#include <dolphin/types.h>

enum {
    EF_MODEL_MIN_UNITS = 64,
    EF_MODEL_UNITS_PER_TICK = 4096,
    EF_MODEL_LIVE_UNITS = 8192,
    EF_MAX_QUEUED_EFFECTS = 256,
};

typedef struct EF_EffectUsage {
    u32 model_units_this_tick;
    u32 requests_this_tick;
    u32 live_model_units;
} EF_EffectUsage;

extern EF_EffectUsage efLib_Usage;
#endif
