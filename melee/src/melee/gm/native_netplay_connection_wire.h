#ifndef MELEE_GM_NATIVE_NETPLAY_CONNECTION_WIRE_H
#define MELEE_GM_NATIVE_NETPLAY_CONNECTION_WIRE_H

#ifdef MELEE_NATIVE

#include <stddef.h>
#include <stdint.h>

#include "native_netplay_ice.h"

typedef enum NativeConnectionMessageType {
    NativeConnectionMessage_GameDescription = 1,
    NativeConnectionMessage_ContentDescription,
    NativeConnectionMessage_Control
} NativeConnectionMessageType;
typedef struct NativeConnectionMessage {
    const uint8_t* payload;
    size_t size;
    uint32_t serial;
    NativeConnectionMessageType type;
    bool sender_owns_transfer;
} NativeConnectionMessage;
size_t nativeNetplayConnectionWire_EncodeControl(const uint8_t* bytes,
                                                 size_t size, uint8_t* output,
                                                 size_t capacity);
size_t nativeNetplayConnectionWire_EncodeContentDescription(
    const char* description, uint32_t serial, bool sender_owns_transfer,
    uint8_t* output, size_t capacity);
bool nativeNetplayConnectionWire_DecodeMessage(
    const uint8_t* bytes, size_t size, NativeConnectionMessage* result);

size_t nativeNetplayConnectionWire_EncodeDescription(const char* description,
                                                     uint8_t* output,
                                                     size_t output_capacity);
bool nativeNetplayConnectionWire_DecodeDescription(
    const uint8_t* message, size_t message_size, char* description,
    size_t description_capacity);

#endif
#endif
