#ifndef MELEE_GM_NATIVE_NETPLAY_CONNECTION_H
#define MELEE_GM_NATIVE_NETPLAY_CONNECTION_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "native_netplay_ice.h"
#include "native_signaling_client.h"
#include "native_room_chat.h"
#include "native_netplay_content_channel.h"

enum {
    NATIVE_NETPLAY_CONNECTION_MAX_REMOTE_PEERS = 3,
    NATIVE_NETPLAY_CONNECTION_SERVER_BYTES = 256,
    NATIVE_NETPLAY_CONNECTION_CREDENTIAL_BYTES = 256,
    NATIVE_NETPLAY_CONNECTION_FAILURE_BYTES = 160,
};

typedef struct NativeNetplayConnectionConfig {
    NativeSignalingClientConfig signaling;
    NativeNetplayIceConfig ice;
} NativeNetplayConnectionConfig;

typedef struct NativeNetplayConnectionPeer {
    NativeNetplayIcePeer ice;
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    uint64_t join_order;
    bool active;
    bool local_description_sent;
    bool remote_description_applied;
} NativeNetplayConnectionPeer;

typedef struct NativeNetplayConnectionDatagram {
    NativeNetplayIceDatagram datagram;
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
} NativeNetplayConnectionDatagram;

typedef struct NativeNetplayConnection {
    NativeSignalingClient signaling;
    NativeRoomChat chat;
    NativeSignalingPresence presence;
    NativeQuickAssignment quick;
    NativeLobbyListing listing;
    NativeRoomLobbyState lobby;
    NativeStageCursor stage_cursor;
    uint32_t lobby_revision;
    bool has_lobby;
    char quick_error[129];
    uint32_t quick_revision;
    NativeNetplayConnectionPeer
        peers[NATIVE_NETPLAY_CONNECTION_MAX_REMOTE_PEERS];
    NativeNetplayContentChannels content;
    NativeNetplayIceConfig ice_config;
    char stun_host[NATIVE_NETPLAY_CONNECTION_SERVER_BYTES];
    char turn_host[NATIVE_NETPLAY_CONNECTION_SERVER_BYTES];
    char turn_username[NATIVE_NETPLAY_CONNECTION_CREDENTIAL_BYTES];
    char turn_password[NATIVE_NETPLAY_CONNECTION_CREDENTIAL_BYTES];
    char local_peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    // Private Welcome identifier; account attachment never uses public peer IDs.
    char connection_id[NATIVE_SIGNALING_MAX_CONNECTION_ID_BYTES + 1];
    char local_build[NATIVE_SIGNALING_MAX_BUILD_BYTES + 1];
    char failure[NATIVE_NETPLAY_CONNECTION_FAILURE_BYTES];
    uint64_t presence_revision;
    uint8_t remote_count;
    uint8_t connected_count;
    uint8_t poll_cursor;
    bool signaling_connected;
    bool has_presence;
    bool topology_locked;
    bool active;
    bool failed;
} NativeNetplayConnection;

// This transition object must remain at a stable address until Stop returns.
bool nativeNetplayConnection_Start(
    NativeNetplayConnection* connection,
    const NativeNetplayConnectionConfig* config);
void nativeNetplayConnection_Stop(NativeNetplayConnection* connection);
bool nativeNetplayConnection_Pump(NativeNetplayConnection* connection);
bool nativeNetplayConnection_LockTopology(NativeNetplayConnection* connection);
bool nativeNetplayConnection_UnlockTopology(NativeNetplayConnection* connection);
void nativeNetplayConnection_CloseSignaling(
    NativeNetplayConnection* connection);
bool nativeNetplayConnection_Send(NativeNetplayConnection* connection,
                                  const char* peer_id, const uint8_t* bytes,
                                  size_t size);
bool nativeNetplayConnection_PollDatagram(
    NativeNetplayConnection* connection,
    NativeNetplayConnectionDatagram* datagram);
uint8_t
nativeNetplayConnection_RemoteCount(const NativeNetplayConnection* connection);
uint8_t nativeNetplayConnection_ConnectedCount(
    const NativeNetplayConnection* connection);
const char*
nativeNetplayConnection_Failure(const NativeNetplayConnection* connection);

#endif
#endif
