#ifndef MELEE_GM_NATIVE_NETPLAY_CURSOR_H
#define MELEE_GM_NATIVE_NETPLAY_CURSOR_H

#include <stdbool.h>
#include <stdint.h>

enum { NATIVE_CURSOR_SEND_MS = 16, NATIVE_CURSOR_IDLE_MS = 1200 };

typedef struct NativeCursorMotion {
    float from_x, from_y, to_x, to_y;
    uint64_t received_ms;
    unsigned blend_ms;
} NativeCursorMotion;

static inline void nativeCursorMotion_Sample(const NativeCursorMotion* motion,
                                            uint64_t now, float* x, float* y)
{
    uint64_t elapsed = now >= motion->received_ms ? now - motion->received_ms : 0;
    float t = elapsed >= motion->blend_ms ? 1.0f : (float)elapsed / motion->blend_ms;
    *x = motion->from_x + (motion->to_x - motion->from_x) * t;
    *y = motion->from_y + (motion->to_y - motion->from_y) * t;
}

static inline void nativeCursorMotion_Update(NativeCursorMotion* motion,
                                            uint64_t now, float x, float y,
                                            bool continuous)
{
    if (!continuous) {
        motion->from_x = motion->to_x = x;
        motion->from_y = motion->to_y = y;
        motion->received_ms = now;
        motion->blend_ms = 0;
        return;
    }
    if (x == motion->to_x && y == motion->to_y) return;

    // Begin moving as soon as a sample arrives. Start from the position
    // currently on screen and limit the lag after a gap. A stationary cursor
    // always reaches its last authoritative position.
    uint64_t interval = now - motion->received_ms;
    nativeCursorMotion_Sample(motion, now, &motion->from_x, &motion->from_y);
    motion->to_x = x;
    motion->to_y = y;
    motion->blend_ms = interval < 12 ? 12 : interval <= 33 ? (unsigned)interval :
                       interval <= 100 ? 33 : NATIVE_CURSOR_SEND_MS;
    motion->received_ms = now;
}

#endif
