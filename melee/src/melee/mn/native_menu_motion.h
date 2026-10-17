#pragma once
#include <stdbool.h>
#include <dolphin/mtx.h>
#ifdef __cplusplus
extern "C" {
#endif
// Scene-owned stock main-menu animation. Load while opening the hub and unload
// before CSS/practice retires its scene heap.
void nativeMenuMotion_Load(void);
void nativeMenuMotion_Unload(void);
void nativeMenuMotion_Start(bool backwards);
bool nativeMenuMotion_Active(void);
float nativeMenuMotion_Frame(void);
bool nativeMenuMotion_Sample(bool outgoing, Mtx view, Mtx44 projection);
#ifdef __cplusplus
}
#endif
