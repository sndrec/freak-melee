#include <sysdolphin/baselib/audio_reverb.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void check_at(int ok, int line)
{
    if (!ok) {
        fprintf(stderr, "Native reverb mismatch at line %d\n", line);
        exit(1);
    }
}
#define check(ok) check_at((ok), __LINE__)

static _Alignas(4) u8 heap[53 * 1024 + 4];
static HSD_AudioReverb reverb;
// Separate arrays with guards: the original assembly assumed contiguous buses.
static long left[162], right[162], surround[162];
static HSD_AudioAuxBuffers update = { left + 1, right + 1, surround + 1 };

static void clear_buffers(void)
{
    memset(left, 0, sizeof(left));
    memset(right, 0, sizeof(right));
    memset(surround, 0, sizeof(surround));
    left[0] = right[0] = surround[0] = 123456;
    left[161] = right[161] = surround[161] = -123456;
}

static void check_guards(void)
{
    check(left[0] == 123456 && right[0] == 123456 && surround[0] == 123456);
    check(left[161] == -123456 && right[161] == -123456 && surround[161] == -123456);
}

static void impulse(float predelay, int arrival)
{
    HSD_AudioReverbConfig config = { 0, 1, 1.88f, 1, predelay };
    check(HSD_AudioReverbInit(&reverb, &config, heap, sizeof(heap) - 4));
    int hits = 0;
    for (int frame = 0; frame < 36; ++frame) {
        clear_buffers();
        if (frame == 0) {
            left[1] = 100000;
            right[1] = -100000;
        }
        HSD_AudioReverbProcess(&update, &reverb);
        check_guards();
        for (int i = 0; i < 160; ++i) {
            int index = frame * 160 + i;
            check(left[i + 1] == -right[i + 1]);
            check(surround[i + 1] == 0);
            if (index < arrival) {
                check(left[i + 1] == 0);
            }
            // First comb reaches the two delayed allpass outputs after
            // 1789 + (433 + 1) + (149 + 1) samples. Their zero coloration
            // leaves a 0.3 lowpass input gain and 0.6 wet output gain.
            if (index == arrival || index == arrival + 210) {
                check(labs(left[i + 1] - 18000) <= 1);
                ++hits;
            }
            if (index == arrival + 1) {
                check(labs(left[i + 1] - 2700) <= 1);
            }
            if (index == arrival + 1790) {
                long expected = (long) (18000.0f * reverb.feedback[0]);
                check(labs(left[i + 1] - expected) <= 1);
                ++hits;
            }
        }
    }
    check(hits == 3);
}

int main(void)
{
    HSD_AudioReverbConfig config = { 0.5f, 1, 1.88f, 0.64f, 0.002f };
    size_t size = HSD_AudioReverbHeapSize(&config);
    check(size == 53304 && size < 53 * 1024);
    memset(heap, 0xA5, sizeof(heap));
    check(HSD_AudioReverbInit(&reverb, &config, heap, size));
    check(heap[size] == 0xA5 && heap[size + 3] == 0xA5);
    check(reverb.predelay_length == 63);
    for (size_t i = 0; i < size; ++i) {
        check(heap[i] == 0);
    }
    for (int frame = 0; frame < 24; ++frame) {
        clear_buffers();
        HSD_AudioReverbProcess(&update, &reverb);
        check_guards();
        for (int i = 1; i <= 160; ++i) {
            check(left[i] == 0 && right[i] == 0 && surround[i] == 0);
        }
    }
    HSD_AudioReverb saved = reverb;
    check(!HSD_AudioReverbInit(&reverb, &config, heap, size - 1));
    check(!memcmp(&saved, &reverb, sizeof(reverb)));
    check(!HSD_AudioReverbInit(&reverb, &config, heap + 1, sizeof(heap) - 1));
    config.time = NAN;
    check(!HSD_AudioReverbHeapSize(&config));
    config.time = 1.88f;
    config.preDelay = INFINITY;
    check(!HSD_AudioReverbHeapSize(&config));

    impulse(0, 2373);
    impulse(0.002f, 2436);
    impulse(1.0f / 32000.0f, 2374);

    // With coloration 0.5, each allpass also has an immediate -0.5
    // response, followed by a delayed +0.75 response. Exercise the wet
    // filter path on surround, independently of left/right storage.
    config = (HSD_AudioReverbConfig) { 0.5f, 1, 1.88f, 1, 0 };
    check(HSD_AudioReverbInit(&reverb, &config, heap, sizeof(heap)));
    int colored_hits = 0;
    for (int frame = 0; frame < 16; ++frame) {
        clear_buffers();
        if (frame == 0) {
            surround[1] = 100000;
        }
        HSD_AudioReverbProcess(&update, &reverb);
        check_guards();
        for (int i = 0; i < 160; ++i) {
            int index = frame * 160 + i;
            check(left[i + 1] == 0 && right[i + 1] == 0);
            if (index < 1789) {
                check(surround[i + 1] == 0);
            }
            if (index == 1789 || index == 1999) {
                check(labs(surround[i + 1] - 4500) <= 1);
                ++colored_hits;
            }
            if (index == 1939 || index == 2223) {
                check(labs(surround[i + 1] + 6750) <= 1);
                ++colored_hits;
            }
        }
    }
    check(colored_hits == 4);

    // Zero wet mix retains the SDK's 0.6 dry gain, without reverb bleed.
    config = (HSD_AudioReverbConfig) { 0.5f, 0, 1.88f, 0.64f, 0.002f };
    check(HSD_AudioReverbInit(&reverb, &config, heap, sizeof(heap)));
    for (int frame = 0; frame < 40; ++frame) {
        clear_buffers();
        for (int i = 1; i <= 160; ++i) {
            left[i] = (frame * 160 + i) * 97 - 300000;
            right[i] = -left[i];
            surround[i] = left[i] / 3;
        }
        long expected[3][160];
        for (int i = 0; i < 160; ++i) {
            expected[0][i] = (long) ((float) left[i + 1] * 0.6f);
            expected[1][i] = (long) ((float) right[i + 1] * 0.6f);
            expected[2][i] = (long) ((float) surround[i + 1] * 0.6f);
        }
        HSD_AudioReverbProcess(&update, &reverb);
        check_guards();
        for (int i = 0; i < 160; ++i) {
            check(left[i + 1] == expected[0][i]);
            check(right[i + 1] == expected[1][i]);
            check(surround[i + 1] == expected[2][i]);
        }
    }
    puts("NATIVE_AUDIO_REVERB_OK");
    return 0;
}
