#pragma once
#include <cstddef>

struct NativeNewsLink {
    unsigned start, length, url_start, url_length;
};
struct NativeNewsText {
    char body[128001];
    NativeNewsLink links[4096];
    unsigned link_count;
};
bool nativeNewsText_ValidUrl(const char* text, size_t length);
// Converts link labels to display text and retains offsets into the source URL.
// Headings and lists remain in the display text for the reader's line layout.
void nativeNewsText_Parse(const char* source, NativeNewsText& result);
