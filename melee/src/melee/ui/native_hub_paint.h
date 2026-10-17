#pragma once
#include <stdint.h>
#include <melee/mn/native_menu_font.h>
#ifdef __cplusplus
extern "C" {
#endif
// Hub coordinates use the original 640 x 480 menu canvas, with Y down.
void nativeHubPaint_Begin(void);
void nativeHubPaint_End(void);
void nativeHubPaint_Rect(float x, float y, float w, float h, uint32_t color);
void nativeHubPaint_Round(float x, float y, float w, float h, float radius,
                          uint32_t color);
void nativeHubPaint_Quad(const float xy[8], uint32_t color);
#ifdef __cplusplus
void nativeHubPaint_Text(const char* text, float x, float y, float w, float h,
                         uint32_t color = 0xEEE9EDFF,
                         NativeMenuFontFace face = NativeMenuFont_Notice,
                         bool centered = false);
#else
void nativeHubPaint_Text(const char* text, float x, float y, float w, float h,
                         uint32_t color, NativeMenuFontFace face,
                         bool centered);
#endif
void nativeHubPaint_Image(void* pixels, unsigned width, unsigned height,
                          float x, float y, float w, float h);
void nativeHubPaint_Hand(float x, float y, bool pressed);
#ifdef __cplusplus
}
#endif
