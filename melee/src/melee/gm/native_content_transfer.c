#include "native_content_transfer.h"

#ifdef MELEE_NATIVE

#include <stdlib.h>
#include <string.h>

enum {
    HEADER_BYTES = 37,
    CONTROL_BYTES = 41,
    DATA_HEADER_BYTES = 43,
    ACK_BYTES = CONTROL_BYTES + 8 * NATIVE_CONTENT_TRANSFER_WINDOW_WORDS,
    RETRY_MS = 250,
    INITIAL_WINDOW = 24,
    MIN_WINDOW = 12,
    REORDER_MARGIN_MS = 20,
};

static const uint8_t magic[4] = { 'F', 'M', 'C', '2' };

_Static_assert(DATA_HEADER_BYTES + NATIVE_CONTENT_TRANSFER_CHUNK_BYTES ==
               NATIVE_CONTENT_TRANSFER_PACKET_CAPACITY, "transfer datagram size");

static void write_u32(uint8_t* output, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) {
        output[i] = (uint8_t) (value >> (i * 8));
    }
}

static uint32_t read_u32(const uint8_t* input)
{
    return (uint32_t) input[0] | (uint32_t) input[1] << 8 |
           (uint32_t) input[2] << 16 | (uint32_t) input[3] << 24;
}

static void write_header(uint8_t* packet, uint8_t type,
                         const NativeContentId* id)
{
    memcpy(packet, magic, sizeof(magic));
    packet[4] = type;
    memcpy(packet + 5, id->bytes, sizeof(id->bytes));
}

NativeContentTransferPacketType nativeContentTransfer_PacketType(
    const uint8_t* packet, size_t size, NativeContentId* id)
{
    if (packet == NULL || size < sizeof(magic) ||
        memcmp(packet, magic, sizeof(magic)) != 0) {
        return NativeContentTransferPacket_Other;
    }
    if (size < HEADER_BYTES || packet[4] < NativeContentTransferPacket_Request ||
        packet[4] > NativeContentTransferPacket_Missing) {
        return NativeContentTransferPacket_Invalid;
    }
    NativeContentTransferPacketType type =
        (NativeContentTransferPacketType) packet[4];
    bool valid = type == NativeContentTransferPacket_Data
                     ? size >= DATA_HEADER_BYTES &&
                           size == DATA_HEADER_BYTES +
                                       ((size_t) packet[41] |
                                        (size_t) packet[42] << 8) &&
                           size <= NATIVE_CONTENT_TRANSFER_PACKET_CAPACITY
                     : size == (type == NativeContentTransferPacket_Ack ? ACK_BYTES :
                                type == NativeContentTransferPacket_Begin ? CONTROL_BYTES :
                                HEADER_BYTES);
    if (!valid) {
        return NativeContentTransferPacket_Invalid;
    }
    if (id != NULL) {
        memcpy(id->bytes, packet + 5, sizeof(id->bytes));
    }
    return type;
}

size_t nativeContentTransfer_Missing(const NativeContentId* id,
                                     uint8_t packet[NATIVE_CONTENT_TRANSFER_PACKET_CAPACITY])
{
    if (id == NULL || packet == NULL) {
        return 0;
    }
    write_header(packet, NativeContentTransferPacket_Missing, id);
    return HEADER_BYTES;
}

bool nativeContentTransfer_StartSend(NativeContentTransferSend* send,
                                     const NativeContentId* id,
                                     const uint8_t* bytes, size_t size)
{
    if (send == NULL || id == NULL || bytes == NULL || size < 32 ||
        size > NATIVE_CONTENT_MAX_PACKAGE_BYTES) {
        return false;
    }
    *send = (NativeContentTransferSend){
        .id = *id,
        .bytes = bytes,
        .size = size,
        .congestion_window = INITIAL_WINDOW,
        .slow_start_threshold = NATIVE_CONTENT_TRANSFER_WINDOW_CHUNKS,
        .active = true,
    };
    return true;
}

static unsigned chunks_for(size_t bytes)
{
    return (unsigned)((bytes + NATIVE_CONTENT_TRANSFER_CHUNK_BYTES - 1) /
                      NATIVE_CONTENT_TRANSFER_CHUNK_BYTES);
}

static void observe_rtt(NativeContentTransferSend* send, uint64_t sample)
{
    unsigned rtt = sample > 4000 ? 4000 : (unsigned)sample;
    if (!rtt) rtt = 1;
    if (!send->rtt_ms) {
        send->rtt_ms = rtt;
        send->rtt_variance_ms = rtt / 2;
    } else {
        unsigned difference = rtt > send->rtt_ms ? rtt - send->rtt_ms : send->rtt_ms - rtt;
        send->rtt_variance_ms = (3 * send->rtt_variance_ms + difference + 2) / 4;
        send->rtt_ms = (7 * send->rtt_ms + rtt + 4) / 8;
    }
}

void nativeContentTransfer_SendPacket(NativeContentTransferSend* send,
                                      const uint8_t* packet, size_t size,
                                      uint64_t now_ms)
{
    NativeContentId id;
    if (send == NULL || !send->active || !send->begin_sent ||
        nativeContentTransfer_PacketType(packet, size, &id) !=
            NativeContentTransferPacket_Ack ||
        memcmp(&id, &send->id, sizeof(id)) != 0) {
        return;
    }
    size_t acknowledged = read_u32(packet + HEADER_BYTES);
    unsigned first = chunks_for(send->acknowledged);
    unsigned boundary = chunks_for(acknowledged);
    if (acknowledged < send->acknowledged || acknowledged > send->size ||
        boundary > send->next_chunk ||
        (acknowledged != send->size &&
         acknowledged % NATIVE_CONTENT_TRANSFER_CHUNK_BYTES != 0) ||
        (packet[CONTROL_BYTES] & 1)) {
        return;
    }
    for (unsigned bit = send->next_chunk - boundary;
         bit < NATIVE_CONTENT_TRANSFER_WINDOW_CHUNKS; ++bit) {
        if (packet[CONTROL_BYTES + bit / 8] & (1u << (bit % 8))) return;
    }
    send->begin_acknowledged = true;
    uint64_t measured_at = 0;
    bool has_rtt = false;
    unsigned newly_acked = 0;
    for (unsigned index = first; index < send->next_chunk; ++index) {
        if (index >= boundary) {
            unsigned bit = index - boundary;
            if (bit >= NATIVE_CONTENT_TRANSFER_WINDOW_CHUNKS ||
                !(packet[CONTROL_BYTES + bit / 8] & (1u << (bit % 8)))) continue;
        }
        NativeContentTransferChunk* chunk =
            &send->chunks[index % NATIVE_CONTENT_TRANSFER_WINDOW_CHUNKS];
        if (chunk->acknowledged) continue;
        chunk->acknowledged = true;
        --send->in_flight;
        ++newly_acked;
        if (index >= send->highest_ack) send->highest_ack = index + 1;
        if (!chunk->retransmitted && (!has_rtt || chunk->sent_ms > measured_at)) {
            measured_at = chunk->sent_ms;
            has_rtt = true;
        }
    }
    if (has_rtt) observe_rtt(send, now_ms - measured_at);
    if (send->congestion_window < send->slow_start_threshold) {
        send->congestion_window += newly_acked;
        if (send->congestion_window > send->slow_start_threshold)
            send->congestion_window = send->slow_start_threshold;
    } else if (send->congestion_window < NATIVE_CONTENT_TRANSFER_WINDOW_CHUNKS) {
        send->increase_credit += newly_acked;
        if (send->increase_credit >= send->congestion_window) {
            send->increase_credit -= send->congestion_window;
            ++send->congestion_window;
        }
    }
    send->acknowledged = acknowledged;
    send->complete = send->acknowledged == send->size;
}

size_t nativeContentTransfer_NextSend(NativeContentTransferSend* send,
                                      uint64_t now_ms, uint8_t packet[NATIVE_CONTENT_TRANSFER_PACKET_CAPACITY])
{
    if (send == NULL || !send->active || send->complete || packet == NULL) {
        return 0;
    }
    if (!send->begin_acknowledged) {
        if (send->begin_sent && now_ms - send->last_begin_ms < RETRY_MS) {
            return 0;
        }
        write_header(packet, NativeContentTransferPacket_Begin, &send->id);
        write_u32(packet + HEADER_BYTES, (uint32_t) send->size);
        send->begin_sent = true;
        send->last_begin_ms = now_ms;
        return CONTROL_BYTES;
    }
    unsigned rtt = send->rtt_ms ? send->rtt_ms : 100;
    unsigned timeout = send->rtt_ms ? rtt + 4 * send->rtt_variance_ms : RETRY_MS;
    if (timeout < 100) timeout = 100;
    unsigned first = chunks_for(send->acknowledged);
    unsigned index = first;
    for (; index < send->next_chunk; ++index) {
        const NativeContentTransferChunk* chunk =
            &send->chunks[index % NATIVE_CONTENT_TRANSFER_WINDOW_CHUNKS];
        if (chunk->acknowledged) continue;
        uint64_t age = now_ms - chunk->sent_ms;
        if (age >= timeout ||
            (!chunk->retransmitted && send->highest_ack > index + 3 &&
             age >= rtt + REORDER_MARGIN_MS)) break;
    }
    NativeContentTransferChunk* entry =
        &send->chunks[index % NATIVE_CONTENT_TRANSFER_WINDOW_CHUNKS];
    if (index < send->next_chunk) {
        // One decrease per window of outstanding chunks, even if a burst
        // loses multiple packets. Each missing chunk has its own retry timer.
        if (index >= send->recovery_through) {
            unsigned reduced = send->congestion_window * 3 / 4;
            send->congestion_window = reduced < MIN_WINDOW ? MIN_WINDOW : reduced;
            send->slow_start_threshold = send->congestion_window;
            send->increase_credit = 0;
            send->recovery_through = send->next_chunk;
        }
        entry->retransmitted = true;
    } else {
        if (index >= chunks_for(send->size) ||
            index - first >= NATIVE_CONTENT_TRANSFER_WINDOW_CHUNKS ||
            send->in_flight >= send->congestion_window) return 0;
        ++send->next_chunk;
        ++send->in_flight;
        *entry = (NativeContentTransferChunk){0};
    }
    entry->sent_ms = now_ms;
    const size_t offset = (size_t)index * NATIVE_CONTENT_TRANSFER_CHUNK_BYTES;
    const size_t remaining = send->size - offset;
    const size_t chunk = remaining < NATIVE_CONTENT_TRANSFER_CHUNK_BYTES
                             ? remaining
                             : NATIVE_CONTENT_TRANSFER_CHUNK_BYTES;
    write_header(packet, NativeContentTransferPacket_Data, &send->id);
    write_u32(packet + HEADER_BYTES, (uint32_t)offset);
    packet[41] = (uint8_t) chunk;
    packet[42] = (uint8_t) (chunk >> 8);
    memcpy(packet + DATA_HEADER_BYTES, send->bytes + offset, chunk);
    return DATA_HEADER_BYTES + chunk;
}

void nativeContentTransfer_StartReceive(NativeContentTransferReceive* receive,
                                        const NativeContentId* id)
{
    if (receive == NULL || id == NULL) {
        return;
    }
    nativeContentTransfer_StopReceive(receive);
    receive->id = *id;
    receive->active = true;
}

void nativeContentTransfer_StopReceive(NativeContentTransferReceive* receive)
{
    if (receive == NULL) {
        return;
    }
    free(receive->bytes);
    *receive = (NativeContentTransferReceive){ 0 };
}

void nativeContentTransfer_ReceivePacket(NativeContentTransferReceive* receive,
                                         const uint8_t* packet, size_t size,
                                         uint64_t now_ms)
{
    NativeContentId id;
    NativeContentTransferPacketType type =
        nativeContentTransfer_PacketType(packet, size, &id);
    if (type == NativeContentTransferPacket_Other ||
        type == NativeContentTransferPacket_Invalid || receive == NULL ||
        !receive->active || receive->failed ||
        memcmp(&id, &receive->id, sizeof(id)) != 0) {
        return;
    }
    if (type == NativeContentTransferPacket_Missing) {
        receive->failed = true;
        return;
    }
    if (type == NativeContentTransferPacket_Begin) {
        size_t length = read_u32(packet + HEADER_BYTES);
        if (length < 32 || length > NATIVE_CONTENT_MAX_PACKAGE_BYTES ||
            (receive->begun && length != receive->size)) {
            receive->failed = true;
            return;
        }
        if (!receive->begun) {
            receive->bytes = malloc(length);
            if (receive->bytes == NULL) {
                receive->failed = true;
                return;
            }
            receive->size = length;
            receive->begun = true;
            receive->last_progress_ms = now_ms;
        }
        receive->ack_pending = true;
    } else if (type == NativeContentTransferPacket_Data && receive->begun) {
        size_t offset = read_u32(packet + HEADER_BYTES);
        size_t length = (size_t) packet[41] | (size_t) packet[42] << 8;
        size_t expected_length = offset < receive->size
                                     ? receive->size - offset
                                     : 0;
        if (expected_length > NATIVE_CONTENT_TRANSFER_CHUNK_BYTES) {
            expected_length = NATIVE_CONTENT_TRANSFER_CHUNK_BYTES;
        }
        if (length == 0 || offset >= receive->size ||
            offset % NATIVE_CONTENT_TRANSFER_CHUNK_BYTES != 0 ||
            length != expected_length) {
            receive->failed = true;
            return;
        }
        if (!receive->complete && offset >= receive->received) {
            size_t index = (offset - receive->received) / NATIVE_CONTENT_TRANSFER_CHUNK_BYTES;
            if (index < NATIVE_CONTENT_TRANSFER_WINDOW_CHUNKS) {
                uint64_t bit = UINT64_C(1) << (index % 64);
                uint64_t* word = &receive->received_window[index / 64];
                if (*word & bit) {
                    if (memcmp(receive->bytes + offset, packet + DATA_HEADER_BYTES, length)) {
                        receive->failed = true;
                        return;
                    }
                } else {
                    memcpy(receive->bytes + offset, packet + DATA_HEADER_BYTES, length);
                    *word |= bit;
                    receive->last_progress_ms = now_ms;
                }
                while (receive->received_window[0] & 1) {
                    size_t step = receive->size - receive->received;
                    if (step > NATIVE_CONTENT_TRANSFER_CHUNK_BYTES)
                        step = NATIVE_CONTENT_TRANSFER_CHUNK_BYTES;
                    receive->received += step;
                    for (unsigned i = 0; i < NATIVE_CONTENT_TRANSFER_WINDOW_WORDS; ++i) {
                        receive->received_window[i] >>= 1;
                        if (i + 1 < NATIVE_CONTENT_TRANSFER_WINDOW_WORDS)
                            receive->received_window[i] |= receive->received_window[i + 1] << 63;
                    }
                }
                receive->complete = receive->received == receive->size;
            }
        }
        receive->ack_pending = true;
    }
}

size_t nativeContentTransfer_NextReceive(
    NativeContentTransferReceive* receive, uint64_t now_ms,
    uint8_t packet[NATIVE_CONTENT_TRANSFER_PACKET_CAPACITY])
{
    if (receive == NULL || !receive->active || receive->failed ||
        packet == NULL) {
        return 0;
    }
    if (!receive->timer_started) {
        receive->last_progress_ms = now_ms;
        receive->timer_started = true;
    }
    if (!receive->ack_pending && receive->control_sent &&
        now_ms - receive->last_control_ms < RETRY_MS) {
        return 0;
    }
    if (receive->begun) {
        write_header(packet, NativeContentTransferPacket_Ack, &receive->id);
        write_u32(packet + HEADER_BYTES, (uint32_t) receive->received);
        for (unsigned i = 0; i < 8 * NATIVE_CONTENT_TRANSFER_WINDOW_WORDS; ++i) {
            packet[CONTROL_BYTES + i] = (uint8_t)(receive->received_window[i / 8] >>
                                                    (8 * (i % 8)));
        }
        receive->ack_pending = false;
        receive->control_sent = true;
        receive->last_control_ms = now_ms;
        return ACK_BYTES;
    }
    write_header(packet, NativeContentTransferPacket_Request, &receive->id);
    receive->control_sent = true;
    receive->last_control_ms = now_ms;
    return HEADER_BYTES;
}

#endif
