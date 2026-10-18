#include "native_content_transfer.h"

#ifdef MELEE_NATIVE

#include <stdlib.h>
#include <string.h>

enum {
    HEADER_BYTES = 37,
    CONTROL_BYTES = 41,
    DATA_HEADER_BYTES = 43,
    RETRY_MS = 250,
};

static const uint8_t magic[4] = { 'F', 'M', 'C', '1' };

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

static void write_header(uint8_t packet[576], uint8_t type,
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
                     : size == (type == NativeContentTransferPacket_Begin ||
                                        type == NativeContentTransferPacket_Ack
                                    ? CONTROL_BYTES
                                    : HEADER_BYTES);
    if (!valid) {
        return NativeContentTransferPacket_Invalid;
    }
    if (id != NULL) {
        memcpy(id->bytes, packet + 5, sizeof(id->bytes));
    }
    return type;
}

size_t nativeContentTransfer_Missing(const NativeContentId* id,
                                     uint8_t packet[576])
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
        .active = true,
    };
    return true;
}

void nativeContentTransfer_SendPacket(NativeContentTransferSend* send,
                                      const uint8_t* packet, size_t size,
                                      uint64_t now_ms)
{
    NativeContentId id;
    if (send == NULL || !send->active ||
        nativeContentTransfer_PacketType(packet, size, &id) !=
            NativeContentTransferPacket_Ack ||
        memcmp(&id, &send->id, sizeof(id)) != 0) {
        return;
    }
    size_t acknowledged = read_u32(packet + HEADER_BYTES);
    if (acknowledged > send->size || acknowledged > send->sent_through ||
        (acknowledged != send->size &&
         acknowledged % NATIVE_CONTENT_TRANSFER_CHUNK_BYTES != 0)) {
        return;
    }
    send->begin_acknowledged = true;
    if (acknowledged > send->acknowledged) {
        send->acknowledged = acknowledged;
        if(send->next_offset<acknowledged)send->next_offset=acknowledged;
        send->last_progress_ms = now_ms;
        send->recovery_probe = false;
    }
    send->complete = send->acknowledged == send->size;
}

size_t nativeContentTransfer_NextSend(NativeContentTransferSend* send,
                                      uint64_t now_ms, uint8_t packet[576])
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
        send->last_progress_ms = now_ms;
        return CONTROL_BYTES;
    }
    if (send->recovery_probe && send->next_offset > send->acknowledged) {
        if (now_ms - send->last_progress_ms < RETRY_MS) {
            return 0;
        }
        send->next_offset = send->acknowledged;
        send->last_progress_ms = now_ms;
    }
    if (send->next_offset >= send->size ||
        send->next_offset - send->acknowledged >=
            NATIVE_CONTENT_TRANSFER_WINDOW_CHUNKS *
                NATIVE_CONTENT_TRANSFER_CHUNK_BYTES) {
        if (now_ms - send->last_progress_ms < RETRY_MS) {
            return 0;
        }
        send->next_offset = send->acknowledged;
        send->last_progress_ms = now_ms;
        send->recovery_probe = true;
    }
    const size_t remaining = send->size - send->next_offset;
    const size_t chunk = remaining < NATIVE_CONTENT_TRANSFER_CHUNK_BYTES
                             ? remaining
                             : NATIVE_CONTENT_TRANSFER_CHUNK_BYTES;
    write_header(packet, NativeContentTransferPacket_Data, &send->id);
    write_u32(packet + HEADER_BYTES, (uint32_t) send->next_offset);
    packet[41] = (uint8_t) chunk;
    packet[42] = (uint8_t) (chunk >> 8);
    memcpy(packet + DATA_HEADER_BYTES, send->bytes + send->next_offset, chunk);
    send->next_offset += chunk;
    if(send->next_offset>send->sent_through)send->sent_through=send->next_offset;
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
            size_t index=(offset-receive->received)/NATIVE_CONTENT_TRANSFER_CHUNK_BYTES;
            if(index<NATIVE_CONTENT_TRANSFER_WINDOW_CHUNKS) {
                uint32_t bit=UINT32_C(1)<<index;
                if(receive->received_window&bit) {
                    if(memcmp(receive->bytes+offset,packet+DATA_HEADER_BYTES,length)) {
                        receive->failed=true;return;
                    }
                } else {
                    memcpy(receive->bytes+offset,packet+DATA_HEADER_BYTES,length);
                    receive->received_window|=bit;receive->last_progress_ms=now_ms;
                }
                while(receive->received_window&1) {
                    size_t step=receive->size-receive->received;
                    if(step>NATIVE_CONTENT_TRANSFER_CHUNK_BYTES)step=NATIVE_CONTENT_TRANSFER_CHUNK_BYTES;
                    receive->received+=step;receive->received_window>>=1;
                }
                receive->complete=receive->received==receive->size;
            }
        }
        receive->ack_pending = true;
    }
}

size_t nativeContentTransfer_NextReceive(
    NativeContentTransferReceive* receive, uint64_t now_ms,
    uint8_t packet[576])
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
        receive->ack_pending = false;
        receive->control_sent = true;
        receive->last_control_ms = now_ms;
        return CONTROL_BYTES;
    }
    write_header(packet, NativeContentTransferPacket_Request, &receive->id);
    receive->control_sent = true;
    receive->last_control_ms = now_ms;
    return HEADER_BYTES;
}

#endif
