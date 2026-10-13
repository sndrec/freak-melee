#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <melee/gm/native_lobby_selection.h>

static NativeLobbySelection example(void)
{
    return (NativeLobbySelection) {
        .players = {
            {
                .character_id = UINT64_C(0x464D434800000003),
                .costume_id = UINT64_C(0x464D435300000301),
                .team = 0,
                .selected = true,
            },
            {
                .character_id = UINT64_C(0x464D434800000015),
                .costume_id = UINT64_C(0x464D435300001502),
                .team = 1,
                .selected = true,
            },
        },
        .player_count = 2,
        .confirmed = true,
    };
}

int main(void)
{
    NativeLobbySelection original = example();
    uint8_t bytes[NATIVE_LOBBY_SELECTION_MAX_WIRE_BYTES];
    size_t size = nativeLobbySelection_Encode(&original, bytes, sizeof(bytes));
    assert(size == 43);
    assert(memcmp(bytes, "FMSL", 4) == 0);

    NativeLobbySelection decoded;
    assert(nativeLobbySelection_Decode(&decoded, bytes, size));
    assert(decoded.player_count == original.player_count);
    assert(decoded.confirmed == original.confirmed);
    assert(!decoded.start_requested);
    for (uint32_t i = 0; i < decoded.player_count; ++i) {
        assert(decoded.players[i].character_id ==
               original.players[i].character_id);
        assert(decoded.players[i].costume_id ==
               original.players[i].costume_id);
        assert(decoded.players[i].team == original.players[i].team);
        assert(decoded.players[i].selected == original.players[i].selected);
    }

    original.start_requested = true;
    assert(nativeLobbySelection_Encode(&original, bytes, sizeof(bytes)) == size);
    assert(nativeLobbySelection_Decode(&decoded, bytes, size));
    assert(decoded.confirmed && decoded.start_requested);
    bytes[6] = 2; // Starting requires a confirmed character selection.
    assert(!nativeLobbySelection_Decode(&decoded, bytes, size));
    bytes[6] = 7; // Remaining flag bits are not assigned.
    assert(!nativeLobbySelection_Decode(&decoded, bytes, size));
    assert(nativeLobbySelection_Encode(&original, bytes, sizeof(bytes)) == size);

    NativeLobbySelection invalid = original;
    invalid.confirmed = false;
    assert(!nativeLobbySelection_Validate(&invalid));

    original.start_requested = false;
    invalid = original;
    invalid.players[1].selected = false;
    assert(!nativeLobbySelection_Validate(&invalid));
    assert(nativeLobbySelection_Encode(&invalid, bytes, sizeof(bytes)) == 0);

    invalid = original;
    invalid.players[1].team = 3;
    assert(!nativeLobbySelection_Validate(&invalid));

    NativeLobbySelection packaged = example();
    for (uint32_t player = 0; player < packaged.player_count; ++player) {
        for (uint32_t byte = 0; byte < NATIVE_CONTENT_ID_BYTES; ++byte) {
            packaged.players[player].character_package_id.bytes[byte] =
                (uint8_t) (byte + player + 1);
            packaged.players[player].costume_package_id.bytes[byte] =
                (uint8_t) (byte + player + 33);
        }
    }
    size_t packaged_size = nativeLobbySelection_Encode(&packaged, bytes,
                                                       sizeof(bytes));
    assert(packaged_size == size + 4 * NATIVE_CONTENT_ID_BYTES);
    assert(nativeLobbySelection_Decode(&decoded, bytes, packaged_size));
    for (uint32_t player = 0; player < packaged.player_count; ++player) {
        assert(memcmp(&decoded.players[player].character_package_id,
                      &packaged.players[player].character_package_id,
                      sizeof(NativeContentId)) == 0);
        assert(memcmp(&decoded.players[player].costume_package_id,
                      &packaged.players[player].costume_package_id,
                      sizeof(NativeContentId)) == 0);
    }
    assert(!nativeLobbySelection_Decode(&decoded, bytes, packaged_size - 1));
    bytes[7] |= 8;
    assert(!nativeLobbySelection_Decode(&decoded, bytes, packaged_size));
    assert(nativeLobbySelection_Encode(&packaged, bytes, sizeof(bytes)) ==
           packaged_size);
    memset(bytes + 25, 0, NATIVE_CONTENT_ID_BYTES);
    assert(!nativeLobbySelection_Decode(&decoded, bytes, packaged_size));

    NativeLobbySelection custom = example();
    custom.players[0].character_id = 0;
    custom.players[0].costume_id = 0;
    custom.players[0].character_package_id =
        packaged.players[0].character_package_id;
    assert(nativeLobbySelection_Validate(&custom));
    const size_t custom_size =
        nativeLobbySelection_Encode(&custom, bytes, sizeof(bytes));
    assert(custom_size != 0);
    assert(nativeLobbySelection_Decode(&decoded, bytes, custom_size));
    assert(decoded.players[0].character_id == 0);
    assert(decoded.players[0].costume_id == 0);
    assert(memcmp(&decoded.players[0].character_package_id,
                  &custom.players[0].character_package_id,
                  sizeof(NativeContentId)) == 0);
    custom.players[0].character_package_id = (NativeContentId) { 0 };
    assert(!nativeLobbySelection_Validate(&custom));

    NativeLobbySelection four = packaged;
    four.player_count = 4;
    four.players[2] = four.players[0];
    four.players[3] = four.players[1];
    assert(nativeLobbySelection_Encode(&four, bytes, sizeof(bytes)) ==
           NATIVE_LOBBY_SELECTION_MAX_WIRE_BYTES);

    invalid = original;
    invalid.confirmed = false;
    invalid.players[1] = (NativeLobbyPlayerSelection) { 0 };
    assert(nativeLobbySelection_Validate(&invalid));

    assert(!nativeLobbySelection_Decode(&decoded, bytes, size - 1));
    bytes[4] = 2;
    assert(!nativeLobbySelection_Decode(&decoded, bytes, size));

    puts("native lobby selection tests passed");
    return 0;
}
