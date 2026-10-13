#ifndef SYSDOLPHIN_AUDIO_SAMPLE_H
#define SYSDOLPHIN_AUDIO_SAMPLE_H
#include "sfxbank.h"

typedef struct HSD_AudioSample {
    u32 cursor, end, loop;
    s16 coefficients[16];
    s16 history[2], loop_history[2];
    u16 predictor, loop_predictor;
    bool active, looping, streaming;
} HSD_AudioSample;

bool HSD_AudioSampleStart(HSD_AudioSample* state, const HSD_SfxVoice* voice,
                          bool streaming);
// Decode into caller storage. No allocation. Returns samples produced; the
// caller supplies silence for the remainder after a voice ends or faults.
size_t HSD_AudioSampleRead(HSD_AudioSample* state, const u8* storage,
                           size_t storage_size, s16* output, size_t count);
#endif
