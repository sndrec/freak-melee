#ifndef MELEE_UI_NATIVE_CHAT_WIRE_H
#define MELEE_UI_NATIVE_CHAT_WIRE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum {
    NATIVE_CHAT_HISTORY = 64,
    NATIVE_CHAT_NAME_BYTES = 32,
    NATIVE_CHAT_TEXT_BYTES = 512,
    NATIVE_CHAT_PACKET_BYTES = 592,
    NATIVE_CHAT_PLAYERS = 128,
};
typedef struct NativeChatPlayer {
    char peer[33];
    char name[NATIVE_CHAT_NAME_BYTES + 1];
} NativeChatPlayer;
typedef struct NativeChatMessage {
    uint64_t id;
    uint32_t nonce;
    char peer[33];
    char name[NATIVE_CHAT_NAME_BYTES + 1];
    char text[NATIVE_CHAT_TEXT_BYTES + 1];
} NativeChatMessage;
typedef struct NativeChatPacket {
    NativeChatMessage message;
    NativeChatPlayer player;
    uint16_t online;
    uint8_t type;
} NativeChatPacket;
bool nativeChatWire_Decode(const uint8_t* bytes, size_t size,
                           NativeChatPacket* packet);
size_t nativeChatWire_Post(uint32_t nonce, const char* name, const char* text,
                           uint8_t* bytes, size_t capacity);
size_t nativeChatWire_Identify(const char* name, uint8_t* bytes,
                               size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
