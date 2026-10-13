#include <sysdolphin/baselib/audio_frame.h>
#include <sysdolphin/baselib/audio_aux.h>
#include <stdio.h>
#include <string.h>

static bool saturate;

void HSD_AudioMixVoices(const u8* storage, size_t size, HSD_AudioBuses* buses)
{
    (void) storage;
    (void) size;
    memset(buses, 0, sizeof(*buses));
    for (int i = 0; i < HSD_AUDIO_MS_SAMPLES; ++i) {
        buses->samples[0][i] = saturate ? 40000 : 1000;
        buses->samples[1][i] = saturate ? -40000 : -1000;
        buses->samples[2][i] = 999999;
        buses->samples[3][i] = 200;
        buses->samples[4][i] = -300;
        buses->samples[5][i] = 999999;
    }
}

static int check(const s16* output, int left, int right)
{
    for (int i = 0; i < HSD_AUDIO_FRAME_SAMPLES; ++i) {
        if (output[2 * i] != left || output[2 * i + 1] != right) {
            fprintf(stderr, "sample %d: %d,%d expected %d,%d\n", i,
                    output[2 * i], output[2 * i + 1], left, right);
            return 1;
        }
    }
    return 0;
}

int main(void)
{
    s16 output[HSD_AUDIO_FRAME_SAMPLES * 2];
    s32 heap[3 * HSD_AUDIO_FRAME_SAMPLES];
    HSD_AudioAuxConfig config = {
        .type = HSD_AUDIO_AUX_DELAY,
        .delay = { { 6, 6, 6 }, { 0, 0, 0 }, { 100, 100, 100 } }
    };
    HSD_AudioAuxReset();
    HSD_AudioFrameReset();
    if (!HSD_AudioAuxConfigure(0, &config, heap, sizeof(heap))) return 1;
    // One delay frame plus the DSP/CPU return pipeline: no wet signal until
    // frame three. Surround must never spill into the stereo front channels.
    for (int frame = 0; frame < 6; ++frame) {
        HSD_AudioRenderFrame(NULL, 0, output);
        if (check(output, frame < 3 ? 1000 : 1200,
                  frame < 3 ? -1000 : -1300)) return 1;
    }
    HSD_AudioAuxReset();
    HSD_AudioRenderFrame(NULL, 0, output);
    if (check(output, 1000, -1000)) return 1;
    saturate = true;
    HSD_AudioRenderFrame(NULL, 0, output);
    return check(output, 32767, -32768);
}
