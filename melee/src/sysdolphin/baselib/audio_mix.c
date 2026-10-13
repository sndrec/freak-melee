#include "audio_mix.h"
#include "audio_internal.h"
#include <string.h>

void HSD_AudioVoiceConfigureMix(HSD_AudioVoice* voice, const AXPBMIX* mix)
{
    voice->mix = *mix;
    bool a = mix->vAuxAL != 0 || mix->vAuxAR != 0;
    bool b = mix->vAuxBL != 0 || mix->vAuxBR != 0;
    bool surround = mix->vS != 0 || mix->vAuxAS != 0 || mix->vAuxBS != 0;
    voice->mix_mask = 3 | (a ? 0x18 : 0) | (b ? 0xC0 : 0) |
        (surround ? 4 | (a ? 0x20 : 0) | (b ? 0x100 : 0) : 0);
}

void HSD_AudioMixVoices(const u8* storage, size_t size, HSD_AudioBuses* buses)
{
    // AX's value structure stores gain/delta pairs in a different bus order.
    static const u8 pair[9] = {0, 1, 7, 2, 3, 8, 4, 5, 6};
    // The original studio fade chooses its slope once per 160-sample frame.
    HSD_AudioReleaseFade* fade = &hsd_audio_release_fade;
    if (fade->block == 0) {
        for (u32 bus = 0; bus < 9; ++bus) {
            s32 slope = fade->pending[bus] / 160;
            if (slope > 20) slope = 20;
            if (slope < -20) slope = -20;
            if (slope == 0) {
                fade->pending[bus] = fade->value[bus] = 0;
            } else {
                fade->value[bus] = fade->pending[bus];
                fade->pending[bus] -= slope * 160;
            }
            fade->delta[bus] = -slope;
        }
    }
    for (u32 bus = 0; bus < 9; ++bus) {
        for (u32 i = 0; i < HSD_AUDIO_MS_SAMPLES; ++i) {
            buses->samples[bus][i] = fade->value[bus];
            fade->value[bus] += fade->delta[bus];
        }
    }
    fade->block = (fade->block + 1) % 5;
    for (u32 index = 0; index < HSD_AUDIO_VOICES; ++index) {
        HSD_AudioVoice* voice = &hsd_audio_voices[index];
        if (!voice->allocated || !voice->running) continue;
        s16 samples[HSD_AUDIO_MS_SAMPLES];
        HSD_AudioVoiceResample(voice, storage, size, samples, HSD_AUDIO_MS_SAMPLES);
        u16 volume = voice->envelope.currentVolume;
        for (u32 i = 0; i < HSD_AUDIO_MS_SAMPLES; ++i) {
            // GC envelope gain is signed; bus gain below is unsigned.
            s32 sample = ((s32) samples[i] * (s16) volume) >> 15;
            if (sample > 32767) sample = 32767;
            if (sample < -32768) sample = -32768;
            samples[i] = sample;
            volume = (u16) (volume + voice->envelope.currentDelta);
        }
        voice->envelope.currentVolume = volume;
        u16 mix[18];
        _Static_assert(sizeof(AXPBMIX) == sizeof(mix), "gain/delta pairs");
        memcpy(mix, &voice->mix, sizeof(mix));
        for (u32 bus = 0; bus < 9; ++bus) {
            if (!(voice->mix_mask & (1 << bus))) continue;
            u32 offset = pair[bus] * 2;
            u16 gain = mix[offset], delta = mix[offset + 1];
            for (u32 i = 0; i < HSD_AUDIO_MS_SAMPLES; ++i) {
                s32 sample = ((s32) samples[i] * gain) >> 15;
                if (sample > 32767) sample = 32767;
                if (sample < -32768) sample = -32768;
                buses->samples[bus][i] = (s32)
                    ((u32) buses->samples[bus][i] + (u32) sample);
                gain = (u16) (gain + delta);
                voice->last_mix[bus] = sample;
            }
            mix[offset] = gain;
        }
        memcpy(&voice->mix, mix, sizeof(mix));
    }
}
