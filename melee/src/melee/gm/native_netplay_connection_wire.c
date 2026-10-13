#include "native_netplay_connection_wire.h"

#ifdef MELEE_NATIVE

#include <string.h>

enum {
    CONNECTION_MAGIC = 0x464D4331,
    CONNECTION_VERSION = 1,
    CONNECTION_DESCRIPTION = 1,
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

size_t nativeNetplayConnectionWire_EncodeDescription(const char* description,
                                                     uint8_t* output,
                                                     size_t output_capacity)
{
    if (description == NULL || output == NULL) {
        return 0;
    }
    size_t length =
        strnlen(description, NATIVE_NETPLAY_ICE_DESCRIPTION_CAPACITY);
    if (length == 0 || length >= NATIVE_NETPLAY_ICE_DESCRIPTION_CAPACITY ||
        length > UINT16_MAX ||
        output_capacity < CONNECTION_HEADER_BYTES + length)
    {
        return 0;
    }
    write_u32_be(output, CONNECTION_MAGIC);
    output[4] = CONNECTION_VERSION;
    output[5] = CONNECTION_DESCRIPTION;
    write_u16_be(output + 6, (uint16_t) length);
    memcpy(output + CONNECTION_HEADER_BYTES, description, length);
    return CONNECTION_HEADER_BYTES + length;
}

bool nativeNetplayConnectionWire_DecodeDescription(const uint8_t* message,
                                                   size_t message_size,
                                                   char* description,
                                                   size_t description_capacity)
{
    if (message == NULL || description == NULL ||
        message_size < CONNECTION_HEADER_BYTES ||
        read_u32_be(message) != CONNECTION_MAGIC ||
        message[4] != CONNECTION_VERSION ||
        message[5] != CONNECTION_DESCRIPTION)
    {
        return false;
    }
    uint16_t length = read_u16_be(message + 6);
    if (length == 0 || message_size != CONNECTION_HEADER_BYTES + length ||
        description_capacity <= length ||
        memchr(message + CONNECTION_HEADER_BYTES, '\0', length) != NULL)
    {
        return false;
    }
    memcpy(description, message + CONNECTION_HEADER_BYTES, length);
    description[length] = '\0';
    return true;
}

#endif
