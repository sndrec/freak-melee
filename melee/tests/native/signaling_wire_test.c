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

int main(void)
{
    test_client_encoding();
    test_server_decoding();
    puts("native signaling wire tests passed");
    return 0;
}
