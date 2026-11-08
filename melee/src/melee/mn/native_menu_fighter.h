#pragma once
#include <melee/ft/forward.h>
#include <dolphin/mtx.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
enum { NATIVE_MENU_FIGHTERS = 6 };
// Standalone costume/idle animation previews. Load only at menu transitions.
bool nativeMenuFighter_Load(unsigned slot, FighterKind kind, const short initial_variants[11]);
unsigned nativeMenuFighter_Generation(unsigned slot);
// Optional flat-colour presentation, with the asset's declared outline meshes.
void nativeMenuFighter_SetFlatStyle(unsigned slot, unsigned body, unsigned outline, float depth, float facing);
void nativeMenuFighter_Unload(unsigned slot);
void nativeMenuFighter_UnloadAll(void);
void nativeMenuFighter_Draw(unsigned slot, Mtx canvas, float x, float feet_y,
                            float height, float yaw, float frame);
// Square, orthographic portrait view. Pan uses normalized view coordinates.
void nativeMenuFighter_DrawCamera(unsigned slot, float yaw, float pitch, float zoom,
                                  float pan_x, float pan_y, float frame);
void nativeMenuFighter_Inspect(void);
#ifdef __cplusplus
}
#endif
