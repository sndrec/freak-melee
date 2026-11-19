#include "audio_internal.h"
#include "debug.h"
#include <string.h>

HSD_AudioVoice hsd_audio_voices[HSD_AUDIO_VOICES];
HSD_AudioReleaseFade hsd_audio_release_fade;
static u8 free_indices[HSD_AUDIO_VOICES];
static u32 free_count;
static u64 order;

void HSD_AudioVoiceInit(void)
{
    memset(hsd_audio_voices, 0, sizeof(hsd_audio_voices));
    memset(&hsd_audio_release_fade, 0, sizeof(hsd_audio_release_fade));
    for (u32 i = 0; i < HSD_AUDIO_VOICES; ++i) {
        hsd_audio_voices[i].index = i;
        free_indices[i] = i;
    }
    free_count = HSD_AUDIO_VOICES;
    order = 0;
}

void HSD_AudioVoicePriority(HSD_AudioVoice* voice, u32 priority)
{
    HSD_ASSERT(0, voice->allocated && priority > 0 && priority < 32);
    voice->priority = priority;
    voice->priority_order = ++order;
}

static void retain_release_tail(HSD_AudioVoice* voice)
{
    if (!voice->running) return;
    for (u32 bus = 0; bus < 9; ++bus) {
        // Preserve the SDK's 32-bit accumulator without signed-overflow UB.
        hsd_audio_release_fade.pending[bus] = (s32)
            ((u32) hsd_audio_release_fade.pending[bus] + (u32) voice->last_mix[bus]);
    }
}

HSD_AudioVoice* HSD_AudioVoiceAcquire(u32 priority, void (*dropped)(void*))
{
    HSD_ASSERT(0, priority > 0 && priority < 32);
    HSD_AudioVoice* chosen = NULL;
    if (free_count) {
        chosen = &hsd_audio_voices[free_indices[--free_count]];
    } else {
        for (u32 i = 0; i < HSD_AUDIO_VOICES; ++i) {
            HSD_AudioVoice* v = &hsd_audio_voices[i];
            if (!v->allocated || v->priority >= priority) continue;
            if (!chosen || v->priority < chosen->priority ||
                (v->priority == chosen->priority &&
                 v->priority_order < chosen->priority_order)) chosen = v;
        }
        if (!chosen) return NULL;
        retain_release_tail(chosen);
        // Reserve this slot while the owner cleans up its associated sound.
        chosen->allocated = false;
        if (chosen->dropped) chosen->dropped(chosen);
    }
    u32 index = chosen->index;
    memset(chosen, 0, sizeof(*chosen));
    chosen->index = index;
    chosen->allocated = true;
    chosen->dropped = dropped;
    HSD_AudioVoicePriority(chosen, priority);
    return chosen;
}

void HSD_AudioVoiceRelease(HSD_AudioVoice* voice)
{
    HSD_ASSERT(0, voice->allocated && free_count < HSD_AUDIO_VOICES);
    retain_release_tail(voice);
    voice->allocated = false;
    voice->running = false;
    voice->sample_data = NULL;
    voice->sample_bytes = 0;
    voice->decoder.active = false;
    voice->priority = 0;
    free_indices[free_count++] = voice->index;
}

bool HSD_AudioVoiceStart(HSD_AudioVoice* voice)
{
    HSD_ASSERT(0, voice->allocated);
    voice->running = HSD_AudioSampleStart(&voice->decoder, &voice->sample,
                                         voice->streaming);
    return voice->running;
}

size_t HSD_AudioVoiceRead(HSD_AudioVoice* voice, const u8* storage,
                         size_t size, s16* output, size_t count)
{
    if (!voice->running) return 0;
    // Streaming changes loop/end addresses as ring slots become available.
    AXPBADDR* a = &voice->sample.address;
    voice->decoder.end = ((u32) a->endAddressHi << 16) | a->endAddressLo;
    voice->decoder.loop = ((u32) a->loopAddressHi << 16) | a->loopAddressLo;
    voice->decoder.looping = a->loopFlag != 0;
    voice->decoder.loop_predictor = voice->sample.loop.loop_pred_scale & 127;
    voice->decoder.loop_history[0] = (s16) voice->sample.loop.loop_yn1;
    voice->decoder.loop_history[1] = (s16) voice->sample.loop.loop_yn2;
    if (voice->sample_data) {
        storage = voice->sample_data;
        size = voice->sample_bytes;
    }
    size_t produced = HSD_AudioSampleRead(&voice->decoder, storage, size, output, count);
    a->currentAddressHi = voice->decoder.cursor >> 16;
    a->currentAddressLo = (u16) voice->decoder.cursor;
    voice->sample.adpcm.yn1 = (u16) voice->decoder.history[0];
    voice->sample.adpcm.yn2 = (u16) voice->decoder.history[1];
    voice->sample.adpcm.pred_scale = voice->decoder.predictor;
    voice->running = voice->decoder.active;
    return produced;
}
