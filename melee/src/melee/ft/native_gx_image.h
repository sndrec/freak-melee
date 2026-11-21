#ifndef MELEE_FT_NATIVE_GX_IMAGE_H
#define MELEE_FT_NATIVE_GX_IMAGE_H

#ifdef MELEE_NATIVE

#ifdef __cplusplus
extern "C" {
#endif

// Returns tightly packed RGBA8 pixels. Release the result with free().
unsigned char* nativeImage_DecodeRGBA(const unsigned char* encoded,
                                      unsigned size, unsigned short* width,
                                      unsigned short* height);

// Decodes a PNG or JPEG at a content-loading transition. The returned
// 32-byte-aligned buffer contains GameCube RGBA8 tiles and must be freed with
// nativeGxImage_Free. Dimensions are limited by the costume import arena.
unsigned char* nativeGxImage_Decode(const unsigned char* encoded,
                                    unsigned size, unsigned short* width,
                                    unsigned short* height);
void nativeGxImage_Free(unsigned char* pixels);

#ifdef __cplusplus
}
#endif
#endif
#endif
