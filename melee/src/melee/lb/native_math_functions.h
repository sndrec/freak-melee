#ifndef MELEE_LB_NATIVE_MATH_FUNCTIONS_H
#define MELEE_LB_NATIVE_MATH_FUNCTIONS_H

#include "native_float_math.h"

#ifdef __cplusplus
extern "C" {
#endif

float cr_sinf(float x);
float cr_cosf(float x);
float cr_tanf(float x);
float cr_atanf(float x);
float cr_atan2f(float y, float x);
float cr_expf(float x);
float cr_exp2f(float x);
float cr_logf(float x);
float cr_log2f(float x);
float cr_log10f(float x);
float cr_powf(float x, float y);
float cr_hypotf(float x, float y);

double cr_sin(double x);
double cr_cos(double x);
double cr_atan2(double y, double x);
double cr_exp(double x);
double cr_log(double x);
double cr_log2(double x);
double cr_pow(double x, double y);

#ifdef __cplusplus
}
#endif
#endif
