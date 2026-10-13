#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <melee/gm/native_netplay_connection_wire.h>

static void require(bool condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

int main(void)
{
    static const char description[] =
        "a=ice-ufrag:friend\r\na=ice-pwd:private\r\na=candidate:1 1 UDP";
    uint8_t encoded[NATIVE_NETPLAY_ICE_DESCRIPTION_CAPACITY + 8];
    size_t size = nativeNetplayConnectionWire_EncodeDescription(
        description, encoded, sizeof(encoded));
    require(size == sizeof(description) - 1 + 8,
            "encode one bounded ICE description");
    require(encoded[0] == 'F' && encoded[1] == 'M' && encoded[2] == 'C' &&
                encoded[3] == '1' && encoded[4] == 1 && encoded[5] == 1,
            "write the versioned connection envelope");

    char decoded[NATIVE_NETPLAY_ICE_DESCRIPTION_CAPACITY];
    require(nativeNetplayConnectionWire_DecodeDescription(
                encoded, size, decoded, sizeof(decoded)) &&
                strcmp(decoded, description) == 0,
            "decode the exact ICE description");
    encoded[4] = 2;
    require(!nativeNetplayConnectionWire_DecodeDescription(
                encoded, size, decoded, sizeof(decoded)),
            "reject an unsupported connection protocol version");
    encoded[4] = 1;
    require(!nativeNetplayConnectionWire_DecodeDescription(
                encoded, size - 1, decoded, sizeof(decoded)),
            "reject a truncated connection message");
    encoded[8] = 0;
    require(!nativeNetplayConnectionWire_DecodeDescription(
                encoded, size, decoded, sizeof(decoded)),
            "reject embedded terminators in text descriptions");
    puts("Connection signaling uses a bounded versioned ICE envelope");
    return 0;
}
