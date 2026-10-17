#pragma once
#include <stdint.h>

#include <dolphin/mtx.h>
#include <melee/mn/native_menu_font.h>
#ifdef __cplusplus
extern "C" {
#endif
// Menu coordinates use a 640 x 480 canvas, with Y down. A supplied matrix
// maps that canvas into the caller's scene and retains its projection. NULL
// selects a full-screen orthographic menu.
void nativeMenuPaint_Begin(const Mtx view);
void nativeMenuPaint_End(void);
void nativeMenuPaint_Rect(float x, float y, float w, float h, uint32_t color);
void nativeMenuPaint_Round(float x, float y, float w, float h, float radius,
                           uint32_t color);
void nativeMenuPaint_Quad(const float xy[8], uint32_t color);
#ifdef __cplusplus
void nativeMenuPaint_Text(const char* text, float x, float y, float w, float h,
                          uint32_t color = 0xEEE9EDFF,
                          NativeMenuFontFace face = NativeMenuFont_Notice,
                          bool centered = false);
#else
void nativeMenuPaint_Text(const char* text, float x, float y, float w, float h,
                          uint32_t color, NativeMenuFontFace face,
                          bool centered);
#endif
void nativeMenuPaint_Image(void* pixels, unsigned width, unsigned height,
                           float x, float y, float w, float h);
#ifdef __cplusplus
}
#endif
