#ifndef SYSDOLPHIN_AUDIO_AUX_H
#define SYSDOLPHIN_AUDIO_AUX_H

#include "audio_bus.h"
#include "audio_delay.h"
#include "audio_reverb.h"

typedef enum HSD_AudioAuxType {
    HSD_AUDIO_AUX_OFF,
    HSD_AUDIO_AUX_REVERB,
    HSD_AUDIO_AUX_DELAY
} HSD_AudioAuxType;

typedef struct HSD_AudioAuxConfig {
    HSD_AudioAuxType type;
    union {
        HSD_AudioReverbConfig reverb;
        HSD_AudioDelayConfig delay;
    };
} HSD_AudioAuxConfig;

// Configuration and processing run on the audio owner (game) thread.
// The caller retains storage until that bus is disabled or reconfigured.
void HSD_AudioAuxReset(void);
bool HSD_AudioAuxIsActive(u32 bus);
bool HSD_AudioAuxConfigure(u32 bus, const HSD_AudioAuxConfig* config,
                          void* storage, size_t size);
void HSD_AudioAuxProcess(u32 bus, HSD_AudioAuxBuffers* buffers);

#endif
