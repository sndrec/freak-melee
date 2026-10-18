#pragma once
#include <Runtime/platform.h>
#ifdef __cplusplus
extern "C" {
#endif
// Shared front-end artwork. Acquire/release at menu and practice transitions.
void nativeMenuSkin_Acquire(void);
void nativeMenuSkin_Release(void);
// Draw in the caller's menu canvas and projection.
void nativeMenuSkin_Frame(float x, float y, float width, float height, u32 rgba);
void nativeMenuSkin_Panel(float x, float y, float width, float height,
                          u32 border, u32 fill);
void nativeMenuSkin_Button(const char* label, float x, float y, float width,
                           float height, float text_height, bool active, bool enabled);
#ifdef __cplusplus
}
#endif
