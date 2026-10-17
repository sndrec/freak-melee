#ifndef MELEE_GM_NATIVE_ROOM_CHAT_H
#define MELEE_GM_NATIVE_ROOM_CHAT_H
#ifdef MELEE_NATIVE
#include "native_signaling_client.h"

typedef enum NativeRoomChatDelivery {
    NativeRoomChatDelivery_None,
    NativeRoomChatDelivery_Sending,
    NativeRoomChatDelivery_Unconfirmed,
} NativeRoomChatDelivery;

typedef struct NativeRoomChat {
    NativeSignalingClient* client;
    NativeRoomChatEntry messages[NATIVE_ROOM_CHAT_HISTORY];
    NativeRoomChatEntry staging[NATIVE_ROOM_CHAT_HISTORY];
    char pending[NATIVE_ROOM_CHAT_TEXT_BYTES + 1];
    char failure[129];
    uint64_t last_sequence, pending_after_sequence, pending_since_ms,
        history_since_ms;
    uint32_t nonce;
    uint8_t count, staging_count, expected;
    NativeRoomChatDelivery delivery;
    bool connected, ready, requested, loading;
} NativeRoomChat;

#ifdef __cplusplus
extern "C" {
#endif
void nativeRoomChat_Init(NativeRoomChat* chat, NativeSignalingClient* client);
void nativeRoomChat_Update(NativeRoomChat* chat, bool connected);
bool nativeRoomChat_Post(NativeRoomChat* chat, const char* text);
bool nativeRoomChat_Retry(NativeRoomChat* chat);
#ifdef __cplusplus
}
#endif
#endif
#endif
