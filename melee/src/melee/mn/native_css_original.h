#ifndef MELEE_MN_NATIVE_CSS_ORIGINAL_H
#define MELEE_MN_NATIVE_CSS_ORIGINAL_H

#ifdef MELEE_NATIVE

#include "native_css_layout.h"

#include <melee/mn/forward.h>
#include <sysdolphin/baselib/forward.h>

#ifdef __cplusplus
extern "C" {
#endif

void nativeCssOriginal_RestoreBounds(CSSIcon* icons);
void nativeCssOriginal_Enter(CSSIcon* icons, HSD_JObj* menu,
                             HSD_CObj* camera);
void nativeCssOriginal_Tick(void);
void nativeCssOriginal_BeginAnimation(void);
void nativeCssOriginal_Exit(void);
void nativeCssOriginal_ClearPortraits(void);
void nativeCssOriginal_SetPortrait(unsigned index, const u8* png, unsigned bytes, const char* title);
const NativeCssLayout* nativeCssOriginal_Layout(void);
bool nativeCssOriginal_CanEdit(void);
bool nativeCssOriginal_UpdateTile(unsigned index, const NativeCssTile* tile);
bool nativeCssOriginal_PreviewTile(unsigned index, const NativeCssTile* tile);
void nativeCssOriginal_CancelPreview(void);
bool nativeCssOriginal_RemoveTile(unsigned index);
bool nativeCssOriginal_AddStock(unsigned stock_index);
bool nativeCssOriginal_AddInstalled(const NativeContentId* id);

bool nativeCssOriginal_FighterBounds(const NativeContentId* id,
                                      float* left, float* right,
                                      float* top, float* bottom);
bool nativeCssOriginal_Reset(void);
void nativeCssOriginal_ReloadInstalledFighters(void);
bool nativeCssOriginal_HitTestTile(float x, float y, NativeCssTile* result);
bool nativeCssOriginal_RandomTile(NativeCssTile* result);
void nativeCssOriginal_Viewport(float* left, float* right, float* top, float* bottom);

#ifdef __cplusplus
}
#endif

#endif
#endif
