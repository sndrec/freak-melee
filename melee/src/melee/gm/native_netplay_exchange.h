#ifndef MELEE_GM_NATIVE_NETPLAY_EXCHANGE_H
#define MELEE_GM_NATIVE_NETPLAY_EXCHANGE_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "native_netplay_input.h"
#include <dolphin/pad.h>

typedef struct NativeNetplayPendingChecksum {
    uint64_t frame;
    uint64_t value;
    bool valid;
} NativeNetplayPendingChecksum;

typedef struct NativeNetplayInputAck {
    uint64_t boundary, selective;
} NativeNetplayInputAck;

typedef struct NativeNetplayExchangePeer {
    NativeNetplayPendingChecksum pending_checksums[
        NATIVE_ROLLBACK_INPUT_CAPACITY];
    uint64_t checksum_frame;
    uint64_t checksum;
    uint64_t sample_frame;
    uint64_t lead_sample_at;
    uint64_t echoed_frame;
    NativeNetplayInputAck received;
    NativeNetplayInputAck acknowledged;
    int32_t lead_milliframes;
    uint8_t port_mask;
    bool configured;
    bool has_checksum;
    bool has_sample;
    bool has_lead;
} NativeNetplayExchangePeer;

typedef struct NativeNetplayExchangeFrame {
    PADStatus pads[NATIVE_ROLLBACK_INPUT_PORTS];
    uint64_t frame;
    bool valid;
} NativeNetplayExchangeFrame;

typedef struct NativeNetplayExchange {
    NativeNetplayExchangePeer peers[4];
    NativeNetplayExchangeFrame frames[NATIVE_ROLLBACK_INPUT_CAPACITY];
    uint64_t session_id;
    uint64_t prior_session_id;
    uint64_t newest_frame;
    uint64_t observed_frames[4];
    uint64_t checksum_frame;
    uint64_t checksum;
    uint32_t received_packets;
    uint32_t rejected_packets;
    uint32_t rejected_header_packets;
    uint32_t rejected_checksum_packets;
    uint32_t rejected_input_packets;
    uint32_t stale_packets;
    uint32_t send_backpressure;
    uint8_t local_peer;
    uint8_t local_port_mask;
    uint8_t artificial_delay_frames;
    uint8_t input_delay_frames;
    bool active;
    bool has_newest_frame;
    bool has_checksum;
    bool has_prior_session;
} NativeNetplayExchange;

bool nativeNetplayExchange_Init(NativeNetplayExchange* exchange,
                                uint64_t session_id, uint8_t local_peer,
                                uint8_t local_port_mask);
bool nativeNetplayExchange_BeginPhase(NativeNetplayExchange* exchange,
                                      uint64_t session_id);
bool nativeNetplayExchange_AddPeer(NativeNetplayExchange* exchange,
                                   uint8_t peer, uint8_t port_mask);
bool nativeNetplayExchange_SetArtificialDelay(NativeNetplayExchange* exchange,
                                              uint8_t frames);
bool nativeNetplayExchange_RecordLocal(NativeNetplayExchange* exchange,
                                       uint64_t frame,
                                       const PADStatus pads[4]);
bool nativeNetplayExchange_GetLocal(const NativeNetplayExchange* exchange,
                                    uint64_t frame, PADStatus pads[4]);
int32_t nativeNetplayExchange_MaxLead(const NativeNetplayExchange* exchange);
bool nativeNetplayExchange_SetChecksum(NativeNetplayExchange* exchange,
                                      uint64_t frame, uint64_t checksum);
bool nativeNetplayExchange_PreparePacket(const NativeNetplayExchange* exchange,
                                         uint8_t destination_peer, bool repair,
                                         uint8_t* packet,
                                         size_t packet_capacity,
                                         size_t* packet_size);
bool nativeNetplayExchange_AcceptPacket(NativeNetplayExchange* exchange,
                                       NativeRollbackInputHistory* inputs,
                                       uint8_t source_peer,
                                       const uint8_t* packet,
                                       size_t packet_size);

#endif
#endif
