#include "native_chat_wire.h"
#include "../lb/native_utf8.h"

#include <string.h>

static uint64_t read_be(const uint8_t* p, size_t size)
{
    uint64_t result = 0;
    while (size--) {
        result = result * 256 + *p++;
    }
    return result;
}


bool nativeChatWire_Decode(const uint8_t* bytes, size_t size,
                           NativeChatPacket* packet)
{
    if (!bytes || !packet || size == 0 || size > NATIVE_CHAT_PACKET_BYTES) {
        return false;
    }
    memset(packet, 0, sizeof(*packet));
    packet->type = bytes[0];
    if (bytes[0] == 0x84 || bytes[0] == 0x87) {
        return size == 1;
    }
    if (bytes[0] == 0x82 || bytes[0] == 0x85) {
        if (size != 3) {
            return false;
        }
        packet->online = (uint16_t) read_be(bytes + 1, 2);
        return packet->online <= NATIVE_CHAT_PLAYERS;
    }
    if (bytes[0] == 0x86) {
        if (size < 35 || !bytes[33] || bytes[33] > NATIVE_CHAT_NAME_BYTES ||
            size != 34 + bytes[33] || !nativeUtf8_IsVisibleText(bytes + 34, bytes[33]))
        {
            return false;
        }
        for (unsigned i = 1; i < 33; ++i) {
            if (!((bytes[i] >= '0' && bytes[i] <= '9') ||
                  (bytes[i] >= 'a' && bytes[i] <= 'f')))
            {
                return false;
            }
        }
        memcpy(packet->player.peer, bytes + 1, 32);
        memcpy(packet->player.name, bytes + 34, bytes[33]);
        return true;
    }
    if (bytes[0] != 0x81 || size < 48) {
        return false;
    }
    size_t name_size = bytes[45];
    size_t text_size = (size_t) read_be(bytes + 46, 2);
    if (!name_size || name_size > NATIVE_CHAT_NAME_BYTES || !text_size ||
        text_size > NATIVE_CHAT_TEXT_BYTES ||
        size != 48 + name_size + text_size ||
        !nativeUtf8_IsVisibleText(bytes + 48, name_size) ||
        !nativeUtf8_IsVisibleText(bytes + 48 + name_size, text_size))
    {
        return false;
    }
    for (size_t i = 13; i < 45; ++i) {
        if (!((bytes[i] >= '0' && bytes[i] <= '9') ||
              (bytes[i] >= 'a' && bytes[i] <= 'f')))
        {
            return false;
        }
    }
    NativeChatMessage* message = &packet->message;
    message->id = read_be(bytes + 1, 8);
    message->nonce = (uint32_t) read_be(bytes + 9, 4);
    memcpy(message->peer, bytes + 13, 32);
    memcpy(message->name, bytes + 48, name_size);
    memcpy(message->text, bytes + 48 + name_size, text_size);
    return message->id != 0 && message->nonce != 0;
}

size_t nativeChatWire_Identify(const char* name, uint8_t* bytes,
                               size_t capacity)
{
    if (!name || !bytes) {
        return 0;
    }
    size_t size = strnlen(name, NATIVE_CHAT_NAME_BYTES + 1);
    if (!size || size > NATIVE_CHAT_NAME_BYTES || capacity < 2 + size ||
        !nativeUtf8_IsVisibleText((const uint8_t*) name, size))
    {
        return 0;
    }
    bytes[0] = 3;
    bytes[1] = (uint8_t) size;
    memcpy(bytes + 2, name, size);
    return 2 + size;
}

size_t nativeChatWire_Post(uint32_t nonce, const char* name, const char* text,
                           uint8_t* bytes, size_t capacity)
{
    if (!nonce || !name || !text || !bytes) {
        return 0;
    }
    size_t n = strnlen(name, NATIVE_CHAT_NAME_BYTES + 1);
    size_t t = strnlen(text, NATIVE_CHAT_TEXT_BYTES + 1);
    if (!n || n > NATIVE_CHAT_NAME_BYTES || !t || t > NATIVE_CHAT_TEXT_BYTES ||
        capacity < 8 + n + t || !nativeUtf8_IsVisibleText((const uint8_t*) name, n) ||
        !nativeUtf8_IsVisibleText((const uint8_t*) text, t))
    {
        return 0;
    }
    bytes[0] = 1;
    for (int i = 0; i < 4; ++i) {
        bytes[1 + i] = (uint8_t) (nonce >> (24 - i * 8));
    }
    bytes[5] = (uint8_t) n;
    bytes[6] = (uint8_t) (t >> 8);
    bytes[7] = (uint8_t) t;
    memcpy(bytes + 8, name, n);
    memcpy(bytes + 8 + n, text, t);
    return 8 + n + t;
}
