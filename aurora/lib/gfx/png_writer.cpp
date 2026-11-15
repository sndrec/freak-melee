#include "png_writer.hpp"
#include <png.h>
#include <cstdio>
#include <cstdlib>

namespace aurora::gfx {
bool write_png(const char* path, const void* pixels, uint32_t width, uint32_t height,
               uint32_t pitch, bool bgra, bool alpha) {
  if (!path || !pixels || !width || !height || uint64_t(width) * 4 > pitch) return false;
  FILE* file = std::fopen(path, "wb");
  if (!file) return false;
  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  png_infop info = png ? png_create_info_struct(png) : nullptr;
  const unsigned channels = alpha ? 4 : 3;
  auto* row = static_cast<png_bytep>(std::malloc(size_t(width) * channels));
  if (!png || !info || !row) {
    std::free(row);
    if (png) png_destroy_write_struct(&png, &info);
    std::fclose(file); std::remove(path); return false;
  }
  bool written = false;
  if (setjmp(png_jmpbuf(png)) == 0) {
    png_init_io(png, file);
    png_set_IHDR(png, info, width, height, 8, alpha ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_set_compression_level(png, 6); png_write_info(png, info);
    const auto* image = static_cast<const uint8_t*>(pixels);
    for (uint32_t y = 0; y < height; ++y) {
      const auto* source = image + size_t(y) * pitch;
      for (uint32_t x = 0; x < width; ++x) {
        const auto* pixel = source + size_t(x) * 4;
        auto* output = row + size_t(x) * channels;
        output[0] = pixel[bgra ? 2 : 0]; output[1] = pixel[1]; output[2] = pixel[bgra ? 0 : 2];
        if (alpha) {
          // Rendered RGBA is premultiplied; PNG stores straight-alpha colour.
          const unsigned coverage = pixel[3]; output[3] = coverage;
          for (unsigned c = 0; c < 3; ++c) {
            const unsigned value = coverage ? (unsigned(output[c]) * 255 + coverage / 2) / coverage : 0;
            output[c] = uint8_t(value > 255 ? 255 : value);
          }
        }
      }
      png_write_row(png, row);
    }
    png_write_end(png, info); written = true;
  }
  png_destroy_write_struct(&png, &info); std::free(row);
  written = std::fclose(file) == 0 && written;
  if (!written) std::remove(path);
  return written;
}
}
