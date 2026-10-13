#include <sysdolphin/baselib/audio_aux.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void check(int ok)
{
    if (!ok) {
        fputs("Native auxiliary bus mismatch\n", stderr);
        exit(1);
    }
}

static _Alignas(4) u8 reverb_heap[53 * 1024];
static _Alignas(4) u8 delay_heap[3 * 160 * sizeof(s32)];

int main(void)
{
    HSD_AudioAuxConfig reverb = {
        .type = HSD_AUDIO_AUX_REVERB,
        .reverb = { 0.5f, 0, 1.88f, 0.64f, 0.002f }
    };
    HSD_AudioAuxConfig delay = {
        .type = HSD_AUDIO_AUX_DELAY,
        .delay = { { 6, 6, 6 }, { 50, 50, 50 }, { 100, 100, 100 } }
    };
    HSD_AudioAuxConfig off = { .type = HSD_AUDIO_AUX_OFF };
    s32 samples[3][160];
    HSD_AudioAuxBuffers buffers = { samples[0], samples[1], samples[2] };

    HSD_AudioAuxReset();
    memset(samples, 1, sizeof(samples));
    HSD_AudioAuxProcess(0, &buffers);
    for (int channel = 0; channel < 3; ++channel) {
        for (int i = 0; i < 160; ++i) {
            check(samples[channel][i] == 0);
        }
    }
    check(HSD_AudioAuxConfigure(0, &reverb, reverb_heap, sizeof(reverb_heap)));
    check(HSD_AudioAuxConfigure(1, &delay, delay_heap, sizeof(delay_heap)));
    samples[0][0] = 10000;
    samples[1][159] = -10000;
    HSD_AudioAuxProcess(0, &buffers);
    check(samples[0][0] == 6000 && samples[1][159] == -6000);

    memset(samples, 0, sizeof(samples));
    samples[2][80] = 12800;
    HSD_AudioAuxProcess(1, &buffers);
    check(samples[2][80] == 0);
    // Failed changes must not destroy the existing effect's pending echo.
    check(!HSD_AudioAuxConfigure(1, &delay, delay_heap, sizeof(delay_heap) - 1));
    check(!HSD_AudioAuxConfigure(2, &delay, delay_heap, sizeof(delay_heap)));
    HSD_AudioAuxConfig invalid = { .type = (HSD_AudioAuxType) 99 };
    check(!HSD_AudioAuxConfigure(1, &invalid, delay_heap, sizeof(delay_heap)));
    check(!HSD_AudioAuxConfigure(1, NULL, delay_heap, sizeof(delay_heap)));
    HSD_AudioAuxProcess(1, &buffers);
    check(samples[2][80] == 12800);

    check(HSD_AudioAuxConfigure(0, &off, NULL, 0));
    samples[0][0] = 10000;
    HSD_AudioAuxProcess(0, &buffers);
    check(samples[0][0] == 0 && samples[2][80] == 0);
    // Disabling bus A leaves bus B's feedback line alone.
    HSD_AudioAuxProcess(1, &buffers);
    check(samples[2][80] == 6400);

    // Reconfiguration clears the old tail in the caller's reused heap.
    check(HSD_AudioAuxConfigure(1, &delay, delay_heap, sizeof(delay_heap)));
    memset(samples, 0, sizeof(samples));
    HSD_AudioAuxProcess(1, &buffers);
    check(samples[2][80] == 0);
    samples[0][0] = 12800;
    HSD_AudioAuxProcess(1, &buffers);
    HSD_AudioAuxReset();
    HSD_AudioAuxProcess(1, &buffers);
    check(samples[0][0] == 0);
    check(HSD_AudioAuxConfigure(1, &delay, delay_heap, sizeof(delay_heap)));
    HSD_AudioAuxProcess(1, &buffers);
    check(samples[0][0] == 0);
    puts("NATIVE_AUDIO_AUX_OK");
    return 0;
}
