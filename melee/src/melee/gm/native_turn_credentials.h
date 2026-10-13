#ifndef MELEE_GM_NATIVE_TURN_CREDENTIALS_H
#define MELEE_GM_NATIVE_TURN_CREDENTIALS_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    NATIVE_TURN_SERVER_BYTES = 256,
    NATIVE_TURN_CREDENTIAL_BYTES = 256,
    NATIVE_TURN_WIRE_MAX_BYTES = 1040,
};

typedef struct NativeTurnCredentials {
    char stun_host[NATIVE_TURN_SERVER_BYTES];
    char turn_host[NATIVE_TURN_SERVER_BYTES];
    char username[NATIVE_TURN_CREDENTIAL_BYTES];
    char password[NATIVE_TURN_CREDENTIAL_BYTES];
    uint32_t ttl_seconds;
    uint16_t stun_port;
    uint16_t turn_port;
    bool has_turn;
} NativeTurnCredentials;

bool nativeTurnCredentials_Decode(const uint8_t* bytes, size_t size,
                                  NativeTurnCredentials* credentials);
bool nativeTurnCredentials_Fetch(const char* signaling_endpoint,
                                 NativeTurnCredentials* credentials);

#endif
#endif
