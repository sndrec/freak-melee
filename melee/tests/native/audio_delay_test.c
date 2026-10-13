#include <sysdolphin/baselib/audio_delay.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void check(int ok)
{
    if (!ok) {
        fputs("Native delay mismatch\n", stderr);
        exit(1);
    }
}

static long reference_gain(long value, u32 gain)
{
    uint64_t bits = (uint64_t) (uint32_t) value * gain;
    int64_t wrapped = bits & UINT32_MAX;
    if (wrapped >= INT64_C(2147483648)) {
        wrapped -= INT64_C(4294967296);
    }
    return (long) (wrapped >= 0 ? wrapped / 128 : -((-wrapped + 127) / 128));
}

int main(void)
{
    HSD_AudioDelayConfig config = { { 6, 11, 16 }, { 50, 25, 0 }, { 100, 50, 100 } };
    HSD_AudioDelay delay;
    _Alignas(4) u8 heap[71 * 1024 + 4];
    long samples[3][160] = { 0 };
    HSD_AudioAuxBuffers update = { samples[0], samples[1], samples[2] };
    size_t size = HSD_AudioDelayHeapSize(&config);
    check(size == 6 * 160 * sizeof(s32));
    memset(heap, 0xA5, sizeof(heap));
    check(HSD_AudioDelayInit(&delay, &config, heap, size));
    check(heap[size] == 0xA5 && heap[size + 3] == 0xA5);
    for (size_t i = 0; i < size; ++i) {
        check(heap[i] == 0);
    }
    const long expected[7][3] = {
        { 0, 0, 0 }, { 12800, 0, 0 }, { 6400, -6400, 0 },
        { 3200, 0, 5000 }, { 1600, -1600, 0 }, { 800, 0, 0 },
        { 400, -400, 0 }
    };
    for (int frame = 0; frame < 7; ++frame) {
        memset(samples, 0, sizeof(samples));
        if (frame == 0) {
            samples[0][0] = 12800;
            samples[1][0] = -12800;
            samples[2][0] = 5000;
        }
        HSD_AudioDelayProcess(&update, &delay);
        for (int channel = 0; channel < 3; ++channel) {
            check(samples[channel][0] == expected[frame][channel]);
            for (int i = 1; i < 160; ++i) {
                check(samples[channel][i] == 0);
            }
        }
    }

    // The actual startup configuration fits its existing 71 KiB allocation.
    config = (HSD_AudioDelayConfig) { { 260, 310, 6 }, { 24, 24, 0 }, { 35, 35, 0 } };
    check(HSD_AudioDelayHeapSize(&config) == 72320);
    check(HSD_AudioDelayInit(&delay, &config, heap, sizeof(heap) - 4));
    for (int frame = 0; frame <= 122; ++frame) {
        memset(samples, 0, sizeof(samples));
        if (frame == 0) {
            samples[0][0] = 25600;
            samples[1][159] = -25600;
        }
        HSD_AudioDelayProcess(&update, &delay);
        check(samples[0][0] == (frame == 51 ? 8800 : frame == 102 ? 2062 : 0));
        check(samples[1][159] == (frame == 61 ? -8800 : frame == 122 ? -2063 : 0));
    }

    // Failed configuration leaves an already initialized effect untouched.
    HSD_AudioDelay saved = delay;
    check(!HSD_AudioDelayInit(&delay, &config, heap, 72319));
    check(!memcmp(&saved, &delay, sizeof(delay)));
    check(!HSD_AudioDelayInit(&delay, &config, heap + 1, sizeof(heap) - 1));
    config.delay[0] = 5;
    check(!HSD_AudioDelayHeapSize(&config));
    config.delay[0] = UINT32_MAX;
    check(!HSD_AudioDelayHeapSize(&config));
    config.delay[0] = 260;
    config.feedback[0] = 101;
    check(!HSD_AudioDelayHeapSize(&config));

    // Overflow and negative rounding must not depend on signed-overflow UB.
    config = (HSD_AudioDelayConfig) { { 6, 6, 6 }, { 100, 100, 100 }, { 100, 50, 35 } };
    check(HSD_AudioDelayInit(&delay, &config, heap, sizeof(heap)));
    const long input[3] = { INT32_MAX, INT32_MIN, -129 };
    for (int channel = 0; channel < 3; ++channel) {
        for (int i = 0; i < 160; ++i) {
            samples[channel][i] = input[channel];
        }
    }
    HSD_AudioDelayProcess(&update, &delay);
    memset(samples, 0, sizeof(samples));
    HSD_AudioDelayProcess(&update, &delay);
    for (int channel = 0; channel < 3; ++channel) {
        for (int i = 0; i < 160; ++i) {
            check(samples[channel][i] == reference_gain(input[channel], delay.output[channel]));
            check(delay.line[channel][i] == reference_gain(input[channel], 128));
        }
    }
    puts("NATIVE_AUDIO_DELAY_OK");
    return 0;
}
