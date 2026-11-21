#include "native_netplay_connection_wire.h"

#ifdef MELEE_NATIVE

#include <string.h>

enum {
    CONNECTION_MAGIC = 0x464D4331,
    CONNECTION_VERSION = 1,
    CONNECTION_HEADER_BYTES = 8,
};

static void write_u16_be(uint8_t* output, uint16_t value)
{
    output[0] = (uint8_t) (value >> 8);
    output[1] = (uint8_t) value;
}

static void write_u32_be(uint8_t* output, uint32_t value)
{
    output[0] = (uint8_t) (value >> 24);
    output[1] = (uint8_t) (value >> 16);
    output[2] = (uint8_t) (value >> 8);
    output[3] = (uint8_t) value;
}

static uint16_t read_u16_be(const uint8_t* input)
{
    return (uint16_t) (((uint16_t) input[0] << 8) | input[1]);
}

static uint32_t read_u32_be(const uint8_t* input)
{
    return ((uint32_t) input[0] << 24) | ((uint32_t) input[1] << 16) |
           ((uint32_t) input[2] << 8) | input[3];
}

static void write_generation(uint8_t* out, NativeRouteGeneration generation)
{
    write_u32_be(out, (uint32_t)(generation.revision >> 32));
    write_u32_be(out + 4, (uint32_t)generation.revision);
    write_u32_be(out + 8, generation.attempt);
}

static NativeRouteGeneration read_generation(const uint8_t* bytes)
{
    return (NativeRouteGeneration){
        ((uint64_t)read_u32_be(bytes) << 32) | read_u32_be(bytes + 4),
        read_u32_be(bytes + 8)
    };
}

size_t nativeNetplayConnectionWire_EncodeGameRoute(NativeRouteGeneration sender,
    NativeRouteGeneration recipient, const char* description, uint8_t* output, size_t capacity)
{
    size_t length = description ? strnlen(description, NATIVE_NETPLAY_ICE_DESCRIPTION_CAPACITY) : 0;
    if (!output || !sender.revision || !sender.attempt ||
        (!!recipient.revision != !!recipient.attempt) ||
        (description && (!recipient.attempt || !length || length >= NATIVE_NETPLAY_ICE_DESCRIPTION_CAPACITY)) ||
        capacity < NATIVE_GAME_SIGNAL_HEADER_BYTES + length) return 0;
    write_u32_be(output, CONNECTION_MAGIC);
    output[4] = CONNECTION_VERSION;
    output[5] = description ? NativeConnectionMessage_GameDescription : NativeConnectionMessage_GameHello;
    write_u16_be(output + 6, (uint16_t)(24 + length));
    write_generation(output + 8, sender);
    write_generation(output + 20, recipient);
    if (length) memcpy(output + NATIVE_GAME_SIGNAL_HEADER_BYTES, description, length);
    return NATIVE_GAME_SIGNAL_HEADER_BYTES + length;
}

size_t nativeNetplayConnectionWire_EncodeControl(const uint8_t* bytes,
                                                 size_t size, uint8_t* output,
                                                 size_t capacity)
{
    if (!bytes || !output || !size ||
        size > NATIVE_NETPLAY_INPUT_PACKET_CAPACITY ||
        capacity < CONNECTION_HEADER_BYTES + size)
    {
        return 0;
    }
    write_u32_be(output, CONNECTION_MAGIC);
    output[4] = CONNECTION_VERSION;
    output[5] = NativeConnectionMessage_Control;
    write_u16_be(output + 6, (uint16_t) size);
    memcpy(output + CONNECTION_HEADER_BYTES, bytes, size);
    return CONNECTION_HEADER_BYTES + size;
}
size_t nativeNetplayConnectionWire_EncodeContentDescription(
    const char* description, uint32_t serial, bool sender_owner,
    uint8_t* output, size_t capacity)
{
    if (!description || !serial || !output) {
        return 0;
    }
    size_t length =
        strnlen(description, NATIVE_NETPLAY_ICE_DESCRIPTION_CAPACITY);
    if (!length || length >= NATIVE_NETPLAY_ICE_DESCRIPTION_CAPACITY ||
        capacity < 13 + length)
    {
        return 0;
    }
    write_u32_be(output, CONNECTION_MAGIC);
    output[4] = CONNECTION_VERSION;
    output[5] = NativeConnectionMessage_ContentDescription;
    write_u16_be(output + 6, (uint16_t) (5 + length));
    write_u32_be(output + 8, serial);
    output[12] = sender_owner;
    memcpy(output + 13, description, length);
    return 13 + length;
}
bool nativeNetplayConnectionWire_DecodeMessage(const uint8_t* bytes,
                                               size_t size,
                                               NativeConnectionMessage* result)
{
    if (!bytes || !result || size < 8 ||
        read_u32_be(bytes) != CONNECTION_MAGIC ||
        bytes[4] != CONNECTION_VERSION ||
        size != 8 + (size_t) read_u16_be(bytes + 6))
    {
        return false;
    }
    *result = (NativeConnectionMessage){ .type = bytes[5],
                                         .payload = bytes + 8,
                                         .size = size - 8 };
    if (result->type == NativeConnectionMessage_Control) {
        return result->size > 0 &&
               result->size <= NATIVE_NETPLAY_INPUT_PACKET_CAPACITY;
    }
    if (result->type == NativeConnectionMessage_ContentDescription) {
        if (result->size <= 5 || !read_u32_be(bytes + 8) || bytes[12] > 1) {
            return false;
        }
        result->serial = read_u32_be(bytes + 8);
        result->sender_owns_transfer = bytes[12];
        result->payload += 5;
        result->size -= 5;
    } else if (result->type == NativeConnectionMessage_GameHello ||
               result->type == NativeConnectionMessage_GameDescription) {
        if (size < NATIVE_GAME_SIGNAL_HEADER_BYTES) return false;
        result->sender = read_generation(bytes + 8);
        result->recipient = read_generation(bytes + 20);
        if (!result->sender.revision || !result->sender.attempt ||
            (!!result->recipient.revision != !!result->recipient.attempt)) return false;
        result->payload += 24;
        result->size -= 24;
        if (result->type == NativeConnectionMessage_GameHello) return result->size == 0;
        if (!result->recipient.attempt) return false;
    } else {
        return false;
    }
    return result->size > 0 &&
           result->size < NATIVE_NETPLAY_ICE_DESCRIPTION_CAPACITY &&
           !memchr(result->payload, 0, result->size);
}

#endif
