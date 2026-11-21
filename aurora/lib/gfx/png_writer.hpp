#pragma once
#include <cstdint>
#include <string>
namespace aurora::gfx {
bool encode_png(std::string& bytes, const void* pixels, uint32_t width, uint32_t height,
                uint32_t pitch, bool bgra);
bool write_png(const char* path, const void* pixels, uint32_t width, uint32_t height,
               uint32_t pitch, bool bgra, bool alpha);
}
