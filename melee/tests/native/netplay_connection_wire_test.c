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
    uint8_t encoded[NATIVE_NETPLAY_ICE_DESCRIPTION_CAPACITY + NATIVE_GAME_SIGNAL_HEADER_BYTES];
    NativeRouteGeneration from = {0x0102030405060708ULL, 2}, to = {9, 3}, unknown = {0};
    NativeConnectionMessage message;
    size_t size = nativeNetplayConnectionWire_EncodeGameRoute(from, unknown, NULL, encoded, sizeof(encoded));
    require(size == NATIVE_GAME_SIGNAL_HEADER_BYTES &&
        nativeNetplayConnectionWire_DecodeMessage(encoded, size, &message) &&
        message.type == NativeConnectionMessage_GameHello && !message.recipient.attempt &&
        message.sender.revision == from.revision && message.sender.attempt == from.attempt,
        "announce the new route before either side has an SDP");
    require(!nativeNetplayConnectionWire_EncodeGameRoute(from, unknown, description, encoded, sizeof(encoded)),
        "a description needs both generation tags");
    size = nativeNetplayConnectionWire_EncodeGameRoute(from, to, description, encoded, sizeof(encoded));
    require(size == sizeof(description) - 1 + NATIVE_GAME_SIGNAL_HEADER_BYTES &&
        nativeNetplayConnectionWire_DecodeMessage(encoded, size, &message) &&
        message.type == NativeConnectionMessage_GameDescription &&
        message.sender.revision == from.revision && message.sender.attempt == from.attempt &&
        message.recipient.revision == to.revision && message.recipient.attempt == to.attempt &&
        message.size == sizeof(description)-1 && !memcmp(message.payload, description, message.size),
        "preserve the gameplay description and both peer generations");
    require(!nativeNetplayConnectionWire_DecodeMessage(encoded, size-1, &message), "refuse partial SDP");
    encoded[NATIVE_GAME_SIGNAL_HEADER_BYTES] = 0;
    require(!nativeNetplayConnectionWire_DecodeMessage(encoded, size, &message), "refuse embedded terminators");
    size = nativeNetplayConnectionWire_EncodeContentDescription(
        description, 0xFEDCBA98, false, encoded, sizeof(encoded));
    require(
        nativeNetplayConnectionWire_DecodeMessage(encoded, size, &message) &&
            message.type == NativeConnectionMessage_ContentDescription &&
            message.serial == 0xFEDCBA98 && !message.sender_owns_transfer &&
            message.size == sizeof(description) - 1 &&
            !memcmp(message.payload, description, message.size),
        "content descriptions retain the transfer owner and serial");
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
