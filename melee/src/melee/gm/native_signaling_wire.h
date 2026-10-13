#ifndef MELEE_GM_NATIVE_SIGNALING_WIRE_H
#define MELEE_GM_NATIVE_SIGNALING_WIRE_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    NATIVE_SIGNALING_MAX_MESSAGE_BYTES = 16 * 1024,
    NATIVE_SIGNALING_MAX_PARTICIPANTS = 32,
    NATIVE_SIGNALING_MAX_PEER_ID_BYTES = 64,
    NATIVE_SIGNALING_MAX_NAME_BYTES = 32,
    NATIVE_SIGNALING_MAX_BUILD_BYTES = 96,
    NATIVE_SIGNALING_MAX_SELECTION_BYTES = 512,
    NATIVE_SIGNALING_MAX_MATCH_BYTES = 4096,
    NATIVE_SIGNALING_MAX_CONNECTION_ID_BYTES = 64,
    NATIVE_SIGNALING_MAX_ERROR_BYTES = 512,
};

typedef enum NativeSignalingServerMessageType {
    NativeSignalingServerMessage_None,
    NativeSignalingServerMessage_Welcome,
    NativeSignalingServerMessage_Presence,
    NativeSignalingServerMessage_Signal,
    NativeSignalingServerMessage_SignalSent,
    NativeSignalingServerMessage_Error,
} NativeSignalingServerMessageType;

typedef enum NativeRoomRotationPolicy {
    NativeRoomRotation_Manual,
    NativeRoomRotation_WinnerStays,
    NativeRoomRotation_WinnerOut,
} NativeRoomRotationPolicy;

typedef struct NativeSignalingParticipant {
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    char name[NATIVE_SIGNALING_MAX_NAME_BYTES + 1];
    char build[NATIVE_SIGNALING_MAX_BUILD_BYTES + 1];
    uint8_t selection[NATIVE_SIGNALING_MAX_SELECTION_BYTES];
    uint64_t join_order;
    uint16_t selection_size;
    uint8_t local_players;
    uint8_t active_mask;
    uint8_t waiting_mask;
    uint8_t waiting_ranks[4];
    bool ready;
} NativeSignalingParticipant;

typedef struct NativeSignalingWelcome {
    char connection_id[NATIVE_SIGNALING_MAX_CONNECTION_ID_BYTES + 1];
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    uint64_t revision;
} NativeSignalingWelcome;

typedef struct NativeSignalingPresence {
    NativeSignalingParticipant participants[NATIVE_SIGNALING_MAX_PARTICIPANTS];
    uint8_t match[NATIVE_SIGNALING_MAX_MATCH_BYTES];
    char owner_peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    uint64_t revision;
    uint64_t match_epoch;
    uint16_t match_size;
    uint8_t participant_count;
    uint8_t rotation_policy;
} NativeSignalingPresence;

typedef struct NativeSignalingSignal {
    uint8_t payload[NATIVE_SIGNALING_MAX_MESSAGE_BYTES];
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    uint16_t payload_size;
} NativeSignalingSignal;

typedef struct NativeSignalingServerMessage {
    NativeSignalingServerMessageType type;
    union {
        NativeSignalingWelcome welcome;
        NativeSignalingPresence presence;
        NativeSignalingSignal signal;
        char error[NATIVE_SIGNALING_MAX_ERROR_BYTES + 1];
    } data;
} NativeSignalingServerMessage;

typedef struct NativeSignalingUpdate {
    const char* name;
    const uint8_t* selection;
    uint16_t selection_size;
    uint8_t local_players;
    uint8_t active_mask;
    uint8_t waiting_mask;
    bool ready;
    bool has_name;
    bool has_local_players;
    bool has_active_mask;
    bool has_waiting_mask;
    bool has_ready;
    bool has_selection;
} NativeSignalingUpdate;

size_t nativeSignalingWire_EncodeUpdate(const NativeSignalingUpdate* update,
                                        uint8_t* output,
                                        size_t output_capacity);
size_t nativeSignalingWire_EncodeSignal(const char* target_peer_id,
                                        const uint8_t* payload,
                                        uint16_t payload_size,
                                        uint8_t* output,
                                        size_t output_capacity);
size_t nativeSignalingWire_EncodeRequestPresence(uint8_t* output,
                                                 size_t output_capacity);
size_t nativeSignalingWire_EncodeSetMatch(const uint8_t* payload,
                                          uint16_t payload_size,
                                          uint8_t* output,
                                          size_t output_capacity);
size_t nativeSignalingWire_EncodeAssignSeats(const char* peer_id,
                                             uint8_t active_mask,
                                             uint8_t waiting_mask,
                                             uint8_t* output,
                                             size_t output_capacity);
size_t nativeSignalingWire_EncodeSetRotation(uint8_t policy,
                                             uint8_t* output,
                                             size_t output_capacity);
size_t nativeSignalingWire_EncodeReportMatch(uint64_t epoch,
                                             const char* winner_peer_id,
                                             uint8_t winner_seat,
                                             bool eligible,
                                             uint8_t* output,
                                             size_t output_capacity);
bool nativeSignalingWire_DecodeServer(const uint8_t* message,
                                      size_t message_size,
                                      NativeSignalingServerMessage* output);

#endif
#endif
