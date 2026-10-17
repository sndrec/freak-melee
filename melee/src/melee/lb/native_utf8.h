#ifndef MELEE_LB_NATIVE_UTF8_H
#define MELEE_LB_NATIVE_UTF8_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Reject truncated/overlong UTF-8, surrogates, embedded NUL, and controls.
static inline bool nativeUtf8_IsVisibleText(const uint8_t* p, size_t size)
{
    bool visible = false;
    for (size_t i = 0; i < size;) {
        uint32_t c = p[i++];
        if (c < 0x20 || c == 0x7f) {
            return false;
        }
        if (c < 0x80) {
            visible |= c != ' ';
            continue;
        }
        size_t extra;
        uint32_t minimum;
        if (c >= 0xc2 && c <= 0xdf) {
            extra = 1;
            minimum = 0x80;
            c &= 31;
        } else if (c >= 0xe0 && c <= 0xef) {
            extra = 2;
            minimum = 0x800;
            c &= 15;
        } else if (c >= 0xf0 && c <= 0xf4) {
            extra = 3;
            minimum = 0x10000;
            c &= 7;
        } else {
            return false;
        }
        if (i + extra > size) {
            return false;
        }
        while (extra--) {
            if ((p[i] & 0xc0) != 0x80) {
                return false;
            }
            c = (c << 6) | (p[i++] & 63);
        }
        if (c < minimum || c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff)) {
            return false;
        }
        visible = true;
    }
    return visible;
}

#endif
