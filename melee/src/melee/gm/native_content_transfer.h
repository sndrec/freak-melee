#ifndef MELEE_GM_NATIVE_CONTENT_TRANSFER_H
#define MELEE_GM_NATIVE_CONTENT_TRANSFER_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "native_content_store.h"

enum {
    NATIVE_CONTENT_TRANSFER_PACKET_CAPACITY = 576,
    NATIVE_CONTENT_TRANSFER_CHUNK_BYTES = 512,
    NATIVE_CONTENT_TRANSFER_WINDOW_CHUNKS = 32,
};

typedef struct NativeContentTransferSend {
    NativeContentId id;
    const uint8_t* bytes;
    size_t size;
    size_t acknowledged;
    size_t next_offset;
    size_t sent_through;
    uint64_t last_progress_ms;
    uint64_t last_begin_ms;
    bool active;
    bool begin_sent;
    bool begin_acknowledged;
    bool recovery_probe;
    bool complete;
} NativeContentTransferSend;

typedef struct NativeContentTransferReceive {
    NativeContentId id;
    uint8_t* bytes;
    size_t size;
    size_t received;
    uint32_t received_window;
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
                                     uint8_t packet[576]);

bool nativeContentTransfer_StartSend(NativeContentTransferSend* send,
                                     const NativeContentId* id,
                                     const uint8_t* bytes, size_t size);
void nativeContentTransfer_SendPacket(NativeContentTransferSend* send,
                                      const uint8_t* packet, size_t size,
                                      uint64_t now_ms);
size_t nativeContentTransfer_NextSend(NativeContentTransferSend* send,
                                      uint64_t now_ms, uint8_t packet[576]);

void nativeContentTransfer_StartReceive(NativeContentTransferReceive* receive,
                                        const NativeContentId* id);
void nativeContentTransfer_StopReceive(NativeContentTransferReceive* receive);
void nativeContentTransfer_ReceivePacket(NativeContentTransferReceive* receive,
                                         const uint8_t* packet, size_t size,
                                         uint64_t now_ms);
size_t nativeContentTransfer_NextReceive(
    NativeContentTransferReceive* receive, uint64_t now_ms,
    uint8_t packet[576]);

#endif
#endif
