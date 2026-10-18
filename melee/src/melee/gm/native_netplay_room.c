#include "native_netplay_room.h"
#include "native_netplay_room_content.h"

#include "native_turn_credentials.h"

#ifdef MELEE_NATIVE

#include <limits.h>
#include <stdlib.h>
#include <string.h>

enum {
    MAX_RECEIVE_PACKETS_PER_POLL = 64,
    SETUP_PACKET_SIZE = 14,
};

static const uint8_t setup_magic[4] = { 'F', 'M', 'R', '1' };

static void write_u64(uint8_t* output, uint64_t value)
{
    for (uint32_t byte = 0; byte < 8; ++byte) {
        output[byte] = (uint8_t) (value >> (byte * 8));
    }
}

static uint64_t read_u64(const uint8_t* input)
{
    uint64_t result = 0;
    for (uint32_t byte = 0; byte < 8; ++byte) {
        result |= (uint64_t) input[byte] << (byte * 8);
    }
    return result;
}

static NativeNetplayRoomPeer* find_peer(NativeNetplayRoom* room,
                                        const char* peer_id)
{
    for (uint32_t i = 0; i < room->peer_count; ++i) {
        NativeNetplayRoomPeer* peer = &room->peers[i];
        if (peer->configured && strcmp(peer->peer_id, peer_id) == 0) {
            return peer;
        }
    }
    return NULL;
}

static const NativeLobbyMatchPeer*
find_match_peer(const NativeLobbyMatch* match, const char* peer_id)
{
    for (uint32_t i = 0; i < match->peer_count; ++i) {
        if (strcmp(match->peers[i].peer_id, peer_id) == 0) {
            return &match->peers[i];
        }
    }
    return NULL;
}

static const NativeSignalingParticipant*
find_participant(const NativeSignalingPresence* presence, const char* peer_id)
{
    for (uint32_t i = 0; i < presence->participant_count; ++i) {
        if (strcmp(presence->participants[i].peer_id, peer_id) == 0) {
            return &presence->participants[i];
        }
    }
    return NULL;
}

typedef enum MatchAgreementProgress {
    MatchAgreement_Wait,
    MatchAgreement_Ready,
    MatchAgreement_Failed,
} MatchAgreementProgress;

static bool configure_exchange(NativeNetplayRoom* room,
                               const NativeLobbyMatch* match)
{
    const NativeLobbyMatchPeer* local =
        find_match_peer(match, room->connection.local_peer_id);
    if (local == NULL || match->peer_count != room->peer_count + 1 ||
        !nativeNetplayExchange_Init(&room->exchange, match->session_id,
                                    local->numeric_peer, local->port_mask) ||
        !nativeNetplayExchange_SetArtificialDelay(
            &room->exchange, room->artificial_delay_frames))
    {
        return false;
    }
    for (uint32_t i = 0; i < room->peer_count; ++i) {
        NativeNetplayRoomPeer* configured = &room->peers[i];
        const NativeLobbyMatchPeer* peer =
            find_match_peer(match, configured->peer_id);
        if (peer == NULL || peer->numeric_peer != configured->peer ||
            peer->port_mask != configured->port_mask ||
            !nativeNetplayExchange_AddPeer(&room->exchange, peer->numeric_peer,
                                           peer->port_mask))
        {
            return false;
        }
    }
    return true;
}

static bool adopt_owner_match(NativeNetplayRoom* room,
                              const NativeSignalingPresence* presence)
{
    NativeLobbyMatch match;
    uint8_t canonical[NATIVE_LOBBY_MATCH_MAX_WIRE_BYTES];
    if (!nativeLobbyMatch_Decode(&match, presence->match,
                                 presence->match_size))
    {
        return false;
    }
    size_t size =
        nativeLobbyMatch_Encode(&match, canonical, sizeof(canonical));
    if (size != presence->match_size ||
        memcmp(canonical, presence->match, size) != 0 ||
        !configure_exchange(room, &match))
    {
        return false;
    }
    nativeNetplayRoom_ClearContent(room);
    room->match = match;
    memcpy(room->match_bytes, canonical, size);
    room->match_size = (uint16_t) size;
    room->ready_request_revision = UINT64_MAX;
    return true;
}

static MatchAgreementProgress pump_match_agreement(NativeNetplayRoom* room,
                                                    bool content_ready)
{
    NativeNetplayConnection* connection = &room->connection;
    if (!connection->has_presence) {
        return MatchAgreement_Wait;
    }
    const NativeSignalingPresence* presence = &connection->presence;
    bool proposal_matches =
        presence->match_size == room->match_size &&
        memcmp(presence->match, room->match_bytes, room->match_size) == 0;
    bool local_is_owner =
        strcmp(presence->owner_peer_id, connection->local_peer_id) == 0;
    if (!proposal_matches) {
        if (local_is_owner &&
            room->publish_request_epoch != presence->match_epoch)
        {
            if (!nativeSignalingClient_SetMatch(&connection->signaling,
                                                room->match_bytes,
                                                room->match_size))
            {
                return MatchAgreement_Failed;
            }
            room->publish_request_epoch = presence->match_epoch;
        } else if (!local_is_owner && presence->match_size != 0) {
            if (!adopt_owner_match(room, presence)) {
                return MatchAgreement_Failed;
            }
            proposal_matches = true;
        }
        if (!proposal_matches) {
            return MatchAgreement_Wait;
        }
    }

    bool complete = true;
    for (uint32_t i = 0; i < room->match.peer_count; ++i) {
        const NativeLobbyMatchPeer* peer = &room->match.peers[i];
        const NativeSignalingParticipant* participant =
            find_participant(presence, peer->peer_id);
        if (participant == NULL) {
            complete = false;
            continue;
        }
        uint8_t selected_seats = 0;
        for (uint32_t port = 0; port < 4; ++port) {
            if ((peer->port_mask & (1U << port)) != 0 &&
                room->match.slots[port].kind == NativeLobbyMatchSlot_Human) {
                selected_seats |= 1U << room->match.slots[port].local_seat;
            }
        }
        if (participant->active_mask != selected_seats ||
            strcmp(participant->build, connection->local_build) != 0 ||
            participant->active_mask == 0)
        {
            return MatchAgreement_Failed;
        }
    }
    for (uint32_t i = 0; i < presence->participant_count; ++i) {
        const NativeSignalingParticipant* participant =
            &presence->participants[i];
        if (participant->active_mask != 0 &&
            find_match_peer(&room->match, participant->peer_id) == NULL)
        {
            return MatchAgreement_Failed;
        }
    }
    if (!complete) {
        return MatchAgreement_Wait;
    }

    const NativeSignalingParticipant* local =
        find_participant(presence, connection->local_peer_id);
    if (local == NULL) {
        return MatchAgreement_Failed;
    }
    if (!content_ready) {
        return MatchAgreement_Wait;
    }
    if (!local->ready) {
        if (room->ready_request_revision != presence->revision) {
            const NativeSignalingUpdate update = {
                .ready = true,
                .has_ready = true,
            };
            if (!nativeSignalingClient_QueueUpdate(&connection->signaling,
                                                   &update))
            {
                return MatchAgreement_Failed;
            }
            room->ready_request_revision = presence->revision;
        }
        return MatchAgreement_Wait;
    }
    for (uint32_t i = 0; i < room->match.peer_count; ++i) {
        const NativeSignalingParticipant* participant =
            find_participant(presence, room->match.peers[i].peer_id);
        if (participant == NULL || !participant->ready) {
            return MatchAgreement_Wait;
        }
    }
    room->agreed_match_epoch = presence->match_epoch;
    return MatchAgreement_Ready;
}

static bool connection_matches_config(const NativeNetplayRoom* room)
{
    uint8_t connected = 0;
    for (uint32_t i = 0; i < NATIVE_NETPLAY_CONNECTION_MAX_REMOTE_PEERS; ++i) {
        const NativeNetplayConnectionPeer* route = &room->connection.peers[i];
        if (!route->active) {
            continue;
        }
        bool found = false;
        for (uint32_t j = 0; j < room->peer_count; ++j) {
            if (room->peers[j].configured &&
                strcmp(room->peers[j].peer_id, route->peer_id) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
        ++connected;
    }
    return connected == room->peer_count;
}

static bool
setup_packet_matches(const NativeNetplayRoom* room,
                     const NativeNetplayRoomPeer* peer,
                     const NativeNetplayConnectionDatagram* datagram)
{
    return peer != NULL && datagram->datagram.size == SETUP_PACKET_SIZE &&
           memcmp(datagram->datagram.bytes, setup_magic,
                  sizeof(setup_magic)) == 0 &&
           read_u64(datagram->datagram.bytes + 4) ==
               room->exchange.session_id &&
           datagram->datagram.bytes[12] == peer->peer &&
           datagram->datagram.bytes[13] ==
               room->exchange.peers[peer->peer].port_mask;
}

bool nativeNetplayRoom_Connect(NativeNetplayRoom* room,
                               const NativeNetplayConnectionConfig* config)
{
    if (room == NULL || config == NULL) {
        return false;
    }
    memset(room, 0, sizeof(*room));
    NativeNetplayConnectionConfig connection = *config;
    NativeTurnCredentials credentials;
    const bool has_turn_host = connection.ice.turn_host != NULL &&
                               connection.ice.turn_host[0] != '\0';
    const bool has_turn_username = connection.ice.turn_username != NULL &&
                                   connection.ice.turn_username[0] != '\0';
    const bool has_turn_password = connection.ice.turn_password != NULL &&
                                   connection.ice.turn_password[0] != '\0';
    const bool any_explicit_turn = has_turn_host ||
                                   connection.ice.turn_port != 0 ||
                                   has_turn_username || has_turn_password;
    const bool explicit_turn = has_turn_host &&
                               connection.ice.turn_port != 0 &&
                               has_turn_username && has_turn_password;
    if (any_explicit_turn && !explicit_turn) {
        return false;
    }
    if (!explicit_turn) {
        if (!nativeTurnCredentials_Fetch(connection.signaling.endpoint,
                                         &credentials))
        {
            return false;
        }
        if (connection.ice.stun_host == NULL ||
            connection.ice.stun_host[0] == '\0')
        {
            connection.ice.stun_host = credentials.stun_host;
            connection.ice.stun_port = credentials.stun_port;
        }
        if (credentials.has_turn) {
            connection.ice.turn_host = credentials.turn_host;
            connection.ice.turn_port = credentials.turn_port;
            connection.ice.turn_username = credentials.username;
            connection.ice.turn_password = credentials.password;
        }
    }
    if (!nativeNetplayConnection_Start(&room->connection, &connection)) {
        return false;
    }
    room->active = true;
    return true;
}

bool nativeNetplayRoom_PumpLobby(NativeNetplayRoom* room)
{
#ifdef MELEE_CONNECTION_PROBE
    return room != NULL && room->active &&
           nativeNetplayConnection_Pump(&room->connection);
#else
    if (!room || !room->active ||
        !nativeNetplayConnection_Pump(&room->connection)) {
        return false;
    }
    if (room->connection.quick.state == NativeQuickState_Aborted) return true;
    if (!room->match_configured) {
        // Old input packets can arrive after a match ends. Lobby delivery
        // owns separate queues, so retire those packets here before reuse.
        NativeNetplayConnectionDatagram stale;
        for (unsigned i = 0; i < NATIVE_NETPLAY_ICE_DATAGRAM_CAPACITY *
                                    NATIVE_NETPLAY_CONNECTION_MAX_REMOTE_PEERS; ++i) {
            if (!nativeNetplayConnection_PollDatagram(&room->connection, &stale)) {
                break;
            }
        }
    }
    if (room->match_configured) return false;
    for (unsigned i = 0; i < MAX_RECEIVE_PACKETS_PER_POLL; ++i) {
        NativeContentControl control;
        if (!nativeNetplayConnection_PollControl(&room->connection, &control)) break;
        nativeNetplayRoom_GiftControl(room, &control);
        nativeNetplayRoom_CacheControl(room, &control);
    }
    for (unsigned i = 0; i < MAX_RECEIVE_PACKETS_PER_POLL; ++i) {
        NativeContentDatagram data;
        if (!nativeNetplayConnection_PollContent(&room->connection, &data)) break;
        nativeNetplayRoom_GiftPacket(room, &data);
        nativeNetplayRoom_CachePacket(room, &data);
    }
    if (!nativeNetplayRoom_PumpGifts(room)) return false;
    nativeNetplayRoom_PumpCache(room);
    return true;
#endif
}

bool nativeNetplayRoom_ConfigureMatch(NativeNetplayRoom* room,
                                      const NativeLobbyMatch* match,
                                      uint8_t artificial_delay_frames)
{
    if (room == NULL || !room->active || room->match_configured ||
        match == NULL || !nativeLobbyMatch_Validate(match) ||
        match->peer_count < 2)
    {
        return false;
    }
    const NativeLobbyMatchPeer* local =
        find_match_peer(match, room->connection.local_peer_id);
    const NativeSignalingParticipant* local_participant =
        find_participant(&room->connection.presence,
                         room->connection.local_peer_id);
    uint8_t selected_seats = 0;
    if (local != NULL) {
        for (uint32_t port = 0; port < 4; ++port) {
            if ((local->port_mask & (1U << port)) != 0 &&
                match->slots[port].kind == NativeLobbyMatchSlot_Human) {
                selected_seats |= 1U << match->slots[port].local_seat;
            }
        }
    }
    if (local == NULL || local_participant == NULL ||
        local_participant->active_mask != selected_seats)
    {
        return false;
    }

    NativeNetplayExchange exchange;
    NativeNetplayRoomPeer peers[NATIVE_NETPLAY_CONNECTION_MAX_REMOTE_PEERS] = {
        0
    };
    uint8_t peer_count = 0;
    if (!nativeNetplayExchange_Init(&exchange, match->session_id,
                                    local->numeric_peer, local->port_mask) ||
        !nativeNetplayExchange_SetArtificialDelay(&exchange,
                                                  artificial_delay_frames))
    {
        return false;
    }
    for (uint32_t i = 0; i < match->peer_count; ++i) {
        const NativeLobbyMatchPeer* source = &match->peers[i];
        if (source == local) {
            continue;
        }
        if (peer_count >= NATIVE_NETPLAY_CONNECTION_MAX_REMOTE_PEERS ||
            !nativeNetplayExchange_AddPeer(&exchange, source->numeric_peer,
                                           source->port_mask))
        {
            return false;
        }
        NativeNetplayRoomPeer* peer = &peers[peer_count++];
        memcpy(peer->peer_id, source->peer_id, strlen(source->peer_id) + 1);
        peer->peer = source->numeric_peer;
        peer->port_mask = source->port_mask;
        peer->configured = true;
    }

    uint8_t match_bytes[NATIVE_LOBBY_MATCH_MAX_WIRE_BYTES];
    size_t match_size =
        nativeLobbyMatch_Encode(match, match_bytes, sizeof(match_bytes));
    if (match_size == 0 || match_size > UINT16_MAX) {
        return false;
    }
#ifndef MELEE_CONNECTION_PROBE
    nativeNetplayRoom_ClearGifts(room);
    nativeNetplayRoom_ClearCacheTransfers(room);
#endif
    nativeNetplayRoom_ClearContent(room);
    room->match = *match;
    room->exchange = exchange;
    memcpy(room->peers, peers, sizeof(peers));
    memcpy(room->match_bytes, match_bytes, match_size);
    room->match_size = (uint16_t) match_size;
    room->peer_count = peer_count;
    room->artificial_delay_frames = artificial_delay_frames;
    room->publish_request_epoch = UINT64_MAX;
    room->ready_request_revision = UINT64_MAX;
    room->match_configured = true;
    return true;
}

void nativeNetplayRoom_Shutdown(NativeNetplayRoom* room)
{
    if (room == NULL) {
        return;
    }
    free(room->stream_storage);
    room->stream_storage = NULL;
    room->stream = (NativeMatchStream){0};
#ifndef MELEE_CONNECTION_PROBE
    nativeNetplayRoom_ClearGifts(room);
    nativeNetplayRoom_ClearCacheTransfers(room);
#endif
    nativeNetplayRoom_ClearContent(room);
    if (room->active) {
        nativeNetplayConnection_Stop(&room->connection);
    }
    room->active = false;
    room->ready = false;
}

bool nativeNetplayRoom_FinishMatch(NativeNetplayRoom* room)
{
    if (room == NULL || !room->active || !room->match_configured ||
        !nativeSignalingClient_IsRunning(&room->connection.signaling) ||
        !nativeNetplayConnection_UnlockTopology(&room->connection))
    {
        return false;
    }
    nativeNetplayRoom_ClearContent(room);
    room->exchange = (NativeNetplayExchange){ 0 };
    room->match = (NativeLobbyMatch){ 0 };
    memset(room->peers, 0, sizeof(room->peers));
    room->match_size = 0;
    room->peer_count = 0;
    room->match_configured = false;
    room->ready = false;
    return true;
}

bool nativeNetplayRoom_PumpSetup(NativeNetplayRoom* room)
{
    if (room == NULL || !room->active || !room->match_configured) {
        return false;
    }
    if (room->ready) {
        return true;
    }
    if (!nativeNetplayConnection_Pump(&room->connection)) {
        return false;
    }
    // Reconcile the owner proposal before starting any dependency transfer.
    if (pump_match_agreement(room, false) == MatchAgreement_Failed) {
        return false;
    }
    const NativeSignalingPresence* presence = &room->connection.presence;
    if (!room->connection.has_presence ||
        presence->match_size != room->match_size ||
        memcmp(presence->match, room->match_bytes, room->match_size) != 0) {
        return true;
    }
    if (!nativeNetplayRoom_PrepareContent(room)) {
        return false;
    }
    for (uint32_t received = 0; received < MAX_RECEIVE_PACKETS_PER_POLL;
         ++received) {
        NativeNetplayConnectionDatagram datagram;
        if (!nativeNetplayConnection_PollDatagram(&room->connection,
                                                  &datagram)) {
            break;
        }
        NativeContentTransferPacketType type =
            nativeContentTransfer_PacketType(datagram.datagram.bytes,
                                              datagram.datagram.size, NULL);
        if (type != NativeContentTransferPacket_Other) {
            if (type != NativeContentTransferPacket_Invalid &&
                !nativeNetplayRoom_ContentPacket(room, &datagram)) {
                return false;
            }
            continue;
        }
        NativeNetplayRoomPeer* peer = find_peer(room, datagram.peer_id);
        if (!setup_packet_matches(room, peer, &datagram)) {
            if (peer != NULL && datagram.datagram.size == SETUP_PACKET_SIZE &&
                memcmp(datagram.datagram.bytes, setup_magic,
                       sizeof(setup_magic)) == 0 &&
                read_u64(datagram.datagram.bytes + 4) ==
                    room->exchange.session_id) {
                return false;
            }
            ++room->exchange.rejected_packets;
            continue;
        }
        peer->setup_ready = true;
    }
    bool content_ready;
    if (!nativeNetplayRoom_PumpContent(room, &content_ready)) {
        return false;
    }
    MatchAgreementProgress agreement =
        pump_match_agreement(room, content_ready);
    if (agreement == MatchAgreement_Failed) {
        return false;
    }
    if (agreement != MatchAgreement_Ready) {
        return true;
    }
    if (!room->connection.topology_locked) {
        if (nativeNetplayConnection_RemoteCount(&room->connection) !=
                room->peer_count ||
            nativeNetplayConnection_ConnectedCount(&room->connection) !=
                room->peer_count)
        {
            return true;
        }
        if (!connection_matches_config(room) ||
            !nativeNetplayConnection_LockTopology(&room->connection))
        {
            return false;
        }
    }

    uint8_t setup[SETUP_PACKET_SIZE];
    memcpy(setup, setup_magic, sizeof(setup_magic));
    write_u64(setup + 4, room->exchange.session_id);
    setup[12] = room->exchange.local_peer;
    setup[13] = room->exchange.local_port_mask;
    for (uint32_t i = 0; i < room->peer_count; ++i) {
        if (!nativeNetplayConnection_Send(&room->connection,
                                          room->peers[i].peer_id, setup,
                                          sizeof(setup)))
        {
            return false;
        }
    }
    room->ready = true;
    for (uint32_t i = 0; i < room->peer_count; ++i) {
        room->ready = room->ready && room->peers[i].setup_ready;
    }
    if (room->ready && !room->retain_signaling) {
        nativeNetplayConnection_CloseSignaling(&room->connection);
    }
    if (room->ready) {
        nativeNetplayRoom_ClearContent(room);
    }
    return true;
}

bool nativeNetplayRoom_Send(NativeNetplayRoom* room)
{
    if (room == NULL || !room->active || !room->ready) {
        return false;
    }
    uint8_t packet[NATIVE_NETPLAY_INPUT_PACKET_CAPACITY];
    size_t packet_size;
    if (!nativeNetplayExchange_PreparePacket(&room->exchange, packet,
                                             sizeof(packet), &packet_size))
    {
        return false;
    }
    if (packet_size == 0) {
        return true;
    }
    for (uint32_t i = 0; i < room->peer_count; ++i) {
        if (!nativeNetplayConnection_Send(&room->connection,
                                          room->peers[i].peer_id, packet,
                                          packet_size))
        {
            return false;
        }
    }
    return true;
}

bool nativeNetplayRoom_Receive(NativeNetplayRoom* room,
                               NativeRollbackInputHistory* inputs)
{
    if (room == NULL || inputs == NULL || !room->active || !room->ready ||
        !nativeNetplayConnection_Pump(&room->connection))
    {
        return false;
    }
    for (uint32_t received = 0; received < MAX_RECEIVE_PACKETS_PER_POLL;
         ++received)
    {
        NativeNetplayConnectionDatagram datagram;
        if (!nativeNetplayConnection_PollDatagram(&room->connection,
                                                  &datagram))
        {
            return true;
        }
        NativeNetplayRoomPeer* peer = find_peer(room, datagram.peer_id);
        if (peer == NULL) {
            ++room->exchange.rejected_packets;
            ++room->exchange.rejected_header_packets;
            continue;
        }
        if (datagram.datagram.size == SETUP_PACKET_SIZE &&
            memcmp(datagram.datagram.bytes, setup_magic,
                   sizeof(setup_magic)) == 0)
        {
            if (!setup_packet_matches(room, peer, &datagram)) {
                ++room->exchange.rejected_packets;
                ++room->exchange.rejected_header_packets;
            }
            continue;
        }
        nativeNetplayExchange_AcceptPacket(&room->exchange, inputs,
                                           peer->peer, datagram.datagram.bytes,
                                           datagram.datagram.size);
    }
    return true;
}

#endif
