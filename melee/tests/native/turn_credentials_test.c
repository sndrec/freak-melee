#include <assert.h>
#include <string.h>

#include "melee/gm/native_turn_credentials.h"

static void write_u16_be(uint8_t* bytes, uint16_t value)
{
    bytes[0] = (uint8_t) (value >> 8);
    bytes[1] = (uint8_t) value;
}

static void write_u32_be(uint8_t* bytes, uint32_t value)
{
    bytes[0] = (uint8_t) (value >> 24);
    bytes[1] = (uint8_t) (value >> 16);
    bytes[2] = (uint8_t) (value >> 8);
    bytes[3] = (uint8_t) value;
}

static size_t encode(uint8_t* output, bool relay, const char* stun,
                     const char* turn, const char* username,
                     const char* password)
{
    size_t lengths[4] = {
        strlen(stun),
        strlen(turn),
        strlen(username),
        strlen(password),
    };
    memcpy(output, "FMT1", 4);
    output[4] = 1;
    output[5] = relay ? 1 : 0;
    for (size_t i = 0; i < 4; ++i) {
        output[6 + i] = (uint8_t) lengths[i];
    }
    output[10] = 0;
    output[11] = 0;
    write_u32_be(output + 12, relay ? 86400 : 0);
    write_u16_be(output + 16, 3478);
    write_u16_be(output + 18, relay ? 3478 : 0);
    size_t offset = 20;
    const char* fields[4] = { stun, turn, username, password };
    for (size_t i = 0; i < 4; ++i) {
        memcpy(output + offset, fields[i], lengths[i]);
        offset += lengths[i];
    }
    return offset;
}

int main(void)
{
    uint8_t wire[NATIVE_TURN_WIRE_MAX_BYTES] = { 0 };
    NativeTurnCredentials decoded;
    size_t size = encode(wire, false, "stun.cloudflare.com", "", "", "");
    assert(nativeTurnCredentials_Decode(wire, size, &decoded));
    assert(!decoded.has_turn);
    assert(decoded.ttl_seconds == 0);
    assert(decoded.stun_port == 3478);
    assert(strcmp(decoded.stun_host, "stun.cloudflare.com") == 0);

    size = encode(wire, true, "stun.cloudflare.com", "turn.cloudflare.com",
                  "issued-user", "issued-password");
    assert(nativeTurnCredentials_Decode(wire, size, &decoded));
    assert(decoded.has_turn);
    assert(decoded.ttl_seconds == 86400);
    assert(decoded.turn_port == 3478);
    assert(strcmp(decoded.turn_host, "turn.cloudflare.com") == 0);
    assert(strcmp(decoded.username, "issued-user") == 0);
    assert(strcmp(decoded.password, "issued-password") == 0);

    assert(!nativeTurnCredentials_Decode(wire, size - 1, &decoded));
    wire[4] = 2;
    assert(!nativeTurnCredentials_Decode(wire, size, &decoded));
    wire[4] = 1;
    wire[5] = 2;
    assert(!nativeTurnCredentials_Decode(wire, size, &decoded));
    wire[5] = 1;
    wire[10] = 1;
    assert(!nativeTurnCredentials_Decode(wire, size, &decoded));
    wire[10] = 0;
    write_u16_be(wire + 18, 0);
    assert(!nativeTurnCredentials_Decode(wire, size, &decoded));
    write_u16_be(wire + 18, 3478);
    wire[20] = 0;
    assert(!nativeTurnCredentials_Decode(wire, size, &decoded));
    return 0;
}
