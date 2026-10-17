#include "native_online_session.h"

#ifdef MELEE_NATIVE

#define _CRT_RAND_S
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native_match_launch.h"
#include "native_services.h"
#include "gmresult.h"
#include "types.h"

static struct NativeOnlineProcessSession {
    NativeLobbySession lobby;
    char room_code[NATIVE_SIGNALING_ROOM_ID_BYTES + 1];
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    uint8_t local_selection[NATIVE_LOBBY_SELECTION_MAX_WIRE_BYTES];
    uint16_t local_selection_size;
    uint8_t local_players;
    bool active;
    bool launch_ready;
    bool match_claimed;
} online;

static const char online_build[] = "freak-melee-native-dev-cache";
static const char room_alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";

static bool valid_room_code(const char* room_code)
{
    size_t length = room_code == NULL ? 0 : strlen(room_code);
    if (length < 6 || length > NATIVE_SIGNALING_ROOM_ID_BYTES) {
        return false;
    }
    for (size_t i = 0; i < length; ++i) {
        if (!(room_code[i] >= 'A' && room_code[i] <= 'Z') &&
            !(room_code[i] >= '0' && room_code[i] <= '9'))
        {
            return false;
        }
    }
    return true;
}

static bool random_value(unsigned int* value)
{
    return value != NULL && rand_s(value) == 0;
}

static bool generate_room_code(char room_code[9])
{
    for (uint32_t i = 0; i < 8; ++i) {
        unsigned int value;
        if (!random_value(&value)) {
            return false;
        }
        room_code[i] = room_alphabet[value % (sizeof(room_alphabet) - 1)];
    }
    room_code[8] = '\0';
    return true;
}

static bool generate_peer_id(
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1])
{
    static const char hex[] = "0123456789abcdef";
    peer_id[0] = 'P';
    for (uint32_t i = 0; i < 16; ++i) {
        unsigned int value;
        if (!random_value(&value)) {
            return false;
        }
        peer_id[1 + i * 2] = hex[value & 0xF];
        peer_id[2 + i * 2] = hex[(value >> 4) & 0xF];
    }
    peer_id[33] = '\0';
    return true;
}

static bool fail_launch(const char* reason)
{
    const char* message = reason != NULL ? reason : "could not prepare match";
    snprintf(online.lobby.failure, sizeof(online.lobby.failure), "%s", message);
    online.lobby.state = NativeLobbySessionState_Failed;
    return false;
}

static bool start_room(const char* room_code, const char* local_name,
                       uint8_t local_players)
{
    if (online.active || !valid_room_code(room_code) || local_players == 0 ||
        local_players > 4)
    {
        return false;
    }
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    if (!generate_peer_id(peer_id)) {
        return false;
    }
    if (local_name == NULL || local_name[0] == '\0') {
        local_name = "Native Player";
    }
    NativeNetplayConnectionConfig config = {
        .signaling = {
            .endpoint = NATIVE_SERVICE_ENDPOINT,
            .room_id = room_code,
            .peer_id = peer_id,
            .name = local_name,
            .build = online_build,
            .local_players = local_players,
        },
    };
    memset(&online, 0, sizeof(online));
    memcpy(online.room_code, room_code, strlen(room_code) + 1);
    memcpy(online.peer_id, peer_id, strlen(peer_id) + 1);
    online.local_players = local_players;
    online.active = true;
    if (!nativeLobbySession_Start(&online.lobby, &config)) {
        return false;
    }
    online.lobby.room.artificial_delay_frames =
        nativeMatchLaunch_ArtificialDelayFrames();
    online.lobby.room.content_root = nativeMatchLaunch_ContentRoot();
    online.lobby.room.retain_signaling = true;
    return true;
}

bool nativeOnlineSession_CreateRoom(const char* local_name,
                                    uint8_t local_players)
{
    char room_code[9];
    return generate_room_code(room_code) &&
           start_room(room_code, local_name, local_players);
}

bool nativeOnlineSession_JoinRoom(const char* room_code,
                                  const char* local_name,
                                  uint8_t local_players)
{
    return start_room(room_code, local_name, local_players);
}

void nativeOnlineSession_Stop(void)
{
    if (online.active) {
        nativeLobbySession_Shutdown(&online.lobby);
    }
    memset(&online, 0, sizeof(online));
}

bool nativeOnlineSession_Pump(void)
{
    if (!online.active || online.match_claimed ||
        !nativeLobbySession_Pump(&online.lobby))
    {
        return false;
    }
    if (online.lobby.state == NativeLobbySessionState_Ready &&
        !online.launch_ready)
    {
        if (!nativeMatchLaunch_AdoptLobbyRoom(&online.lobby.room)) {
            return fail_launch("could not apply the agreed room match");
        }
        online.launch_ready = true;
    }
    return true;
}

bool nativeOnlineSession_BeginMatch(const NativeLobbyMatch* selection)
{
    if (!online.active || online.launch_ready || online.match_claimed ||
        selection == NULL || selection->session_id != 0)
    {
        return false;
    }
    unsigned int low;
    unsigned int high;
    if (!random_value(&low) || !random_value(&high)) {
        return false;
    }
    NativeLobbyMatch match = *selection;
    match.session_id = ((uint64_t) high << 32) | low;
    if (match.session_id == 0) {
        match.session_id = 1;
    }
    return nativeLobbySession_BeginMatch(
        &online.lobby, &match, online.lobby.room.artificial_delay_frames);
}

NativeLobbySessionState nativeOnlineSession_State(void)
{
    return online.active ? online.lobby.state
                         : NativeLobbySessionState_Disconnected;
}

uint32_t nativeOnlineSession_CacheRevision(void)
{
    return online.active ? online.lobby.room.cache.revision : 0;
}

NativeRoomChat* nativeOnlineSession_Chat(void)
{
    return online.active ? &online.lobby.room.connection.chat : NULL;
}

const NativeSignalingPresence* nativeOnlineSession_Presence(void)
{
    return online.active ? nativeLobbySession_Presence(&online.lobby) : NULL;
}

const char* nativeOnlineSession_Failure(void)
{
    return online.active ? nativeLobbySession_Failure(&online.lobby) : NULL;
}

const char* nativeOnlineSession_RoomCode(void)
{
    return online.room_code[0] != '\0' ? online.room_code : NULL;
}

const char* nativeOnlineSession_LocalPeerId(void)
{
    return online.active ? online.peer_id : NULL;
}

bool nativeOnlineSession_ContentProgress(NativeNetplayContentProgress* progress)
{
    if (!online.active || !progress) return false;
    if (online.lobby.state == NativeLobbySessionState_Starting)
        return nativeNetplayRoom_ContentProgress(&online.lobby.room, progress);
    const NativeRoomCache* cache = &online.lobby.room.cache;
    if (!cache->serial || !cache->choice_count) return false;
    unsigned remaining = 0;
    for (unsigned i = 0; i < cache->choice_count; ++i) remaining += !cache->choices[i].cached;
    *progress = (NativeNetplayContentProgress) {
        .bytes_done = (uint32_t) cache->receive.received,
        .bytes_total = (uint32_t) cache->receive.size,
        .package_index = cache->choice_count - remaining + 1,
        .package_count = cache->choice_count,
        .direction = NativeNetplayContent_Receiving,
    };
    return true;
}

bool nativeOnlineSession_OfferGift(const char* peer_id,
                                   const NativeContentId* id)
{
    return online.active &&
           online.lobby.state == NativeLobbySessionState_Waiting &&
           nativeNetplayRoom_OfferGift(&online.lobby.room, peer_id, id);
}

bool nativeOnlineSession_AcceptGift(void)
{
    return online.active &&
           online.lobby.state == NativeLobbySessionState_Waiting &&
           nativeNetplayRoom_AcceptGift(&online.lobby.room);
}

void nativeOnlineSession_DeclineGift(void)
{
    if (online.active &&
        online.lobby.state == NativeLobbySessionState_Waiting) {
        nativeNetplayRoom_DeclineGift(&online.lobby.room);
    }
}

bool nativeOnlineSession_GiftStatus(bool incoming,
                                    NativeNetplayGiftStatus* status)
{
    return online.active &&
           nativeNetplayRoom_GiftStatus(&online.lobby.room, incoming,
                                        status);
}

bool nativeOnlineSession_UpdateSelection(
    const NativeLobbySelection* selection)
{
    uint8_t bytes[NATIVE_LOBBY_SELECTION_MAX_WIRE_BYTES];
    size_t size = nativeLobbySelection_Encode(selection, bytes, sizeof(bytes));
    if (!online.active || size == 0) {
        return false;
    }
    if (size == online.local_selection_size &&
        memcmp(bytes, online.local_selection, size) == 0)
    {
        return true;
    }
    NativeSignalingUpdate update = {
        .selection = bytes,
        .selection_size = (uint16_t) size,
        .has_selection = true,
    };
    if (!nativeLobbySession_UpdateParticipant(&online.lobby, &update)) {
        return false;
    }
    memcpy(online.local_selection, bytes, size);
    online.local_selection_size = (uint16_t) size;
    return true;
}

bool nativeOnlineSession_ChooseSeat(const char* peer_id, uint8_t seat,
                                    NativeRoomSeatState state)
{
    const NativeSignalingPresence* presence = nativeOnlineSession_Presence();
    if (peer_id == NULL || presence == NULL ||
        online.lobby.state != NativeLobbySessionState_Waiting ||
        state > NativeRoomSeatState_Spectating ||
        presence->match_size != 0)
    {
        return false;
    }
    const NativeSignalingParticipant* target = NULL;
    uint8_t occupied = 0;
    for (uint32_t i = 0; i < presence->participant_count; ++i) {
        const NativeSignalingParticipant* participant =
            &presence->participants[i];
        if (strcmp(participant->peer_id, peer_id) == 0) {
            target = participant;
        } else {
            for (uint8_t local = 0; local < participant->local_players; ++local) {
                occupied += (participant->active_mask >> local) & 1U;
            }
        }
    }
    if (target == NULL || seat >= target->local_players ||
        (strcmp(peer_id, online.peer_id) != 0 &&
         !nativeOnlineSession_IsOwner()))
    {
        return false;
    }
    uint8_t bit = 1U << seat;
    uint8_t active_mask = target->active_mask & ~bit;
    uint8_t waiting_mask = target->waiting_mask & ~bit;
    if (state == NativeRoomSeatState_Fighting) active_mask |= bit;
    if (state == NativeRoomSeatState_Waiting) waiting_mask |= bit;
    for (uint8_t local = 0; local < target->local_players; ++local) {
        occupied += (active_mask >> local) & 1U;
    }
    if (occupied > 4 ||
        (active_mask == target->active_mask &&
         waiting_mask == target->waiting_mask))
    {
        return false;
    }
    if (strcmp(peer_id, online.peer_id) == 0) {
        NativeSignalingUpdate update = {
            .active_mask = active_mask,
            .waiting_mask = waiting_mask,
            .has_active_mask = true,
            .has_waiting_mask = true,
        };
        return nativeLobbySession_UpdateParticipant(&online.lobby, &update);
    }
    return nativeLobbySession_AssignSeats(&online.lobby, peer_id,
                                          active_mask, waiting_mask);
}

uint8_t nativeOnlineSession_LocalPlayers(void)
{
    return online.active ? online.local_players : 0;
}

bool nativeOnlineSession_IsOwner(void)
{
    const NativeSignalingPresence* presence = nativeOnlineSession_Presence();
    return presence != NULL &&
           strcmp(presence->owner_peer_id, online.peer_id) == 0;
}

bool nativeOnlineSession_SetRotation(NativeRoomRotationPolicy policy)
{
    return online.active && nativeOnlineSession_IsOwner() &&
           online.lobby.state == NativeLobbySessionState_Waiting &&
           nativeSignalingClient_SetRotation(
               &online.lobby.room.connection.signaling, policy);
}

bool nativeOnlineSession_ReportMatch(const MatchEnd* result)
{
    if (!online.active || !online.match_claimed ||
        online.lobby.state != NativeLobbySessionState_Ready ||
        !online.lobby.room.match_configured)
    {
        return false;
    }
    const NativeLobbyMatch* match = &online.lobby.room.match;
    uint8_t human_count = 0;
    bool eligible = result != NULL && !gm_WasMatchCanceled(result->outcome) &&
                    result->n_winners == 1 && !match->teams;
    for (uint8_t port = 0; port < 4; ++port) {
        if (match->slots[port].kind == NativeLobbyMatchSlot_Human) {
            ++human_count;
        } else if (match->slots[port].kind != NativeLobbyMatchSlot_Empty) {
            eligible = false;
        }
    }
    eligible = eligible && human_count == 2;
    const char* winner_peer_id = "";
    uint8_t winner_seat = UINT8_MAX;
    if (eligible) {
        uint8_t winner_port = result->winners[0];
        if (winner_port >= 4 ||
            match->slots[winner_port].kind != NativeLobbyMatchSlot_Human)
        {
            eligible = false;
        } else {
            for (uint8_t peer = 0; peer < match->peer_count; ++peer) {
                if ((match->peers[peer].port_mask & (1U << winner_port)) != 0) {
                    winner_peer_id = match->peers[peer].peer_id;
                    winner_seat = match->slots[winner_port].local_seat;
                    break;
                }
            }
            eligible = winner_peer_id[0] != '\0' && winner_seat < 4;
        }
    }
    if (!eligible) {
        winner_peer_id = "";
        winner_seat = UINT8_MAX;
    }
    return nativeSignalingClient_ReportMatch(
        &online.lobby.room.connection.signaling,
        online.lobby.room.agreed_match_epoch, winner_peer_id,
        winner_seat, eligible);
}

NativeNetplayRoom* nativeOnlineSession_ClaimMatchRoom(void)
{
    if (!online.active || !online.launch_ready || online.match_claimed ||
        online.lobby.state != NativeLobbySessionState_Ready)
    {
        return NULL;
    }
    online.match_claimed = true;
    return &online.lobby.room;
}

void nativeOnlineSession_FinishMatch(void)
{
    if (!online.active) {
        return;
    }
    nativeLobbySession_FinishMatch(&online.lobby);
    online.launch_ready = false;
    online.match_claimed = false;
    online.local_selection_size = 0;
}

#endif
