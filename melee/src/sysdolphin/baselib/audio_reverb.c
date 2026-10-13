#include "audio_reverb.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

static const u32 lengths[4] = { 1789, 1999, 433, 149 };

size_t HSD_AudioReverbHeapSize(const HSD_AudioReverbConfig* config)
{
    if (!config || !(config->coloration >= 0 && config->coloration <= 1) ||
        !(config->mix >= 0 && config->mix <= 1) ||
        !(config->time >= 0.01f && config->time <= 10) ||
        !(config->damping >= 0 && config->damping <= 1) ||
        !(config->preDelay >= 0 && config->preDelay <= 0.1f)) {
        return 0;
    }
    size_t samples = (u32) (32000.0f * config->preDelay);
    for (int i = 0; i < 4; ++i) {
        samples += lengths[i] + 2;
    }
    return 3 * samples * sizeof(float);
}

bool HSD_AudioReverbInit(HSD_AudioReverb* reverb,
                        const HSD_AudioReverbConfig* config,
                        void* storage, size_t size)
{
    size_t required = HSD_AudioReverbHeapSize(config);
    if (!reverb || !required || !storage || size < required ||
        (uintptr_t) storage % _Alignof(float)) {
        return false;
    }
    HSD_AudioReverb next = { 0 };
    u32 predelay_samples = (u32) (32000.0f * config->preDelay);
    // The SDK assembly wraps after incrementing to (allocated_samples - 1).
    // Keep that audible delay for normal settings; a one-sample allocation
    // must wrap after one sample instead of escaping its storage.
    next.predelay_length = predelay_samples > 1 ? predelay_samples - 1 : predelay_samples;
    next.coloration = config->coloration;
    next.wet = config->mix * 0.6f;
    next.dry = 0.6f - next.wet;
    float damping = config->damping < 0.05f ? 0.05f : config->damping;
    next.damping = 1.0f - (0.05f + 0.8f * damping);
    for (int i = 0; i < 2; ++i) {
        next.feedback[i] = powf(10.0f, (-3.0f * lengths[i]) /
                                        (32000.0f * config->time));
    }
    float* cursor = storage;
    for (int channel = 0; channel < 3; ++channel) {
        HSD_AudioReverbChannel* state = &next.channel[channel];
        for (int i = 0; i < 4; ++i) {
            HSD_AudioReverbLine* line = i < 2 ? &state->comb[i] : &state->allpass[i - 2];
            line->samples = cursor;
            line->length = lengths[i] + 2;
            line->output = 2;
            cursor += line->length;
        }
        if (predelay_samples) {
            state->predelay = cursor;
            cursor += predelay_samples;
        }
    }
    memset(storage, 0, required);
    *reverb = next;
    return true;
}

static inline void advance_line(HSD_AudioReverbLine* line)
{
    if (++line->input == line->length) {
        line->input = 0;
    }
    if (++line->output == line->length) {
        line->output = 0;
    }
}

static inline float allpass(HSD_AudioReverbLine* line, float input, float coefficient)
{
    float value = fmaf(coefficient, line->last, input);
    float output = -fmaf(coefficient, value, -line->last);
    line->samples[line->input] = value;
    line->last = line->samples[line->output];
    advance_line(line);
    return output;
}

void HSD_AudioReverbProcess(HSD_AudioAuxBuffers* update, HSD_AudioReverb* reverb)
{
    s32* channels[3] = { update->left, update->right, update->surround };
    for (int channel = 0; channel < 3; ++channel) {
        HSD_AudioReverbChannel* state = &reverb->channel[channel];
        for (int sample = 0; sample < 160; ++sample) {
            float dry = (float) channels[channel][sample];
            float input = dry;
            if (reverb->predelay_length) {
                input = state->predelay[state->position];
                state->predelay[state->position] = dry;
                if (++state->position == reverb->predelay_length) {
                    state->position = 0;
                }
            }
            float output = 0;
            for (int i = 0; i < 2; ++i) {
                HSD_AudioReverbLine* line = &state->comb[i];
                line->samples[line->input] = fmaf(reverb->feedback[i], line->last, input);
                line->last = line->samples[line->output];
                advance_line(line);
                output += line->last;
            }
            output = allpass(&state->allpass[0], output, reverb->coloration);
            state->lowpass = fmaf(reverb->damping, state->lowpass, output * 0.3f);
            output = allpass(&state->allpass[1], state->lowpass, reverb->coloration);
            output = fmaf(reverb->wet, output, reverb->dry * dry);
            // PPC fctiwz truncates and saturates rather than overflowing a C cast.
            channels[channel][sample] = isnan(output) || output <= -2147483648.0f ?
                INT32_MIN : output >= 2147483648.0f ? INT32_MAX : (s32) output;
        }
    }
}
