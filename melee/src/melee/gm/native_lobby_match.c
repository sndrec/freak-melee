#include "native_lobby_match.h"

#ifdef MELEE_NATIVE

#include <string.h>

enum {
    MATCH_FLAG_TEAMS = 1 << 0,
    MATCH_FLAG_FRIENDLY_FIRE = 1 << 1,
    MATCH_FLAG_PAUSE = 1 << 2,
    MATCH_FLAG_SCORE_DISPLAY = 1 << 3,
    MATCH_FLAG_ALL = (1 << 6) - 1,
};

typedef struct MatchWriter {
    uint8_t* bytes;
    size_t capacity;
    size_t offset;
} MatchWriter;

typedef struct MatchReader {
    const uint8_t* bytes;
    size_t size;
    size_t offset;
} MatchReader;

static bool valid_peer_id(const char* peer_id)
{
    size_t length = strnlen(peer_id, NATIVE_LOBBY_MATCH_PEER_ID_BYTES + 1);
    if (length < 8 || length > NATIVE_LOBBY_MATCH_PEER_ID_BYTES) {
        return false;
    }
    for (size_t i = 0; i < length; ++i) {
        char c = peer_id[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-'))
        {
            return false;
        }
    }
    return true;
}

static bool has_package_id(const NativeContentId* id)
{
    for (uint32_t i = 0; i < sizeof(id->bytes); ++i) {
        if (id->bytes[i] != 0) {
            return true;
        }
    }
    return false;
}

bool nativeLobbyMatch_Validate(const NativeLobbyMatch* match)
{
    bool has_content_digest = false;
    if (match != NULL) {
        for (size_t i = 0; i < sizeof(match->content_digest); ++i) {
            has_content_digest |= match->content_digest[i] != 0;
        }
    }
    if (match == NULL || match->peer_count == 0 ||
        match->peer_count > NATIVE_LOBBY_MATCH_MAX_PEERS ||
        match->mode > NativeLobbyMatchMode_Coin ||
        match->time_limit_minutes > 99 ||
        match->stock_time_limit_minutes > 99 || match->stock_count == 0 ||
        match->stock_count > 99 || match->item_frequency < -1 ||
        match->item_frequency > 4 || match->handicap > 2 ||
        match->damage_ratio < 5 || match->damage_ratio > 20 || match->sd_penalty>2 ||
        match->session_id == 0 || match->stage_id == 0 ||
        !has_content_digest ||
        ((match->mode == NativeLobbyMatchMode_Time ||
          match->mode == NativeLobbyMatchMode_Coin) &&
         match->time_limit_minutes == 0) ||
        (match->item_frequency < 0 && match->item_mask != 0) ||
        (match->item_frequency >= 0 && match->item_mask == 0))
    {
        return false;
    }

    uint8_t numeric_peers = 0;
    uint8_t assigned_ports = 0;
    for (uint32_t i = 0; i < match->peer_count; ++i) {
        const NativeLobbyMatchPeer* peer = &match->peers[i];
        if (!valid_peer_id(peer->peer_id) || peer->numeric_peer >= 4 ||
            peer->port_mask == 0 || peer->port_mask > 0xF ||
            (numeric_peers & (1U << peer->numeric_peer)) != 0 ||
            (assigned_ports & peer->port_mask) != 0 ||
            (i != 0 && match->peers[i - 1].numeric_peer >= peer->numeric_peer))
        {
            return false;
        }
        for (uint32_t prior = 0; prior < i; ++prior) {
            if (strcmp(peer->peer_id, match->peers[prior].peer_id) == 0) {
                return false;
            }
        }
        numeric_peers |= 1U << peer->numeric_peer;
        assigned_ports |= peer->port_mask;
    }
    if (assigned_ports != 0xF) {
        return false;
    }

    uint32_t active = 0;
    uint32_t humans = 0;
    uint8_t human_ports = 0;
    uint8_t team_mask = 0;
    for (uint32_t i = 0; i < 4; ++i) {
        const NativeLobbyMatchSlot* slot = &match->slots[i];
        if (slot->kind > NativeLobbyMatchSlot_Cpu || slot->team > 2 ||
            slot->local_seat >= 4) {
            return false;
        }
        if (slot->kind == NativeLobbyMatchSlot_Empty) {
            if (slot->local_seat != 0 || slot->character_id != 0 || slot->costume_id != 0 ||
                slot->cpu_level != 0 || slot->team != 0 || slot->handicap != 0 ||
                has_package_id(&slot->character_package_id) ||
                has_package_id(&slot->costume_package_id))
            {
                return false;
            }
            continue;
        }
        if ((slot->character_id == 0 &&
             !has_package_id(&slot->character_package_id)) ||
            slot->handicap < 1 ||
            slot->handicap > 9 ||
            (slot->kind == NativeLobbyMatchSlot_Cpu && slot->local_seat != 0) ||
            (slot->kind == NativeLobbyMatchSlot_Cpu
                 ? slot->cpu_level < 1 || slot->cpu_level > 9
                 : slot->cpu_level != 0))
        {
            return false;
        }
        ++active;
        humans += slot->kind == NativeLobbyMatchSlot_Human;
        if (slot->kind == NativeLobbyMatchSlot_Human) {
            human_ports |= 1U << i;
        }
        team_mask |= 1U << slot->team;
    }
    if (active < 2 || humans == 0 ||
        (match->teams && (team_mask & (team_mask - 1)) == 0))
    {
        return false;
    }
    for (uint32_t i = 0; i < match->peer_count; ++i) {
        if ((match->peers[i].port_mask & human_ports) == 0) {
            return false;
        }
        uint8_t local_seats = 0;
        for (uint32_t port = 0; port < 4; ++port) {
            if ((match->peers[i].port_mask & (1U << port)) == 0 ||
                match->slots[port].kind != NativeLobbyMatchSlot_Human)
            {
                continue;
            }
            uint8_t seat = match->slots[port].local_seat;
            if ((local_seats & (1U << seat)) != 0) {
                return false;
            }
            local_seats |= 1U << seat;
        }
    }
    return true;
}

static bool write_raw(MatchWriter* writer, const void* bytes, size_t size)
{
    if (writer->offset > writer->capacity ||
        size > writer->capacity - writer->offset)
    {
        return false;
    }
    memcpy(writer->bytes + writer->offset, bytes, size);
    writer->offset += size;
    return true;
}

static bool write_u8(MatchWriter* writer, uint8_t value)
{
    return write_raw(writer, &value, 1);
}

static bool write_u64(MatchWriter* writer, uint64_t value)
{
    uint8_t bytes[8];
    for (int32_t i = 7; i >= 0; --i) {
        bytes[i] = (uint8_t) value;
        value >>= 8;
    }
    return write_raw(writer, bytes, sizeof(bytes));
}

static bool write_string8(MatchWriter* writer, const char* value)
{
    size_t length = strnlen(value, NATIVE_LOBBY_MATCH_PEER_ID_BYTES + 1);
    return length <= UINT8_MAX && write_u8(writer, (uint8_t) length) &&
           write_raw(writer, value, length);
}

size_t nativeLobbyMatch_Encode(const NativeLobbyMatch* match, uint8_t* output,
                               size_t output_capacity)
{
    if (output == NULL || !nativeLobbyMatch_Validate(match)) {
        return 0;
    }
    MatchWriter writer = {
        output,
        output_capacity < NATIVE_LOBBY_MATCH_MAX_WIRE_BYTES
            ? output_capacity
            : NATIVE_LOBBY_MATCH_MAX_WIRE_BYTES,
        0,
    };
    const uint8_t magic[4] = { 'F', 'M', 'L', 'M' };
    uint8_t flags = (match->teams ? MATCH_FLAG_TEAMS : 0) |
                    (match->friendly_fire ? MATCH_FLAG_FRIENDLY_FIRE : 0) |
                    (match->pause ? MATCH_FLAG_PAUSE : 0) |
                    (match->score_display ? MATCH_FLAG_SCORE_DISPLAY : 0) | (match->sd_penalty<<4);
    if (!write_raw(&writer, magic, sizeof(magic)) ||
        !write_u64(&writer, match->session_id) ||
        !write_raw(&writer, match->content_digest,
                   sizeof(match->content_digest)) ||
        !write_u64(&writer, match->stage_id) ||
        !write_u64(&writer, match->item_mask) ||
        !write_u8(&writer, match->peer_count) ||
        !write_u8(&writer, match->mode) ||
        !write_u8(&writer, match->time_limit_minutes) ||
        !write_u8(&writer, match->stock_time_limit_minutes) ||
        !write_u8(&writer, match->stock_count) ||
        !write_u8(&writer, (uint8_t) match->item_frequency) ||
        !write_u8(&writer, match->handicap) ||
        !write_u8(&writer, match->damage_ratio) || !write_u8(&writer, flags))
    {
        return 0;
    }
    for (uint32_t i = 0; i < 4; ++i) {
        const NativeLobbyMatchSlot* slot = &match->slots[i];
        if (!write_u8(&writer, slot->kind) ||
            !write_u8(&writer, slot->local_seat) ||
            !write_u8(&writer, slot->cpu_level) ||
            !write_u8(&writer, slot->team) ||
            !write_u8(&writer, slot->handicap) ||
            !write_u64(&writer, slot->character_id) ||
            !write_u64(&writer, slot->costume_id) ||
            !write_raw(&writer, slot->character_package_id.bytes,
                       NATIVE_CONTENT_ID_BYTES) ||
            !write_raw(&writer, slot->costume_package_id.bytes,
                       NATIVE_CONTENT_ID_BYTES))
        {
            return 0;
        }
    }
    for (uint32_t i = 0; i < match->peer_count; ++i) {
        const NativeLobbyMatchPeer* peer = &match->peers[i];
        if (!write_u8(&writer, peer->numeric_peer) ||
            !write_u8(&writer, peer->port_mask) ||
            !write_string8(&writer, peer->peer_id))
        {
            return 0;
        }
    }
    return writer.offset;
}

static bool read_raw(MatchReader* reader, void* output, size_t size)
{
    if (reader->offset > reader->size || size > reader->size - reader->offset)
    {
        return false;
    }
    memcpy(output, reader->bytes + reader->offset, size);
    reader->offset += size;
    return true;
}

static bool read_u8(MatchReader* reader, uint8_t* output)
{
    return read_raw(reader, output, 1);
}

static bool read_u64(MatchReader* reader, uint64_t* output)
{
    uint8_t bytes[8];
    if (!read_raw(reader, bytes, sizeof(bytes))) {
        return false;
    }
    *output = 0;
    for (uint32_t i = 0; i < sizeof(bytes); ++i) {
        *output = *output << 8 | bytes[i];
    }
    return true;
}

static bool read_string8(MatchReader* reader, char* output, size_t capacity)
{
    uint8_t length;
    if (!read_u8(reader, &length) || length >= capacity ||
        !read_raw(reader, output, length))
    {
        return false;
    }
    output[length] = '\0';
    return true;
}

bool nativeLobbyMatch_Decode(NativeLobbyMatch* match, const uint8_t* bytes,
                             size_t size)
{
    if (match == NULL || bytes == NULL ||
        size > NATIVE_LOBBY_MATCH_MAX_WIRE_BYTES)
    {
        return false;
    }
    MatchReader reader = { bytes, size, 0 };
    NativeLobbyMatch decoded = { 0 };
    uint8_t magic[4];
    uint8_t flags;
    uint8_t item_frequency;
    if (!read_raw(&reader, magic, sizeof(magic)) ||
        memcmp(magic, "FMLM", sizeof(magic)) != 0 ||
        !read_u64(&reader, &decoded.session_id) ||
        !read_raw(&reader, decoded.content_digest,
                  sizeof(decoded.content_digest)) ||
        !read_u64(&reader, &decoded.stage_id) ||
        !read_u64(&reader, &decoded.item_mask) ||
        !read_u8(&reader, &decoded.peer_count) ||
        !read_u8(&reader, &decoded.mode) ||
        !read_u8(&reader, &decoded.time_limit_minutes) ||
        !read_u8(&reader, &decoded.stock_time_limit_minutes) ||
        !read_u8(&reader, &decoded.stock_count) ||
        !read_u8(&reader, &item_frequency) ||
        !read_u8(&reader, &decoded.handicap) ||
        !read_u8(&reader, &decoded.damage_ratio) ||
        !read_u8(&reader, &flags) || (flags & ~MATCH_FLAG_ALL) != 0 ||
        decoded.peer_count > NATIVE_LOBBY_MATCH_MAX_PEERS)
    {
        return false;
    }
    decoded.item_frequency =
        item_frequency == UINT8_MAX ? -1 : (int8_t) item_frequency;
    decoded.teams = (flags & MATCH_FLAG_TEAMS) != 0;
    decoded.friendly_fire = (flags & MATCH_FLAG_FRIENDLY_FIRE) != 0;
    decoded.pause = (flags & MATCH_FLAG_PAUSE) != 0;
    decoded.score_display = (flags & MATCH_FLAG_SCORE_DISPLAY) != 0;
    decoded.sd_penalty=(flags>>4)&3;
    for (uint32_t i = 0; i < 4; ++i) {
        NativeLobbyMatchSlot* slot = &decoded.slots[i];
        if (!read_u8(&reader, &slot->kind) ||
            !read_u8(&reader, &slot->local_seat) ||
            !read_u8(&reader, &slot->cpu_level) ||
            !read_u8(&reader, &slot->team) ||
            !read_u8(&reader, &slot->handicap) ||
            !read_u64(&reader, &slot->character_id) ||
            !read_u64(&reader, &slot->costume_id) ||
            !read_raw(&reader, slot->character_package_id.bytes,
                      NATIVE_CONTENT_ID_BYTES) ||
            !read_raw(&reader, slot->costume_package_id.bytes,
                      NATIVE_CONTENT_ID_BYTES))
        {
            return false;
        }
    }
    for (uint32_t i = 0; i < decoded.peer_count; ++i) {
        NativeLobbyMatchPeer* peer = &decoded.peers[i];
        if (!read_u8(&reader, &peer->numeric_peer) ||
            !read_u8(&reader, &peer->port_mask) ||
            !read_string8(&reader, peer->peer_id, sizeof(peer->peer_id)))
        {
            return false;
        }
    }
    if (reader.offset != reader.size || !nativeLobbyMatch_Validate(&decoded)) {
        return false;
    }
    *match = decoded;
    return true;
}

#endif
