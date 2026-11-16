#include "native_signaling_wire.h"
#include "native_account_identity.h"

#ifdef MELEE_NATIVE

#include <string.h>
#include "../lb/native_utf8.h"

enum {
    SIGNALING_MAGIC = 0x464D5331,
    SIGNALING_VERSION = 5,
    SIGNALING_HEADER_BYTES = 8,
    CLIENT_UPDATE = 1,
    CLIENT_SIGNAL = 2,
    CLIENT_REQUEST_PRESENCE = 3,
    CLIENT_SET_MATCH = 4,
    CLIENT_ASSIGN_SEATS = 5,
    CLIENT_SET_ROTATION = 6,
    CLIENT_REPORT_MATCH = 7,
    CLIENT_POST_CHAT = 8,
    CLIENT_REQUEST_CHAT = 9,
    CLIENT_CONFIGURE_QUICK = 10,
    CLIENT_QUEUE_QUICK = 11,
    CLIENT_REQUEST_QUICK = 12,
    CLIENT_PREPARE_QUICK = 13,
    CLIENT_REPORT_QUICK = 14,
    CLIENT_VOTE_QUICK_STAGE = 15,
    CLIENT_UNREADY_QUICK = 16,
    CLIENT_PICK_LOBBY_STAGE = 17,
    CLIENT_SET_LISTING = 18,
    CLIENT_REQUEST_LOBBIES = 19,
    CLIENT_SET_LOBBY_RULES = 20,
    CLIENT_REQUEST_LOBBY_STATE = 21,
    CLIENT_STAGE_CURSOR = 22,
    SERVER_WELCOME = 0x81,
    SERVER_PRESENCE = 0x82,
    SERVER_SIGNAL = 0x83,
    SERVER_SIGNAL_SENT = 0x84,
    SERVER_CHAT_ENTRY = 0x85,
    SERVER_CHAT_BEGIN = 0x86,
    SERVER_CHAT_END = 0x87,
    SERVER_CHAT_REJECTED = 0x88,
    SERVER_QUICK_STATE = 0x89,
    SERVER_QUICK_REJECTED = 0x8A,
    SERVER_LOBBIES = 0x8B,
    SERVER_LISTING = 0x8C,
    SERVER_LOBBY_STATE = 0x8D,
    SERVER_STAGE_CURSOR = 0x8E,
    SERVER_ERROR = 0xFF,
    UPDATE_NAME = 1 << 0,
    UPDATE_ACTIVE_MASK = 1 << 1,
    UPDATE_LOCAL_PLAYERS = 1 << 2,
    UPDATE_READY = 1 << 3,
    UPDATE_SELECTION = 1 << 4,
    UPDATE_WAITING_MASK = 1 << 5,
};

typedef struct WireWriter {
    uint8_t* bytes;
    size_t capacity;
    size_t offset;
} WireWriter;

typedef struct WireReader {
    const uint8_t* bytes;
    size_t size;
    size_t offset;
} WireReader;

static uint16_t read_u16(const uint8_t* bytes)
{
    return (uint16_t) ((uint16_t) bytes[0] << 8 | bytes[1]);
}

static uint32_t read_u32(const uint8_t* bytes)
{
    return (uint32_t) bytes[0] << 24 | (uint32_t) bytes[1] << 16 |
           (uint32_t) bytes[2] << 8 | bytes[3];
}

static void write_u16(uint8_t* bytes, uint16_t value)
{
    bytes[0] = (uint8_t) (value >> 8);
    bytes[1] = (uint8_t) value;
}

static void write_u32(uint8_t* bytes, uint32_t value)
{
    bytes[0] = (uint8_t) (value >> 24);
    bytes[1] = (uint8_t) (value >> 16);
    bytes[2] = (uint8_t) (value >> 8);
    bytes[3] = (uint8_t) value;
}

static bool writer_raw(WireWriter* writer, const void* bytes, size_t size)
{
    if (writer->offset > writer->capacity ||
        size > writer->capacity - writer->offset)
    {
        return false;
    }
    if (size != 0) {
        memcpy(writer->bytes + writer->offset, bytes, size);
    }
    writer->offset += size;
    return true;
}

static bool writer_u8(WireWriter* writer, uint8_t value)
{
    return writer_raw(writer, &value, sizeof(value));
}

static bool writer_u16(WireWriter* writer, uint16_t value)
{
    uint8_t bytes[2];
    write_u16(bytes, value);
    return writer_raw(writer, bytes, sizeof(bytes));
}

static bool writer_u32(WireWriter* writer, uint32_t value)
{
    uint8_t bytes[4]; write_u32(bytes, value);
    return writer_raw(writer, bytes, sizeof(bytes));
}

static bool writer_u64(WireWriter* writer, uint64_t value)
{
    uint8_t bytes[8];
    for (uint32_t index = 0; index < 8; ++index) {
        bytes[7 - index] = (uint8_t) (value >> (index * 8));
    }
    return writer_raw(writer, bytes, sizeof(bytes));
}

static bool writer_bytes16(WireWriter* writer, const uint8_t* value,
                           uint16_t size)
{
    return (size == 0 || value != NULL) && writer_u16(writer, size) &&
           writer_raw(writer, value, size);
}

static bool writer_string8(WireWriter* writer, const char* value,
                           size_t maximum_length)
{
    if (value == NULL) {
        return false;
    }
    size_t length = strnlen(value, maximum_length + 1);
    return length <= maximum_length && length <= UINT8_MAX &&
           writer_u8(writer, (uint8_t) length) &&
           writer_raw(writer, value, length);
}

static bool writer_begin(WireWriter* writer, uint8_t* output,
                         size_t output_capacity, uint8_t type)
{
    if (output == NULL || output_capacity < SIGNALING_HEADER_BYTES ||
        output_capacity > UINT16_MAX + SIGNALING_HEADER_BYTES)
    {
        return false;
    }
    *writer = (WireWriter) {
        .bytes = output,
        .capacity = output_capacity,
        .offset = SIGNALING_HEADER_BYTES,
    };
    write_u32(output, SIGNALING_MAGIC);
    output[4] = SIGNALING_VERSION;
    output[5] = type;
    output[6] = 0;
    output[7] = 0;
    return true;
}

static size_t writer_finish(WireWriter* writer)
{
    size_t payload_size = writer->offset - SIGNALING_HEADER_BYTES;
    if (payload_size > UINT16_MAX) {
        return 0;
    }
    write_u16(writer->bytes + 6, (uint16_t) payload_size);
    return writer->offset;
}

static bool reader_raw(WireReader* reader, void* output, size_t size)
{
    if (reader->offset > reader->size || size > reader->size - reader->offset) {
        return false;
    }
    if (size != 0 && output != NULL) {
        memcpy(output, reader->bytes + reader->offset, size);
    }
    reader->offset += size;
    return true;
}

static bool reader_u8(WireReader* reader, uint8_t* output)
{
    return reader_raw(reader, output, sizeof(*output));
}

static bool reader_u16(WireReader* reader, uint16_t* output)
{
    uint8_t bytes[2];
    if (!reader_raw(reader, bytes, sizeof(bytes))) {
        return false;
    }
    *output = read_u16(bytes);
    return true;
}

static bool reader_u32(WireReader* reader, uint32_t* output)
{
    uint8_t bytes[4];
    if (!reader_raw(reader, bytes, sizeof(bytes))) return false;
    *output = read_u32(bytes); return true;
}

static bool reader_u64(WireReader* reader, uint64_t* output)
{
    uint8_t bytes[8];
    if (!reader_raw(reader, bytes, sizeof(bytes))) {
        return false;
    }
    *output = 0;
    for (size_t i = 0; i < sizeof(bytes); ++i) {
        *output = *output << 8 | bytes[i];
    }
    return true;
}

static bool reader_string8(WireReader* reader, char* output,
                           size_t output_capacity)
{
    uint8_t length;
    if (output == NULL || output_capacity == 0 ||
        !reader_u8(reader, &length) || length >= output_capacity ||
        !reader_raw(reader, output, length))
    {
        return false;
    }
    output[length] = '\0';
    return true;
}

static bool reader_bytes16(WireReader* reader, uint8_t* output,
                           size_t output_capacity, uint16_t* output_size)
{
    uint16_t size;
    if (!reader_u16(reader, &size) || size > output_capacity ||
        !reader_raw(reader, output, size))
    {
        return false;
    }
    *output_size = size;
    return true;
}

static bool reader_finished(const WireReader* reader)
{
    return reader->offset == reader->size;
}

static bool reader_begin(const uint8_t* message, size_t message_size,
                         uint8_t* type, WireReader* reader)
{
    if (message == NULL || type == NULL || reader == NULL ||
        message_size < SIGNALING_HEADER_BYTES ||
        message_size > NATIVE_SIGNALING_MAX_MESSAGE_BYTES ||
        read_u32(message) != SIGNALING_MAGIC ||
        message[4] != SIGNALING_VERSION ||
        read_u16(message + 6) != message_size - SIGNALING_HEADER_BYTES)
    {
        return false;
    }
    *type = message[5];
    *reader = (WireReader) {
        .bytes = message + SIGNALING_HEADER_BYTES,
        .size = message_size - SIGNALING_HEADER_BYTES,
    };
    return true;
}

size_t nativeSignalingWire_EncodeUpdate(const NativeSignalingUpdate* update,
                                        uint8_t* output,
                                        size_t output_capacity)
{
    if (update == NULL || output_capacity > NATIVE_SIGNALING_MAX_MESSAGE_BYTES) {
        return 0;
    }
    uint8_t fields = (update->has_name ? UPDATE_NAME : 0) |
                     (update->has_active_mask ? UPDATE_ACTIVE_MASK : 0) |
                     (update->has_local_players ? UPDATE_LOCAL_PLAYERS : 0) |
                     (update->has_ready ? UPDATE_READY : 0) |
                     (update->has_selection ? UPDATE_SELECTION : 0) |
                     (update->has_waiting_mask ? UPDATE_WAITING_MASK : 0);
    WireWriter writer;
    if (fields == 0 || !writer_begin(&writer, output, output_capacity,
                                     CLIENT_UPDATE) ||
        !writer_u8(&writer, fields) ||
        (update->has_name &&
         !writer_string8(&writer, update->name,
                         NATIVE_SIGNALING_MAX_NAME_BYTES)) ||
        (update->has_active_mask &&
         (update->active_mask > 0xF ||
          !writer_u8(&writer, update->active_mask))) ||
        (update->has_local_players &&
         (update->local_players > 4 ||
          !writer_u8(&writer, update->local_players))) ||
        (update->has_ready && !writer_u8(&writer, update->ready ? 1 : 0)) ||
        (update->has_selection &&
         (update->selection_size > NATIVE_SIGNALING_MAX_SELECTION_BYTES ||
          !writer_bytes16(&writer, update->selection,
                          update->selection_size))) ||
        (update->has_waiting_mask &&
         (update->waiting_mask > 0xF ||
          !writer_u8(&writer, update->waiting_mask))))
    {
        return 0;
    }
    return writer_finish(&writer);
}

size_t nativeSignalingWire_EncodeSignal(const char* target_peer_id,
                                        const uint8_t* payload,
                                        uint16_t payload_size,
                                        uint8_t* output,
                                        size_t output_capacity)
{
    WireWriter writer;
    if (output_capacity > NATIVE_SIGNALING_MAX_MESSAGE_BYTES ||
        (payload_size != 0 && payload == NULL) ||
        !writer_begin(&writer, output, output_capacity, CLIENT_SIGNAL) ||
        !writer_string8(&writer, target_peer_id,
                        NATIVE_SIGNALING_MAX_PEER_ID_BYTES) ||
        !writer_bytes16(&writer, payload, payload_size))
    {
        return 0;
    }
    return writer_finish(&writer);
}

size_t nativeSignalingWire_EncodeSetMatch(const uint8_t* payload,
                                          uint16_t payload_size,
                                          uint8_t* output,
                                          size_t output_capacity)
{
    WireWriter writer;
    if (payload_size > NATIVE_SIGNALING_MAX_MATCH_BYTES ||
        output_capacity > NATIVE_SIGNALING_MAX_MESSAGE_BYTES ||
        !writer_begin(&writer, output, output_capacity, CLIENT_SET_MATCH) ||
        !writer_bytes16(&writer, payload, payload_size))
    {
        return 0;
    }
    return writer_finish(&writer);
}

size_t nativeSignalingWire_EncodeAssignSeats(const char* peer_id,
                                             uint8_t active_mask,
                                             uint8_t waiting_mask,
                                             uint8_t* output,
                                             size_t output_capacity)
{
    WireWriter writer;
    if (active_mask > 0xF || waiting_mask > 0xF ||
        output_capacity > NATIVE_SIGNALING_MAX_MESSAGE_BYTES ||
        !writer_begin(&writer, output, output_capacity, CLIENT_ASSIGN_SEATS) ||
        !writer_string8(&writer, peer_id, NATIVE_SIGNALING_MAX_PEER_ID_BYTES) ||
        !writer_u8(&writer, active_mask) ||
        !writer_u8(&writer, waiting_mask))
    {
        return 0;
    }
    return writer_finish(&writer);
}

size_t nativeSignalingWire_EncodeSetRotation(uint8_t policy,
                                             uint8_t* output,
                                             size_t output_capacity)
{
    WireWriter writer;
    if (policy > NativeRoomRotation_WinnerOut ||
        output_capacity > NATIVE_SIGNALING_MAX_MESSAGE_BYTES ||
        !writer_begin(&writer, output, output_capacity,
                      CLIENT_SET_ROTATION) ||
        !writer_u8(&writer, policy))
    {
        return 0;
    }
    return writer_finish(&writer);
}

size_t nativeSignalingWire_EncodeReportMatch(uint64_t epoch,
                                             const char* winner_peer_id,
                                             uint8_t winner_seat,
                                             bool eligible,
                                             uint8_t loser_port,
                                             uint8_t* output,
                                             size_t output_capacity)
{
    WireWriter writer;
    if (output_capacity > NATIVE_SIGNALING_MAX_MESSAGE_BYTES ||
        (eligible && winner_seat >= 4) ||
        (loser_port >= 4 && loser_port != UINT8_MAX) ||
        !writer_begin(&writer, output, output_capacity,
                      CLIENT_REPORT_MATCH) ||
        !writer_u64(&writer, epoch) ||
        !writer_string8(&writer, winner_peer_id,
                        NATIVE_SIGNALING_MAX_PEER_ID_BYTES) ||
        !writer_u8(&writer, winner_seat) ||
        !writer_u8(&writer, eligible ? 1 : 0) ||
        !writer_u8(&writer, loser_port))
    {
        return 0;
    }
    return writer_finish(&writer);
}

size_t nativeSignalingWire_EncodePickLobbyStage(uint64_t epoch, uint8_t seat,
                                                uint8_t stage,
                                                uint8_t* output, size_t capacity)
{
    WireWriter writer;
    return seat < 4 && stage != UINT8_MAX &&
        writer_begin(&writer, output, capacity, CLIENT_PICK_LOBBY_STAGE) &&
        writer_u64(&writer, epoch) && writer_u8(&writer, seat) &&
        writer_u8(&writer, stage) ? writer_finish(&writer) : 0;
}

size_t nativeSignalingWire_EncodeListing(const char* title, bool listed,
                                         uint8_t* output, size_t capacity)
{
    WireWriter writer;
    size_t length = title ? strnlen(title, 65) : 0;
    return length && length <= 64 &&
        nativeUtf8_IsVisibleText((const uint8_t*) title, length) &&
        writer_begin(&writer, output, capacity, CLIENT_SET_LISTING) &&
        writer_string8(&writer, title, 64) && writer_u8(&writer, listed) ? writer_finish(&writer) : 0;
}
size_t nativeSignalingWire_EncodeLobbies(uint16_t offset, uint8_t* output, size_t capacity)
{
    WireWriter writer;
    return writer_begin(&writer, output, capacity, CLIENT_REQUEST_LOBBIES) &&
        writer_u16(&writer, offset) ? writer_finish(&writer) : 0;
}
size_t nativeSignalingWire_EncodeLobbyRules(const NativeRoomRules* rules, uint8_t* output, size_t capacity)
{
    WireWriter writer;
    uint8_t bytes[NATIVE_ROOM_RULES_BYTES];
    if (!nativeRoomRules_Encode(rules,bytes) ||
        !writer_begin(&writer,output,capacity,CLIENT_SET_LOBBY_RULES) ||
        !writer_bytes16(&writer,bytes,sizeof(bytes))) return 0;
    return writer_finish(&writer);
}

size_t nativeSignalingWire_EncodeStageCursor(const NativeStageCursor* cursor,
                                             uint8_t* output, size_t capacity)
{
    WireWriter writer;
    if (!cursor || !cursor->sequence || cursor->seat > 3 || cursor->tile > 29 ||
        cursor->x < -27*256 || cursor->x > 27*256 ||
        cursor->y < -19*256 || cursor->y > 19*256 ||
        !writer_begin(&writer, output, capacity, CLIENT_STAGE_CURSOR) ||
        !writer_u64(&writer, cursor->epoch) || !writer_u64(&writer, cursor->revision) ||
        !writer_u32(&writer, cursor->sequence) ||
        !writer_u16(&writer, (uint16_t) cursor->x) ||
        !writer_u16(&writer, (uint16_t) cursor->y) ||
        !writer_u8(&writer, cursor->tile) || !writer_u8(&writer, cursor->seat)) return 0;
    return writer_finish(&writer);
}

size_t nativeSignalingWire_EncodeRequestLobbyState(uint8_t* output, size_t capacity)
{
    WireWriter writer;
    return writer_begin(&writer,output,capacity,CLIENT_REQUEST_LOBBY_STATE) ? writer_finish(&writer) : 0;
}

size_t nativeSignalingWire_EncodeRequestPresence(uint8_t* output,
                                                 size_t output_capacity)
{
    WireWriter writer;
    if (output_capacity > NATIVE_SIGNALING_MAX_MESSAGE_BYTES ||
        !writer_begin(&writer, output, output_capacity,
                      CLIENT_REQUEST_PRESENCE))
    {
        return 0;
    }
    return writer_finish(&writer);
}

static bool decode_participant(WireReader* reader,
                               NativeSignalingParticipant* participant)
{
    uint8_t ready;
    if (!reader_string8(reader, participant->peer_id,
                        sizeof(participant->peer_id)) ||
        !reader_string8(reader, participant->name,
                        sizeof(participant->name)) ||
        !reader_string8(reader, participant->build,
                        sizeof(participant->build)) ||
        !reader_u8(reader, &participant->local_players) ||
        !reader_u8(reader, &participant->active_mask) ||
        !reader_u8(reader, &participant->waiting_mask) ||
        participant->local_players > 4 || !reader_u8(reader, &ready) ||
        ready > 1 || !reader_u64(reader, &participant->join_order) ||
        !reader_raw(reader, participant->waiting_ranks,
                    sizeof(participant->waiting_ranks)) ||
        !reader_bytes16(reader, participant->selection,
                        sizeof(participant->selection),
                        &participant->selection_size) ||
        !reader_string8(reader, participant->account_id, sizeof(participant->account_id)) ||
        !reader_string8(reader, participant->account_handle, sizeof(participant->account_handle)) ||
        !nativeAccountIdentity_Valid(participant->account_id, participant->account_handle))
    {
        return false;
    }
    participant->ready = ready != 0;
    uint8_t seats = (uint8_t) ((1U << participant->local_players) - 1);
    if (((participant->active_mask | participant->waiting_mask) & ~seats) != 0 ||
        (participant->active_mask & participant->waiting_mask) != 0 ||
        (participant->ready && participant->active_mask == 0)) {
        return false;
    }
    for (uint32_t seat = 0; seat < 4; ++seat) {
        if (((participant->waiting_mask & (1U << seat)) != 0) !=
            (participant->waiting_ranks[seat] != 0)) {
            return false;
        }
    }
    return true;
}

size_t nativeSignalingWire_EncodeConfigureQuick(uint8_t mode,
                                                uint8_t* output, size_t capacity)
{
    WireWriter writer;
    return mode <= NativeQuickMode_Doubles &&
        writer_begin(&writer, output, capacity, CLIENT_CONFIGURE_QUICK) &&
        writer_u8(&writer, mode) ? writer_finish(&writer) : 0;
}

size_t nativeSignalingWire_EncodeQueueQuick(bool searching,
                                            uint8_t* output, size_t capacity)
{
    WireWriter writer;
    return writer_begin(&writer, output, capacity, CLIENT_QUEUE_QUICK) &&
        writer_u8(&writer, searching ? 1 : 0) ? writer_finish(&writer) : 0;
}

size_t nativeSignalingWire_EncodeReportQuick(uint64_t epoch, uint8_t winner,
                                            uint8_t* output, size_t capacity)
{
    WireWriter writer;
    return epoch && (winner <= 1 || winner == 255) &&
        writer_begin(&writer, output, capacity, CLIENT_REPORT_QUICK) &&
        writer_u64(&writer, epoch) && writer_u8(&writer, winner) ? writer_finish(&writer) : 0;
}

size_t nativeSignalingWire_EncodeVoteQuickStage(uint64_t epoch, uint8_t seat,
                                               uint8_t stage, uint8_t* output,
                                               size_t capacity)
{
    WireWriter writer;
    return epoch && seat < 2 && (stage < 6 || stage == 255) &&
        writer_begin(&writer, output, capacity, CLIENT_VOTE_QUICK_STAGE) &&
        writer_u64(&writer, epoch) && writer_u8(&writer, seat) &&
        writer_u8(&writer, stage) ? writer_finish(&writer) : 0;
}

size_t nativeSignalingWire_EncodeUnreadyQuick(uint64_t completed_epoch,
                                              uint8_t* output, size_t capacity)
{
    WireWriter writer;
    return writer_begin(&writer, output, capacity, CLIENT_UNREADY_QUICK) &&
        writer_u64(&writer, completed_epoch) ? writer_finish(&writer) : 0;
}

size_t nativeSignalingWire_EncodePrepareQuick(uint64_t revision, bool prepared,
                                              const uint8_t* digest,
                                              uint8_t* output, size_t capacity)
{
    WireWriter writer;
    return writer_begin(&writer, output, capacity, CLIENT_PREPARE_QUICK) &&
        writer_u64(&writer, revision) && writer_u8(&writer, prepared ? 1 : 0) &&
        writer_bytes16(&writer, digest, prepared ? 32 : 0) ? writer_finish(&writer) : 0;
}

size_t nativeSignalingWire_EncodeRequestQuick(uint8_t* output, size_t capacity)
{
    WireWriter writer;
    return writer_begin(&writer, output, capacity, CLIENT_REQUEST_QUICK)
        ? writer_finish(&writer) : 0;
}

size_t nativeSignalingWire_EncodeRequestChat(uint8_t* output, size_t capacity)
{
    WireWriter writer;
    return writer_begin(&writer, output, capacity, CLIENT_REQUEST_CHAT)
        ? writer_finish(&writer) : 0;
}

size_t nativeSignalingWire_EncodePostChat(uint32_t nonce, const char* text,
                                        uint8_t* output, size_t capacity)
{
    if (!text || !nonce) return 0;
    size_t length = strnlen(text, NATIVE_ROOM_CHAT_TEXT_BYTES + 1);
    WireWriter writer;
    if (length > NATIVE_ROOM_CHAT_TEXT_BYTES ||
        !nativeUtf8_IsVisibleText((const uint8_t*) text, length) ||
        !writer_begin(&writer, output, capacity, CLIENT_POST_CHAT) ||
        !writer_u32(&writer, nonce) ||
        !writer_bytes16(&writer, (const uint8_t*) text, (uint16_t) length)) return 0;
    return writer_finish(&writer);
}

bool nativeSignalingWire_DecodeServer(const uint8_t* message,
                                      size_t message_size,
                                      NativeSignalingServerMessage* output)
{
    uint8_t type;
    WireReader reader;
    if (output == NULL ||
        !reader_begin(message, message_size, &type, &reader))
    {
        return false;
    }
    memset(output, 0, sizeof(*output));
    switch (type) {
    case SERVER_STAGE_CURSOR: {
        NativeStageCursor* cursor = &output->data.stage_cursor;
        uint16_t x, y;
        output->type = NativeSignalingServerMessage_StageCursor;
        if (!reader_string8(&reader, cursor->peer_id, sizeof(cursor->peer_id)) ||
            !cursor->peer_id[0] || !reader_u64(&reader, &cursor->epoch) ||
            !reader_u64(&reader, &cursor->revision) ||
            !reader_u32(&reader, &cursor->sequence) || !cursor->sequence ||
            !reader_u16(&reader, &x) || !reader_u16(&reader, &y) ||
            !reader_u8(&reader, &cursor->tile) || cursor->tile > 29 ||
            !reader_u8(&reader, &cursor->seat) || cursor->seat > 3 ||
            !reader_finished(&reader)) return false;
        cursor->x = (int16_t) x;
        cursor->y = (int16_t) y;
        return cursor->x >= -27*256 && cursor->x <= 27*256 &&
               cursor->y >= -19*256 && cursor->y <= 19*256;
    }
    case SERVER_LOBBY_STATE: {
        NativeRoomLobbyState* state=&output->data.lobby;
        uint8_t bytes[NATIVE_ROOM_RULES_BYTES]; uint16_t size;
        output->type=NativeSignalingServerMessage_LobbyState;
        if (!reader_u64(&reader,&state->revision) || !reader_u8(&reader,&state->phase) || state->phase>2 ||
            !reader_u16(&reader,&state->remaining_ms) || state->remaining_ms>3000 ||
            !reader_bytes16(&reader,bytes,sizeof(bytes),&size) || !reader_finished(&reader)) return false;
        state->has_rules=size!=0;
        return !size || nativeRoomRules_Decode(&state->rules,bytes,size);
    }
    case SERVER_LISTING:
        output->type = NativeSignalingServerMessage_Listing;
        return reader_string8(&reader, output->data.listing.title, sizeof(output->data.listing.title)) &&
            reader_u8(&reader, &output->data.listing.listed) && output->data.listing.listed <= 1 && reader_finished(&reader);
    case SERVER_LOBBIES: {
        output->type = NativeSignalingServerMessage_Lobbies;
        NativeLobbyDirectory* page = &output->data.directory;
        if (!reader_u16(&reader, &page->offset) || !reader_u16(&reader, &page->total) ||
            !reader_u8(&reader, &page->count) || page->count > 8 ||
            page->offset + page->count > page->total) return false;
        for (unsigned i = 0; i < page->count; ++i) {
            NativeLobbyEntry* entry = &page->entries[i];
            if (!reader_string8(&reader, entry->code, sizeof(entry->code)) ||
                !reader_string8(&reader, entry->title, sizeof(entry->title)) ||
                !reader_string8(&reader, entry->host, sizeof(entry->host)) ||
                !reader_u8(&reader, &entry->fighting) || entry->fighting > 4 ||
                !reader_u8(&reader, &entry->waiting) ||
                !reader_u8(&reader, &entry->spectating) ||
                !reader_u8(&reader, &entry->participants) || entry->participants > 32 ||
                !reader_u8(&reader, &entry->playing) || entry->playing > 1) return false;
        }
        return reader_finished(&reader);
    }
    case SERVER_QUICK_STATE: {
        NativeQuickAssignment* quick = &output->data.quick;
        output->type = NativeSignalingServerMessage_QuickState;
        if (!reader_u8(&reader, &quick->mode) ||
            !reader_u8(&reader, &quick->state) ||
            !reader_u64(&reader, &quick->generation) ||
            !reader_string8(&reader, quick->match_room, sizeof(quick->match_room)) ||
            !reader_string8(&reader, quick->admission, sizeof(quick->admission)) ||
            !reader_u8(&reader, &quick->team) ||
            !reader_u8(&reader, &quick->stage) ||
            !reader_u8(&reader, &quick->phase) ||
            !reader_u16(&reader, &quick->remaining_ms) ||
            !reader_u64(&reader, &quick->completed_epoch) ||
            !reader_u8(&reader, &quick->chooser) ||
            !reader_raw(&reader, quick->votes, sizeof(quick->votes)) || !reader_finished(&reader) ||
            quick->mode > NativeQuickMode_Doubles ||
            quick->state > NativeQuickState_Aborted || quick->team > 1 ||
            quick->stage >= 6 || quick->phase > 4 || quick->remaining_ms > 3000 ||
            (quick->chooser > 1 && quick->chooser != 255) ||
            (quick->phase != 1 && quick->remaining_ms != 0) ||
            (quick->phase && quick->state != NativeQuickState_Assigned)) return false;
        for (unsigned i = 0; i < 4; ++i)
            if (quick->votes[i] > 5 && quick->votes[i] != 255) return false;
        if (quick->state != NativeQuickState_Assigned) return true;
        if (!quick->mode || strlen(quick->match_room) != 12 ||
            strlen(quick->admission) != 32) return false;
        for (unsigned i = 0; i < 12; ++i) {
            char c = quick->match_room[i];
            if (!(c >= 'A' && c <= 'F') && !(c >= '0' && c <= '9')) return false;
        }
        for (unsigned i = 0; i < 32; ++i) {
            char c = quick->admission[i];
            if (!(c >= 'a' && c <= 'f') && !(c >= '0' && c <= '9')) return false;
        }
        return true;
    }
    case SERVER_QUICK_REJECTED:
        output->type = NativeSignalingServerMessage_QuickRejected;
        return reader_string8(&reader, output->data.error, 129) && reader_finished(&reader);
    case SERVER_CHAT_ENTRY: {
        NativeRoomChatEntry* entry = &output->data.chat.entry;
        uint16_t length;
        output->type = NativeSignalingServerMessage_Chat;
        output->data.chat.kind = NativeRoomChatEvent_Entry;
        if (!reader_u64(&reader, &entry->sequence) || !entry->sequence ||
            !reader_u32(&reader, &entry->nonce) || !entry->nonce ||
            !reader_string8(&reader, entry->peer_id, sizeof(entry->peer_id)) ||
            !reader_string8(&reader, entry->name, sizeof(entry->name)) ||
            !reader_bytes16(&reader, (uint8_t*) entry->text, NATIVE_ROOM_CHAT_TEXT_BYTES, &length)) return false;
        entry->text[length] = 0;
        return entry->peer_id[0] && entry->name[0] &&
            nativeUtf8_IsVisibleText((const uint8_t*) entry->text, length) && reader_finished(&reader);
    }
    case SERVER_CHAT_BEGIN:
        output->type = NativeSignalingServerMessage_Chat;
        output->data.chat.kind = NativeRoomChatEvent_Begin;
        return reader_u8(&reader, &output->data.chat.count) &&
            output->data.chat.count <= NATIVE_ROOM_CHAT_HISTORY && reader_finished(&reader);
    case SERVER_CHAT_END:
        output->type = NativeSignalingServerMessage_Chat;
        output->data.chat.kind = NativeRoomChatEvent_End;
        return reader_u64(&reader, &output->data.chat.entry.sequence) && reader_finished(&reader);
    case SERVER_CHAT_REJECTED:
        output->type = NativeSignalingServerMessage_Chat;
        output->data.chat.kind = NativeRoomChatEvent_Rejected;
        return reader_u32(&reader, &output->data.chat.entry.nonce) &&
            reader_string8(&reader, output->data.chat.reason, sizeof(output->data.chat.reason)) && reader_finished(&reader);
    case SERVER_WELCOME:
        output->type = NativeSignalingServerMessage_Welcome;
        return reader_string8(&reader, output->data.welcome.connection_id,
                              sizeof(output->data.welcome.connection_id)) &&
               reader_string8(&reader, output->data.welcome.peer_id,
                              sizeof(output->data.welcome.peer_id)) &&
               reader_u64(&reader, &output->data.welcome.revision) &&
               reader_finished(&reader);
    case SERVER_PRESENCE: {
        output->type = NativeSignalingServerMessage_Presence;
        NativeSignalingPresence* presence = &output->data.presence;
        if (!reader_u64(&reader, &presence->revision) ||
            !reader_string8(&reader, presence->owner_peer_id,
                            sizeof(presence->owner_peer_id)) ||
            !reader_u8(&reader, &presence->participant_count) ||
            presence->participant_count > NATIVE_SIGNALING_MAX_PARTICIPANTS)
        {
            return false;
        }
        for (uint32_t i = 0; i < presence->participant_count; ++i) {
            if (!decode_participant(&reader, &presence->participants[i])) {
                return false;
            }
        }
        return reader_u64(&reader, &presence->match_epoch) &&
               reader_bytes16(&reader, presence->match,
                              sizeof(presence->match),
                              &presence->match_size) &&
               reader_u8(&reader, &presence->rotation_policy) &&
               presence->rotation_policy <= NativeRoomRotation_WinnerOut &&
               reader_string8(&reader, presence->stage_peer_id,
                              sizeof(presence->stage_peer_id)) &&
               reader_u8(&reader, &presence->stage_seat) &&
               (presence->stage_seat < 4 ||
                (presence->stage_seat == UINT8_MAX && !presence->stage_peer_id[0])) &&
               reader_u8(&reader, &presence->stage) &&
               reader_finished(&reader);
    }
    case SERVER_SIGNAL:
        output->type = NativeSignalingServerMessage_Signal;
        return reader_string8(&reader, output->data.signal.peer_id,
                              sizeof(output->data.signal.peer_id)) &&
               reader_bytes16(&reader, output->data.signal.payload,
                              sizeof(output->data.signal.payload),
                              &output->data.signal.payload_size) &&
               reader_finished(&reader);
    case SERVER_SIGNAL_SENT:
        output->type = NativeSignalingServerMessage_SignalSent;
        return reader_string8(&reader, output->data.signal.peer_id,
                              sizeof(output->data.signal.peer_id)) &&
               reader_finished(&reader);
    case SERVER_ERROR: {
        output->type = NativeSignalingServerMessage_Error;
        uint16_t size;
        if (!reader_bytes16(&reader, (uint8_t*) output->data.error,
                            NATIVE_SIGNALING_MAX_ERROR_BYTES, &size) ||
            !reader_finished(&reader))
        {
            return false;
        }
        output->data.error[size] = '\0';
        return true;
    }
    default:
        return false;
    }
}

#endif
