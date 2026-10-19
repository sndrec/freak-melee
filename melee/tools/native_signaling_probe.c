#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <melee/gm/native_signaling_client.h>

typedef struct ProbePeer {
    NativeSignalingClient client;
    const char* id;
    bool connected;
    bool welcomed;
    bool saw_both;
    bool saw_assignment;
    bool saw_host_override;
    bool saw_self_reassignment;
    bool signal_received;
    bool signal_acknowledged;
    bool saw_rotation;
    bool saw_two_active;
    bool saw_match;
    bool saw_rotated;
    uint64_t match_epoch;
} ProbePeer;

static bool consume(ProbePeer* peer)
{
    NativeSignalingClientEvent event;
    while (nativeSignalingClient_Poll(&peer->client, &event)) {
        if (event.type == NativeSignalingClientEvent_Connected) {
            peer->connected = true;
            printf("%s connected\n", peer->id);
            fflush(stdout);
            continue;
        }
        if (event.type == NativeSignalingClientEvent_Disconnected) {
            fprintf(stderr, "%s disconnected: %s (%lu)\n", peer->id,
                    event.detail, (unsigned long) event.error_code);
            return false;
        }
        switch (event.message.type) {
        case NativeSignalingServerMessage_Welcome:
            printf("%s welcome revision %llu\n", peer->id,
                   (unsigned long long) event.message.data.welcome.revision);
            fflush(stdout);
            peer->welcomed = strcmp(event.message.data.welcome.peer_id,
                                    peer->id) == 0;
            break;
        case NativeSignalingServerMessage_Presence: {
            const NativeSignalingPresence* presence =
                &event.message.data.presence;
            printf("%s presence count %u\n", peer->id,
                   presence->participant_count);
            fflush(stdout);
            peer->saw_both =
                presence->participant_count == 2 &&
                strcmp(presence->owner_peer_id, "peer_one") == 0;
            peer->saw_rotation |=
                presence->rotation_policy ==
                NativeRoomRotation_WinnerStays;
            uint8_t first_active = UINT8_MAX;
            uint8_t first_waiting = UINT8_MAX;
            uint8_t second_active = UINT8_MAX;
            uint8_t second_waiting = UINT8_MAX;
            for (uint32_t i = 0;
                 i < presence->participant_count; ++i)
            {
                const NativeSignalingParticipant* participant =
                    &presence->participants[i];
                if (strcmp(participant->peer_id, "peer_one") == 0) {
                    first_active = participant->active_mask;
                    first_waiting = participant->waiting_mask;
                }
                if (strcmp(participant->peer_id, peer->id) == 0) {
                    peer->saw_assignment |= participant->local_players == 3 &&
                        participant->active_mask ==
                            (strcmp(peer->id, "peer_one") == 0 ? 7 : 1) &&
                        participant->waiting_mask ==
                            (strcmp(peer->id, "peer_one") == 0 ? 0 : 6) &&
                        participant->waiting_ranks[0] == 0 &&
                        participant->waiting_ranks[1] ==
                            (strcmp(peer->id, "peer_one") == 0 ? 0 : 1) &&
                        participant->waiting_ranks[2] ==
                            (strcmp(peer->id, "peer_one") == 0 ? 0 : 2);
                }
                if (strcmp(participant->peer_id, "peer_two") == 0) {
                    second_active = participant->active_mask;
                    second_waiting = participant->waiting_mask;
                    if (participant->active_mask == 2 &&
                        participant->waiting_mask == 5 &&
                        participant->waiting_ranks[0] == 2 &&
                        participant->waiting_ranks[2] == 1)
                    {
                        peer->saw_host_override = true;
                    }
                    if (peer->saw_host_override &&
                        participant->active_mask == 1 &&
                        participant->waiting_mask == 6 &&
                        participant->waiting_ranks[1] == 2 &&
                        participant->waiting_ranks[2] == 1)
                    {
                        peer->saw_self_reassignment = true;
                    }
                }
            }
            peer->saw_two_active |=
                first_active == 1 && first_waiting == 6 &&
                second_active == 1 && second_waiting == 6 &&
                presence->match_size == 0;
            if (first_active == 1 && second_active == 1 &&
                presence->match_size == 1 && presence->match[0] == 'R') {
                peer->saw_match = true;
                peer->match_epoch = presence->match_epoch;
            }
            peer->saw_rotated |=
                peer->saw_match && presence->match_size == 0 &&
                presence->match_epoch > peer->match_epoch &&
                first_active == 0 && first_waiting == 7 &&
                second_active == 5 && second_waiting == 2;
            break;
        }
        case NativeSignalingServerMessage_Signal:
            printf("%s received signal\n", peer->id);
            fflush(stdout);
            peer->signal_received =
                strcmp(event.message.data.signal.peer_id, "peer_one") == 0 &&
                event.message.data.signal.payload_size == 4 &&
                memcmp(event.message.data.signal.payload, "ICE!", 4) == 0;
            break;
        case NativeSignalingServerMessage_SignalSent:
            printf("%s signal acknowledged\n", peer->id);
            fflush(stdout);
            peer->signal_acknowledged =
                strcmp(event.message.data.signal.peer_id, "peer_two") == 0;
            break;
        case NativeSignalingServerMessage_Error:
            fprintf(stderr, "%s server error: %s\n", peer->id,
                    event.message.data.error);
            return false;
        default:
            break;
        }
    }
    return true;
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <http-or-https-signaling-origin>\n", argv[0]);
        return 2;
    }
    static ProbePeer first = { .id = "peer_one" };
    static ProbePeer second = { .id = "peer_two" };
    char room_id[10];
    snprintf(room_id, sizeof(room_id), "P%08lX",
             (unsigned long) GetCurrentProcessId());
    NativeSignalingClientConfig first_config = {
        .endpoint = argv[1],
        .room_id = room_id,
        .peer_id = first.id,
        .name = "Alice Local",
        .build = "native-probe-build",
        .local_players = 3,
    };
    NativeSignalingClientConfig second_config = {
        .endpoint = argv[1],
        .room_id = room_id,
        .peer_id = second.id,
        .name = "Bob Local",
        .build = "native-probe-build",
        .local_players = 3,
    };
    if (!nativeSignalingClient_Start(&first.client, &first_config))
    {
        fprintf(stderr, "could not start first native signaling client\n");
        nativeSignalingClient_Stop(&first.client);
        return 1;
    }

    bool ok = true;
    uint64_t first_deadline = GetTickCount64() + 5000;
    while (GetTickCount64() < first_deadline &&
           (!first.connected || !first.welcomed))
    {
        if (!consume(&first)) {
            ok = false;
            break;
        }
        Sleep(10);
    }
    if (!ok || !first.connected || !first.welcomed ||
        !nativeSignalingClient_Start(&second.client, &second_config))
    {
        fprintf(stderr, "could not establish ordered signaling clients\n");
        nativeSignalingClient_Stop(&first.client);
        nativeSignalingClient_Stop(&second.client);
        return 1;
    }

    bool override_queued = false;
    bool reassignment_queued = false;
    bool signal_queued = false;
    bool rotation_queued = false;
    bool two_active_queued = false;
    bool match_queued = false;
    bool report_queued = false;
    uint64_t deadline = GetTickCount64() + 10000;
    while (GetTickCount64() < deadline) {
        ok = consume(&first) && consume(&second);
        if (!ok) break;
        if (!override_queued && first.saw_both && second.saw_both &&
            first.saw_assignment && second.saw_assignment) {
            override_queued = nativeSignalingClient_AssignSeats(
                &first.client, "peer_two", 2, 5);
            if (!override_queued) break;
        }
        if (!reassignment_queued && first.saw_host_override &&
            second.saw_host_override) {
            NativeSignalingUpdate update = {
                .active_mask = 1,
                .waiting_mask = 6,
                .has_active_mask = true,
                .has_waiting_mask = true,
            };
            reassignment_queued = nativeSignalingClient_QueueUpdate(
                &second.client, &update);
            if (!reassignment_queued) break;
        }
        if (!signal_queued && first.saw_self_reassignment &&
            second.saw_self_reassignment) {
            signal_queued = nativeSignalingClient_QueueSignal(
                &first.client, "peer_two", (const uint8_t*) "ICE!", 4);
            if (!signal_queued) break;
        }
        if (!rotation_queued && first.signal_acknowledged &&
            second.signal_received) {
            rotation_queued = nativeSignalingClient_SetRotation(
                &first.client, NativeRoomRotation_WinnerStays);
            if (!rotation_queued) break;
        }
        if (!two_active_queued && first.saw_rotation &&
            second.saw_rotation) {
            NativeSignalingUpdate update = {
                .active_mask = 1,
                .waiting_mask = 6,
                .has_active_mask = true,
                .has_waiting_mask = true,
            };
            two_active_queued = nativeSignalingClient_QueueUpdate(
                &first.client, &update);
            if (!two_active_queued) break;
        }
        if (!match_queued && first.saw_two_active &&
            second.saw_two_active) {
            match_queued = nativeSignalingClient_SetMatch(
                &first.client, (const uint8_t*) "R", 1);
            if (!match_queued) break;
        }
        if (!report_queued && first.saw_match && second.saw_match) {
            report_queued = nativeSignalingClient_ReportMatch(
                &second.client, second.match_epoch, "peer_two", 0, true, 0);
            if (!report_queued) break;
        }
        if (first.connected && second.connected && first.welcomed &&
            second.welcomed && first.saw_both && second.saw_both &&
            first.saw_assignment && second.saw_assignment &&
            first.saw_host_override && second.saw_host_override &&
            first.saw_self_reassignment && second.saw_self_reassignment &&
            first.signal_acknowledged && second.signal_received &&
            first.saw_rotation && second.saw_rotation &&
            first.saw_rotated && second.saw_rotated)
        {
            break;
        }
        Sleep(10);
    }
    bool passed = ok && first.connected && second.connected && first.welcomed &&
                  second.welcomed && first.saw_both && second.saw_both &&
                  first.saw_assignment && second.saw_assignment &&
                  first.saw_host_override && second.saw_host_override &&
                  first.saw_self_reassignment && second.saw_self_reassignment &&
                  first.signal_acknowledged && second.signal_received &&
                  first.saw_rotation && second.saw_rotation &&
                  first.saw_rotated && second.saw_rotated;
    nativeSignalingClient_Stop(&first.client);
    nativeSignalingClient_Stop(&second.client);
    if (!passed) {
        fprintf(stderr, "native signaling probe timed out or failed\n");
        return 1;
    }
    puts("native signaling probe passed");
    return 0;
}
