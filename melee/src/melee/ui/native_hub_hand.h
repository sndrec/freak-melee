#pragma once
#include <stdbool.h>
#include <melee/sc/forward.h>
#ifdef __cplusplus
extern "C" {
#endif
// The hand and camera are scene-owned HSD objects. Load on CSS construction
// and release before its archive is unloaded.
void nativeHubHand_Load(const StaticModelDesc* model);
void nativeHubHand_Unload(void);
bool nativeHubHand_Ready(void);
void nativeHubHand_Draw(float x, float y, bool pressed);
#ifdef __cplusplus
}
#endif
