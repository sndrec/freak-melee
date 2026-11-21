#ifndef MELEE_GM_NATIVE_CONTENT_TRANSFER_H
#define MELEE_GM_NATIVE_CONTENT_TRANSFER_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "native_content_store.h"

enum {
    NATIVE_CONTENT_TRANSFER_PACKET_CAPACITY = 1067,
    NATIVE_CONTENT_TRANSFER_CHUNK_BYTES = 1024,
    NATIVE_CONTENT_TRANSFER_WINDOW_CHUNKS = 128,
    NATIVE_CONTENT_TRANSFER_WINDOW_WORDS = NATIVE_CONTENT_TRANSFER_WINDOW_CHUNKS / 64,
};

typedef struct NativeContentTransferChunk {
    uint64_t sent_ms;
    bool acknowledged, retransmitted;
} NativeContentTransferChunk;

typedef struct NativeContentTransferSend {
    NativeContentId id;
    const uint8_t* bytes;
    NativeContentTransferChunk chunks[NATIVE_CONTENT_TRANSFER_WINDOW_CHUNKS];
    size_t size;
    size_t acknowledged;
    unsigned next_chunk, highest_ack, recovery_through;
    unsigned in_flight, congestion_window, slow_start_threshold, increase_credit;
    unsigned rtt_ms, rtt_variance_ms;
    uint64_t last_begin_ms;
    bool active;
    bool begin_sent;
    bool begin_acknowledged;
    bool complete;
} NativeContentTransferSend;

typedef struct NativeContentTransferReceive {
    NativeContentId id;
    NativeContentPreview preview;
    uint8_t* bytes;
    size_t size;
    size_t received;
    uint64_t received_window[NATIVE_CONTENT_TRANSFER_WINDOW_WORDS];
    uint64_t last_control_ms;
    uint64_t last_progress_ms;
    bool active;
    bool begun;
    bool control_sent;
    bool timer_started;
    bool ack_pending;
    bool complete;
    bool failed;
} NativeContentTransferReceive;

typedef enum NativeContentTransferPacketType {
    NativeContentTransferPacket_Other,
    NativeContentTransferPacket_Request,
    NativeContentTransferPacket_Begin,
    NativeContentTransferPacket_Data,
    NativeContentTransferPacket_Ack,
    NativeContentTransferPacket_Missing,
    NativeContentTransferPacket_Invalid,
} NativeContentTransferPacketType;

NativeContentTransferPacketType nativeContentTransfer_PacketType(
    const uint8_t* packet, size_t size, NativeContentId* id);
size_t nativeContentTransfer_Missing(const NativeContentId* id,
                                     uint8_t packet[NATIVE_CONTENT_TRANSFER_PACKET_CAPACITY]);

bool nativeContentTransfer_StartSend(NativeContentTransferSend* send,
                                     const NativeContentId* id,
                                     const uint8_t* bytes, size_t size);
void nativeContentTransfer_SendPacket(NativeContentTransferSend* send,
                                      const uint8_t* packet, size_t size,
                                      uint64_t now_ms);
size_t nativeContentTransfer_NextSend(NativeContentTransferSend* send,
                                      uint64_t now_ms, uint8_t packet[NATIVE_CONTENT_TRANSFER_PACKET_CAPACITY]);

void nativeContentTransfer_StartReceive(NativeContentTransferReceive* receive,
                                        const NativeContentId* id);
void nativeContentTransfer_StopReceive(NativeContentTransferReceive* receive);
void nativeContentTransfer_ReceivePacket(NativeContentTransferReceive* receive,
                                         const uint8_t* packet, size_t size,
                                         uint64_t now_ms);
size_t nativeContentTransfer_NextReceive(
    NativeContentTransferReceive* receive, uint64_t now_ms,
    uint8_t packet[NATIVE_CONTENT_TRANSFER_PACKET_CAPACITY]);

#endif
#endif
