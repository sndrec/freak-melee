#include "native_netplay_connection.h"

#ifdef MELEE_NATIVE

#include <stdio.h>
#include <string.h>

#include "native_netplay_connection_wire.h"

static bool copy_optional(char* output, size_t capacity, const char* input)
{
    if (output == NULL || capacity == 0) {
        return false;
    }
    if (input == NULL || input[0] == '\0') {
        output[0] = '\0';
        return true;
    }
    size_t length = strnlen(input, capacity);
    if (length >= capacity) {
        return false;
    }
    memcpy(output, input, length + 1);
    return true;
}

static void fail(NativeNetplayConnection* connection, const char* reason)
{
    if (!connection->failed) {
        snprintf(connection->failure, sizeof(connection->failure), "%s",
                 reason == NULL ? "connection setup failed" : reason);
    }
    connection->failed = true;
}

static NativeNetplayConnectionPeer*
find_peer(NativeNetplayConnection* connection, const char* peer_id)
{
    for (uint32_t i = 0; i < NATIVE_NETPLAY_CONNECTION_MAX_REMOTE_PEERS; ++i) {
        NativeNetplayConnectionPeer* peer = &connection->peers[i];
        if (peer->active && strcmp(peer->peer_id, peer_id) == 0) {
            return peer;
        }
    }
    return NULL;
}

static void remove_peer(NativeNetplayConnectionPeer* peer)
{
    nativeNetplayIce_Shutdown(&peer->ice);
    memset(peer, 0, sizeof(*peer));
}

static bool add_peer(NativeNetplayConnection* connection,
                     const NativeSignalingParticipant* participant)
{
    for (uint32_t i = 0; i < NATIVE_NETPLAY_CONNECTION_MAX_REMOTE_PEERS; ++i) {
        NativeNetplayConnectionPeer* peer = &connection->peers[i];
        if (peer->active) {
            continue;
        }
        size_t id_length =
            strnlen(participant->peer_id, sizeof(peer->peer_id));
        if (id_length == 0 || id_length >= sizeof(peer->peer_id) ||
            !nativeNetplayIce_Init(&peer->ice, &connection->ice_config) ||
            !nativeNetplayIce_StartGathering(&peer->ice))
        {
            nativeNetplayIce_Shutdown(&peer->ice);
            return false;
        }
        memcpy(peer->peer_id, participant->peer_id, id_length + 1);
        peer->join_order = participant->join_order;
        peer->active = true;
        return true;
    }
    return false;
}

static bool
participant_is_remote_active(const NativeNetplayConnection* connection,
                             const NativeSignalingParticipant* participant)
{
    return strcmp(participant->peer_id, connection->local_peer_id) != 0 &&
           strcmp(participant->build, connection->local_build) == 0 &&
           participant->active_mask != 0 &&
           participant->local_players != 0;
}

static bool presence_has_peer(const NativeNetplayConnection* connection,
                              const NativeSignalingPresence* presence,
                              const NativeNetplayConnectionPeer* peer)
{
    for (uint32_t i = 0; i < presence->participant_count; ++i) {
        const NativeSignalingParticipant* participant =
            &presence->participants[i];
        if (participant_is_remote_active(connection, participant) &&
            participant->join_order == peer->join_order &&
            strcmp(participant->peer_id, peer->peer_id) == 0)
        {
            return true;
        }
    }
    return false;
}

static bool apply_presence(NativeNetplayConnection* connection,
                           const NativeSignalingPresence* presence)
{
    if (presence->revision < connection->presence_revision) {
        return true;
    }
    connection->presence_revision = presence->revision;
    bool local_active = false;
    for (uint32_t i = 0; i < presence->participant_count; ++i) {
        const NativeSignalingParticipant* participant =
            &presence->participants[i];
        if (strcmp(participant->peer_id, connection->local_peer_id) == 0) {
            local_active = participant->active_mask != 0;
            break;
        }
    }
    // Gameplay routes exist between active players. When a local seat leaves
    // that set, both ends must close their old ICE session before rejoining.
    for (uint32_t i = 0; i < NATIVE_NETPLAY_CONNECTION_MAX_REMOTE_PEERS; ++i) {
        NativeNetplayConnectionPeer* peer = &connection->peers[i];
        if (peer->active &&
            (!local_active || !presence_has_peer(connection, presence, peer)))
        {
            remove_peer(peer);
        }
    }

    for (uint32_t i = 0; local_active && i < presence->participant_count; ++i)
    {
        const NativeSignalingParticipant* participant =
            &presence->participants[i];
        if (!participant_is_remote_active(connection, participant)) {
            continue;
        }
        NativeNetplayConnectionPeer* peer =
            find_peer(connection, participant->peer_id);
        if (peer != NULL && peer->join_order != participant->join_order) {
            remove_peer(peer);
            peer = NULL;
        }
        if (peer == NULL && !add_peer(connection, participant)) {
            return false;
        }
    }
    connection->presence = *presence;
    connection->has_presence = true;
    nativeNetplayContentChannels_Reconcile(connection);
    return true;
}

static bool apply_signal(NativeNetplayConnection* connection,
                         const NativeSignalingSignal* signal)
{
    NativeConnectionMessage message;
    if (!nativeNetplayConnectionWire_DecodeMessage(
            signal->payload, signal->payload_size, &message)) {
        return false;
    }
    if (message.type == NativeConnectionMessage_Control) {
        nativeNetplayContentChannels_Control(
            connection, signal->peer_id, message.payload, message.size);
        return true;
    }
    if (message.type == NativeConnectionMessage_ContentDescription) {
        nativeNetplayContentChannels_Description(
            connection, signal->peer_id, message.serial,
            message.sender_owns_transfer, message.payload, message.size);
        return true;
    }
    NativeNetplayConnectionPeer* peer = find_peer(connection, signal->peer_id);
    if (peer == NULL) {
        // A description already in transit can arrive after a seat becomes
        // waiting/spectating and its gameplay route has closed.
        return true;
    }
    if (peer->remote_description_applied) {
        return true;
    }
    char description[NATIVE_NETPLAY_ICE_DESCRIPTION_CAPACITY];
    if (!nativeNetplayConnectionWire_DecodeDescription(
            signal->payload, signal->payload_size, description,
            sizeof(description)) ||
        !nativeNetplayIce_SetRemoteDescription(&peer->ice, description) ||
        !nativeNetplayIce_SetRemoteGatheringDone(&peer->ice))
    {
        return false;
    }
    peer->remote_description_applied = true;
    return true;
}

static bool process_signaling(NativeNetplayConnection* connection)
{
    NativeSignalingClientEvent event;
    while (nativeSignalingClient_Poll(&connection->signaling, &event)) {
        if (event.type == NativeSignalingClientEvent_Connected) {
            connection->signaling_connected = true;
            continue;
        }
        if (event.type == NativeSignalingClientEvent_Disconnected) {
            connection->signaling_connected = false;
            if (connection->topology_locked) {
                continue;
            }
            fail(connection, event.detail[0] == '\0'
                                 ? "room signaling disconnected"
                                 : event.detail);
            return false;
        }
        if (connection->topology_locked &&
            event.message.type == NativeSignalingServerMessage_Presence)
        {
            connection->presence = event.message.data.presence;
            connection->has_presence = true;
            nativeNetplayContentChannels_Reconcile(connection);
            continue;
        }
        if (connection->topology_locked) {
            if (event.message.type == NativeSignalingServerMessage_Signal) {
                apply_signal(connection, &event.message.data.signal);
            }
            continue;
        }
        switch (event.message.type) {
        case NativeSignalingServerMessage_Presence:
            if (!apply_presence(connection, &event.message.data.presence)) {
                fail(connection, "could not reconcile room participants");
                return false;
            }
            break;
        case NativeSignalingServerMessage_Signal:
            if (!apply_signal(connection, &event.message.data.signal)) {
                fail(connection, "could not apply a peer ICE description");
                return false;
            }
            break;
        case NativeSignalingServerMessage_Error:
            fail(connection, event.message.data.error);
            return false;
        default:
            break;
        }
    }
    return true;
}

static bool send_local_descriptions(NativeNetplayConnection* connection)
{
    uint8_t message[NATIVE_NETPLAY_ICE_DESCRIPTION_CAPACITY + 8];
    char description[NATIVE_NETPLAY_ICE_DESCRIPTION_CAPACITY];
    for (uint32_t i = 0; i < NATIVE_NETPLAY_CONNECTION_MAX_REMOTE_PEERS; ++i) {
        NativeNetplayConnectionPeer* peer = &connection->peers[i];
        if (!peer->active || peer->local_description_sent ||
            !nativeNetplayIce_GatheringDone(&peer->ice))
        {
            continue;
        }
        if (!nativeNetplayIce_GetLocalDescription(&peer->ice, description,
                                                  sizeof(description)))
        {
            return false;
        }
        size_t size = nativeNetplayConnectionWire_EncodeDescription(
            description, message, sizeof(message));
        if (size == 0) {
            return false;
        }
        if (!nativeSignalingClient_QueueSignal(&connection->signaling,
                                               peer->peer_id, message,
                                               (uint16_t) size))
        {
            continue;
        }
        peer->local_description_sent = true;
    }
    return true;
}

static void count_peers(NativeNetplayConnection* connection)
{
    connection->remote_count = 0;
    connection->connected_count = 0;
    for (uint32_t i = 0; i < NATIVE_NETPLAY_CONNECTION_MAX_REMOTE_PEERS; ++i) {
        NativeNetplayConnectionPeer* peer = &connection->peers[i];
        if (!peer->active) {
            continue;
        }
        ++connection->remote_count;
        NativeNetplayIceState state = nativeNetplayIce_State(&peer->ice);
        if (state == NativeNetplayIceState_Connected ||
            state == NativeNetplayIceState_Completed)
        {
            ++connection->connected_count;
        } else if (state == NativeNetplayIceState_Failed) {
            fail(connection, "ICE candidate selection failed");
        }
    }
}

bool nativeNetplayConnection_Start(NativeNetplayConnection* connection,
                                   const NativeNetplayConnectionConfig* config)
{
    if (connection == NULL || config == NULL ||
        config->signaling.peer_id == NULL || config->signaling.build == NULL)
    {
        return false;
    }
    memset(connection, 0, sizeof(*connection));
    if (!copy_optional(connection->local_peer_id,
                       sizeof(connection->local_peer_id),
                       config->signaling.peer_id) ||
        !copy_optional(connection->local_build,
                       sizeof(connection->local_build),
                       config->signaling.build) ||
        !copy_optional(connection->stun_host, sizeof(connection->stun_host),
                       config->ice.stun_host) ||
        !copy_optional(connection->turn_host, sizeof(connection->turn_host),
                       config->ice.turn_host) ||
        !copy_optional(connection->turn_username,
                       sizeof(connection->turn_username),
                       config->ice.turn_username) ||
        !copy_optional(connection->turn_password,
                       sizeof(connection->turn_password),
                       config->ice.turn_password))
    {
        return false;
    }
    connection->ice_config = config->ice;
    connection->ice_config.stun_host =
        connection->stun_host[0] != '\0' ? connection->stun_host : NULL;
    connection->ice_config.turn_host =
        connection->turn_host[0] != '\0' ? connection->turn_host : NULL;
    connection->ice_config.turn_username = connection->turn_username[0] != '\0'
                                               ? connection->turn_username
                                               : NULL;
    connection->ice_config.turn_password = connection->turn_password[0] != '\0'
                                               ? connection->turn_password
                                               : NULL;
    if (!nativeSignalingClient_Start(&connection->signaling,
                                     &config->signaling))
    {
        return false;
    }
    nativeRoomChat_Init(&connection->chat, &connection->signaling);
    connection->active = true;
    return true;
}

void nativeNetplayConnection_Stop(NativeNetplayConnection* connection)
{
    if (connection == NULL) {
        return;
    }
    nativeNetplayContentChannels_Clear(connection);
    for (uint32_t i = 0; i < NATIVE_NETPLAY_CONNECTION_MAX_REMOTE_PEERS; ++i) {
        if (connection->peers[i].active) {
            remove_peer(&connection->peers[i]);
        }
    }
    nativeSignalingClient_Stop(&connection->signaling);
    nativeRoomChat_Update(&connection->chat, false);
    connection->active = false;
}

bool nativeNetplayConnection_Pump(NativeNetplayConnection* connection)
{
    if (connection == NULL || !connection->active || connection->failed) {
        return false;
    }
    if (!process_signaling(connection) ||
        (!connection->topology_locked && !send_local_descriptions(connection)))
    {
        nativeRoomChat_Update(&connection->chat, false);
        fail(connection, "connection setup failed");
        return false;
    }
    nativeRoomChat_Update(&connection->chat, connection->signaling_connected && connection->has_presence);
    count_peers(connection);
    nativeNetplayContentChannels_Pump(connection);
    return !connection->failed;
}

bool nativeNetplayConnection_LockTopology(NativeNetplayConnection* connection)
{
    if (connection == NULL || !connection->active || connection->failed) {
        return false;
    }
    count_peers(connection);
    if (connection->remote_count == 0 ||
        connection->connected_count != connection->remote_count)
    {
        return false;
    }
    connection->topology_locked = true;
    return true;
}

bool nativeNetplayConnection_UnlockTopology(
    NativeNetplayConnection* connection)
{
    if (connection == NULL || !connection->active || connection->failed ||
        !connection->topology_locked || !connection->has_presence)
    {
        return false;
    }
    connection->topology_locked = false;
    if (!apply_presence(connection, &connection->presence)) {
        fail(connection, "could not reconcile room participants");
        return false;
    }
    count_peers(connection);
    return true;
}

void nativeNetplayConnection_CloseSignaling(
    NativeNetplayConnection* connection)
{
    if (connection == NULL || !connection->topology_locked) {
        return;
    }
    nativeSignalingClient_Stop(&connection->signaling);
    connection->signaling_connected = false;
    nativeRoomChat_Update(&connection->chat, false);
}

bool nativeNetplayConnection_Send(NativeNetplayConnection* connection,
                                  const char* peer_id, const uint8_t* bytes,
                                  size_t size)
{
    if (connection == NULL || peer_id == NULL || connection->failed) {
        return false;
    }
    NativeNetplayConnectionPeer* peer = find_peer(connection, peer_id);
    return peer != NULL && nativeNetplayIce_Send(&peer->ice, bytes, size);
}

bool nativeNetplayConnection_PollDatagram(
    NativeNetplayConnection* connection,
    NativeNetplayConnectionDatagram* datagram)
{
    if (connection == NULL || datagram == NULL) {
        return false;
    }
    for (uint32_t offset = 0;
         offset < NATIVE_NETPLAY_CONNECTION_MAX_REMOTE_PEERS; ++offset)
    {
        uint32_t index = (connection->poll_cursor + offset) %
                         NATIVE_NETPLAY_CONNECTION_MAX_REMOTE_PEERS;
        NativeNetplayConnectionPeer* peer = &connection->peers[index];
        if (!peer->active ||
            !nativeNetplayIce_PollDatagram(&peer->ice, &datagram->datagram))
        {
            continue;
        }
        memcpy(datagram->peer_id, peer->peer_id, sizeof(datagram->peer_id));
        connection->poll_cursor =
            (uint8_t) ((index + 1) %
                       NATIVE_NETPLAY_CONNECTION_MAX_REMOTE_PEERS);
        return true;
    }
    return false;
}

uint8_t
nativeNetplayConnection_RemoteCount(const NativeNetplayConnection* connection)
{
    return connection == NULL ? 0 : connection->remote_count;
}

uint8_t nativeNetplayConnection_ConnectedCount(
    const NativeNetplayConnection* connection)
{
    return connection == NULL ? 0 : connection->connected_count;
}

const char*
nativeNetplayConnection_Failure(const NativeNetplayConnection* connection)
{
    return connection == NULL || !connection->failed ? NULL
                                                     : connection->failure;
}

#endif
