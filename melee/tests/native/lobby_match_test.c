#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <melee/gm/native_lobby_match.h>

static NativeLobbyMatch example(void)
{
    NativeLobbyMatch match = {
        .session_id = UINT64_C(0xA11CE5E5510),
        .stage_id = 0x20,
        .item_mask = UINT64_MAX,
        .peer_count = 2,
        .mode = NativeLobbyMatchMode_Stock,
        .time_limit_minutes = 2,
        .stock_time_limit_minutes = 0,
        .stock_count = 4,
        .item_frequency = 2,
        .handicap = 0,
        .damage_ratio = 10,
        .teams = true,
        .friendly_fire = true,
        .pause = true,
        .score_display = false,
        .peers = {
            { .peer_id = "peer_one", .numeric_peer = 0, .port_mask = 0x5 },
            { .peer_id = "peer_two", .numeric_peer = 1, .port_mask = 0xA },
        },
        .slots = {
            { .character_id = 0x1A, .costume_id = 2,
              .kind = NativeLobbyMatchSlot_Human, .team = 0,
              .handicap = 9 },
            { .character_id = 0x13, .costume_id = 1,
              .kind = NativeLobbyMatchSlot_Human, .team = 1,
              .handicap = 9, .local_seat = 2 },
            { .character_id = 0x0F, .kind = NativeLobbyMatchSlot_Cpu,
              .cpu_level = 9, .team = 0, .handicap = 9 },
            { .kind = NativeLobbyMatchSlot_Empty },
        },
    };
    for (uint32_t i = 0; i < sizeof(match.content_digest); ++i) {
        match.content_digest[i] = (uint8_t) (i * 7 + 3);
        match.slots[0].character_package_id.bytes[i] = (uint8_t) (i + 1);
        match.slots[0].costume_package_id.bytes[i] = (uint8_t) (i + 33);
    }
    return match;
}

int main(void)
{
    NativeLobbyMatch original = example();
    assert(nativeLobbyMatch_Validate(&original));

    uint8_t bytes[NATIVE_LOBBY_MATCH_MAX_WIRE_BYTES];
    size_t size = nativeLobbyMatch_Encode(&original, bytes, sizeof(bytes));
    assert(size != 0 && size < sizeof(bytes));

    NativeLobbyMatch decoded;
    assert(nativeLobbyMatch_Decode(&decoded, bytes, size));
    assert(decoded.session_id == original.session_id);
    assert(decoded.stage_id == original.stage_id);
    assert(decoded.item_mask == original.item_mask);
    assert(decoded.teams && decoded.friendly_fire && decoded.pause);
    assert(decoded.peer_count == 2);
    assert(strcmp(decoded.peers[1].peer_id, "peer_two") == 0);
    assert(decoded.peers[1].port_mask == 0xA);
    assert(decoded.slots[0].character_id == 0x1A);
    assert(decoded.slots[1].costume_id == 1);
    assert(decoded.slots[1].local_seat == 2);
    assert(decoded.slots[2].cpu_level == 9);
    assert(memcmp(&decoded.slots[0].character_package_id,
                  &original.slots[0].character_package_id,
                  sizeof(NativeContentId)) == 0);
    assert(memcmp(&decoded.slots[0].costume_package_id,
                  &original.slots[0].costume_package_id,
                  sizeof(NativeContentId)) == 0);
    assert(memcmp(decoded.content_digest, original.content_digest,
                  sizeof(decoded.content_digest)) == 0);

    NativeLobbyMatch custom = original;
    custom.slots[0].character_id = 0;
    custom.slots[0].costume_id = 0;
    assert(nativeLobbyMatch_Validate(&custom));
    size_t custom_size = nativeLobbyMatch_Encode(&custom, bytes,
                                                  sizeof(bytes));
    assert(custom_size == size);
    assert(nativeLobbyMatch_Decode(&decoded, bytes, custom_size));
    assert(decoded.slots[0].character_id == 0);
    assert(decoded.slots[0].costume_id == 0);
    assert(memcmp(&decoded.slots[0].character_package_id,
                  &custom.slots[0].character_package_id,
                  sizeof(NativeContentId)) == 0);
    assert(memcmp(&decoded.slots[0].costume_package_id,
                  &custom.slots[0].costume_package_id,
                  sizeof(NativeContentId)) == 0);
    size = nativeLobbyMatch_Encode(&original, bytes, sizeof(bytes));
    assert(size != 0);
    assert(nativeLobbyMatch_Decode(&decoded, bytes, size));

    uint8_t encoded_again[NATIVE_LOBBY_MATCH_MAX_WIRE_BYTES];
    size_t second_size = nativeLobbyMatch_Encode(&decoded, encoded_again,
                                                 sizeof(encoded_again));
    assert(second_size == size && memcmp(bytes, encoded_again, size) == 0);
    assert(!nativeLobbyMatch_Decode(&decoded, bytes, size - 1));
    bytes[size] = 0;
    assert(!nativeLobbyMatch_Decode(&decoded, bytes, size + 1));

    NativeLobbyMatch invalid = original;
    invalid.peers[1].port_mask = 0x5;
    assert(!nativeLobbyMatch_Validate(&invalid));
    invalid = original;
    strcpy(invalid.peers[1].peer_id, "peer_one");
    assert(!nativeLobbyMatch_Validate(&invalid));
    invalid = original;
    NativeLobbyMatchPeer peer = invalid.peers[0];
    invalid.peers[0] = invalid.peers[1];
    invalid.peers[1] = peer;
    assert(!nativeLobbyMatch_Validate(&invalid));
    invalid = original;
    invalid.item_frequency = -1;
    assert(!nativeLobbyMatch_Validate(&invalid));
    invalid = original;
    invalid.slots[1].kind = NativeLobbyMatchSlot_Empty;
    assert(!nativeLobbyMatch_Validate(&invalid));
    invalid = original;
    invalid.slots[3].costume_package_id.bytes[0] = 1;
    assert(!nativeLobbyMatch_Validate(&invalid));
    invalid = original;
    invalid.teams = true;
    invalid.slots[1].team = 0;
    assert(!nativeLobbyMatch_Validate(&invalid));
    invalid = original;
    memset(invalid.content_digest, 0, sizeof(invalid.content_digest));
    assert(!nativeLobbyMatch_Validate(&invalid));
    invalid = original;
    invalid.session_id = 0;
    assert(!nativeLobbyMatch_Validate(&invalid));
    invalid = original;
    invalid.slots[1].kind = NativeLobbyMatchSlot_Cpu;
    invalid.slots[1].cpu_level = 9;
    assert(!nativeLobbyMatch_Validate(&invalid));
    invalid = original;
    invalid.slots[1].local_seat = 4;
    assert(!nativeLobbyMatch_Validate(&invalid));
    invalid = original;
    invalid.slots[2].kind = NativeLobbyMatchSlot_Human;
    invalid.slots[2].cpu_level = 0;
    assert(!nativeLobbyMatch_Validate(&invalid));
    invalid.slots[2].local_seat = 1;
    assert(nativeLobbyMatch_Validate(&invalid));

    uint8_t oversized[1024];
    assert(nativeLobbyMatch_Encode(&original, oversized, sizeof(oversized)) ==
           size);

    puts("canonical lobby match proposals round-trip and reject ambiguity");
    return 0;
}
