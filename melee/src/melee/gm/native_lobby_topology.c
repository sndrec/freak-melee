#include "native_lobby_topology.h"

#ifdef MELEE_NATIVE

#include <string.h>

static bool valid_participant(const NativeSignalingParticipant* participant,
                              const char* required_build)
{
    if (participant->local_players > 4) {
        return false;
    }
    size_t id_length = strnlen(participant->peer_id,
                               NATIVE_LOBBY_MATCH_PEER_ID_BYTES + 1);
    uint8_t seats = (uint8_t) ((1U << participant->local_players) - 1);
    return participant->active_mask != 0 &&
           participant->local_players >= 1 &&
           participant->local_players <= 4 &&
           ((participant->active_mask | participant->waiting_mask) & ~seats) == 0 &&
           (participant->active_mask & participant->waiting_mask) == 0 &&
           id_length >= 8 &&
           id_length <= NATIVE_LOBBY_MATCH_PEER_ID_BYTES &&
           strcmp(participant->build, required_build) == 0;
}

bool nativeLobbyTopology_AssignMatchPeers(
    NativeLobbyMatch* match, const NativeSignalingPresence* presence,
    const char* required_build)
{
    if (match == NULL || presence == NULL || required_build == NULL ||
        required_build[0] == '\0')
    {
        return false;
    }

    const NativeSignalingParticipant*
        active[NATIVE_LOBBY_MATCH_MAX_PEERS] = { 0 };
    uint8_t active_count = 0;
    uint8_t local_player_count = 0;
    for (uint32_t i = 0; i < presence->participant_count; ++i) {
        const NativeSignalingParticipant* participant =
            &presence->participants[i];
        if (participant->active_mask == 0) {
            continue;
        }
        if (active_count >= NATIVE_LOBBY_MATCH_MAX_PEERS ||
            !valid_participant(participant, required_build))
        {
            return false;
        }
        uint8_t active_players = 0;
        for (uint32_t seat = 0; seat < participant->local_players; ++seat) {
            active_players += (participant->active_mask >> seat) & 1U;
        }
        if (local_player_count > 4 - active_players)
        {
            return false;
        }
        for (uint32_t prior = 0; prior < active_count; ++prior) {
            if (strcmp(active[prior]->peer_id, participant->peer_id) == 0 ||
                active[prior]->join_order == participant->join_order)
            {
                return false;
            }
        }
        uint32_t insertion = active_count;
        while (insertion != 0 &&
               active[insertion - 1]->join_order > participant->join_order)
        {
            active[insertion] = active[insertion - 1];
            --insertion;
        }
        active[insertion] = participant;
        ++active_count;
        local_player_count += active_players;
    }
    if (active_count == 0) {
        return false;
    }

    uint8_t human_ports = 0;
    uint8_t human_count = 0;
    for (uint32_t port = 0; port < 4; ++port) {
        if (match->slots[port].kind == NativeLobbyMatchSlot_Human) {
            human_ports |= 1U << port;
            ++human_count;
        }
    }
    if (human_count != local_player_count) {
        return false;
    }

    NativeLobbyMatch candidate = *match;
    memset(candidate.peers, 0, sizeof(candidate.peers));
    candidate.peer_count = active_count;
    uint8_t remaining_humans = human_ports;
    for (uint32_t peer_index = 0; peer_index < active_count; ++peer_index) {
        const NativeSignalingParticipant* participant = active[peer_index];
        NativeLobbyMatchPeer* peer = &candidate.peers[peer_index];
        memcpy(peer->peer_id, participant->peer_id,
               strlen(participant->peer_id) + 1);
        peer->numeric_peer = (uint8_t) peer_index;
        for (uint32_t local = 0; local < participant->local_players; ++local) {
            if ((participant->active_mask & (1U << local)) == 0) {
                continue;
            }
            uint8_t port_bit = 1;
            while (port_bit != 0 && (remaining_humans & port_bit) == 0) {
                port_bit <<= 1;
            }
            if (port_bit == 0) {
                return false;
            }
            peer->port_mask |= port_bit;
            uint32_t port = 0;
            while ((port_bit & (1U << port)) == 0) {
                ++port;
            }
            candidate.slots[port].local_seat = (uint8_t) local;
            remaining_humans &= (uint8_t) ~port_bit;
        }
    }
    if (remaining_humans != 0) {
        return false;
    }
    candidate.peers[0].port_mask |= (uint8_t) (~human_ports & 0xF);
    if (!nativeLobbyMatch_Validate(&candidate)) {
        return false;
    }
    *match = candidate;
    return true;
}

#endif
