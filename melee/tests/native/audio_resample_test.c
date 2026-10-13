#include <sysdolphin/baselib/audio_voice.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const s16 coeffs[512] = {
#include <sysdolphin/baselib/audio_coefficients.inc>
};
static void check(int ok) { if (!ok) { fputs("resampling mismatch\n", stderr); exit(1); } }
void __assert(char* file, u32 line, char* expression)
{ fprintf(stderr, "%s:%lu: %s\n", file, line, expression); abort(); }
static const u8 data[16] = {0,0x17,0x8F,0x23,0x45,0x67,0x89,0xAB,
                          0,0xCD,0xEF,0x01,0x23,0x45,0x67,0x89};
static HSD_AudioVoice* voice(u32 ratio)
{
    HSD_AudioVoice* v = HSD_AudioVoiceAcquire(1, NULL); check(v != NULL);
    v->sample.address.currentAddressLo = 2; v->sample.address.endAddressLo = 31;
    v->sample.address.loopAddressLo = 2; v->sample.address.loopFlag = 1;
    v->source.ratioHi = ratio >> 16; v->source.ratioLo = (u16) ratio;
    check(HSD_AudioVoiceStart(v)); return v;
}
static s16 original_sample(s64 sample)
{
    if (sample < 0) return 0;
    u32 address = (sample % 28) / 14 * 16 + 2 + sample % 14;
    int n = address & 1 ? data[address / 2] & 15 : data[address / 2] >> 4;
    return n >= 8 ? n - 16 : n;
}
static s16 reference(u64 phase)
{
    s64 sum = 0;
    for (int tap = 0; tap < 4; ++tap)
        sum += (s64) original_sample((s64)(phase >> 16) - 4 + tap) *
               coeffs[((phase & 65535) >> 9) * 4 + tap];
    sum >>= 15;
    return sum > 32767 ? 32767 : sum < -32768 ? -32768 : (s16) sum;
}
static void reference_resample(HSD_AudioVoice* voice, const u8* storage,
                            size_t storage_size, s16* output, size_t count)
{
    u32 ratio = ((u32) voice->source.ratioHi << 16) | voice->source.ratioLo;
    u32 fraction = voice->source.currentAddressFrac;
    s16 history[4];
    for (u32 i = 0; i < 4; ++i) history[i] = (s16) voice->source.last_samples[i];
    for (size_t i = 0; i < count; ++i) {
        // Wide phase addition also handles a caller-supplied ratio above 4x.
        u64 phase = (u64) fraction + ratio;
        u32 advance = phase >> 16;
        fraction = phase & 0xFFFF;
        while (advance--) {
            s16 sample = 0;
            HSD_AudioVoiceRead(voice, storage, storage_size, &sample, 1);
            history[0] = history[1]; history[1] = history[2];
            history[2] = history[3]; history[3] = sample;
        }
        const s16* filter = &coeffs[(fraction >> 9) * 4];
        s64 value = 0;
        for (u32 tap = 0; tap < 4; ++tap) value += (s64) history[tap] * filter[tap];
        value >>= 15;
        if (value > 32767) value = 32767;
        if (value < -32768) value = -32768;
        output[i] = (s16) value;
    }
    voice->source.currentAddressFrac = fraction;
    for (u32 i = 0; i < 4; ++i) voice->source.last_samples[i] = (u16) history[i];
}


static void batch_equivalence(void)
{
    const u32 ratios[] = {0, 1, 0x8000, 0xFFFF, 0x10000, 0x14000,
                         0x40000, 0x54001, 0x800000, 0xFFFFFFFF};
    const size_t counts[] = {0, 1, 31, 32, 33, 160, 257};
    unsigned comparisons = 0;
    for (unsigned r = 0; r < sizeof(ratios)/sizeof(ratios[0]); ++r) {
        for (unsigned mode = 0; mode < 8; ++mode) {
            HSD_AudioVoice* v = voice(ratios[r]);
            v->streaming = (mode & 1) != 0;
            v->sample.address.loopFlag = (mode & 2) != 0;
            v->sample.address.endAddressLo = mode & 4 ? 3 : 31;
            v->sample.loop.loop_pred_scale = 0x23;
            v->sample.loop.loop_yn1 = 100;
            v->sample.loop.loop_yn2 = (u16)-120;
            v->sample.adpcm.a[0][0] = 1900;
            v->sample.adpcm.a[0][1] = (u16)-700;
            v->sample.adpcm.yn1 = (u16)-32768;
            v->source.currentAddressFrac = 65535;
            for (unsigned k = 0; k < 4; ++k)
                v->source.last_samples[k] = (u16)(k * 1999 - 3000);
            check(HSD_AudioVoiceStart(v));
            HSD_AudioVoice start = *v;
            for (unsigned n = 0; n < sizeof(counts)/sizeof(counts[0]); ++n) {
                size_t count = ratios[r] == 0xFFFFFFFF ? (counts[n] != 0) : counts[n];
                for (unsigned bounds = 0; bounds < 4; ++bounds) {
                    const size_t sizes[] = {0, 3, 7, sizeof(data)};
                    HSD_AudioVoice expected = start, actual = start;
                    s16 a[257], b[257];
                    memset(a, 0x5A, sizeof(a)); memset(b, 0x5A, sizeof(b));
                    reference_resample(&expected, data, sizes[bounds], a, count);
                    HSD_AudioVoiceResample(&actual, data, sizes[bounds], b, count);
                    check(!memcmp(a, b, sizeof(a)));
                    check(!memcmp(&expected, &actual, sizeof(actual)));
                    // Retarget controls between calls, as streaming does, and
                    // check the next block plus the final decoder/voice state.
                    expected.sample.address.loopAddressLo = actual.sample.address.loopAddressLo = 3;
                    expected.sample.address.endAddressLo = actual.sample.address.endAddressLo = 31;
                    expected.source.ratioHi = actual.source.ratioHi = 1;
                    expected.source.ratioLo = actual.source.ratioLo = 0x2345;
                    reference_resample(&expected, data, sizeof(data), a, 33);
                    HSD_AudioVoiceResample(&actual, data, sizeof(data), b, 33);
                    check(!memcmp(a, b, sizeof(a)));
                    check(!memcmp(&expected, &actual, sizeof(actual)));
                    ++comparisons;
                }
            }
            HSD_AudioVoiceRelease(v);
        }
    }
    printf("%u batched/sample-at-a-time PCM and complete voice-state pairs verified\n", comparisons);
}

int main(void)
{
    HSD_AudioVoiceInit();
    batch_equivalence();
    const u32 ratios[] = {0, 0x8000, 0x10000, 0x14000, 0x40000};
    for (int r = 0; r < 5; ++r) {
        HSD_AudioVoice* a = voice(ratios[r]);
        HSD_AudioVoice* b = voice(ratios[r]);
        s16 whole[160], split[160];
        HSD_AudioVoiceResample(a, data, sizeof(data), whole, 160);
        for (int i = 0; i < 160; ++i) HSD_AudioVoiceResample(b, data, sizeof(data), split+i, 1);
        u64 phase = 0;
        for (int i = 0; i < 160; ++i) {
            phase += ratios[r]; check(whole[i] == reference(phase) && whole[i] == split[i]);
        }
        check(!memcmp(&a->source, &b->source, sizeof(a->source)));
        check(!memcmp(&a->decoder, &b->decoder, sizeof(a->decoder)));
        HSD_AudioVoiceRelease(a); HSD_AudioVoiceRelease(b);
    }
    HSD_AudioVoice* v = voice(0x8000); s16 output[100];
    HSD_AudioVoiceResample(v, data, sizeof(data), output, 40);
    v->source.ratioHi = 1; v->source.ratioLo = 0x4000;
    HSD_AudioVoiceResample(v, data, sizeof(data), output+40, 60);
    u64 phase = 0;
    for (int i = 0; i < 100; ++i) { phase += i < 40 ? 0x8000 : 0x14000; check(output[i] == reference(phase)); }
    HSD_AudioVoiceRelease(v);
    puts("Polyphase ratios, local DSP table, split blocks and live pitch changes verified");
    return 0;
}
