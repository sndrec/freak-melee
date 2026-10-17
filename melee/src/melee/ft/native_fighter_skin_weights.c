#include "native_fighter_skin_weights.h"

#include <math.h>
#include <stddef.h>

bool nativeSkinWeights_Normalize(NativeSkinWeights* skin)
{
    if (skin == NULL || skin->count == 0 ||
        skin->count > NATIVE_SKIN_MAX_INFLUENCES) return false;
    NativeSkinWeights result = { 0 };
    double sums[NATIVE_SKIN_MAX_INFLUENCES] = { 0 };
    double total = 0;
    for (unsigned i = 0; i < skin->count; ++i) {
        const float weight = skin->weights[i];
        if (!isfinite(weight) || weight < 0) return false;
        if (weight == 0) continue;
        unsigned at = 0;
        while (at < result.count && result.joints[at] < skin->joints[i]) ++at;
        if (at == result.count || result.joints[at] != skin->joints[i]) {
            for (unsigned j = result.count; j > at; --j) {
                result.joints[j] = result.joints[j - 1];
                sums[j] = sums[j - 1];
            }
            result.joints[at] = skin->joints[i];
            sums[at] = 0;
            ++result.count;
        }
        sums[at] += weight;
        total += weight;
    }
    if (!(total > 0) || !isfinite(total)) return false;
    unsigned retained = 0;
    for (unsigned i = 0; i < result.count; ++i) {
        const float weight = (float) (sums[i] / total);
        if (weight == 0) continue;
        result.joints[retained] = result.joints[i];
        result.weights[retained++] = weight;
    }
    result.count = retained;
    *skin = result;
    return true;
}

float nativeSkinWeights_Get(const NativeSkinWeights* skin, uint16_t joint)
{
    if (skin == NULL || skin->count > NATIVE_SKIN_MAX_INFLUENCES) return 0;
    for (unsigned i = 0; i < skin->count; ++i)
        if (skin->joints[i] == joint) return skin->weights[i];
    return 0;
}

bool nativeSkinWeights_Equal(const NativeSkinWeights* a,
                             const NativeSkinWeights* b)
{
    if (a == NULL || b == NULL || a->count != b->count ||
        a->count > NATIVE_SKIN_MAX_INFLUENCES) return false;
    for (unsigned i = 0; i < a->count; ++i)
        if (a->joints[i] != b->joints[i] || a->weights[i] != b->weights[i])
            return false;
    return true;
}

bool nativeSkinWeights_Set(NativeSkinWeights* skin, uint16_t joint, float weight)
{
    if (skin == NULL || !isfinite(weight) || weight < 0 || weight > 1)
        return false;
    NativeSkinWeights result = *skin;
    if (!nativeSkinWeights_Normalize(&result)) return false;
    if (weight == 1) {
        result = (NativeSkinWeights){ .count = 1, .joints = { joint },
                                      .weights = { 1 } };
        *skin = result;
        return true;
    }
    unsigned at = result.count;
    double remainder = 0;
    for (unsigned i = 0; i < result.count; ++i) {
        if (result.joints[i] == joint) at = i;
        else remainder += result.weights[i];
    }
    if (at == result.count && weight == 0) { *skin = result; return true; }
    if (at == result.count) {
        if (result.count == NATIVE_SKIN_MAX_INFLUENCES) return false;
        result.joints[result.count++] = joint;
    } else if (!(remainder > 0)) {
        // Removing the only influence would leave an unbound vertex.
        *skin = result;
        return true;
    }
    for (unsigned i = 0; i < result.count; ++i)
        if (i != at)
            result.weights[i] = (float) (result.weights[i] * (1.0 - weight) / remainder);
    result.weights[at] = weight;
    if (!nativeSkinWeights_Normalize(&result)) return false;
    *skin = result;
    return true;
}
