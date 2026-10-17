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
    uint8_t encoded[NATIVE_NETPLAY_ICE_DESCRIPTION_CAPACITY + 13];
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
    NativeConnectionMessage message;
    size = nativeNetplayConnectionWire_EncodeContentDescription(
        description, 0xFEDCBA98, false, encoded, sizeof(encoded));
    require(
        nativeNetplayConnectionWire_DecodeMessage(encoded, size, &message) &&
            message.type == NativeConnectionMessage_ContentDescription &&
            message.serial == 0xFEDCBA98 && !message.sender_owns_transfer &&
            message.size == sizeof(description) - 1 &&
            !memcmp(message.payload, description, message.size),
        "content descriptions retain the transfer owner and serial");
    require(!nativeNetplayConnectionWire_DecodeDescription(
                encoded, size, decoded, sizeof(decoded)),
            "content descriptions cannot be applied to gameplay routes");
    encoded[12] = 2;
    require(
        !nativeNetplayConnectionWire_DecodeMessage(encoded, size, &message),
        "reject invalid ownership");
    encoded[12] = 0;
    memset(encoded + 8, 0, 4);
    require(
        !nativeNetplayConnectionWire_DecodeMessage(encoded, size, &message),
        "reject zero transfer serial");
    const uint8_t control[] = { 0, 1, 255, 0, 42 };
    size = nativeNetplayConnectionWire_EncodeControl(control, sizeof(control),
                                                     encoded, sizeof(encoded));
    require(
        nativeNetplayConnectionWire_DecodeMessage(encoded, size, &message) &&
            message.type == NativeConnectionMessage_Control &&
            message.size == sizeof(control) &&
            !memcmp(message.payload, control, sizeof(control)),
        "control messages retain binary contents");
    require(!nativeNetplayConnectionWire_EncodeControl(control, 0, encoded,
                                                       sizeof(encoded)) &&
                !nativeNetplayConnectionWire_EncodeControl(
                    control, NATIVE_NETPLAY_INPUT_PACKET_CAPACITY + 1, encoded,
                    sizeof(encoded)),
            "bound control payloads");
    require(!nativeNetplayConnectionWire_DecodeMessage(encoded, size - 1,
                                                       &message),
            "reject partial control payloads");
    puts("Gameplay, content descriptions and binary controls have distinct "
         "bounded envelopes");
    return 0;
}
