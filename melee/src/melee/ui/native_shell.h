#ifndef MELEE_UI_NATIVE_SHELL_H
#define MELEE_UI_NATIVE_SHELL_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool nativeShell_Init(void);
void nativeShell_Update(void);
void nativeShell_Shutdown(void);
void nativeShell_FocusRoom(void);
void nativeShell_BlurRoom(void);
void nativeShell_NavigateRoom(int direction, bool activate);
bool nativeShell_IsEditingText(void);
void nativeShell_ShowRoomError(const char* message);
// Diagnostic input in native client pixels, without desktop focus or cursor
// use.
void nativeShell_ClickAt(int x, int y);
void nativeShell_DragAt(int start_x, int start_y, int end_x, int end_y);
void nativeShell_EnterText(const char* text);
void nativeShell_MovePointer(int x, int y);
// Control-channel modifiers: bit 0 Shift, bit 1 Ctrl, bit 2 Alt.
void nativeShell_PressKey(const char* key, unsigned modifiers);
void nativeShell_ShowCustomResults(const char* const titles[4], int winner);
void nativeShell_ClearCustomResults(void);

#ifdef __cplusplus
}
#endif

#endif
