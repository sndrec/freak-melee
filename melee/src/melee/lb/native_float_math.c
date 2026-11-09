#include "native_float_math.h"

#include <math.h>

#if defined(_M_X64) || defined(__x86_64__)
#include <emmintrin.h>
#endif

#pragma STDC FP_CONTRACT OFF

// asin(x) for |x| <= 1/2. The remaining Taylor terms together are
// smaller than 2.5e-10 on this interval, before rounding to float.
static double asin_small(double x)
{
    double t = x * x;
    double p = 88179.0 / 12058624.0;
    p = 46189.0 / 5505024.0 + t * p;
    p = 12155.0 / 1245184.0 + t * p;
    p = 6435.0 / 557056.0 + t * p;
    p = 143.0 / 10240.0 + t * p;
    p = 231.0 / 13312.0 + t * p;
    p = 63.0 / 2816.0 + t * p;
    p = 35.0 / 1152.0 + t * p;
    p = 5.0 / 112.0 + t * p;
    p = 3.0 / 40.0 + t * p;
    p = 1.0 / 6.0 + t * p;
    return x + (x * t) * p;
}

static double half_angle_root(float x)
{
    double square = 0.5 * (1.0 - (x < 0.0f ? -(double)x : (double)x));
#if defined(_M_X64) || defined(__x86_64__)
    __m128d input = _mm_set_sd(square);
    return _mm_cvtsd_f64(_mm_sqrt_sd(input, input));
#else
    return sqrt(square);
#endif
}

float nativeFloatMath_Acos(float x)
{
    const double pi = 0x1.921fb54442d18p+1;
    if (x >= -0.5f && x <= 0.5f) {
        return (float)(0.5 * pi - asin_small(x));
    }
    if (!(x >= -1.0f && x <= 1.0f)) return NAN;

    // The half-angle identity retains precision near x = 1.
    double angle = 2.0 * asin_small(half_angle_root(x));
    return (float)(x < 0.0f ? pi - angle : angle);
}

float nativeFloatMath_Asin(float x)
{
    const double half_pi = 0x1.921fb54442d18p+0;
    if (x >= -0.5f && x <= 0.5f) {
        if (x == 0.0f) return x;
        return (float)asin_small(x);
    }
    if (!(x >= -1.0f && x <= 1.0f)) return NAN;

    double angle = half_pi - 2.0 * asin_small(half_angle_root(x));
    return (float)(x < 0.0f ? -angle : angle);
}
