#include "native_script_math.h"

#include <math.h>

float nativeScriptMath_WrapAngle(float angle)
{
    const double tau = 6.28318530717958647692;
    if (!isfinite(angle)) return 0;
    // Preserve the original float assignment rounding for ordinary angles.
    // Very large finite angles cannot make progress by repeatedly subtracting
    // tau in float precision; reduce those in one bounded operation first.
    if (angle < -1024.0f || angle > 1024.0f)
        angle = (float) fmod((double) angle, tau);
    while (angle < 0.0f) angle = (float) (angle + tau);
    while (angle > tau) angle = (float) (angle - tau);
    return angle;
}

// Shared item angle normalization, with the original double constants and
// rounding at each float assignment. Reject nonfinite/unbounded iteration.
float nativeScriptMath_WrapSignedAngle(float angle)
{
    const double pi = 3.14159265358979323846;
    const double tau = 6.28318530717958647692;
    if (!isfinite(angle)) return 0;
    if (angle < -1024.0f || angle > 1024.0f)
        angle = (float) fmod((double) angle, tau);
    while (angle < -pi) angle = (float) (angle + tau);
    while (angle > pi) angle = (float) (angle - tau);
    return angle;
}
