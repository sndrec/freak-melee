#ifndef MELEE_UI_NATIVE_SHELL_CSS_ORIGINAL_H
#define MELEE_UI_NATIVE_SHELL_CSS_ORIGINAL_H

#ifdef MELEE_NATIVE

#include <RmlUi/Core.h>

void nativeShellCssOriginal_Init(Rml::ElementDocument* document);
void nativeShellCssOriginal_Update(void);
void nativeShellCssOriginal_RefreshInstalledFighters(void);
void nativeShellCssOriginal_Resize(int left, int top, int width, int height);
void nativeShellCssOriginal_Close(void);
bool nativeShellCssOriginal_IsEditing(void);

#endif
#endif
