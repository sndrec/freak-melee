#include "native_online_session.h"

#ifdef MELEE_NATIVE

#define _CRT_RAND_S
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "native_match_launch.h"
#include "native_services.h"
#include "native_stock_content.h"
#include "native_home.h"
#include "gm_1601.h"
#include "gmresult.h"
#include "types.h"

static struct NativeOnlineProcessSession {
    NativeLobbySession lobby;
    char room_code[NATIVE_SIGNALING_ROOM_ID_BYTES + 1];
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    char local_name[NATIVE_SIGNALING_MAX_NAME_BYTES + 1];
    uint8_t local_selection[NATIVE_LOBBY_SELECTION_MAX_WIRE_BYTES];
    uint16_t local_selection_size;
    uint8_t local_players;
    bool active;
    bool launch_ready;
    bool match_claimed;
    bool watch_claimed;
} online;

static struct NativeQuickSession {
    NativeQuickAssignment assignment;
    char party_room[NATIVE_SIGNALING_ROOM_ID_BYTES + 1];
    uint64_t heartbeat_ms;
    uint64_t countdown_end_ms, prepared_revision;
    uint32_t seen_revision;
    bool configure_sent, assigned_room, cancelling, cancel_sent;
    bool prepared_sent, prepared_value;
    bool unready_pending, unready_accepted;
    uint64_t unready_revision, unready_epoch;
} quick;

static const char online_build[] = "freak-melee-native-dev-quick-return";
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
                       uint8_t local_players, const char* existing_peer,
                       const char* admission)
{
    if (online.active || !valid_room_code(room_code) || local_players == 0 ||
        local_players > 4)
    {
        return false;
    }
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    if (existing_peer) {
        snprintf(peer_id, sizeof(peer_id), "%s", existing_peer);
    } else if (!generate_peer_id(peer_id)) {
        return false;
    }
    if (local_name == NULL || local_name[0] == '\0') {
        local_name = "Native Player";
    }
    NativeNetplayConnectionConfig config = {
        .signaling = {
            .endpoint = getenv("FREAK_MELEE_SERVICE_ENDPOINT"),
            .room_id = room_code,
            .peer_id = peer_id,
            .name = local_name,
            .build = online_build,
            .admission = admission,
            .local_players = local_players,
        },
    };
    if (!config.signaling.endpoint || !config.signaling.endpoint[0])
        config.signaling.endpoint = NATIVE_SERVICE_ENDPOINT;
    memset(&online, 0, sizeof(online));
    memcpy(online.room_code, room_code, strlen(room_code) + 1);
    memcpy(online.peer_id, peer_id, strlen(peer_id) + 1);
    snprintf(online.local_name, sizeof(online.local_name), "%s", local_name);
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
           start_room(room_code, local_name, local_players, NULL, NULL);
}

bool nativeOnlineSession_JoinRoom(const char* room_code,
                                  const char* local_name,
                                  uint8_t local_players)
{
    return start_room(room_code, local_name, local_players, NULL, NULL);
}

void nativeOnlineSession_Stop(void)
{
    if (online.active) {
        nativeLobbySession_Shutdown(&online.lobby);
    }
    memset(&online, 0, sizeof(online));
    memset(&quick, 0, sizeof(quick));
}

bool nativeOnlineSession_EnterQuick(uint8_t mode, const char* local_name,
                                    uint8_t local_players)
{
    if (mode < NativeQuickMode_Singles || mode > NativeQuickMode_Doubles ||
        local_players == 0 || local_players > mode || online.match_claimed)
        return false;
    nativeOnlineSession_Stop();
    if (!nativeOnlineSession_CreateRoom(local_name, local_players)) return false;
    quick.assignment.mode = mode;
    memcpy(quick.party_room, online.room_code, sizeof(quick.party_room));
    return true;
}

static bool switch_quick_room(const char* room, const char* admission)
{
    struct NativeQuickSession retained = quick;
    char code[13], token[33], peer[65], name[33];
    snprintf(code, sizeof(code), "%s", room);
    snprintf(token, sizeof(token), "%s", admission ? admission : "");
    memcpy(peer, online.peer_id, sizeof(peer));
    memcpy(name, online.local_name, sizeof(name));
    uint8_t players = online.local_players;
    nativeOnlineSession_Stop();
    bool started = start_room(code, name, players, peer, token);
    quick = retained;
    quick.seen_revision = 0;
    quick.prepared_sent = false;
    quick.cancel_sent = false;
    quick.unready_pending = quick.unready_accepted = false;
    return started;
}

bool nativeOnlineSession_SearchQuick(bool searching)
{
    if (!online.active || !quick.assignment.mode || online.match_claimed)
        return false;
    if (!searching && quick.assigned_room) {
        quick.cancelling = true;
        quick.assigned_room = false;
        return switch_quick_room(quick.party_room, NULL);
    }
    if (!nativeOnlineSession_IsOwner() || quick.cancelling ||
        quick.assignment.state == NativeQuickState_Assigned ||
        online.lobby.room.connection.quick.mode != quick.assignment.mode)
        return false;
    if (!nativeSignalingClient_QueueQuick(&online.lobby.room.connection.signaling,
                                         searching)) return false;
    quick.cancelling = !searching;
    quick.cancel_sent = !searching;
    quick.heartbeat_ms = GetTickCount64();
    return true;
}

const NativeQuickAssignment* nativeOnlineSession_Quick(void)
{
    return quick.assignment.mode ? &quick.assignment : NULL;
}

bool nativeOnlineSession_PickQuickStage(uint8_t local_seat, uint8_t stage)
{
    return online.active && !online.match_claimed && quick.assigned_room &&
        online.lobby.state == NativeLobbySessionState_Waiting &&
        quick.assignment.phase == 3 && quick.assignment.chooser == quick.assignment.team &&
        local_seat < online.local_players && (stage < 6 || stage == 255) &&
        nativeSignalingClient_VoteQuickStage(&online.lobby.room.connection.signaling,
            quick.assignment.completed_epoch, local_seat, stage);
}

const char* nativeOnlineSession_QuickError(void)
{
    return online.lobby.room.connection.quick_error;
}

uint16_t nativeOnlineSession_QuickCountdownMs(void)
{
    uint64_t now = GetTickCount64();
    return quick.assignment.phase == 1 && quick.countdown_end_ms > now
        ? (uint16_t) (quick.countdown_end_ms - now) : 0;
}

bool nativeOnlineSession_UnreadyQuick(void)
{
    if (!online.active || online.match_claimed || !quick.assigned_room ||
        quick.assignment.phase >= 2 ||
        online.lobby.state != NativeLobbySessionState_Waiting) return false;
    if (quick.unready_pending || quick.unready_accepted) return true;
    if (!nativeSignalingClient_UnreadyQuick(&online.lobby.room.connection.signaling,
                                            quick.assignment.completed_epoch)) return false;
    quick.unready_pending = true;
    quick.unready_revision = online.lobby.room.connection.presence.revision;
    quick.unready_epoch = quick.assignment.completed_epoch;
    return true;
}

bool nativeOnlineSession_TakeQuickUnready(void)
{
    bool accepted = quick.unready_accepted;
    quick.unready_accepted = false;
    return accepted;
}

static void prepare_quick(void)
{
    NativeNetplayConnection* c = &online.lobby.room.connection;
    NativeLobbySelection local;
    bool prepared = nativeLobbySelection_Decode(&local, online.local_selection,
                                                online.local_selection_size) && local.confirmed &&
        c->connected_count == c->remote_count &&
        online.lobby.room.cache.presence_revision == c->presence.revision;
    unsigned seats = 0;
    for (unsigned i = 0; i < c->presence.participant_count; ++i) {
        const NativeSignalingParticipant* p = &c->presence.participants[i];
        NativeLobbySelection selected;
        seats += p->local_players;
        prepared &= p->active_mask == (1U << p->local_players) - 1 && !p->waiting_mask &&
            !strcmp(p->build, c->local_build) &&
            nativeLobbySelection_Decode(&selected, p->selection, p->selection_size) &&
            selected.player_count == p->local_players && selected.confirmed;
    }
    prepared &= seats == quick.assignment.mode * 2 &&
        c->remote_count + 1 == c->presence.participant_count;
    for (unsigned i = 0; i < online.lobby.room.cache.choice_count; ++i)
        prepared &= online.lobby.room.cache.choices[i].cached;
    if ((!quick.prepared_sent || quick.prepared_revision != c->presence.revision ||
         quick.prepared_value != prepared) &&
        nativeSignalingClient_PrepareQuick(&c->signaling, c->presence.revision,
                                           prepared, nativeStockContent_Digest())) {
        quick.prepared_revision = c->presence.revision;
        quick.prepared_value = prepared;
        quick.prepared_sent = true;
    }
}

static bool pump_quick(void)
{
    NativeNetplayConnection* connection = &online.lobby.room.connection;
    if (connection->quick_revision != quick.seen_revision) {
        quick.seen_revision = connection->quick_revision;
        if (connection->quick.mode) {
            if (!quick.assignment.mode) {
                // A second PC joining a doubles party learns its mode here.
                memcpy(quick.party_room, online.room_code, sizeof(quick.party_room));
            }
            quick.assignment = connection->quick;
            quick.countdown_end_ms = GetTickCount64() + quick.assignment.remaining_ms;
        }
        if (quick.cancelling && connection->quick.state == NativeQuickState_Idle)
            quick.cancelling = quick.cancel_sent = false;
    }
    if (!quick.assignment.mode || !connection->has_presence) return true;
    if (quick.unready_pending) {
        if (quick.assignment.phase >= 2 ||
            quick.assignment.completed_epoch != quick.unready_epoch) {
            quick.unready_pending = false;
        } else if (connection->presence.revision > quick.unready_revision) {
            for (unsigned i = 0; i < connection->presence.participant_count; ++i) {
                const NativeSignalingParticipant* peer = &connection->presence.participants[i];
                NativeLobbySelection selected;
                if (!strcmp(peer->peer_id, online.peer_id) &&
                    nativeLobbySelection_Decode(&selected, peer->selection, peer->selection_size) &&
                    !selected.confirmed) {
                    quick.unready_pending = false;
                    quick.unready_accepted = true;
                    break;
                }
            }
        }
    }
    if (quick.assigned_room && quick.assignment.state == NativeQuickState_Aborted) {
        quick.cancelling = true;
        quick.assigned_room = false;
        return switch_quick_room(quick.party_room, NULL);
    }
    if (quick.cancelling) {
        if (!quick.cancel_sent && nativeOnlineSession_IsOwner()) {
            quick.cancel_sent = nativeSignalingClient_QueueQuick(&connection->signaling, false);
        }
        return true;
    }
    if (quick.assignment.state == NativeQuickState_Assigned &&
        strcmp(quick.assignment.match_room, online.room_code)) {
        quick.assigned_room = true;
        return switch_quick_room(quick.assignment.match_room, quick.assignment.admission);
    }
    if (!quick.assigned_room && !connection->quick.mode &&
        !quick.configure_sent && nativeOnlineSession_IsOwner()) {
        quick.configure_sent = nativeSignalingClient_ConfigureQuick(
            &connection->signaling, quick.assignment.mode);
    }
    uint64_t now = GetTickCount64();
    if (!quick.assigned_room && quick.assignment.state == NativeQuickState_Searching &&
        nativeOnlineSession_IsOwner() && now - quick.heartbeat_ms >= 10000 &&
        nativeSignalingClient_QueueQuick(&connection->signaling, true)) {
        quick.heartbeat_ms = now;
    }
    if (quick.assigned_room && quick.assignment.phase < 2 &&
        !quick.unready_pending && !quick.unready_accepted &&
        online.lobby.state == NativeLobbySessionState_Waiting) prepare_quick();
    return true;
}

bool nativeOnlineSession_Pump(void)
{
    if (!online.active || online.match_claimed) return false;
    if (online.watch_claimed)
        return nativeNetplayRoom_PumpLobby(&online.lobby.room);
    if (quick.assigned_room &&
        !nativeSignalingClient_IsRunning(&online.lobby.room.connection.signaling)) {
        // A borrowed room remains intact through gameplay, even after signaling
        // stops. Once released, reconnect to the source party instead of waiting
        // for a result acknowledgement that this closed socket cannot receive.
        quick.cancelling = true;
        quick.assigned_room = false;
        return switch_quick_room(quick.party_room, NULL) ||
            fail_launch("could not return to the Quick Play party");
    }
    bool pumped = nativeLobbySession_Pump(&online.lobby);
    if (!pumped && online.lobby.room.connection.quick.state != NativeQuickState_Aborted) return false;
    if (!pump_quick()) return fail_launch("could not connect to the Quick Play match");
    if (!pumped) return online.lobby.state != NativeLobbySessionState_Failed;
    if (online.lobby.state == NativeLobbySessionState_Ready &&
        !online.launch_ready && (!quick.assignment.mode || !nativeHome_IsPractice()))
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

const NativeLobbyMatch* nativeOnlineSession_Match(void)
{
    return online.active && online.lobby.room.match_configured ? &online.lobby.room.match : NULL;
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
    // Retain the locked local choice until the service decides the cancellation.
    if (quick.unready_pending) return true;
    NativeLobbySelection normalized;
    if (selection && quick.assigned_room) {
        normalized = *selection;
        for (unsigned i = 0; i < normalized.player_count; ++i) {
            NativeLobbyPlayerSelection* p = &normalized.players[i];
            if (!p->selected) continue;
            p->team = quick.assignment.team;
            uint8_t character;
            if (quick.assignment.mode == NativeQuickMode_Doubles &&
                nativeStockContent_CharacterFromId(p->character_id, &character)) {
                uint8_t color = p->team == 0 ? gm_80169264(character) : gm_801692BC(character);
                p->costume_id = nativeStockContent_CostumeId(character, color);
            }
        }
        selection = &normalized;
    }
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
    if (quick.assignment.phase == 1 && !selection->confirmed)
        nativeSignalingClient_PrepareQuick(&online.lobby.room.connection.signaling,
            online.lobby.room.connection.presence.revision, false, NULL);
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

void nativeOnlineSession_Inspect(void)
{
    const NativeSignalingPresence* p = nativeOnlineSession_Presence();
    fprintf(stderr, "[room-state] state=%u local=%s owner=%s revision=%llu epoch=%llu match=%u rotation=%u members=%u\n",
            online.lobby.state, online.peer_id, p ? p->owner_peer_id : "",
            p ? (unsigned long long)p->revision : 0,
            p ? (unsigned long long)p->match_epoch : 0,
            p ? p->match_size : 0, p ? p->rotation_policy : 0,
            p ? p->participant_count : 0);
    for (unsigned i = 0; p && i < p->participant_count; ++i) {
        const NativeSignalingParticipant* member = &p->participants[i];
        NativeLobbySelection selection = {0};
        bool decoded = nativeLobbySelection_Decode(&selection,
            member->selection, member->selection_size);
        fprintf(stderr, "[room-member] index=%u peer=%s seats=%u active=%u waiting=%u ranks=%u,%u,%u,%u confirmed=%u name=%s\n",
                i, member->peer_id, member->local_players, member->active_mask,
                member->waiting_mask, member->waiting_ranks[0], member->waiting_ranks[1],
                member->waiting_ranks[2], member->waiting_ranks[3],
                decoded && selection.confirmed, member->name);
    }
    const NativeMatchStream* stream = &online.lobby.room.stream;
    NativeMatchStreamSample tail = {0};
    if (stream->count) nativeMatchStream_Read(stream, stream->count-1, &tail);
    fprintf(stderr, "[room-stream] session=%016llx records=%u phase=%u next=%u finished=%u unavailable=%u seed=%08x digest=%016llx tail_kind=%u tail_frame=%u tail_state=%016llx\n",
            (unsigned long long)stream->description.match.session_id, stream->count,
            stream->phase, stream->next_frame, stream->finished, stream->unavailable,
            stream->description.initial_seed, (unsigned long long)stream->digest,
            tail.kind, tail.frame, (unsigned long long)tail.state_digest);
    const NativeRoomWatchReceive* watch=&online.lobby.room.watch.receive;
    fprintf(stderr,"[room-watch] state=%u session=%016llx records=%u available=%u metadata=%u finished=%u digest=%016llx seed=%08x senders=%u practice=%u failure=%s\n",
            watch->state,(unsigned long long)watch->session,watch->stream.count,
            watch->remote_count,watch->metadata_mask,watch->stream.finished,
            (unsigned long long)watch->stream.digest,watch->stream.description.initial_seed,
            online.lobby.room.watch.sender_count,nativeHome_IsPractice(),watch->failure);
    const NativeRoomCache* cache=&online.lobby.room.cache;
    unsigned cached=0;
    for(unsigned i=0;i<cache->choice_count;++i)cached+=cache->choices[i].cached;
    fprintf(stderr,"[room-cache] retained=%u bytes=%zu reads=%u requests=%u active_packets=%llu choices=%u cached=%u watch_ready=%u\n",
            cache->retained_count,cache->retained_bytes,cache->file_reads,cache->active_requests,
            (unsigned long long)cache->active_packets,cache->choice_count,cached,
            nativeOnlineSession_WatchContentReady());
}

bool nativeOnlineSession_WatchMatch(void)
{
    return online.active && !online.match_claimed && !online.watch_claimed && !online.launch_ready &&
        nativeNetplayRoom_WatchMatch(&online.lobby.room);
}
void nativeOnlineSession_StopWatching(void)
{
    if(online.active && !online.match_claimed && !online.watch_claimed)
        nativeNetplayRoom_StopWatching(&online.lobby.room);
}
const NativeRoomWatchReceive* nativeOnlineSession_ClaimWatch(void)
{
    if (!online.active || online.match_claimed || online.watch_claimed ||
        online.launch_ready || !nativeOnlineSession_WatchContentReady()) return NULL;
    const NativeRoomWatchReceive* watch = &online.lobby.room.watch.receive;
    if (watch->state != NativeRoomWatch_Receiving &&
        watch->state != NativeRoomWatch_Complete) return NULL;
    online.watch_claimed = true;
    return watch;
}
void nativeOnlineSession_ReleaseWatch(void)
{
    online.watch_claimed = false;
    nativeOnlineSession_StopWatching();
}
const NativeRoomWatchReceive* nativeOnlineSession_Watch(void)
{
    return online.active ? &online.lobby.room.watch.receive : NULL;
}
bool nativeOnlineSession_WatchContentReady(void)
{
    return online.active && nativeNetplayRoom_WatchContentReady(&online.lobby.room);
}

uint8_t nativeOnlineSession_LocalActiveSeats(void)
{
    const NativeSignalingPresence* presence=nativeOnlineSession_Presence();
    for(unsigned i=0;presence && i<presence->participant_count;++i)
        if(!strcmp(presence->participants[i].peer_id,online.peer_id))
            return presence->participants[i].active_mask;
    return 0;
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
    return online.active && !quick.assignment.mode && nativeOnlineSession_IsOwner() &&
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
    if (quick.assigned_room) {
        uint8_t winner = 255;
        if (result && !gm_WasMatchCanceled(result->outcome)) {
            if (match->teams && result->n_team_winners == 1 && result->team_winners[0] < 2)
                winner = result->team_winners[0];
            else if (!match->teams && result->n_winners == 1 && result->winners[0] < 4)
                winner = match->slots[result->winners[0]].team;
        }
        bool queued = nativeSignalingClient_ReportQuick(&online.lobby.room.connection.signaling,
            online.lobby.room.agreed_match_epoch, winner);
        fprintf(stderr, "[quick-play] result epoch=%llu winner=%u queued=%u\n",
            (unsigned long long) online.lobby.room.agreed_match_epoch, winner, queued);
        return queued;
    }
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
    if (!quick.assigned_room ||
        nativeSignalingClient_IsRunning(&online.lobby.room.connection.signaling))
        nativeLobbySession_FinishMatch(&online.lobby);
    online.launch_ready = false;
    online.match_claimed = false;
    online.local_selection_size = 0;
    quick.prepared_sent = false;
    quick.countdown_end_ms = 0;
}

#endif
