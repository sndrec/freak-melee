#pragma once
#include <stdbool.h>
#include <stdint.h>

#include <dolphin/mtx.h>
#include <melee/mn/native_css_pointer.h>
#ifdef __cplusplus
extern "C" {
#endif
void nativePlayMenu_Tick(bool selection, bool practice);
void nativePlayMenu_DrawCss(Mtx view);
void nativePlayMenu_DrawPractice(void);
bool nativePlayMenu_Hand(float x, float y, uint32_t trigger);
bool nativePlayMenu_Pointer(float u, float v, NativeCssPointerEvent event);
bool nativePlayMenu_PointerActive(void);
bool nativePlayMenu_TakeReadyRequest(void);
bool nativePlayMenu_Inspect(const char* id);
void nativePlayMenu_Shutdown(void);
#ifdef __cplusplus
}
#endif
