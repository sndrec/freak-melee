#include "audio_voice.h"

static const s16 coefficients[512] = {
#include <sysdolphin/baselib/audio_coefficients.inc>
};

void HSD_AudioVoiceResample(HSD_AudioVoice* voice, const u8* storage,
                            size_t storage_size, s16* output, size_t count)
{
    u32 ratio = ((u32) voice->source.ratioHi << 16) | voice->source.ratioLo;
    u32 fraction = voice->source.currentAddressFrac;
    s16 history[4];
    for (u32 i = 0; i < 4; ++i) history[i] = (s16) voice->source.last_samples[i];
    for (size_t base = 0; base < count;) {
        size_t block = count - base;
        if (block > 32) block = 32;
        // Read only the source samples this block consumes. Control metadata
        // cannot change inside this owner-thread call; syncing it once per
        // decoded batch avoids a voice/control round trip for every sample.
        // Bound scratch storage even for caller-supplied ratios above 4x.
        u64 remaining = ((u64) fraction + (u64) ratio * block) >> 16;
        s16 decoded[128];
        size_t used = 0, available = 0;
        for (size_t i = 0; i < block; ++i) {
            // Wide phase addition also handles a caller-supplied ratio above 4x.
            u64 phase = (u64) fraction + ratio;
            u32 advance = phase >> 16;
            fraction = phase & 0xFFFF;
            while (advance--) {
                if (used == available) {
                    available = remaining > 128 ? 128 : (size_t) remaining;
                    size_t produced = HSD_AudioVoiceRead(
                        voice, storage, storage_size, decoded, available);
                    for (size_t silent = produced; silent < available; ++silent)
                        decoded[silent] = 0;
                    remaining -= available;
                    used = 0;
                }
                s16 sample = decoded[used++];
                history[0] = history[1]; history[1] = history[2];
                history[2] = history[3]; history[3] = sample;
            }
            const s16* filter = &coefficients[(fraction >> 9) * 4];
            s64 value = 0;
            for (u32 tap = 0; tap < 4; ++tap) value += (s64) history[tap] * filter[tap];
            value >>= 15;
            if (value > 32767) value = 32767;
            if (value < -32768) value = -32768;
            output[base + i] = (s16) value;
        }
        base += block;
    }
    voice->source.currentAddressFrac = fraction;
    for (u32 i = 0; i < 4; ++i) voice->source.last_samples[i] = (u16) history[i];
}
