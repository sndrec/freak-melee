#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include <melee/gm/native_lobby_session.h>
#include "native_probe_identity.h"

enum {
    PROBE_TIMEOUT_MS = 10000,
};

static NativeLobbySession first;
static NativeLobbySession second;
static NativeLobbySession spectator;
static bool spectator_started;

static bool begin_direct_match(NativeLobbySession* session,
                               const NativeLobbyMatch* selection,
                               uint8_t artificial_delay_frames)
{
    if (session == NULL || selection == NULL || session->match_source != NativeLobbyMatchSource_Direct ||
        session->state != NativeLobbySessionState_Waiting ||
        session->room.match_configured ||
        !session->room.connection.has_presence)
    {
        return false;
    }
    const NativeSignalingPresence* presence =
        &session->room.connection.presence;
    if (strcmp(presence->owner_peer_id,
               session->room.connection.local_peer_id) != 0 ||
        presence->match_size != 0)
    {
        return false;
    }
    NativeLobbyMatch match = *selection;
    if (!nativeLobbyTopology_AssignMatchPeers(
            &match, presence, session->room.connection.local_build))
    {
        return false;
    }
    bool local_in_match = false;
    for (uint32_t peer = 0; peer < match.peer_count; ++peer) {
        local_in_match |= strcmp(match.peers[peer].peer_id,
                                 session->room.connection.local_peer_id) == 0;
    }
    if (!local_in_match) {
        // The lobby owner can organize a match while continuing local practice.
        // Active peers adopt this proposal through the ordinary lobby pump.
        uint8_t bytes[NATIVE_LOBBY_MATCH_MAX_WIRE_BYTES];
        size_t size = nativeLobbyMatch_Encode(&match, bytes, sizeof(bytes));
        if (session->room.publish_request_epoch == presence->match_epoch || !size ||
            !nativeSignalingClient_SetMatch(&session->room.connection.signaling,
                                             bytes, size)) return false;
        session->room.publish_request_epoch = presence->match_epoch;
        return true;
    }
    if (!nativeNetplayRoom_ConfigureMatch(&session->room, &match,
                                          artificial_delay_frames)) {
        return false;
    }
    session->state = NativeLobbySessionState_Starting;
    return true;
}

static NativeLobbyMatch create_match(uint64_t session_id)
{
    NativeLobbyMatch match = {
        .slots = {
            { .character_id = 1, .costume_id = 1,
              .kind = NativeLobbyMatchSlot_Human, .handicap = 9 },
            { .character_id = 2, .costume_id = 1,
              .kind = NativeLobbyMatchSlot_Human, .handicap = 9 },
            { .character_id = 3, .costume_id = 1,
              .kind = NativeLobbyMatchSlot_Cpu, .cpu_level = 9,
              .handicap = 9 },
            { .character_id = 4, .costume_id = 1,
              .kind = NativeLobbyMatchSlot_Cpu, .cpu_level = 9,
              .handicap = 9 },
        },
        .session_id = session_id,
        .stage_id = 1,
        .mode = NativeLobbyMatchMode_Stock,
        .time_limit_minutes = 2,
        .stock_count = 1,
        .item_frequency = -1,
        .damage_ratio = 10,
        .pause = true,
    };
    match.content_digest[0] = 1;
    NativeSignalingPresence presence = {
        .participant_count = 3,
        .participants = {
            {
                .peer_id = "peer_two",
                .build = "native-room-probe",
                .join_order = 2,
                .local_players = 1,
                .active_mask = 1,
            },
            {
                .peer_id = "probe_spectator",
                .build = "native-room-probe",
                .join_order = 3,
            },
            {
                .peer_id = "peer_one",
                .build = "native-room-probe",
                .join_order = 1,
                .local_players = 1,
                .active_mask = 1,
            },
        },
    };
    if (!nativeLobbyTopology_AssignMatchPeers(&match, &presence,
                                              "native-room-probe"))
    {
        fprintf(stderr, "could not derive probe match topology\n");
        exit(1);
    }
    return match;
}

static bool pump_setup(void)
{
    uint64_t deadline = GetTickCount64() + PROBE_TIMEOUT_MS;
    while (GetTickCount64() < deadline) {
        if (!nativeLobbySession_Pump(&first) ||
            !nativeLobbySession_Pump(&second) ||
            (spectator_started && !nativeLobbySession_Pump(&spectator)))
        {
            return false;
        }
        if (first.state == NativeLobbySessionState_Ready &&
            second.state == NativeLobbySessionState_Ready)
        {
            return true;
        }
        Sleep(5);
    }
    fprintf(stderr,
            "setup timeout: first remote=%u connected=%u ready=%u failed=%u; "
            "second remote=%u connected=%u ready=%u failed=%u\n",
            first.room.connection.remote_count,
            first.room.connection.connected_count, first.room.ready,
            first.room.connection.failed, second.room.connection.remote_count,
            second.room.connection.connected_count, second.room.ready,
            second.room.connection.failed);
    return false;
}

static bool wait_for_owner(NativeLobbySession* session, const char* peer_id)
{
    uint64_t deadline = GetTickCount64() + PROBE_TIMEOUT_MS;
    while (GetTickCount64() < deadline) {
        if (!nativeLobbySession_Pump(session)) {
            return false;
        }
        const NativeSignalingPresence* presence =
            nativeLobbySession_Presence(session);
        if (presence != NULL &&
            strcmp(presence->owner_peer_id, peer_id) == 0)
        {
            return true;
        }
        Sleep(5);
    }
    return false;
}

static NativeNetplayConnectionPeer* route_to(NativeNetplayConnection* from, const char* peer_id)
{
    for (unsigned i = 0; i < NATIVE_NETPLAY_CONNECTION_MAX_REMOTE_PEERS; ++i) {
        NativeNetplayConnectionPeer* route = &from->peers[i];
        if (route->active && !strcmp(route->peer_id, peer_id)) return route;
    }
    return NULL;
}

static bool check_three_player_recovery(void)
{
    NativeNetplayConnection* rooms[] = {
        &first.room.connection, &second.room.connection, &spectator.room.connection
    };
    uint64_t deadline = GetTickCount64() + PROBE_TIMEOUT_MS;
    for (;;) {
        bool ready = true;
        for (unsigned i = 0; i < 3; ++i) {
            if (!nativeNetplayConnection_Pump(rooms[i])) return false;
            ready &= rooms[i]->connected_count == 2;
        }
        if (ready) break;
        if (GetTickCount64() >= deadline) return false;
        Sleep(5);
    }
    NativeNetplayConnectionPeer* interrupted = route_to(rooms[0], "peer_two");
    NativeNetplayConnectionPeer* healthy = route_to(rooms[2], "peer_two");
    if (!interrupted || !healthy) return false;
    const uintptr_t original_agent = healthy->ice.agent;
    NativeRouteGeneration original = interrupted->local;
    for (unsigned i = 0; i < 3; ++i) {
        if (!nativeNetplayConnection_LockTopology(rooms[i])) return false;
    }
    nativeNetplayIce_Shutdown(&interrupted->ice);
    uint8_t marker = 37;
    if (nativeNetplayConnection_Send(rooms[0], "peer_two", &marker, 1) != NativeNetplayIceSend_Retry ||
        nativeNetplayConnection_Send(rooms[2], "peer_two", &marker, 1) != NativeNetplayIceSend_Sent)
        return false;
    bool other_delivered = false, repaired_delivered = false;
    deadline = GetTickCount64() + PROBE_TIMEOUT_MS;
    while (GetTickCount64() < deadline) {
        for (unsigned i = 0; i < 3; ++i)
            if (!nativeNetplayConnection_Pump(rooms[i])) return false;
        if (!nativeNetplayGameRoute_Ready(healthy) || healthy->ice.agent != original_agent)
            return false;
        if (nativeNetplayGameRoute_Ready(interrupted))
            nativeNetplayConnection_Send(rooms[0], "peer_two", &marker, 1);
        if (!other_delivered)
            nativeNetplayConnection_Send(rooms[2], "peer_two", &marker, 1);
        NativeNetplayConnectionDatagram packet;
        while (nativeNetplayConnection_PollDatagram(rooms[1], &packet)) {
            if (packet.datagram.size != 1 || packet.datagram.bytes[0] != marker) return false;
            if (!strcmp(packet.peer_id, "probe_spectator")) other_delivered = true;
            if (!strcmp(packet.peer_id, "peer_one")) repaired_delivered = true;
        }
        if (other_delivered && repaired_delivered) break;
        Sleep(5);
    }
    if (!other_delivered || !repaired_delivered ||
        (interrupted->local.revision == original.revision &&
         interrupted->local.attempt == original.attempt)) return false;
    for (unsigned i = 0; i < 3; ++i)
        if (!nativeNetplayConnection_UnlockTopology(rooms[i])) return false;
    puts("Third player retained its ICE route and sent data while the other pair reconnected.");
    return true;
}

static bool pump_packets(NativeRollbackInputHistory* first_inputs,
                         NativeRollbackInputHistory* second_inputs)
{
    uint64_t deadline = GetTickCount64() + PROBE_TIMEOUT_MS;
    uint64_t next_send = 0;
    while (GetTickCount64() < deadline) {
        if (!nativeNetplayRoom_Receive(&first.room, first_inputs) ||
            !nativeNetplayRoom_Receive(&second.room, second_inputs))
        {
            return false;
        }
        if (first.room.exchange.received_packets != 0 &&
            second.room.exchange.received_packets != 0)
        {
            return true;
        }
        uint64_t now = GetTickCount64();
        if (now >= next_send) {
            if (!nativeNetplayRoom_Send(&first.room) || !nativeNetplayRoom_Send(&second.room))
                return false;
            next_send = now + 16;
        }
        Sleep(1);
    }
    fprintf(stderr,
            "packet timeout: first accepted=%u rejected=%u; second "
            "accepted=%u rejected=%u\n",
            first.room.exchange.received_packets,
            first.room.exchange.rejected_packets,
            second.room.exchange.received_packets,
            second.room.exchange.rejected_packets);
    return false;
}

int main(int argc, char** argv)
{
    if (argc == 2 && !strcmp(argv[1], "--check-signal-handling")) {
        NativeNetplayConnection* room = &first.room.connection;
        NativeSignalingEventQueue* events = &room->signaling.events;
        room->active = true;
        events->slots[0].type = NativeSignalingClientEvent_Connected;
        events->slots[1].type = NativeSignalingClientEvent_Message;
        events->slots[1].message.type = NativeSignalingServerMessage_Signal;
        // An empty peer envelope is discarded and the next room event still arrives.
        events->slots[2].type = NativeSignalingClientEvent_Message;
        events->slots[2].message.type = NativeSignalingServerMessage_RoomActionRejected;
        strcpy(events->slots[2].message.data.error, "Stage selection has ended");
        atomic_store(&events->write, 3);
        if (!nativeNetplayConnection_Pump(room) || !room->signaling_connected ||
            strcmp(room->action_rejection, "Stage selection has ended")) return 1;
        events->slots[3].type = NativeSignalingClientEvent_Disconnected;
        events->slots[3].terminal = true;
        atomic_store(&events->write, 4);
        return nativeNetplayConnection_Pump(room) || !room->failed ? 1 : 0;
    }
    char guest_keys[3][65];
    for (unsigned i = 0; i < 3; ++i) if (!nativeProbeGuestKey(guest_keys[i])) return 2;
    const bool inactive_owner = argc == 3 && !strcmp(argv[2], "--inactive-owner");
    if (argc != 2 && !inactive_owner) {
        fprintf(stderr, "usage: %s <http-or-https-signaling-origin> [--inactive-owner]\n",
                argv[0]);
        return 2;
    }
    char room_id[10];
    snprintf(room_id, sizeof(room_id), "C%08lX",
             (unsigned long) GetCurrentProcessId());
    const uint64_t session_id = UINT64_C(0x123456789ABCDEF0);
    NativeLobbyMatch match = create_match(session_id);
    NativeNetplayConnectionConfig first_config = {
        .signaling = {
            .endpoint = argv[1],
            .room_id = room_id,
            .peer_id = "peer_one",
            .guest_key = guest_keys[0],
            .name = "Alice Local",
            .build = "native-room-probe",
            .local_players = 1,
        },
    };
    NativeNetplayConnectionConfig second_config = {
        .signaling = {
            .endpoint = argv[1],
            .room_id = room_id,
            .peer_id = "peer_two",
            .guest_key = guest_keys[1],
            .name = "Bob Local",
            .build = "native-room-probe",
            .local_players = 1,
        },
    };
    bool owner_ready = true;
    if (inactive_owner) {
        NativeNetplayConnectionConfig config = first_config;
        config.signaling.peer_id = "probe_spectator";
        config.signaling.guest_key = guest_keys[2];
        spectator_started = nativeLobbySession_Start(&spectator, &config, NativeLobbyMatchSource_Direct);
        owner_ready = spectator_started && wait_for_owner(&spectator, "probe_spectator");
    }
    bool first_started = owner_ready && nativeLobbySession_Start(&first, &first_config, NativeLobbyMatchSource_Direct);
    bool second_started = first_started &&
                          wait_for_owner(&first, inactive_owner ? "probe_spectator" : "peer_one") &&
                          nativeLobbySession_Start(&second, &second_config, NativeLobbyMatchSource_Direct);
    bool recovery_ok = true;
    if (inactive_owner && second_started) {
        first.room.retain_signaling = second.room.retain_signaling = true;
        const NativeSignalingUpdate update = { .has_active_mask = true, .active_mask = 0 };
        recovery_ok = check_three_player_recovery() && nativeLobbySession_UpdateParticipant(&spectator, &update);
    }
    bool owner_saw_guest = false;
    uint64_t presence_deadline = GetTickCount64() + PROBE_TIMEOUT_MS;
    while (second_started && recovery_ok && GetTickCount64() < presence_deadline) {
        if (!nativeLobbySession_Pump(&first) ||
            !nativeLobbySession_Pump(&second) ||
            (spectator_started && !nativeLobbySession_Pump(&spectator)))
        {
            break;
        }
        const NativeSignalingPresence* presence =
            nativeLobbySession_Presence(&first);
        bool owner_inactive = !inactive_owner;
        if (presence && inactive_owner) {
            for (unsigned i = 0; i < presence->participant_count; ++i)
                if (!strcmp(presence->participants[i].peer_id, "probe_spectator"))
                    owner_inactive = presence->participants[i].active_mask == 0;
        }
        if (presence != NULL && owner_inactive &&
            presence->participant_count == (inactive_owner ? 3 : 2) &&
            (!inactive_owner || spectator.room.connection.presence.participant_count == 3)) {
            owner_saw_guest = true;
            break;
        }
        Sleep(5);
    }
    bool match_started = owner_saw_guest &&
        (!inactive_owner || !begin_direct_match(&first, &match, 0)) &&
        begin_direct_match(inactive_owner ? &spectator : &first, &match, 0);
    if (inactive_owner && match_started)
        match_started = !begin_direct_match(&spectator, &match, 0);
    bool passed = match_started && pump_setup();
    if (passed) {
        uint8_t owner_bytes[NATIVE_LOBBY_MATCH_MAX_WIRE_BYTES];
        uint8_t guest_bytes[NATIVE_LOBBY_MATCH_MAX_WIRE_BYTES];
        size_t owner_size = nativeLobbyMatch_Encode(
            &first.room.match, owner_bytes, sizeof(owner_bytes));
        size_t guest_size = nativeLobbyMatch_Encode(
            &second.room.match, guest_bytes, sizeof(guest_bytes));
        passed = owner_size != 0 && owner_size == guest_size &&
                 memcmp(owner_bytes, guest_bytes, owner_size) == 0 &&
                 first.room.exchange.session_id == session_id &&
                 second.room.exchange.session_id == session_id;
        if (inactive_owner) {
            passed &= spectator.state == NativeLobbySessionState_Waiting &&
                      !spectator.room.match_configured && !spectator.room.ready &&
                      !spectator.room.exchange.session_id && !spectator.room.peer_count &&
                      !strcmp(first.room.match.peers[0].peer_id, "peer_one") &&
                      !strcmp(first.room.match.peers[1].peer_id, "peer_two");
        }
    }

    NativeRollbackInputHistory first_inputs;
    NativeRollbackInputHistory second_inputs;
    passed = passed &&
             nativeRollbackInput_Init(&first_inputs,
                                      first.room.exchange.local_port_mask) &&
             nativeRollbackInput_Init(&second_inputs,
                                      second.room.exchange.local_port_mask);
    PADStatus first_pads[4] = {
        { .button = PAD_BUTTON_A, .stickX = 10, .err = PAD_ERR_NONE },
        { 0 },
        { .button = PAD_BUTTON_X, .stickX = 30, .err = PAD_ERR_NONE },
        { .button = PAD_BUTTON_Y, .stickX = 40, .err = PAD_ERR_NONE },
    };
    PADStatus second_pads[4] = {
        { 0 },
        { .button = PAD_BUTTON_B, .stickX = 20, .err = PAD_ERR_NONE },
        { 0 },
        { 0 },
    };
    if (passed && inactive_owner) {
        NativeNetplayConnectionPeer* a = route_to(&first.room.connection, "peer_two");
        NativeNetplayConnectionPeer* b = route_to(&second.room.connection, "peer_one");
        passed = a && b && first.room.connection.topology_locked && second.room.connection.topology_locked;
        if (passed) {
            nativeNetplayIce_Shutdown(&a->ice);
            nativeNetplayIce_Shutdown(&b->ice);
        }
    }
    if (passed) {
        passed = nativeNetplayExchange_RecordLocal(&first.room.exchange, 0,
                                                   first_pads) &&
                 nativeNetplayExchange_RecordLocal(&second.room.exchange, 0,
                                                   second_pads) &&
                 nativeNetplayRoom_Send(&first.room) &&
                 nativeNetplayRoom_Send(&second.room) &&
                 pump_packets(&first_inputs, &second_inputs);
    }
    PADStatus resolved[4];
    if (passed) {
        passed = nativeRollbackInput_PrepareFrame(&first_inputs, 0, first_pads,
                                                  resolved) &&
                 resolved[0].button == PAD_BUTTON_A &&
                 resolved[1].button == PAD_BUTTON_B &&
                 resolved[2].button == PAD_BUTTON_X &&
                 resolved[3].button == PAD_BUTTON_Y &&
                 nativeRollbackInput_PrepareFrame(&second_inputs, 0,
                                                  second_pads, resolved) &&
                 resolved[0].button == PAD_BUTTON_A &&
                 resolved[1].button == PAD_BUTTON_B &&
                 resolved[2].button == PAD_BUTTON_X &&
                 resolved[3].button == PAD_BUTTON_Y;
    }
    if (passed && inactive_owner) {
        // A fighting peer reports Results; the service clears the proposal even
        // when the owner never watches. Both fighters can return for a rematch.
        passed = nativeSignalingClient_ReportMatch(&first.room.connection.signaling,
                     first.room.agreed_match_epoch, 0, UINT8_MAX) &&
                 nativeLobbySession_FinishMatch(&first) &&
                 nativeLobbySession_FinishMatch(&second);
        bool returned = false;
        uint64_t deadline = GetTickCount64() + PROBE_TIMEOUT_MS;
        while (passed && GetTickCount64() < deadline) {
            passed = nativeLobbySession_Pump(&first) && nativeLobbySession_Pump(&second) &&
                     nativeLobbySession_Pump(&spectator);
            if (passed && first.state == NativeLobbySessionState_Waiting &&
                second.state == NativeLobbySessionState_Waiting &&
                spectator.room.connection.presence.match_size == 0) {
                returned = true;
                break;
            }
            Sleep(5);
        }
        match.session_id++;
        passed = passed && returned &&
                 begin_direct_match(&spectator, &match, 0) && pump_setup() &&
                 first.room.exchange.session_id == match.session_id &&
                 second.room.exchange.session_id == match.session_id &&
                 spectator.state == NativeLobbySessionState_Waiting &&
                 !spectator.room.match_configured;
    }
    if (second_started) {
        nativeLobbySession_Shutdown(&second);
    }
    if (first_started) {
        nativeLobbySession_Shutdown(&first);
    }
    if (spectator_started) nativeLobbySession_Shutdown(&spectator);
    if (!passed) {
        fprintf(stderr, "room-backed rollback exchange probe failed\n");
        return 1;
    }
    puts(inactive_owner ? "spectating owner started two active-peer matches and stayed outside simulation" :
         "room ICE carried validated shared-PC rollback input after signaling closed");
    return 0;
}
