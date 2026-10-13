#ifndef SYSDOLPHIN_AUDIO_INTERNAL_H
#define SYSDOLPHIN_AUDIO_INTERNAL_H
#include "audio_voice.h"
extern HSD_AudioVoice hsd_audio_voices[HSD_AUDIO_VOICES];
typedef struct HSD_AudioReleaseFade {
    s32 pending[9], value[9];
    s16 delta[9];
    u32 block;
} HSD_AudioReleaseFade;
extern HSD_AudioReleaseFade hsd_audio_release_fade;
#endif
