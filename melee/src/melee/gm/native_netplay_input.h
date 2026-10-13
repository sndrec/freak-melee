#ifndef MELEE_GM_NATIVE_NETPLAY_INPUT_H
#define MELEE_GM_NATIVE_NETPLAY_INPUT_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "native_rollback_input.h"
#include "native_confirmed_checkpoint.h"
#include <dolphin/pad.h>

enum {
    NATIVE_NETPLAY_INPUT_MAX_FRAMES = 8,
    NATIVE_NETPLAY_INPUT_PACKET_CAPACITY = 576,
};

typedef struct NativeNetplayInputBatch {
    PADStatus pads[NATIVE_NETPLAY_INPUT_MAX_FRAMES]
                  [NATIVE_ROLLBACK_INPUT_PORTS];
    uint64_t session_id;
    uint64_t start_frame;
    uint64_t checksum_frame;
    uint64_t checksum;
    uint8_t sender_peer;
    uint8_t port_mask;
    uint8_t frame_count;
    bool has_checksum;
} NativeNetplayInputBatch;

size_t nativeNetplayInput_Encode(const NativeNetplayInputBatch* batch,
                                 uint8_t* packet, size_t packet_capacity);
bool nativeNetplayInput_Decode(NativeNetplayInputBatch* batch,
                               const uint8_t* packet, size_t packet_size);
bool nativeNetplayInput_SubmitBatch(NativeRollbackInputHistory* history,
                                    const NativeNetplayInputBatch* batch,
                                    uint64_t expected_session_id,
                                    uint8_t expected_sender_peer,
                                    uint8_t expected_port_mask);

#endif
#endif
