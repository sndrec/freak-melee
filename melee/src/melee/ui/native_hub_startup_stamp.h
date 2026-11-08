#pragma once
#include <Runtime/platform.h>
#include <dolphin/mtx.h>
bool nativeHubStartupStamp_Load();
void nativeHubStartupStamp_Unload();
bool nativeHubStartupStamp_BeginPlacement();
void nativeHubStartupStamp_Move();
bool nativeHubStartupStamp_Place();
void nativeHubStartupStamp_CancelPlacement();
#ifdef __cplusplus
extern "C" {
#endif
void nativeHubStartupStamp_DrawCss(Mtx view);
bool nativeHubStartupStamp_IsPlacing(void);
#ifdef __cplusplus
}
#endif
void nativeHubStartupStamp_Inspect();
