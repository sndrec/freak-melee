#ifndef MELEE_UI_NATIVE_SHELL_H
#define MELEE_UI_NATIVE_SHELL_H

#include <stdbool.h>
#include <melee/mn/native_css_pointer.h>

#ifdef __cplusplus
extern "C" {
#endif

bool nativeShell_Init(void);
void nativeShell_Update(void);
void nativeShell_UpdateAuthoring(void);
void nativeShell_CapturePracticeFrame(void);
void nativeShell_Shutdown(void);
void nativeShell_Blur(void);
bool nativeShell_IsEditingText(void);
// Diagnostic input in native client pixels, without desktop focus or cursor
// use.
void nativeShell_InspectElement(const char* id);
void nativeShell_ClickAt(int x, int y, int button);
void nativeShell_PointerButtonAt(int x, int y, int button, bool down);
void nativeShell_DragAt(int start_x, int start_y, int end_x, int end_y,
                        int button, unsigned modifiers);
void nativeShell_WheelAt(int x, int y, int steps);
void nativeShell_EnterText(const char* text);
void nativeShell_MovePointer(int x, int y);
bool nativeShell_CssPointerEvent(int x, int y, NativeCssPointerEvent event);
// Control-channel modifiers: bit 0 Shift, bit 1 Ctrl, bit 2 Alt.
void nativeShell_PressKey(const char* key, unsigned modifiers);

#ifdef __cplusplus
}
#endif

#endif
