#ifndef MELEE_LB_NATIVE_FLOAT_MATH_H
#define MELEE_LB_NATIVE_FLOAT_MATH_H

// Nearest-even rounding, gradual underflow, and masked FP exceptions on x86-64.
enum { NativeFloatMath_DefaultMxcsr = 0x1f80 };

#ifdef __cplusplus
extern "C" {
#endif

// Fixed rounding paths for the inverse trig used in the simulation.
float nativeFloatMath_Acos(float x);
float nativeFloatMath_Asin(float x);

#ifdef __cplusplus
}
#endif
#endif
