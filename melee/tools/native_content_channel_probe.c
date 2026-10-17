#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <string.h>
#include <windows.h>

#include <melee/gm/native_netplay_connection.h>

enum {
    CLIENTS = 5,
    DEADLINE_MS = 20000
};
static NativeNetplayConnection clients[CLIENTS];
static char peers[CLIENTS][65];
static bool running[CLIENTS];
#define CHECK(test, text)                                                     \
    do {                                                                      \
        if (!(test)) {                                                        \
            fprintf(stderr, "FAIL: %s\n", text);                              \
            result = 1;                                                       \
            goto done;                                                        \
        }                                                                     \
    } while (0)

static bool pump(void)
{
    for (unsigned i = 0; i < CLIENTS; ++i) {
        if (running[i] && !nativeNetplayConnection_Pump(&clients[i])) {
            fprintf(stderr, "client %u: %s\n", i,
                    nativeNetplayConnection_Failure(&clients[i]));
            return false;
        }
    }
    Sleep(2);
    return true;
}
static bool gameplay_ready(void)
{
    for (unsigned i = 0; i < CLIENTS; ++i) {
        unsigned expected = i == 4 ? 0 : 3;
        if (!clients[i].has_presence ||
            clients[i].presence.participant_count != CLIENTS ||
            nativeNetplayConnection_RemoteCount(&clients[i]) != expected ||
            nativeNetplayConnection_ConnectedCount(&clients[i]) != expected)
        {
            return false;
        }
    }
    return true;
}
static bool channels_ready(void)
{
    for (unsigned owner = 0; owner < 2; ++owner) {
        if (nativeNetplayConnection_ContentState(&clients[0], peers[4], 23,
                                                 owner) !=
                NativeContentChannel_Connected ||
            nativeNetplayConnection_ContentState(&clients[4], peers[0], 23,
                                                 owner) !=
                NativeContentChannel_Connected)
        {
            return false;
        }
    }
    return true;
}
static bool receive(unsigned client, bool owner, const char* text)
{
    uint64_t deadline = GetTickCount64() + DEADLINE_MS;
    while (GetTickCount64() < deadline) {
        if (!pump()) {
            return false;
        }
        NativeContentDatagram packet;
        while (nativeNetplayConnection_PollContent(&clients[client], &packet))
        {
            if (packet.serial == 23 && packet.local_owner == owner &&
                packet.message.datagram.size == strlen(text) + 1 &&
                !memcmp(packet.message.datagram.bytes, text, strlen(text) + 1))
            {
                return true;
            }
        }
    }
    return false;
}
int main(int argc, char** argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s <service-origin> <fresh-room-code>\n",
                argv[0]);
        return 2;
    }
    int result = 0;
    for (unsigned i = 0; i < CLIENTS; ++i) {
        snprintf(peers[i], sizeof(peers[i]), "content_%lu_%u",
                 GetCurrentProcessId(), i);
        NativeNetplayConnectionConfig config = {
            .signaling = { .endpoint = argv[1],
                           .room_id = argv[2],
                           .peer_id = peers[i],
                           .name = "Local content probe",
                           .build = "native-content-channel-probe",
                           .local_players = 1 }
        };
        running[i] = nativeNetplayConnection_Start(&clients[i], &config);
        CHECK(running[i], "start five isolated clients");
        uint64_t deadline = GetTickCount64() + DEADLINE_MS;
        while ((!clients[i].has_presence ||
                clients[i].presence.participant_count != i + 1) &&
               GetTickCount64() < deadline)
        {
            CHECK(pump(), "pump room join");
        }
        CHECK(clients[i].has_presence, "receive room presence");
    }
    uint64_t deadline = GetTickCount64() + DEADLINE_MS;
    while (!gameplay_ready() && GetTickCount64() < deadline) {
        CHECK(pump(), "connect four gameplay peers");
    }
    CHECK(gameplay_ready(),
          "four players have three gameplay routes; the fifth has none");
    for (unsigned owner = 0; owner < 2; ++owner) {
        CHECK(nativeNetplayConnection_ReserveContent(&clients[0], peers[4], 23,
                                                     owner),
              "reserve first endpoint");
        CHECK(nativeNetplayConnection_ReserveContent(&clients[4], peers[0], 23,
                                                     !owner),
              "reserve other endpoint");
    }
    for (unsigned i = 2; i < NATIVE_CONTENT_CHANNELS; ++i) {
        CHECK(nativeNetplayConnection_ReserveContent(&clients[0], peers[1], 24 + i, true),
              "reserve the remaining bounded content slots");
    }
    CHECK(!nativeNetplayConnection_ReserveContent(&clients[0], peers[1], 100, true),
          "content route pool is bounded");
    for (unsigned i = 2; i < NATIVE_CONTENT_CHANNELS; ++i)
        nativeNetplayConnection_ReleaseContent(&clients[0], peers[1], 24 + i, true);
    CHECK(!nativeNetplayConnection_ReserveContent(&clients[0], "not-a-member",
                                                  24, true),
          "require current room membership");
    CHECK(!clients[0].content.routes[0].ice.agent &&
              !clients[0].content.routes[1].ice.agent,
          "reserving offers creates no ICE agents");
    const uint8_t binary[] = { 0, 255, 1, 0 };
    CHECK(nativeNetplayConnection_SendControl(&clients[0], peers[4], binary,
                                              sizeof(binary)),
          "control works without gameplay routes");
    bool got_control = false;
    deadline = GetTickCount64() + DEADLINE_MS;
    while (!got_control && GetTickCount64() < deadline) {
        CHECK(pump(), "pump control");
        NativeContentControl packet;
        if (nativeNetplayConnection_PollControl(&clients[4], &packet)) {
            got_control =
                packet.datagram.size == sizeof(binary) &&
                !memcmp(packet.datagram.bytes, binary, sizeof(binary));
        }
    }
    CHECK(got_control, "binary control arrives at the waiting peer");
    for (unsigned i = 0; i < NATIVE_CONTENT_CONTROL_QUEUE + 3; ++i) {
        nativeNetplayContentChannels_Control(&clients[4], peers[0], binary,
                                             sizeof(binary));
    }
    unsigned controls = 0;
    NativeContentControl packet;
    while (nativeNetplayConnection_PollControl(&clients[4], &packet)) {
        ++controls;
    }
    CHECK(controls == NATIVE_CONTENT_CONTROL_QUEUE,
          "control inbox is bounded");
    for (unsigned owner = 0; owner < 2; ++owner) {
        CHECK(nativeNetplayConnection_StartContent(&clients[0], peers[4], 23,
                                                   owner),
              "start accepted content");
        CHECK(nativeNetplayConnection_StartContent(&clients[4], peers[0], 23,
                                                   owner),
              "start remote accepted content");
    }
    deadline = GetTickCount64() + DEADLINE_MS;
    while (!channels_ready() && GetTickCount64() < deadline) {
        CHECK(pump(), "negotiate both directions");
    }
    if (!channels_ready()) {
        for (unsigned c = 0; c < CLIENTS; c += 4) {
            for (unsigned k = 0; k < 2; ++k) {
                NativeContentChannel* r = &clients[c].content.routes[k];
                fprintf(stderr,
                        "channel client=%u owner=%u started=%u sent=%u "
                        "remote=%u state=%u\n",
                        c, r->local_owner, r->started,
                        r->local_description_sent,
                        r->remote_description_applied,
                        nativeNetplayIce_State(&r->ice));
            }
        }
    }
    CHECK(channels_ready(),
          "independent simultaneous transfers connect with equal serials");
    CHECK(gameplay_ready(),
          "content negotiation leaves gameplay counts unchanged");
    CHECK(nativeNetplayConnection_Send(&clients[0], peers[1],
                                       (const uint8_t*) "game", 5),
          "gameplay route remains usable during content transfer");
    bool got_game = false;
    deadline = GetTickCount64() + DEADLINE_MS;
    while (!got_game && GetTickCount64() < deadline) {
        CHECK(pump(), "pump isolated gameplay packet");
        NativeNetplayConnectionDatagram game;
        if (nativeNetplayConnection_PollDatagram(&clients[1], &game)) {
            got_game = game.datagram.size == 5 &&
                       !memcmp(game.datagram.bytes, "game", 5);
        }
    }
    CHECK(got_game, "content polling does not consume gameplay packets");
    CHECK(nativeNetplayConnection_SendContent(&clients[0], peers[4], 23, true,
                                              (const uint8_t*) "forward", 8) &&
              nativeNetplayConnection_SendContent(
                  &clients[4], peers[0], 23, true, (const uint8_t*) "reverse",
                  8),
          "send both ways");
    CHECK(receive(4, false, "forward") && receive(0, false, "reverse"),
          "preserve both transfer owners");
    NativeSignalingUpdate update = { .has_active_mask = true,
                                     .has_waiting_mask = true,
                                     .active_mask = 0,
                                     .waiting_mask = 0 };
    CHECK(nativeSignalingClient_QueueUpdate(&clients[4].signaling, &update),
          "become a spectator");
    deadline = GetTickCount64() + DEADLINE_MS;
    bool spectator = false;
    while (!spectator && GetTickCount64() < deadline) {
        CHECK(pump(), "pump spectator change");
        for (unsigned i = 0; i < clients[0].presence.participant_count; ++i) {
            NativeSignalingParticipant* p =
                &clients[0].presence.participants[i];
            if (!strcmp(p->peer_id, peers[4])) {
                spectator = !p->active_mask && !p->waiting_mask;
            }
        }
    }
    CHECK(spectator && channels_ready() && gameplay_ready(),
          "role changes preserve content independently");
    CHECK(nativeNetplayConnection_SendContent(&clients[4], peers[0], 23, false,
                                              (const uint8_t*) "reply", 6) &&
              receive(0, true, "reply"),
          "reply reaches the original transfer owner");
    nativeNetplayConnection_ReleaseContent(&clients[0], peers[4], 23, true);
    CHECK(nativeNetplayConnection_ReserveContent(&clients[0], peers[4], 24,
                                                 true),
          "reuse a released slot");
    nativeNetplayContentChannels_Description(&clients[0], peers[4], 23, false,
                                             (const uint8_t*) "stale", 5);
    CHECK(nativeNetplayConnection_ContentState(&clients[0], peers[4], 24,
                                               true) ==
              NativeContentChannel_Reserved,
          "late descriptions cannot start a newer transfer");
    nativeNetplayConnection_Stop(&clients[4]);
    running[4] = false;
    deadline = GetTickCount64() + DEADLINE_MS;
    while (clients[0].presence.participant_count != 4 &&
           GetTickCount64() < deadline)
    {
        CHECK(pump(), "observe departing member");
    }
    CHECK(nativeNetplayConnection_ContentState(
              &clients[0], peers[4], 24, true) == NativeContentChannel_None &&
              nativeNetplayConnection_ContentState(&clients[0], peers[4], 23,
                                                   false) ==
                  NativeContentChannel_None,
          "leaving the room releases content routes");
    puts("PASS: four gameplay players plus a waiting/spectating recipient; "
         "bounded controls, simultaneous content, role changes and cleanup.");
done:
    for (unsigned i = 0; i < CLIENTS; ++i) {
        if (running[i]) {
            nativeNetplayConnection_Stop(&clients[i]);
        }
    }
    return result;
}
