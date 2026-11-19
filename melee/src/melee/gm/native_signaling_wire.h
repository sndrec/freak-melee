#ifndef MELEE_GM_NATIVE_SIGNALING_WIRE_H
#define MELEE_GM_NATIVE_SIGNALING_WIRE_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "native_room_rules.h"

#ifdef __cplusplus
extern "C" {
#endif

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
    NATIVE_ROOM_CHAT_HISTORY = 64,
    NATIVE_ROOM_CHAT_TEXT_BYTES = 512,
};

typedef enum NativeSignalingServerMessageType {
    NativeSignalingServerMessage_None,
    NativeSignalingServerMessage_Welcome,
    NativeSignalingServerMessage_Presence,
    NativeSignalingServerMessage_Signal,
    NativeSignalingServerMessage_SignalSent,
    NativeSignalingServerMessage_Error,
    NativeSignalingServerMessage_Chat,
    NativeSignalingServerMessage_QuickState,
    NativeSignalingServerMessage_QuickRejected,
    NativeSignalingServerMessage_Lobbies,
    NativeSignalingServerMessage_Listing,
    NativeSignalingServerMessage_LobbyState,
    NativeSignalingServerMessage_StageCursor,
    NativeSignalingServerMessage_CssPointers,
    NativeSignalingServerMessage_RoomActionRejected,
} NativeSignalingServerMessageType;

typedef struct NativeStageCursor {
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    uint64_t epoch, revision;
    uint32_t sequence;
    int16_t x, y; // Original stage-menu coordinates, in units of 1/256.
    uint8_t tile, seat;
} NativeStageCursor;

enum { NATIVE_CSS_POINTER_UNITS = 16384 };
typedef struct NativeCssPointers {
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    int16_t point[4][2];
    uint8_t visible, held;
} NativeCssPointers;

typedef struct NativeLobbyListing {
    char title[65];
    uint8_t listed, capacity, locked;
} NativeLobbyListing;
typedef struct NativeLobbyEntry {
    char code[13], title[65], host[33];
    uint8_t fighting, waiting, spectating, participants, playing, capacity, locked;
} NativeLobbyEntry;
typedef struct NativeLobbyDirectory {
    NativeLobbyEntry entries[8];
    uint16_t offset, total;
    uint8_t count;
} NativeLobbyDirectory;

typedef enum NativeRoomRotationPolicy {
    NativeRoomRotation_Manual,
    NativeRoomRotation_WinnerStays,
    NativeRoomRotation_WinnerOut,
} NativeRoomRotationPolicy;

typedef struct NativeSignalingParticipant {
    char account_id[44], account_handle[25];
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
    uint8_t card_slots[4];
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
    char stage_peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    uint64_t revision;
    uint64_t match_epoch;
    uint16_t match_size;
    uint8_t participant_count;
    uint8_t rotation_policy;
    uint8_t stage_seat;
    uint8_t stage;
} NativeSignalingPresence;

typedef struct NativeSignalingSignal {
    uint8_t payload[NATIVE_SIGNALING_MAX_MESSAGE_BYTES];
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    uint16_t payload_size;
} NativeSignalingSignal;

typedef enum NativeRoomChatEventKind {
    NativeRoomChatEvent_Entry, NativeRoomChatEvent_Begin,
    NativeRoomChatEvent_End, NativeRoomChatEvent_Rejected,
} NativeRoomChatEventKind;

typedef struct NativeRoomChatEntry {
    uint64_t sequence;
    uint32_t nonce;
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    char name[NATIVE_SIGNALING_MAX_NAME_BYTES + 1];
    char text[NATIVE_ROOM_CHAT_TEXT_BYTES + 1];
} NativeRoomChatEntry;

typedef struct NativeRoomChatEvent {
    NativeRoomChatEventKind kind;
    NativeRoomChatEntry entry;
    uint8_t count;
    char reason[129];
} NativeRoomChatEvent;

typedef enum NativeQuickMode {
    NativeQuickMode_None, NativeQuickMode_Singles, NativeQuickMode_Doubles,
} NativeQuickMode;

typedef enum NativeQuickState {
    NativeQuickState_Idle, NativeQuickState_Searching,
    NativeQuickState_Assigned, NativeQuickState_Aborted,
} NativeQuickState;

typedef struct NativeQuickAssignment {
    uint64_t generation;
    uint64_t completed_epoch;
    char match_room[13];
    char admission[33];
    uint16_t remaining_ms;
    uint8_t mode, state, team, stage;
    uint8_t phase;
    uint8_t chooser, votes[4];
} NativeQuickAssignment;

typedef struct NativeSignalingServerMessage {
    NativeSignalingServerMessageType type;
    union {
        NativeSignalingWelcome welcome;
        NativeSignalingPresence presence;
        NativeSignalingSignal signal;
        NativeRoomChatEvent chat;
        NativeQuickAssignment quick;
        NativeLobbyListing listing;
        NativeLobbyDirectory directory;
        NativeRoomLobbyState lobby;
        NativeStageCursor stage_cursor;
        NativeCssPointers css_pointers;
        char error[NATIVE_SIGNALING_MAX_ERROR_BYTES + 1];
    } data;
} NativeSignalingServerMessage;

size_t nativeSignalingWire_EncodeListing(const char* title, bool listed, uint8_t fighter_limit,
                                         const char* password,
                                         uint8_t* output, size_t capacity);
size_t nativeSignalingWire_EncodeLobbies(uint16_t offset, uint8_t* output,
                                        size_t capacity);
size_t nativeSignalingWire_EncodeLobbyRules(const NativeRoomRules* rules, uint8_t* output, size_t capacity);
size_t nativeSignalingWire_EncodeRequestLobbyState(uint8_t* output, size_t capacity);
size_t nativeSignalingWire_EncodeStageCursor(const NativeStageCursor* cursor,
                                             uint8_t* output, size_t capacity);
size_t nativeSignalingWire_EncodeCssPointers(const NativeCssPointers* pointers,
                                             uint8_t* output, size_t capacity);

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
size_t nativeSignalingWire_EncodeChangeCard(uint8_t seat, uint8_t card,
                                           uint8_t expected, uint8_t* output,
                                           size_t capacity);
size_t nativeSignalingWire_EncodeSetRotation(uint8_t policy,
                                             uint8_t* output,
                                             size_t output_capacity);
size_t nativeSignalingWire_EncodeReportMatch(uint64_t epoch,
                                             uint8_t winners,
                                             uint8_t loser_port,
                                             uint8_t* output,
                                             size_t output_capacity);
size_t nativeSignalingWire_EncodePickLobbyStage(uint64_t epoch, uint8_t seat,
                                                uint8_t stage,
                                                uint8_t* output, size_t capacity);
size_t nativeSignalingWire_EncodePostChat(uint32_t nonce, const char* text,
                                        uint8_t* output, size_t capacity);
size_t nativeSignalingWire_EncodeRequestChat(uint8_t* output, size_t capacity);
size_t nativeSignalingWire_EncodeConfigureQuick(uint8_t mode,
                                                uint8_t* output, size_t capacity);
size_t nativeSignalingWire_EncodeQueueQuick(bool searching,
                                            uint8_t* output, size_t capacity);
size_t nativeSignalingWire_EncodeRequestQuick(uint8_t* output, size_t capacity);
size_t nativeSignalingWire_EncodePrepareQuick(uint64_t revision, bool prepared,
                                              const uint8_t* digest,
                                              uint8_t* output, size_t capacity);
size_t nativeSignalingWire_EncodeReportQuick(uint64_t epoch, uint8_t winner,
                                            uint8_t* output, size_t capacity);
size_t nativeSignalingWire_EncodeUnreadyQuick(uint64_t completed_epoch,
                                              uint8_t* output, size_t capacity);
size_t nativeSignalingWire_EncodeVoteQuickStage(uint64_t epoch, uint8_t seat,
                                               uint8_t stage, uint8_t* output,
                                               size_t capacity);
bool nativeSignalingWire_DecodeServer(const uint8_t* message,
                                      size_t message_size,
                                      NativeSignalingServerMessage* output);

#ifdef __cplusplus
}
#endif
#endif
#endif
