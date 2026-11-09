#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <xmmintrin.h>

#include <melee/lb/native_float_math.h>

static uint64_t add_sample(uint64_t hash, double sample)
{
    uint64_t bits;
    memcpy(&bits, &sample, sizeof(bits));
    return (hash ^ bits) * UINT64_C(1099511628211);
}

static uint64_t sample_functions(void)
{
    uint64_t hash = UINT64_C(14695981039346656037);
    uint32_t state = 290;
    for (unsigned i = 0; i < 16384; ++i) {
        state = state * UINT32_C(1664525) + UINT32_C(1013904223);
        float x = (float)(state >> 16) * 0x1p-15f - 1.0f;
        float y = (float)(state & 0xffff) * 0x1p-15f - 1.0f;
        float positive = 1.125f + y;
        hash = add_sample(hash, sinf(x * 512.0f));
        hash = add_sample(hash, cosf(x * 512.0f));
        hash = add_sample(hash, tanf(x * 6.0f));
        hash = add_sample(hash, acosf(x));
        hash = add_sample(hash, asinf(x));
        hash = add_sample(hash, atanf(x));
        hash = add_sample(hash, atan2f(x, y));
        hash = add_sample(hash, atan2((double)x, (double)y));
        hash = add_sample(hash, expf(x));
        hash = add_sample(hash, exp2f(x));
        hash = add_sample(hash, logf(positive));
        hash = add_sample(hash, log2f(positive));
        hash = add_sample(hash, log10f(positive));
        hash = add_sample(hash, powf(positive, x));
        hash = add_sample(hash, hypotf(x, y));
        hash = add_sample(hash, sin((double)x * 512.0 + y));
        hash = add_sample(hash, cos((double)x * 512.0 + y));
        hash = add_sample(hash, exp((double)x));
        hash = add_sample(hash, log((double)positive));
        hash = add_sample(hash, log2((double)positive));
        hash = add_sample(hash, pow((double)positive, (double)x));
    }
    return hash;
}

int main(void)
{
    _mm_setcsr(NativeFloatMath_DefaultMxcsr);
    int previous_mode = _get_FMA3_enable();
    _set_FMA3_enable(0);
    uint64_t without_fma = sample_functions();
    int fma_supported = _set_FMA3_enable(1);
    uint64_t with_fma = sample_functions();
    _set_FMA3_enable(previous_mode);

    // This pair returned different bits in the Windows 10 and Windows 11 CRTs.
    volatile double y = -0x1.2d258748e8710p-1;
    volatile double x = 0x1.3dab26baa9e56p+3;
    if (atan2(y, x) != -0x1.e4cdd7d1cb98bp-5 || without_fma != with_fma ||
        !signbit(atan2(-0.0, -1.0)) || exp2f(3.0f) != 8.0f ||
        log2f(8.0f) != 3.0f || acosf(1.0f) != 0.0f) {
        fprintf(stderr, "Native math check failed: atan2=%a, CRT FMA off=%016llx, on=%016llx.\n",
                atan2(y, x), (unsigned long long)without_fma, (unsigned long long)with_fma);
        return 1;
    }
    printf("Native math: 344064 results, digest %016llx, CRT FMA available %d.\n",
           (unsigned long long)with_fma, fma_supported);
    return 0;
}
