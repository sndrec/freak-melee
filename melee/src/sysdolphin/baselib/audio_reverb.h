#ifndef SYSDOLPHIN_AUDIO_REVERB_H
#define SYSDOLPHIN_AUDIO_REVERB_H

#include "audio_bus.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct HSD_AudioReverbConfig {
    float coloration;
    float mix;
    float time;
    float damping;
    float preDelay;
} HSD_AudioReverbConfig;

typedef struct HSD_AudioReverbLine {
    float* samples;
    u32 length;
    u32 input;
    u32 output;
    float last;
} HSD_AudioReverbLine;

typedef struct HSD_AudioReverbChannel {
    HSD_AudioReverbLine comb[2];
    HSD_AudioReverbLine allpass[2];
    float lowpass;
    float* predelay;
    u32 position;
} HSD_AudioReverbChannel;

typedef struct HSD_AudioReverb {
    HSD_AudioReverbChannel channel[3];
    float feedback[2];
    float coloration;
    float damping;
    float wet;
    float dry;
    u32 predelay_length;
} HSD_AudioReverb;

size_t HSD_AudioReverbHeapSize(const HSD_AudioReverbConfig* config);
bool HSD_AudioReverbInit(HSD_AudioReverb* reverb,
                        const HSD_AudioReverbConfig* config,
                        void* storage, size_t size);
// Process one standard-reverb auxiliary frame in place.
void HSD_AudioReverbProcess(HSD_AudioAuxBuffers* buffers, HSD_AudioReverb* state);

#endif
