#include "audio_delay.h"

#include <stdint.h>
#include <string.h>

enum { DELAY_FRAME_SAMPLES = 160 };

size_t HSD_AudioDelayHeapSize(const HSD_AudioDelayConfig* config)
{
    if (!config) {
        return 0;
    }
    u64 frames = 0;
    for (int channel = 0; channel < 3; ++channel) {
        if (config->delay[channel] <= 5 || config->feedback[channel] > 100 ||
            config->output[channel] > 100) {
            return 0;
        }
        frames += ((u64) (config->delay[channel] - 5) * 32 + 159) / 160;
    }
    u64 bytes = frames * DELAY_FRAME_SAMPLES * sizeof(s32);
    // The audio heap sizing interface returns a signed 32-bit byte count.
    return bytes <= INT32_MAX ? (size_t) bytes : 0;
}

bool HSD_AudioDelayInit(HSD_AudioDelay* delay,
                       const HSD_AudioDelayConfig* config,
                       void* storage, size_t size)
{
    size_t required = HSD_AudioDelayHeapSize(config);
    if (!delay || !required || !storage || size < required ||
        (uintptr_t) storage % _Alignof(s32)) {
        return false;
    }
    HSD_AudioDelay next = { 0 };
    s32* cursor = storage;
    for (int channel = 0; channel < 3; ++channel) {
        next.line[channel] = cursor;
        next.frames[channel] =
            ((u64) (config->delay[channel] - 5) * 32 + 159) / 160;
        next.feedback[channel] = (config->feedback[channel] << 7) / 100;
        next.output[channel] = (config->output[channel] << 7) / 100;
        cursor += next.frames[channel] * DELAY_FRAME_SAMPLES;
    }
    memset(storage, 0, required);
    *delay = next;
    return true;
}

void HSD_AudioDelayProcess(HSD_AudioAuxBuffers* update, HSD_AudioDelay* delay)
{
    s32* channels[3] = { update->left, update->right, update->surround };
    for (int channel = 0; channel < 3; ++channel) {
        s32* line = delay->line[channel] +
                    delay->position[channel] * DELAY_FRAME_SAMPLES;
        s32* samples = channels[channel];
        for (int i = 0; i < DELAY_FRAME_SAMPLES; ++i) {
            u32 old = (u32) line[i];
            // The original effect wraps the product to 32 bits before the
            // signed shift. Keep that arithmetic explicit on the native CPU.
            s32 feedback = (s32) (old * delay->feedback[channel]) >> 7;
            line[i] = (s32) ((u32) samples[i] + (u32) feedback);
            samples[i] = (s32) (old * delay->output[channel]) >> 7;
        }
        if (++delay->position[channel] == delay->frames[channel]) {
            delay->position[channel] = 0;
        }
    }
}
