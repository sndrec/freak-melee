#ifndef HSD_ANIMATION_COLOR_H
#define HSD_ANIMATION_COLOR_H

#include <Runtime/platform.h>

static inline u8 HSD_AnimationColorByte(f32 channel)
{
#ifdef MELEE_NATIVE
    // A spline can overshoot even when both authored colors are in range.
    if (!(channel > 0.0f)) return 0;
    if (channel >= 1.0f) return 255;
#endif
    return (u8)(255.0 * channel);
}

#endif
