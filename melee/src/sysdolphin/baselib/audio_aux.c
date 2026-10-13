#include "audio_aux.h"

#include <string.h>

typedef struct HSD_AudioAuxState {
    HSD_AudioAuxType type;
    union {
        HSD_AudioReverb reverb;
        HSD_AudioDelay delay;
    };
} HSD_AudioAuxState;

static HSD_AudioAuxState effects[2];

bool HSD_AudioAuxIsActive(u32 bus)
{
    return bus < 2 && effects[bus].type != HSD_AUDIO_AUX_OFF;
}

void HSD_AudioAuxReset(void)
{
    memset(effects, 0, sizeof(effects));
}

bool HSD_AudioAuxConfigure(u32 bus, const HSD_AudioAuxConfig* config,
                          void* storage, size_t size)
{
    if (bus >= 2 || !config) {
        return false;
    }
    HSD_AudioAuxState next = { 0 };
    next.type = config->type;
    switch (config->type) {
    case HSD_AUDIO_AUX_OFF:
        break;
    case HSD_AUDIO_AUX_REVERB:
        if (!HSD_AudioReverbInit(&next.reverb, &config->reverb, storage, size)) {
            return false;
        }
        break;
    case HSD_AUDIO_AUX_DELAY:
        if (!HSD_AudioDelayInit(&next.delay, &config->delay, storage, size)) {
            return false;
        }
        break;
    default:
        return false;
    }
    effects[bus] = next;
    return true;
}

void HSD_AudioAuxProcess(u32 bus, HSD_AudioAuxBuffers* buffers)
{
    if (bus < 2) {
        HSD_AudioAuxState* effect = &effects[bus];
        switch (effect->type) {
        case HSD_AUDIO_AUX_REVERB:
            HSD_AudioReverbProcess(buffers, &effect->reverb);
            return;
        case HSD_AUDIO_AUX_DELAY:
            HSD_AudioDelayProcess(buffers, &effect->delay);
            return;
        case HSD_AUDIO_AUX_OFF:
            break;
        }
    }
    // A disabled bus contributes no effect return, including any old tail.
    memset(buffers->left, 0, 160 * sizeof(s32));
    memset(buffers->right, 0, 160 * sizeof(s32));
    memset(buffers->surround, 0, 160 * sizeof(s32));
}
