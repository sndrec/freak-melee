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
void nativeMenuScene_Tick(bool allow_input);
bool nativeMenuScene_Begin(const char* caption, bool decorated);
void nativeMenuScene_End(void);
void nativeMenuScene_DrawRibbon(unsigned slot, const Mtx canvas, float x, float y, float width,
                                float height, bool selected, const char* label);
void nativeMenuScene_Unload(void);
void nativeMenuScene_Start(bool backwards);
bool nativeMenuScene_Active(void);
float nativeMenuScene_Frame(void);
bool nativeMenuScene_View(bool transition, bool outgoing, Mtx view, Mtx44 projection);
// Project/unproject between normalized screen positions and the menu canvas.
bool nativeMenuScene_Project(float x, float y, float* u, float* v);
bool nativeMenuScene_Unproject(float u, float v, float* x, float* y);
#ifdef __cplusplus
}
#endif
