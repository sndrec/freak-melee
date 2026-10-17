#pragma once
#include <stdbool.h>

#include <melee/gm/native_content_store.h>
#ifdef __cplusplus
extern "C" {
#endif
union SDL_Event;
typedef enum NativeHubPage {
    NativeHubPage_QuickPlay,
    NativeHubPage_Lobbies,
    NativeHubPage_Chat,
    NativeHubPage_Friends,
    NativeHubPage_Inbox,
    NativeHubPage_Workshop,
    NativeHubPage_Settings,
    NativeHubPage_Profile,
    NativeHubPage_Clone,
    NativeHubPage_Send,
    NativeHubPage_RoomChat
} NativeHubPage;
void nativeHub_Tick(void);
void nativeHub_Draw(void);
void nativeHub_Shutdown(void);
bool nativeHub_IsVisible(void);
bool nativeHub_ConsumesGameInput(void);
void nativeHub_Event(const union SDL_Event* event);
void nativeHub_Open(NativeHubPage page);
void nativeHub_Notice(const char* text);
const char* nativeHub_NoticeText(void);
const char* nativeHub_LocalName(void);
bool nativeHub_OpenFighterDelivery(const NativeContentId* fighter,
                                   const char* recipient);
// Diagnostic input uses native client pixels, as ordinary mouse input does.
void nativeHub_ClickAt(int x, int y, int button);
void nativeHub_MovePointer(int x, int y);
void nativeHub_EnterText(const char* text);
void nativeHub_PressKey(const char* key, unsigned modifiers);
void nativeHub_Inspect(const char* id);
#ifdef __cplusplus
}
#endif
