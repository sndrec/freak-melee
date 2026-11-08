#pragma once
#include <Runtime/platform.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct NativeMenuTilePortrait { float x, y, width, height; } NativeMenuTilePortrait;
void nativeMenuTile_Acquire(void);
void nativeMenuTile_Release(void);
unsigned nativeMenuTile_PortraitRect(const char* title, float x, float y,
                                     float width, float height, NativeMenuTilePortrait* portrait);
void nativeMenuTile_Draw(void* pixels, unsigned image_width, unsigned image_height,
                         const char* title, float x, float y, float width, float height);
#ifdef __cplusplus
}
#endif
