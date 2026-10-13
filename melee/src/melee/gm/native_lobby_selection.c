#include "native_lobby_selection.h"

#ifdef MELEE_NATIVE

#include <string.h>

enum {
    SELECTION_MAGIC = 0x464D534C,
    SELECTION_VERSION = 1,
    SELECTION_HEADER_BYTES = 7,
    SELECTION_PLAYER_BYTES = 18,
    SELECTION_FLAG_SELECTED = 1,
    SELECTION_FLAG_CHARACTER_PACKAGE = 2,
    SELECTION_FLAG_COSTUME_PACKAGE = 4,
};

static bool has_package_id(const NativeContentId* id)
{
    for (uint32_t i = 0; i < sizeof(id->bytes); ++i) {
        if (id->bytes[i] != 0) {
            return true;
        }
    }
    return false;
}

static uint32_t read_u32(const uint8_t* bytes)
{
    return (uint32_t) bytes[0] << 24 | (uint32_t) bytes[1] << 16 |
           (uint32_t) bytes[2] << 8 | bytes[3];
}

static uint64_t read_u64(const uint8_t* bytes)
{
    uint64_t value = 0;
    for (uint32_t i = 0; i < 8; ++i) {
        value = value << 8 | bytes[i];
    }
    return value;
}

static void write_u32(uint8_t* bytes, uint32_t value)
{
    bytes[0] = (uint8_t) (value >> 24);
    bytes[1] = (uint8_t) (value >> 16);
    bytes[2] = (uint8_t) (value >> 8);
    bytes[3] = (uint8_t) value;
}

static void write_u64(uint8_t* bytes, uint64_t value)
{
    for (int32_t i = 7; i >= 0; --i) {
        bytes[i] = (uint8_t) value;
        value >>= 8;
    }
}

bool nativeLobbySelection_Validate(const NativeLobbySelection* selection)
{
    if (selection == NULL || selection->player_count == 0 ||
        selection->player_count > NATIVE_LOBBY_SELECTION_MAX_PLAYERS ||
        (selection->start_requested && !selection->confirmed))
    {
        return false;
    }
    for (uint32_t i = 0; i < NATIVE_LOBBY_SELECTION_MAX_PLAYERS; ++i) {
        const NativeLobbyPlayerSelection* player = &selection->players[i];
        if (i >= selection->player_count) {
            if (player->selected || player->character_id != 0 ||
                player->costume_id != 0 || player->team != 0 ||
                has_package_id(&player->character_package_id) ||
                has_package_id(&player->costume_package_id))
            {
                return false;
            }
            continue;
        }
        if (player->team > 2 ||
            (selection->confirmed && !player->selected) ||
            (player->selected &&
             (player->character_id == 0 || player->costume_id == 0) &&
             !has_package_id(&player->character_package_id)) ||
            (!player->selected &&
             (player->character_id != 0 || player->costume_id != 0 ||
              player->team != 0 ||
              has_package_id(&player->character_package_id) ||
              has_package_id(&player->costume_package_id))))
        {
            return false;
        }
    }
    return true;
}

size_t nativeLobbySelection_Encode(const NativeLobbySelection* selection,
                                   uint8_t* output,
                                   size_t output_capacity)
{
    if (!nativeLobbySelection_Validate(selection) || output == NULL) {
        return 0;
    }
    size_t size = SELECTION_HEADER_BYTES +
                  selection->player_count * SELECTION_PLAYER_BYTES;
    for (uint32_t i = 0; i < selection->player_count; ++i) {
        if (has_package_id(&selection->players[i].character_package_id)) {
            size += NATIVE_CONTENT_ID_BYTES;
        }
        if (has_package_id(&selection->players[i].costume_package_id)) {
            size += NATIVE_CONTENT_ID_BYTES;
        }
    }
    if (size > output_capacity) {
        return 0;
    }
    write_u32(output, SELECTION_MAGIC);
    output[4] = SELECTION_VERSION;
    output[5] = selection->player_count;
    output[6] = (selection->confirmed ? 1 : 0) |
                (selection->start_requested ? 2 : 0);
    size_t offset = SELECTION_HEADER_BYTES;
    for (uint32_t i = 0; i < selection->player_count; ++i) {
        const NativeLobbyPlayerSelection* player = &selection->players[i];
        bool character_packaged = has_package_id(&player->character_package_id);
        bool costume_packaged = has_package_id(&player->costume_package_id);
        output[offset++] = (player->selected ? SELECTION_FLAG_SELECTED : 0) |
                           (character_packaged
                                ? SELECTION_FLAG_CHARACTER_PACKAGE : 0) |
                           (costume_packaged
                                ? SELECTION_FLAG_COSTUME_PACKAGE : 0);
        output[offset++] = player->team;
        write_u64(output + offset, player->character_id);
        offset += 8;
        write_u64(output + offset, player->costume_id);
        offset += 8;
        if (character_packaged) {
            memcpy(output + offset, player->character_package_id.bytes,
                   NATIVE_CONTENT_ID_BYTES);
            offset += NATIVE_CONTENT_ID_BYTES;
        }
        if (costume_packaged) {
            memcpy(output + offset, player->costume_package_id.bytes,
                   NATIVE_CONTENT_ID_BYTES);
            offset += NATIVE_CONTENT_ID_BYTES;
        }
    }
    return offset;
}

bool nativeLobbySelection_Decode(NativeLobbySelection* selection,
                                 const uint8_t* bytes, size_t size)
{
    if (selection == NULL || bytes == NULL || size < SELECTION_HEADER_BYTES ||
        read_u32(bytes) != SELECTION_MAGIC || bytes[4] != SELECTION_VERSION ||
        bytes[5] == 0 || bytes[5] > NATIVE_LOBBY_SELECTION_MAX_PLAYERS ||
        bytes[6] > 3 || size > NATIVE_LOBBY_SELECTION_MAX_WIRE_BYTES ||
        size < SELECTION_HEADER_BYTES + bytes[5] * SELECTION_PLAYER_BYTES)
    {
        return false;
    }
    NativeLobbySelection decoded = {
        .player_count = bytes[5],
        .confirmed = (bytes[6] & 1) != 0,
        .start_requested = (bytes[6] & 2) != 0,
    };
    size_t offset = SELECTION_HEADER_BYTES;
    for (uint32_t i = 0; i < decoded.player_count; ++i) {
        NativeLobbyPlayerSelection* player = &decoded.players[i];
        if (offset > size || SELECTION_PLAYER_BYTES > size - offset) {
            return false;
        }
        uint8_t flags = bytes[offset++];
        if ((flags & ~(SELECTION_FLAG_SELECTED |
                       SELECTION_FLAG_CHARACTER_PACKAGE |
                       SELECTION_FLAG_COSTUME_PACKAGE)) != 0) {
            return false;
        }
        player->selected = (flags & SELECTION_FLAG_SELECTED) != 0;
        player->team = bytes[offset++];
        player->character_id = read_u64(bytes + offset);
        offset += 8;
        player->costume_id = read_u64(bytes + offset);
        offset += 8;
        if ((flags & SELECTION_FLAG_CHARACTER_PACKAGE) != 0) {
            if (NATIVE_CONTENT_ID_BYTES > size - offset) {
                return false;
            }
            memcpy(player->character_package_id.bytes, bytes + offset,
                   NATIVE_CONTENT_ID_BYTES);
            offset += NATIVE_CONTENT_ID_BYTES;
            if (!has_package_id(&player->character_package_id)) {
                return false;
            }
        }
        if ((flags & SELECTION_FLAG_COSTUME_PACKAGE) != 0) {
            if (NATIVE_CONTENT_ID_BYTES > size - offset) {
                return false;
            }
            memcpy(player->costume_package_id.bytes, bytes + offset,
                   NATIVE_CONTENT_ID_BYTES);
            offset += NATIVE_CONTENT_ID_BYTES;
            if (!has_package_id(&player->costume_package_id)) {
                return false;
            }
        }
    }
    if (offset != size || !nativeLobbySelection_Validate(&decoded)) {
        return false;
    }
    *selection = decoded;
    return true;
}

#endif
