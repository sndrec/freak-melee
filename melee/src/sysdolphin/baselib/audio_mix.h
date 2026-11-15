#ifndef SYSDOLPHIN_AUDIO_MIX_H
#define SYSDOLPHIN_AUDIO_MIX_H
#include "audio_voice.h"
#define HSD_AUDIO_MS_SAMPLES 32
// Three channels per bus: main L/R/S, auxiliary A L/R/S, auxiliary B L/R/S.
typedef struct HSD_AudioBuses { s32 samples[9][HSD_AUDIO_MS_SAMPLES]; } HSD_AudioBuses;
// Render one millisecond at 32 kHz into cleared, unsaturated accumulation buses.
void HSD_AudioMixVoices(const u8* storage, size_t size, HSD_AudioBuses* buses);
// Local presentation gains, in percent. Voice timing and envelopes stay unchanged.
void HSD_AudioMixSetVolumes(unsigned effects, unsigned music);
#endif
