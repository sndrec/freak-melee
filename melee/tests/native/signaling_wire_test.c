#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <melee/gm/native_signaling_wire.h>

static void put_u16(uint8_t* bytes, uint16_t value)
{
    bytes[0] = (uint8_t) (value >> 8);
    bytes[1] = (uint8_t) value;
}

static void put_u64(uint8_t* bytes, uint64_t value)
{
    for (int i = 7; i >= 0; --i) {
        bytes[i] = (uint8_t) value;
        value >>= 8;
    }
}

static size_t header(uint8_t* bytes, uint8_t type)
{
    memcpy(bytes, "FMS1", 4);
    bytes[4] = 5;
    bytes[5] = type;
    bytes[6] = 0;
    bytes[7] = 0;
    return 8;
}

static size_t string8(uint8_t* bytes, size_t offset, const char* value)
{
    size_t length = strlen(value);
    bytes[offset++] = (uint8_t) length;
    memcpy(bytes + offset, value, length);
    return offset + length;
}

static void finish(uint8_t* bytes, size_t size)
{
    put_u16(bytes + 6, (uint16_t) (size - 8));
}

static void test_client_encoding(void)
{
    uint8_t bytes[NATIVE_SIGNALING_MAX_MESSAGE_BYTES];
    const uint8_t selection[] = { 0x46, 0x4D, 0x53, 0x4C };
    NativeSignalingUpdate update = {
        .selection = selection,
        .selection_size = sizeof(selection),
        .local_players = 2,
        .active_mask = 1,
        .waiting_mask = 2,
        .ready = true,
        .has_active_mask = true,
        .has_waiting_mask = true,
        .has_local_players = true,
        .has_ready = true,
        .has_selection = true,
    };
    size_t size = nativeSignalingWire_EncodeUpdate(&update, bytes,
                                                   sizeof(bytes));
    const uint8_t expected[] = {
        'F', 'M', 'S', '1', 5, 1, 0, 11, 0x3E, 1, 2, 1,
        0, 4, 0x46, 0x4D, 0x53, 0x4C, 2,
    };
    assert(size == sizeof(expected));
    assert(memcmp(bytes, expected, sizeof(expected)) == 0);

    uint8_t full_roster[335] = { 0 };
    update.selection = full_roster;
    update.selection_size = sizeof(full_roster);
    size = nativeSignalingWire_EncodeUpdate(&update, bytes, sizeof(bytes));
    assert(size == 8 + 1 + 1 + 1 + 1 + 2 + sizeof(full_roster) + 1);
    assert(memcmp(bytes + size - 1 - sizeof(full_roster), full_roster,
                  sizeof(full_roster)) == 0);
    update.selection_size = NATIVE_SIGNALING_MAX_SELECTION_BYTES + 1;
    assert(nativeSignalingWire_EncodeUpdate(&update, bytes,
                                           sizeof(bytes)) == 0);

    const uint8_t candidate[] = { 0x04, 0x72, 0x00, 0xFF };
    size = nativeSignalingWire_EncodeSignal(
        "peer_two", candidate, sizeof(candidate), bytes, sizeof(bytes));
    assert(size == 8 + 1 + 8 + 2 + sizeof(candidate));
    assert(bytes[5] == 2 && bytes[8] == 8);
    assert(memcmp(bytes + 9, "peer_two", 8) == 0);
    assert(memcmp(bytes + size - sizeof(candidate), candidate,
                  sizeof(candidate)) == 0);

    size = nativeSignalingWire_EncodeRequestPresence(bytes, sizeof(bytes));
    assert(size == 8 && bytes[5] == 3 && bytes[6] == 0 && bytes[7] == 0);

    const uint8_t match[] = { 0x4D, 0x41, 0x54, 0x43, 0x48 };
    size = nativeSignalingWire_EncodeSetMatch(match, sizeof(match), bytes,
                                              sizeof(bytes));
    assert(size == 8 + 2 + sizeof(match));
    assert(bytes[5] == 4 && bytes[8] == 0 && bytes[9] == sizeof(match));
    assert(memcmp(bytes + 10, match, sizeof(match)) == 0);
    assert(nativeSignalingWire_EncodeSetMatch(
               match, NATIVE_SIGNALING_MAX_MATCH_BYTES + 1, bytes,
               sizeof(bytes)) == 0);

    size = nativeSignalingWire_EncodeAssignSeats(
        "peer_two", 2, 1, bytes, sizeof(bytes));
    const uint8_t assignment[] = {
        'F', 'M', 'S', '1', 5, 5, 0, 11,
        8, 'p', 'e', 'e', 'r', '_', 't', 'w', 'o', 2, 1,
    };
    assert(size == sizeof(assignment));
    assert(memcmp(bytes, assignment, sizeof(assignment)) == 0);
    assert(nativeSignalingWire_EncodeAssignSeats(
               "peer_two", 16, 0, bytes, sizeof(bytes)) == 0);

    size = nativeSignalingWire_EncodeSetRotation(
        NativeRoomRotation_WinnerStays, bytes, sizeof(bytes));
    assert(size == 9 && bytes[5] == 6 && bytes[8] == 1);
    assert(nativeSignalingWire_EncodeSetRotation(3, bytes, sizeof(bytes)) == 0);

    size = nativeSignalingWire_EncodeReportMatch(
        42, "peer_two", 1, true, bytes, sizeof(bytes));
    assert(size == 8 + 8 + 1 + 8 + 1 + 1);
    assert(bytes[5] == 7 && bytes[15] == 42 && bytes[16] == 8);
    assert(bytes[size - 2] == 1 && bytes[size - 1] == 1);
    assert(nativeSignalingWire_EncodeReportMatch(
               42, "peer_two", 4, true, bytes, sizeof(bytes)) == 0);
}

static void test_server_decoding(void)
{
    uint8_t bytes[NATIVE_SIGNALING_MAX_MESSAGE_BYTES] = { 0 };
    const uint8_t selection[] = { 0x46, 0x4D, 0x53, 0x4C };
    size_t size = header(bytes, 0x82);
    put_u64(bytes + size, 42);
    size += 8;
    size = string8(bytes, size, "peer_one");
    bytes[size++] = 2;

    size = string8(bytes, size, "peer_one");
    size = string8(bytes, size, "Alice");
    size = string8(bytes, size, "build-a");
    bytes[size++] = 2;
    bytes[size++] = 1;
    bytes[size++] = 2;
    bytes[size++] = 1;
    put_u64(bytes + size, 7);
    size += 8;
    size_t first_rank_offset = size;
    bytes[size++] = 0;
    bytes[size++] = 1;
    bytes[size++] = 0;
    bytes[size++] = 0;
    put_u16(bytes + size, sizeof(selection));
    size += 2;
    memcpy(bytes + size, selection, sizeof(selection));
    size += sizeof(selection);

    size = string8(bytes, size, "peer_two");
    size = string8(bytes, size, "Bob");
    size = string8(bytes, size, "build-a");
    bytes[size++] = 0;
    bytes[size++] = 0;
    bytes[size++] = 0;
    bytes[size++] = 0;
    put_u64(bytes + size, 9);
    size += 8;
    memset(bytes + size, 0, 4);
    size += 4;
    put_u16(bytes + size, 0);
    size += 2;
    put_u64(bytes + size, 3);
    size += 8;
    const uint8_t match[] = { 0x46, 0x4D, 0x4C };
    put_u16(bytes + size, sizeof(match));
    size += 2;
    memcpy(bytes + size, match, sizeof(match));
    size += sizeof(match);
    bytes[size++] = NativeRoomRotation_WinnerOut;
    finish(bytes, size);

    NativeSignalingServerMessage decoded;
    assert(nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    assert(decoded.type == NativeSignalingServerMessage_Presence);
    assert(decoded.data.presence.revision == 42);
    assert(strcmp(decoded.data.presence.owner_peer_id, "peer_one") == 0);
    assert(decoded.data.presence.participant_count == 2);
    assert(strcmp(decoded.data.presence.participants[0].name, "Alice") == 0);
    assert(decoded.data.presence.participants[0].local_players == 2);
    assert(decoded.data.presence.participants[0].active_mask == 1);
    assert(decoded.data.presence.participants[0].waiting_mask == 2);
    assert(decoded.data.presence.participants[0].waiting_ranks[1] == 1);
    assert(decoded.data.presence.participants[0].ready);
    assert(decoded.data.presence.participants[0].selection_size ==
           sizeof(selection));
    assert(memcmp(decoded.data.presence.participants[0].selection, selection,
                  sizeof(selection)) == 0);
    assert(decoded.data.presence.participants[1].active_mask == 0);
    assert(decoded.data.presence.participants[1].join_order == 9);
    assert(decoded.data.presence.participants[1].selection_size == 0);
    assert(decoded.data.presence.match_epoch == 3);
    assert(decoded.data.presence.match_size == sizeof(match));
    assert(decoded.data.presence.rotation_policy == NativeRoomRotation_WinnerOut);
    assert(memcmp(decoded.data.presence.match, match, sizeof(match)) == 0);

    assert(!nativeSignalingWire_DecodeServer(bytes, size - 1, &decoded));
    bytes[first_rank_offset + 1] = 0;
    assert(!nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    bytes[first_rank_offset + 1] = 1;
    bytes[4] = 2;
    assert(!nativeSignalingWire_DecodeServer(bytes, size, &decoded));
}

static void test_chat_wire(void)
{
    uint8_t bytes[1024];
    NativeSignalingServerMessage decoded;
    assert(nativeSignalingWire_EncodePostChat(0, "Hello", bytes, sizeof(bytes)) == 0);
    assert(nativeSignalingWire_EncodePostChat(1, "   ", bytes, sizeof(bytes)) == 0);
    assert(nativeSignalingWire_EncodePostChat(1, "bad\ntext", bytes, sizeof(bytes)) == 0);
    assert(nativeSignalingWire_EncodePostChat(1, "\xc0\xaf", bytes, sizeof(bytes)) == 0);
    size_t size = nativeSignalingWire_EncodePostChat(1, "Hello", bytes, sizeof(bytes));
    assert(size == 19 && bytes[5] == 8 && bytes[11] == 1 && bytes[13] == 5);
    assert(nativeSignalingWire_EncodePostChat(1, "Hello", bytes, 18) == 0);
    size = header(bytes, 0x85);
    put_u64(bytes + size, 42); size += 8;
    bytes[size++] = 0; bytes[size++] = 0; bytes[size++] = 0; bytes[size++] = 7;
    size = string8(bytes, size, "spectator_two");
    size = string8(bytes, size, "Reader");
    put_u16(bytes + size, 5); size += 2;
    memcpy(bytes + size, "Hello", 5); size += 5;
    finish(bytes, size);
    assert(nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    assert(decoded.type == NativeSignalingServerMessage_Chat);
    assert(decoded.data.chat.kind == NativeRoomChatEvent_Entry);
    assert(decoded.data.chat.entry.sequence == 42 && decoded.data.chat.entry.nonce == 7);
    assert(!strcmp(decoded.data.chat.entry.text, "Hello"));
    for (size_t i = 0; i < size; ++i) assert(!nativeSignalingWire_DecodeServer(bytes, i, &decoded));
    bytes[size - 1] = 0xff;
    assert(!nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    size = header(bytes, 0x86); bytes[size++] = 64; finish(bytes, size);
    assert(nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    bytes[8] = 65;
    assert(!nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    size = header(bytes, 0x87); put_u64(bytes + size, 42); size += 8; finish(bytes, size);
    assert(nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    size = header(bytes, 0x88); memset(bytes + size, 0, 4); size += 4;
    size = string8(bytes, size, "Wait a moment."); finish(bytes, size);
    assert(nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    assert(decoded.data.chat.kind == NativeRoomChatEvent_Rejected);
}

static void test_quick_wire(void)
{
    uint8_t bytes[160];
    NativeSignalingServerMessage decoded;
    size_t size = nativeSignalingWire_EncodeConfigureQuick(2, bytes, sizeof(bytes));
    assert(size == 9 && bytes[5] == 10 && bytes[8] == 2);
    assert(!nativeSignalingWire_EncodeConfigureQuick(3, bytes, sizeof(bytes)));
    assert(!nativeSignalingWire_EncodeConfigureQuick(1, bytes, 8));
    size = nativeSignalingWire_EncodeQueueQuick(true, bytes, sizeof(bytes));
    assert(size == 9 && bytes[5] == 11 && bytes[8] == 1);
    size = nativeSignalingWire_EncodeQueueQuick(false, bytes, sizeof(bytes));
    assert(size == 9 && bytes[8] == 0);
    assert(nativeSignalingWire_EncodeRequestQuick(bytes, sizeof(bytes)) == 8 && bytes[5] == 12);

    size = header(bytes, 0x89);
    bytes[size++] = 2; bytes[size++] = 2;
    put_u64(bytes + size, UINT64_C(0x0102030405060708)); size += 8;
    size = string8(bytes, size, "ABCDEF123456");
    size = string8(bytes, size, "0123456789abcdef0123456789abcdef");
    bytes[size++] = 1; bytes[size++] = 5; finish(bytes, size);
    assert(nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    assert(decoded.type == NativeSignalingServerMessage_QuickState);
    assert(decoded.data.quick.generation == UINT64_C(0x0102030405060708));
    assert(decoded.data.quick.team == 1 && decoded.data.quick.stage == 5);
    assert(!strcmp(decoded.data.quick.match_room, "ABCDEF123456"));
    assert(!strcmp(decoded.data.quick.admission, "0123456789abcdef0123456789abcdef"));
    for (size_t i = 0; i < size; ++i) assert(!nativeSignalingWire_DecodeServer(bytes, i, &decoded));
    bytes[size - 1] = 6;
    assert(!nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    bytes[size - 1] = 5; bytes[size - 2] = 2;
    assert(!nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    bytes[size - 2] = 1; bytes[19] = '/';
    assert(!nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    bytes[19] = 'A'; bytes[32] = '&';
    assert(!nativeSignalingWire_DecodeServer(bytes, size, &decoded));

    size = header(bytes, 0x89);
    bytes[size++] = 1; bytes[size++] = 1; put_u64(bytes + size, 9); size += 8;
    bytes[size++] = 0; bytes[size++] = 0; bytes[size++] = 0; bytes[size++] = 0;
    finish(bytes, size);
    assert(nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    assert(decoded.data.quick.state == NativeQuickState_Searching);
    size = header(bytes, 0x8A);
    size = string8(bytes, size, "Only the party owner can enter the queue."); finish(bytes, size);
    assert(nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    assert(decoded.type == NativeSignalingServerMessage_QuickRejected);
    assert(strstr(decoded.data.error, "party owner"));
}

int main(void)
{
    test_client_encoding();
    test_server_decoding();
    test_chat_wire();
    test_quick_wire();
    puts("native signaling wire tests passed");
    return 0;
}
