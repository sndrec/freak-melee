#ifndef SYSDOLPHIN_AUDIO_DELAY_H
#define SYSDOLPHIN_AUDIO_DELAY_H

#include "audio_bus.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct HSD_AudioDelayConfig {
    u32 delay[3]; // Milliseconds, including the auxiliary bus's 5 ms latency.
    u32 feedback[3]; // Percent.
    u32 output[3]; // Percent.
} HSD_AudioDelayConfig;

typedef struct HSD_AudioDelay {
    s32* line[3];
    u32 frames[3];
    u32 position[3];
    u32 feedback[3];
    u32 output[3];
} HSD_AudioDelay;

// Zero means invalid configuration. Storage belongs to the caller and is
// initialized only when switching effects, never while processing samples.
size_t HSD_AudioDelayHeapSize(const HSD_AudioDelayConfig* config);
bool HSD_AudioDelayInit(HSD_AudioDelay* delay,
                       const HSD_AudioDelayConfig* config,
                       void* storage, size_t size);
// Process one auxiliary frame in place.
void HSD_AudioDelayProcess(HSD_AudioAuxBuffers* buffers, HSD_AudioDelay* state);

#endif
