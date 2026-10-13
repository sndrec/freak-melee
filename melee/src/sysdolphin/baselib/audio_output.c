#include "audio_output.h"
#include "audio_frame.h"
#include "synth.h"
#include "debug.h"

#include <SDL3/SDL.h>
#include <dolphin/ar.h>

static SDL_AudioStream* output_stream;
static bool polling;
static bool muted;

void HSD_AudioOutputSetMuted(bool value)
{
    muted = value;
    if (output_stream) {
        bool applied = SDL_SetAudioStreamGain(output_stream, muted ? 0.0f : 1.0f);
        HSD_ASSERTREPORT(0, applied, "Audio gain failed: %s", SDL_GetError());
    }
}

bool HSD_AudioOutputInit(void)
{
    if (output_stream) {
        return true;
    }
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        OSReport("Audio initialization failed: %s\n", SDL_GetError());
        return false;
    }
    const SDL_AudioSpec spec = { SDL_AUDIO_S16, 2, 32000 };
    output_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                               &spec, NULL, NULL);
    if (!output_stream) {
        OSReport("Audio device open failed: %s\n", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return false;
    }
    HSD_AudioFrameReset();
    HSD_AudioOutputSetMuted(muted);
    if (!SDL_ResumeAudioStreamDevice(output_stream)) {
        OSReport("Audio device resume failed: %s\n", SDL_GetError());
        HSD_AudioOutputShutdown();
        return false;
    }
    return true;
}

void HSD_AudioOutputPoll(void)
{
    if (!output_stream || polling) {
        return;
    }
    polling = true;
    // Keep at most 20 ms of input PCM ahead. Limit work per poll even if
    // the device drains concurrently or the host has just resumed.
    enum { FRAME_BYTES = HSD_AUDIO_FRAME_SAMPLES * 2 * sizeof(s16) };
    for (int frame = 0; frame < 4; ++frame) {
        int queued = SDL_GetAudioStreamQueued(output_stream);
        HSD_ASSERTREPORT(0, queued >= 0, "Audio queue error: %s", SDL_GetError());
        if (queued >= 4 * FRAME_BYTES) {
            break;
        }
        s16 samples[HSD_AUDIO_FRAME_SAMPLES * 2];
        HSD_AudioRenderFrame(ARGetStorageAddress(), ARGetSize(), samples);
        HSD_SynthCallback();
        bool submitted = SDL_PutAudioStreamData(output_stream, samples, sizeof(samples));
        HSD_ASSERTREPORT(0, submitted, "Audio submission failed: %s", SDL_GetError());
    }
    polling = false;
}

void HSD_AudioOutputShutdown(void)
{
    if (output_stream) {
        SDL_DestroyAudioStream(output_stream);
        output_stream = NULL;
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
    polling = false;
}
