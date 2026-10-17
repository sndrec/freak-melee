#pragma once
#include <stdbool.h>
#include <melee/sc/forward.h>
#ifdef __cplusplus
extern "C" {
#endif
// The hand and camera are scene-owned HSD objects. A NULL model imports just
// the hand from the disc's CSS archive. Release before CSS/practice retires its
// scene heap; borrowed CSS models also require the archive to remain alive.
void nativeHubHand_Load(const StaticModelDesc* model);
void nativeHubHand_Unload(void);
bool nativeHubHand_Ready(void);
void nativeHubHand_Draw(float x, float y, bool pressed);
#ifdef __cplusplus
}
#endif
