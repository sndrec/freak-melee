#pragma once
#include <melee/ft/forward.h>
#include <dolphin/mtx.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
enum { NATIVE_MENU_FIGHTERS = 6 };
// Standalone costume/animation previews. NULL animation selects the idle.
// Load only at menu or authoring transitions.
bool nativeMenuFighter_Load(unsigned slot, FighterKind kind, unsigned costume,
                            const char* animation, const short initial_variants[11]);
struct NativeFighterProject;
bool nativeMenuFighter_LoadProject(unsigned slot, const struct NativeFighterProject* project);
unsigned nativeMenuFighter_Generation(unsigned slot);
unsigned nativeMenuFighter_AnimationCount(unsigned slot);
const char* nativeMenuFighter_AnimationName(unsigned slot, unsigned index);
const char* nativeMenuFighter_CurrentAnimation(unsigned slot);
float nativeMenuFighter_AnimationFrames(unsigned slot);
bool nativeMenuFighter_SelectAnimation(unsigned slot, const char* animation);
// Optional flat-colour presentation, with the asset's declared outline meshes.
void nativeMenuFighter_SetFlatStyle(unsigned slot, unsigned body, unsigned outline, float depth, float facing);
void nativeMenuFighter_Unload(unsigned slot);
void nativeMenuFighter_UnloadAll(void);
void nativeMenuFighter_Draw(unsigned slot, Mtx canvas, float x, float feet_y,
                            float height, float yaw, float frame);
typedef enum NativeMenuFighterStyle { NativeMenuFighter_Normal, NativeMenuFighter_Shadow } NativeMenuFighterStyle;
// Orthographic portrait view with the output's aspect ratio. Pan uses view coordinates.
void nativeMenuFighter_DrawCamera(unsigned slot, float yaw, float pitch, float zoom,
                                  float pan_x, float pan_y, float frame, float aspect, NativeMenuFighterStyle style);
void nativeMenuFighter_Inspect(void);
#ifdef __cplusplus
}
#endif
