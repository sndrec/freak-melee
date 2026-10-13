#ifndef MELEE_UI_NATIVE_CHAT_H
#define MELEE_UI_NATIVE_CHAT_H
#include "native_chat_wire.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum NativeChatState {
    NativeChat_Connecting,
    NativeChat_Online,
    NativeChat_Reconnecting,
    NativeChat_Offline,
} NativeChatState;
typedef enum NativeChatDelivery {
    NativeChatDelivery_None,
    NativeChatDelivery_Sending,
    NativeChatDelivery_Unconfirmed,
} NativeChatDelivery;
typedef struct NativeChatView {
    NativeChatMessage messages[NATIVE_CHAT_HISTORY];
    NativeChatMessage pending;
    uint64_t revision;
    uint32_t count;
    uint16_t online;
    NativeChatState state;
    NativeChatDelivery delivery;
} NativeChatView;
// All public calls belong to the presentation thread. The worker owns network
// I/O.
bool nativeChat_Start(const char* endpoint);
void nativeChat_Update(void);
bool nativeChat_Send(const char* name, const char* text);
const NativeChatView* nativeChat_View(void);
void nativeChat_Stop(void);
#ifdef __cplusplus
}
#endif
#endif
