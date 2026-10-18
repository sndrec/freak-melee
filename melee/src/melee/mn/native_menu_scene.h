#pragma once
#include <stdbool.h>
#include <dolphin/mtx.h>
#ifdef __cplusplus
extern "C" {
#endif
// Scene-owned stock main-menu models and animation. Load while opening the hub and unload
// before CSS/practice retires its scene heap.
void nativeMenuScene_Load(void);
void nativeMenuScene_Inspect(void);
void nativeMenuScene_DrawBackdrop(void);
// Nine-slice stock panel border; corners retain their authored proportions.
void nativeMenuScene_DrawFrame(float x, float y, float width, float height, u32 rgba);
void nativeMenuScene_DrawRibbon(unsigned slot, float x, float y, float width,
                                float height, bool selected);
void nativeMenuScene_Unload(void);
void nativeMenuScene_Start(bool backwards);
bool nativeMenuScene_Active(void);
float nativeMenuScene_Frame(void);
bool nativeMenuScene_Sample(bool outgoing, Mtx view, Mtx44 projection);
#ifdef __cplusplus
}
#endif
