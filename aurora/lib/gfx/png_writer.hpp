#pragma once
#include <cstdint>
namespace aurora::gfx {
bool write_png(const char* path, const void* pixels, uint32_t width, uint32_t height,
               uint32_t pitch, bool bgra, bool alpha);
}
