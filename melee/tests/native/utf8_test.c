#include <melee/lb/native_utf8.h>
#include <assert.h>
#include <stdio.h>

static unsigned encode(uint32_t c, uint8_t out[4])
{
    if (c < 0x80) { out[0] = c; return 1; }
    if (c < 0x800) { out[0] = 0xc0 | (c >> 6); out[1] = 0x80 | (c & 63); return 2; }
    if (c < 0x10000) {
        out[0] = 0xe0 | (c >> 12); out[1] = 0x80 | ((c >> 6) & 63);
        out[2] = 0x80 | (c & 63); return 3;
    }
    out[0] = 0xf0 | (c >> 18); out[1] = 0x80 | ((c >> 12) & 63);
    out[2] = 0x80 | ((c >> 6) & 63); out[3] = 0x80 | (c & 63); return 4;
}
int main(void)
{
    for (uint32_t c = 0; c <= 0x10ffff; ++c) {
        if (c >= 0xd800 && c <= 0xdfff) continue;
        uint8_t bytes[4]; unsigned size = encode(c, bytes);
        const uint8_t* cursor = bytes; uint32_t decoded = 0;
        assert(nativeUtf8_Decode(&cursor, bytes + size, &decoded));
        assert(decoded == c && cursor == bytes + size);
        for (unsigned prefix = 1; prefix < size; ++prefix) {
            cursor = bytes;
            assert(!nativeUtf8_Decode(&cursor, bytes + prefix, &decoded));
            assert(cursor == bytes + 1 && decoded == 0xfffd);
        }
    }
    const uint8_t malformed[][4] = {
        {0x80}, {0xbf}, {0xc0, 0xaf}, {0xc1, 0xbf}, {0xc2, 'a'},
        {0xe0, 0x80, 0x80}, {0xed, 0xa0, 0x80}, {0xef, 0xbf, 'a'},
        {0xf0, 0x80, 0x80, 0x80}, {0xf4, 0x90, 0x80, 0x80}, {0xff}
    };
    for (unsigned i = 0; i < sizeof(malformed) / sizeof(*malformed); ++i) {
        const uint8_t* cursor = malformed[i]; uint32_t decoded;
        assert(!nativeUtf8_Decode(&cursor, cursor + 4, &decoded));
        assert(cursor == malformed[i] + 1 && decoded == 0xfffd);
        assert(!nativeUtf8_IsVisibleText(malformed[i], 4));
    }
    const uint8_t text[] = {'Z', 'o', 0xc3, 0xab};
    assert(nativeUtf8_IsVisibleText(text, sizeof(text)));
    assert(!nativeUtf8_IsVisibleText(text, sizeof(text) - 1));
    assert(!nativeUtf8_IsVisibleText(NULL, 0));
    assert(!nativeUtf8_IsVisibleText((const uint8_t*)" ", 1));
    assert(!nativeUtf8_IsVisibleText((const uint8_t*)"a\nb", 3));
    puts("UTF-8 scalar round trips, truncated prefixes and malformed input passed.");
    return 0;
}
