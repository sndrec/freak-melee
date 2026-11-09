#ifndef MELEE_LB_NATIVE_MATH_BUILD_H
#define MELEE_LB_NATIVE_MATH_BUILD_H

// The native game and its matrix library compile against the same libm.
// Read the CRT declarations first. These names select implementations at
// compile time, including when the engine stores a function pointer.
#include <math.h>
#include "native_math_functions.h"

#define acosf(...) nativeFloatMath_Acos(__VA_ARGS__)
#define asinf(...) nativeFloatMath_Asin(__VA_ARGS__)
#define sinf(...) cr_sinf(__VA_ARGS__)
#define cosf(...) cr_cosf(__VA_ARGS__)
#define tanf(...) cr_tanf(__VA_ARGS__)
#define atanf(...) cr_atanf(__VA_ARGS__)
#define atan2f(...) cr_atan2f(__VA_ARGS__)
#define expf(...) cr_expf(__VA_ARGS__)
#define exp2f(...) cr_exp2f(__VA_ARGS__)
#define logf(...) cr_logf(__VA_ARGS__)
#define log2f(...) cr_log2f(__VA_ARGS__)
#define log10f(...) cr_log10f(__VA_ARGS__)
#define powf(...) cr_powf(__VA_ARGS__)
#define hypotf(...) cr_hypotf(__VA_ARGS__)

#define sin(...) cr_sin(__VA_ARGS__)
#define cos(...) cr_cos(__VA_ARGS__)
#define atan2(...) cr_atan2(__VA_ARGS__)
#define exp(...) cr_exp(__VA_ARGS__)
#define log(...) cr_log(__VA_ARGS__)
#define log2(...) cr_log2(__VA_ARGS__)
#define pow(...) cr_pow(__VA_ARGS__)

#endif
