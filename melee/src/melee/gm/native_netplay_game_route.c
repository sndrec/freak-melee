#include "native_netplay_connection.h"
#ifdef MELEE_NATIVE
#include <string.h>

enum {
    RETRY_INITIAL_MS = 500, RETRY_MAX_MS = 16000, NEGOTIATION_MS = 20000,
    ANNOUNCE_MS = 1000, QUEUE_RETRY_MS = 100, CONSTRUCTION_INTERVAL_MS = 250
};

static int compare_generation(NativeRouteGeneration a, NativeRouteGeneration b)
{
    if (a.revision != b.revision) return a.revision < b.revision ? -1 : 1;
    if (a.attempt != b.attempt) return a.attempt < b.attempt ? -1 : 1;
    return 0;
}

bool nativeNetplayGameRoute_Ready(const NativeNetplayConnectionPeer* p)
{
    if (p->local_restart || p->remote_restart || !p->local_description_sent ||
        !p->remote_description_applied) return false;
    NativeNetplayIceState state = nativeNetplayIce_State(&p->ice);
    return state == NativeNetplayIceState_Connected || state == NativeNetplayIceState_Completed;
}

void nativeNetplayGameRoute_Apply(NativeNetplayConnectionPeer* p, const NativeConnectionMessage* msg)
{
    const int generation = compare_generation(msg->sender, p->remote);
    const bool addressed = compare_generation(msg->recipient, p->local) == 0;
    if (msg->type == NativeConnectionMessage_GameHello) {
        if (generation < 0) return;
        if (generation > 0) {
            if (p->remote.attempt && (p->local_description_sent || p->remote_description_applied))
                p->remote_restart = true;
            p->remote = msg->sender;
        }
        if (!addressed) p->hello_requested = true;
        p->description_requested = true;
        return;
    }
    if (msg->type != NativeConnectionMessage_GameDescription || !addressed || generation ||
        p->local_restart || p->remote_restart || !p->ice.agent || p->remote_description_applied)
        return;
    char description[NATIVE_NETPLAY_ICE_DESCRIPTION_CAPACITY];
    memcpy(description, msg->payload, msg->size);
    description[msg->size] = 0;
    if (nativeNetplayIce_SetRemoteDescription(&p->ice, description) &&
        nativeNetplayIce_SetRemoteGatheringDone(&p->ice)) p->remote_description_applied = true;
}

static void start_agent(NativeNetplayConnection* c, NativeNetplayConnectionPeer* p, uint64_t now)
{
    // Three initial opponents can gather together. Seat changes and peer
    // retries subsequently share one refillable construction budget.
    uint64_t burst = (NATIVE_NETPLAY_CONNECTION_MAX_REMOTE_PEERS - 1) * CONSTRUCTION_INTERVAL_MS;
    uint64_t floor = now > burst ? now - burst : 0;
    uint64_t next = c->route_creation_budget_ms > floor ? c->route_creation_budget_ms : floor;
    if (next > now || now < p->restart_after) return;
    if (p->local_restart && !p->remote_restart) {
        if (c->presence.revision > p->local.revision)
            p->local = (NativeRouteGeneration){ c->presence.revision, 1 };
        else if (p->local.attempt < UINT32_MAX) ++p->local.attempt;
        else return;
    }
    c->route_creation_budget_ms = next + CONSTRUCTION_INTERVAL_MS;
    nativeNetplayIce_Shutdown(&p->ice);
    p->local_restart = p->remote_restart = p->was_connected = false;
    p->local_description_sent = p->remote_description_applied = false;
    p->hello_requested = p->description_requested = true;
    p->hello_after = p->description_after = 0;
    p->negotiate_until = now + NEGOTIATION_MS;
    unsigned delay = p->retry_delay_ms ? p->retry_delay_ms : RETRY_INITIAL_MS;
    p->restart_after = now + delay;
    p->retry_delay_ms = delay < RETRY_MAX_MS ? 2 * delay : RETRY_MAX_MS;
    if (!nativeNetplayIce_Init(&p->ice, &c->ice_config) || !nativeNetplayIce_StartGathering(&p->ice)) {
        nativeNetplayIce_Shutdown(&p->ice);
        p->local_restart = true;
    }
}

static bool send_route(NativeNetplayConnection* c, NativeNetplayConnectionPeer* p,
                       const char* description)
{
    uint8_t bytes[NATIVE_GAME_SIGNAL_HEADER_BYTES + NATIVE_NETPLAY_ICE_DESCRIPTION_CAPACITY];
    size_t size = nativeNetplayConnectionWire_EncodeGameRoute(p->local, p->remote, description,
                                                             bytes, sizeof(bytes));
    return size && nativeSignalingClient_QueueSignal(&c->signaling, p->peer_id, bytes, (uint16_t)size);
}

void nativeNetplayGameRoute_Pump(NativeNetplayConnection* c, NativeNetplayConnectionPeer* p,
                                 uint64_t now)
{
    if (!c->signaling_connected) return;
    bool ready = nativeNetplayGameRoute_Ready(p);
    if (ready && !p->was_connected) {
        p->was_connected = true;
        p->retry_delay_ms = RETRY_INITIAL_MS;
        if (p->restart_after > now + RETRY_INITIAL_MS) p->restart_after = now + RETRY_INITIAL_MS;
    } else if (!ready && !p->remote_restart &&
               (p->was_connected || (p->ice.agent && now >= p->negotiate_until) ||
                nativeNetplayIce_State(&p->ice) == NativeNetplayIceState_Failed)) {
        p->local_restart = true;
    }
    if (!p->ice.agent && (p->local_description_sent || p->remote_description_applied))
        p->local_restart = true;
    if (!p->ice.agent || p->local_restart || p->remote_restart) {
        start_agent(c, p, now);
        if (!p->ice.agent || p->local_restart || p->remote_restart) return;
        ready = false;
    }
    if (now >= p->hello_after && (!ready || p->hello_requested)) {
        bool sent = send_route(c, p, NULL);
        p->hello_requested = !sent;
        p->hello_after = now + (sent ? ANNOUNCE_MS : QUEUE_RETRY_MS);
    }
    if (!p->remote.attempt || now < p->description_after ||
        (p->local_description_sent && !p->description_requested) ||
        !nativeNetplayIce_GatheringDone(&p->ice)) return;

    char description[NATIVE_NETPLAY_ICE_DESCRIPTION_CAPACITY];
    if (!nativeNetplayIce_GetLocalDescription(&p->ice, description, sizeof(description))) {
        p->local_restart = true;
        return;
    }
    bool sent = send_route(c, p, description);
    if (sent) p->local_description_sent = true;
    p->description_requested = !sent;
    p->description_after = now + (sent ? ANNOUNCE_MS : QUEUE_RETRY_MS);
}
#endif
