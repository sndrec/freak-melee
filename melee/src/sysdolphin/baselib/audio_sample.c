#include "audio_sample.h"
#include <string.h>

bool HSD_AudioSampleStart(HSD_AudioSample* s, const HSD_SfxVoice* voice,
                          bool streaming)
{
    const AXPBADDR* a = &voice->address;
    memset(s, 0, sizeof(*s));
    s->cursor = ((u32) a->currentAddressHi << 16) | a->currentAddressLo;
    s->end = ((u32) a->endAddressHi << 16) | a->endAddressLo;
    s->loop = ((u32) a->loopAddressHi << 16) | a->loopAddressLo;
    s->looping = a->loopFlag != 0;
    s->streaming = streaming;
    if (a->format != 0 || (s->cursor & 15) < 2 || (s->end & 15) < 2 ||
        s->cursor > s->end || (s->looping && ((s->loop & 15) < 2 || s->loop > s->end)))
        return false;
    for (u32 i = 0; i < 16; ++i)
        s->coefficients[i] = (s16) voice->adpcm.a[i / 2][i % 2];
    s->history[0] = (s16) voice->adpcm.yn1;
    s->history[1] = (s16) voice->adpcm.yn2;
    s->loop_history[0] = (s16) voice->loop.loop_yn1;
    s->loop_history[1] = (s16) voice->loop.loop_yn2;
    s->predictor = voice->adpcm.pred_scale & 127;
    s->loop_predictor = voice->loop.loop_pred_scale & 127;
    s->active = true;
    return true;
}

size_t HSD_AudioSampleRead(HSD_AudioSample* s, const u8* storage,
                           size_t storage_size, s16* output, size_t count)
{
    size_t produced = 0;
    while (produced < count && s->active) {
        size_t byte = s->cursor >> 1;
        if (byte >= storage_size) { s->active = false; break; }
        int nibble = (s->cursor & 1) ? storage[byte] & 15 : storage[byte] >> 4;
        if (nibble >= 8) nibble -= 16;
        u32 pair = ((s->predictor >> 4) & 7) * 2;
        // Signed 64-bit accumulation avoids overflow before rounding/clamping.
        s64 prediction = (s64) s->coefficients[pair] * s->history[0] +
                         (s64) s->coefficients[pair + 1] * s->history[1];
        s64 value = nibble * (1 << (s->predictor & 15)) +
                    ((prediction + 1024) >> 11);
        if (value > 32767) value = 32767;
        if (value < -32768) value = -32768;
        s->history[1] = s->history[0];
        s->history[0] = (s16) value;
        output[produced++] = (s16) value;
        if (s->cursor == s->end) {
            s->cursor = s->loop;
            if (!s->looping) { s->active = false; break; }
            s->predictor = s->loop_predictor;
            if (!s->streaming) {
                s->history[0] = s->loop_history[0];
                s->history[1] = s->loop_history[1];
            }
        } else {
            ++s->cursor;
            if (!(s->cursor & 15)) {
                byte = s->cursor >> 1;
                if (byte >= storage_size) { s->active = false; break; }
                s->predictor = storage[byte] & 127;
                s->cursor += 2;
            }
        }
    }
    return produced;
}
