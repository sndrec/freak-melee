#ifndef MELEE_LB_NATIVE_UTF8_H
#define MELEE_LB_NATIVE_UTF8_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Decode one scalar within [cursor,end). Invalid input advances one byte and
// yields U+FFFD; callers validating external text can reject the false result.
static inline bool nativeUtf8_Decode(const uint8_t** cursor, const uint8_t* end,
                                     uint32_t* codepoint)
{
    const uint8_t* start = *cursor;
    *codepoint = 0xfffd;
    if (start == end) return false;
    const uint8_t* p = start + 1;
    *cursor = p;
    uint32_t c = *start, minimum;
    unsigned extra;
    if (c < 0x80) { *codepoint = c; return true; }
    if (c >= 0xc2 && c <= 0xdf) { extra = 1; minimum = 0x80; c &= 31; }
    else if (c >= 0xe0 && c <= 0xef) { extra = 2; minimum = 0x800; c &= 15; }
    else if (c >= 0xf0 && c <= 0xf4) { extra = 3; minimum = 0x10000; c &= 7; }
    else return false;
    if ((size_t)(end - p) < extra) return false;
    while (extra--) {
        if ((*p & 0xc0) != 0x80) return false;
        c = (c << 6) | (*p++ & 63);
    }
    if (c < minimum || c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff)) return false;
    *cursor = p;
    *codepoint = c;
    return true;
}

// Reject truncated/overlong UTF-8, surrogates, embedded NUL, and ASCII controls.
static inline bool nativeUtf8_IsVisibleText(const uint8_t* p, size_t size)
{
    if (!p) return false;
    const uint8_t* end = p + size;
    bool visible = false;
    while (p != end) {
        uint32_t c;
        if (!nativeUtf8_Decode(&p, end, &c) || c < 0x20 || c == 0x7f) return false;
        visible |= c != ' ';
    }
    return visible;
}

#endif
