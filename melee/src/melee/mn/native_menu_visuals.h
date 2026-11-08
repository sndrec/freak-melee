#pragma once
#ifdef MELEE_NATIVE
#include <Runtime/platform.h>
#include "native_menu_font.h"
#include <sysdolphin/baselib/forward.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct NativeMenuVisuals NativeMenuVisuals;
// Scene-owned GX content. Positions are in the scene's initial world plane;
// the anchor's subsequent transform and the current camera apply at draw time.
void nativeMenuVisuals_Begin(NativeMenuVisuals** owner, HSD_JObj* root, u8 gx_link, u8 priority);
void nativeMenuVisuals_End(NativeMenuVisuals** owner);
void nativeMenuVisuals_Text(NativeMenuVisuals* scene, HSD_JObj* anchor,
                           float x, float y, float z, float width, float height,
                           NativeMenuFontFace face, const char* title);
void nativeMenuVisuals_ClearSprites(NativeMenuVisuals* scene);
bool nativeMenuVisuals_Sprite(NativeMenuVisuals* scene, unsigned slot,
                             const u8* encoded, unsigned bytes, const char* title,
                             NativeMenuFontFace face);
void nativeMenuVisuals_Rect(NativeMenuVisuals* scene, unsigned slot,
                           float x, float y, float width, float height);
#ifdef __cplusplus
}
#endif
#endif
