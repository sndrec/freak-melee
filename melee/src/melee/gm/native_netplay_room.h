#ifndef MELEE_GM_NATIVE_NETPLAY_ROOM_H
#define MELEE_GM_NATIVE_NETPLAY_ROOM_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stdint.h>

#include "native_lobby_match.h"
#include "native_content_transfer.h"
#include "native_netplay_connection.h"
#include "native_netplay_exchange.h"
#include "native_netplay_room_gift.h"
#include "native_netplay_room_cache.h"

typedef struct NativeNetplayRoomPeer {
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    uint8_t peer;
    uint8_t port_mask;
    bool configured;
    bool setup_ready;
} NativeNetplayRoomPeer;

typedef enum NativeNetplayContentDirection {
    NativeNetplayContent_None,
    NativeNetplayContent_Receiving,
    NativeNetplayContent_Sending,
} NativeNetplayContentDirection;

typedef struct NativeNetplayContentProgress {
    uint32_t bytes_done;
    uint32_t bytes_total;
    uint8_t package_index;
    uint8_t package_count;
    NativeNetplayContentDirection direction;
} NativeNetplayContentProgress;

typedef struct NativeNetplayRoom {
    NativeNetplayConnection connection;
    NativeNetplayExchange exchange;
    NativeLobbyMatch match;
    NativeNetplayRoomPeer peers[NATIVE_NETPLAY_CONNECTION_MAX_REMOTE_PEERS];
    NativeContentTransferSend content_sends[NATIVE_NETPLAY_CONNECTION_MAX_REMOTE_PEERS];
    uint8_t* content_send_bytes[NATIVE_NETPLAY_CONNECTION_MAX_REMOTE_PEERS];
    NativeContentTransferReceive content_receive;
    NativeNetplayGift outgoing_gift;
    NativeNetplayGift incoming_gift;
    NativeRoomCache cache;
    NativeContentId needed_content[8];
    char needed_sources[8][NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    char content_failure[NATIVE_NETPLAY_CONNECTION_FAILURE_BYTES];
    const char* content_root;
    uint8_t match_bytes[NATIVE_LOBBY_MATCH_MAX_WIRE_BYTES];
    uint64_t publish_request_epoch;
    uint64_t ready_request_revision;
    uint64_t agreed_match_epoch;
    uint16_t match_size;
    uint8_t peer_count;
    uint8_t needed_count;
    uint8_t needed_index;
    uint32_t content_serial;
    uint8_t artificial_delay_frames;
    bool active;
    bool match_configured;
    bool content_prepared;
    bool ready;
    bool retain_signaling;
} NativeNetplayRoom;

// Storage must remain at a stable address until Shutdown returns.
bool nativeNetplayRoom_Connect(NativeNetplayRoom* room,
                               const NativeNetplayConnectionConfig* config);
bool nativeNetplayRoom_PumpLobby(NativeNetplayRoom* room);
bool nativeNetplayRoom_ConfigureMatch(NativeNetplayRoom* room,
                                      const NativeLobbyMatch* match,
                                      uint8_t artificial_delay_frames);
void nativeNetplayRoom_Shutdown(NativeNetplayRoom* room);
bool nativeNetplayRoom_FinishMatch(NativeNetplayRoom* room);
bool nativeNetplayRoom_PumpSetup(NativeNetplayRoom* room);
bool nativeNetplayRoom_ContentProgress(const NativeNetplayRoom* room,
                                       NativeNetplayContentProgress* progress);
bool nativeNetplayRoom_Send(NativeNetplayRoom* room);
bool nativeNetplayRoom_Receive(NativeNetplayRoom* room,
                               NativeRollbackInputHistory* inputs);

#endif
#endif
