#pragma once
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
union SDL_Event;
void nativeHub_Tick(void);
void nativeHub_Draw(void);
void nativeHub_Shutdown(void);
bool nativeHub_IsVisible(void);
void nativeHub_Event(const union SDL_Event* event);
void nativeHub_OpenLobbies(void);
void nativeHub_Notice(const char* text);
// Diagnostic input uses native client pixels, as ordinary mouse input does.
void nativeHub_ClickAt(int x, int y, int button);
void nativeHub_MovePointer(int x, int y);
void nativeHub_EnterText(const char* text);
void nativeHub_PressKey(const char* key, unsigned modifiers);
void nativeHub_Inspect(const char* id);
#ifdef __cplusplus
}
#endif
