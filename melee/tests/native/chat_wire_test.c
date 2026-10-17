#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <melee/ui/native_chat_wire.h>

int main(void)
{
    uint8_t bytes[NATIVE_CHAT_PACKET_BYTES] = { 0x81 };
    // An independently specified server record, including multibyte UTF-8.
    bytes[8] = 7;
    bytes[12] = 9;
    memset(bytes + 13, 'a', 32);
    bytes[45] = 3;
    bytes[47] = 4;
    memcpy(bytes + 48, "A\xc3\xa9", 3);
    memcpy(bytes + 51, "<hi>", 4);
    NativeChatPacket packet;
    assert(nativeChatWire_Decode(bytes, 55, &packet));
    assert(packet.message.id == 7 && packet.message.nonce == 9);
    assert(strcmp(packet.message.name, "A\xc3\xa9") == 0);
    assert(strcmp(packet.message.text, "<hi>") == 0);
    for (size_t size = 0; size < 55; ++size) {
        assert(!nativeChatWire_Decode(bytes, size, &packet));
    }
    assert(!nativeChatWire_Decode(bytes, 56, &packet));
    bytes[49] = 0xc0;
    assert(!nativeChatWire_Decode(bytes, 55, &packet));
    bytes[49] = 0xc3;
    bytes[13] = 'z';
    assert(!nativeChatWire_Decode(bytes, 55, &packet));

    size_t size =
        nativeChatWire_Post(0x01020304, "A", "hi", bytes, sizeof(bytes));
    const uint8_t expected[] = { 1, 1, 2, 3, 4, 1, 0, 2, 'A', 'h', 'i' };
    assert(size == sizeof(expected) && memcmp(bytes, expected, size) == 0);
    assert(!nativeChatWire_Post(0, "A", "hi", bytes, sizeof(bytes)));
    assert(!nativeChatWire_Post(1, "A", "  ", bytes, sizeof(bytes)));
    assert(!nativeChatWire_Post(1, "A", "\xed\xa0\x80", bytes, sizeof(bytes)));
    assert(!nativeChatWire_Post(1, "A", "\xf4\x90\x80\x80", bytes,
                                sizeof(bytes)));
    assert(!nativeChatWire_Post(1, "A", "\xf0\x9f\x92", bytes, sizeof(bytes)));
    assert(!nativeChatWire_Post(1, "A", "line\nbreak", bytes, sizeof(bytes)));
    assert(nativeChatWire_Post(1, "A", "\xf0\x9f\x92\x9b", bytes,
                               sizeof(bytes)) == 13);
    char text[NATIVE_CHAT_TEXT_BYTES + 2];
    memset(text, 'a', sizeof(text));
    text[512] = 0;
    assert(nativeChatWire_Post(1, "A", text, bytes, sizeof(bytes)) == 521);
    text[512] = 'a';
    text[513] = 0;
    assert(!nativeChatWire_Post(1, "A", text, bytes, sizeof(bytes)));
    const uint8_t ready[] = { 0x82, 0, 5 };
    assert(nativeChatWire_Decode(ready, sizeof(ready), &packet));
    assert(packet.online == 5);
    const uint8_t begin[] = { 0x85, 0, 128 }, end[] = { 0x87 };
    assert(nativeChatWire_Decode(begin, sizeof(begin), &packet));
    assert(packet.online == NATIVE_CHAT_PLAYERS);
    assert(nativeChatWire_Decode(end, sizeof(end), &packet));
    const uint8_t oversized[] = { 0x85, 0, 129 };
    assert(!nativeChatWire_Decode(oversized, sizeof(oversized), &packet));
    memset(bytes, 0, sizeof(bytes));
    bytes[0] = 0x86;
    memset(bytes + 1, 'b', 32);
    bytes[33] = 3;
    memcpy(bytes + 34, "A\xc3\xa9", 3);
    assert(nativeChatWire_Decode(bytes, 37, &packet));
    assert(!strcmp(packet.player.name, "A\xc3\xa9"));
    assert(!nativeChatWire_Decode(bytes, 36, &packet));
    bytes[1] = 'z';
    assert(!nativeChatWire_Decode(bytes, 37, &packet));
    const uint8_t identity[] = { 3, 3, 'A', 0xc3, 0xa9 };
    assert(nativeChatWire_Identify("A\xc3\xa9", bytes, sizeof(bytes)) ==
           sizeof(identity));
    assert(!memcmp(bytes, identity, sizeof(identity)));
    assert(!nativeChatWire_Identify(" ", bytes, sizeof(bytes)));
    puts("chat wire: bounded UTF-8, server fields, and exact client encoding "
         "passed");
    return 0;
}
