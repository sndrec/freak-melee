#include "native_netplay_ice.h"

#ifdef MELEE_NATIVE

#include <string.h>

#include <juice/juice.h>

static juice_agent_t* ice_agent(const NativeNetplayIcePeer* peer)
{
    return (juice_agent_t*) peer->agent;
}

static NativeNetplayIceState translate_state(juice_state_t state)
{
    switch (state) {
    case JUICE_STATE_GATHERING:
        return NativeNetplayIceState_Gathering;
    case JUICE_STATE_CONNECTING:
        return NativeNetplayIceState_Connecting;
    case JUICE_STATE_CONNECTED:
        return NativeNetplayIceState_Connected;
    case JUICE_STATE_COMPLETED:
        return NativeNetplayIceState_Completed;
    case JUICE_STATE_FAILED:
        return NativeNetplayIceState_Failed;
    case JUICE_STATE_DISCONNECTED:
    default:
        return NativeNetplayIceState_Disconnected;
    }
}

static void state_changed(juice_agent_t* agent, juice_state_t state,
                          void* user_ptr)
{
    (void) agent;
    NativeNetplayIcePeer* peer = user_ptr;
    atomic_store_explicit(&peer->state, (int) translate_state(state),
                          memory_order_release);
}

static void gathering_done(juice_agent_t* agent, void* user_ptr)
{
    (void) agent;
    NativeNetplayIcePeer* peer = user_ptr;
    atomic_store_explicit(&peer->gathering_done, true, memory_order_release);
}

static void received(juice_agent_t* agent, const char* data, size_t size,
                     void* user_ptr)
{
    (void) agent;
    NativeNetplayIcePeer* peer = user_ptr;
    if (size == 0 || size > NATIVE_NETPLAY_ICE_DATAGRAM_BYTES) {
        atomic_fetch_add_explicit(&peer->dropped_datagrams, 1,
                                  memory_order_relaxed);
        return;
    }

    uint32_t write =
        atomic_load_explicit(&peer->write_position, memory_order_relaxed);
    uint32_t read =
        atomic_load_explicit(&peer->read_position, memory_order_acquire);
    if (write - read >= NATIVE_NETPLAY_ICE_DATAGRAM_CAPACITY) {
        atomic_fetch_add_explicit(&peer->dropped_datagrams, 1,
                                  memory_order_relaxed);
        return;
    }

    NativeNetplayIceDatagram* datagram =
        &peer->datagrams[write % NATIVE_NETPLAY_ICE_DATAGRAM_CAPACITY];
    datagram->size = (uint16_t) size;
    memcpy(datagram->bytes, data, size);
    atomic_store_explicit(&peer->write_position, write + 1,
                          memory_order_release);
}

static bool has_text(const char* text)
{
    return text != NULL && text[0] != '\0';
}

bool nativeNetplayIce_Init(NativeNetplayIcePeer* peer,
                           const NativeNetplayIceConfig* config)
{
    if (peer == NULL || config == NULL ||
        ((config->local_port_begin == 0) != (config->local_port_end == 0)) ||
        (config->local_port_begin != 0 &&
         config->local_port_begin > config->local_port_end))
    {
        return false;
    }

    bool has_turn = has_text(config->turn_host);
    if (has_turn &&
        (!has_text(config->turn_username) || !has_text(config->turn_password)))
    {
        return false;
    }

    memset(peer, 0, sizeof(*peer));
    atomic_init(&peer->read_position, 0);
    atomic_init(&peer->write_position, 0);
    atomic_init(&peer->dropped_datagrams, 0);
    atomic_init(&peer->state, NativeNetplayIceState_Disconnected);
    atomic_init(&peer->gathering_done, false);

    juice_turn_server_t turn_server = {
        .host = config->turn_host,
        .username = config->turn_username,
        .password = config->turn_password,
        .port = config->turn_port,
    };
    juice_config_t juice_config = {
        .concurrency_mode = JUICE_CONCURRENCY_MODE_POLL,
        .stun_server_host =
            has_text(config->stun_host) ? config->stun_host : NULL,
        .stun_server_port = config->stun_port,
        .turn_servers = has_turn ? &turn_server : NULL,
        .turn_servers_count = has_turn ? 1 : 0,
        .local_port_range_begin = config->local_port_begin,
        .local_port_range_end = config->local_port_end,
        .cb_state_changed = state_changed,
        .cb_gathering_done = gathering_done,
        .cb_recv = received,
        .user_ptr = peer,
    };
    juice_agent_t* agent = juice_create(&juice_config);
    if (agent == NULL) {
        return false;
    }
    peer->agent = (uintptr_t) agent;
    atomic_store_explicit(&peer->state,
                          (int) translate_state(juice_get_state(agent)),
                          memory_order_release);
    return true;
}

void nativeNetplayIce_Shutdown(NativeNetplayIcePeer* peer)
{
    if (peer == NULL || peer->agent == 0) {
        return;
    }
    juice_agent_t* agent = ice_agent(peer);
    peer->agent = 0;
    juice_destroy(agent);
    atomic_store_explicit(&peer->state, NativeNetplayIceState_Disconnected,
                          memory_order_release);
}

bool nativeNetplayIce_StartGathering(NativeNetplayIcePeer* peer)
{
    if (peer == NULL || peer->agent == 0 || peer->gathering_started) {
        return false;
    }
    if (juice_gather_candidates(ice_agent(peer)) != JUICE_ERR_SUCCESS) {
        return false;
    }
    peer->gathering_started = true;
    return true;
}

bool nativeNetplayIce_GatheringDone(const NativeNetplayIcePeer* peer)
{
    return peer != NULL &&
           atomic_load_explicit(&peer->gathering_done, memory_order_acquire);
}

bool nativeNetplayIce_GetLocalDescription(const NativeNetplayIcePeer* peer,
                                          char* description,
                                          size_t description_capacity)
{
    return peer != NULL && peer->agent != 0 && description != NULL &&
           description_capacity != 0 && nativeNetplayIce_GatheringDone(peer) &&
           juice_get_local_description(ice_agent(peer), description,
                                       description_capacity) ==
               JUICE_ERR_SUCCESS;
}

bool nativeNetplayIce_SetRemoteDescription(NativeNetplayIcePeer* peer,
                                           const char* description)
{
    return peer != NULL && peer->agent != 0 && has_text(description) &&
           juice_set_remote_description(ice_agent(peer), description) ==
               JUICE_ERR_SUCCESS;
}

bool nativeNetplayIce_SetRemoteGatheringDone(NativeNetplayIcePeer* peer)
{
    return peer != NULL && peer->agent != 0 &&
           juice_set_remote_gathering_done(ice_agent(peer)) ==
               JUICE_ERR_SUCCESS;
}

NativeNetplayIceState nativeNetplayIce_State(const NativeNetplayIcePeer* peer)
{
    return peer == NULL ? NativeNetplayIceState_Disconnected
                        : (NativeNetplayIceState) atomic_load_explicit(
                              &peer->state, memory_order_acquire);
}

NativeNetplayIceSend nativeNetplayIce_Send(NativeNetplayIcePeer* peer,
                                          const uint8_t* bytes, size_t size)
{
    if (peer == NULL || peer->agent == 0 || bytes == NULL || size == 0 ||
        size > NATIVE_NETPLAY_ICE_DATAGRAM_BYTES)
    {
        return NativeNetplayIceSend_Failed;
    }
    NativeNetplayIceState state = nativeNetplayIce_State(peer);
    if (state != NativeNetplayIceState_Connected && state != NativeNetplayIceState_Completed)
        return NativeNetplayIceSend_Failed;
    int status = juice_send(ice_agent(peer), (const char*)bytes, size);
    if (status == JUICE_ERR_SUCCESS) return NativeNetplayIceSend_Sent;
    if (status == JUICE_ERR_AGAIN) return NativeNetplayIceSend_Retry;
    return NativeNetplayIceSend_Failed;
}

bool nativeNetplayIce_PollDatagram(NativeNetplayIcePeer* peer,
                                   NativeNetplayIceDatagram* datagram)
{
    if (peer == NULL || datagram == NULL) {
        return false;
    }
    uint32_t read =
        atomic_load_explicit(&peer->read_position, memory_order_relaxed);
    uint32_t write =
        atomic_load_explicit(&peer->write_position, memory_order_acquire);
    if (read == write) {
        return false;
    }
    *datagram = peer->datagrams[read % NATIVE_NETPLAY_ICE_DATAGRAM_CAPACITY];
    atomic_store_explicit(&peer->read_position, read + 1,
                          memory_order_release);
    return true;
}

uint32_t nativeNetplayIce_DroppedDatagrams(const NativeNetplayIcePeer* peer)
{
    return peer == NULL ? 0
                        : atomic_load_explicit(&peer->dropped_datagrams,
                                               memory_order_relaxed);
}

const char* nativeNetplayIce_SelectedRoute(const NativeNetplayIcePeer* peer)
{
    char local[JUICE_MAX_CANDIDATE_SDP_STRING_LEN];
    char remote[JUICE_MAX_CANDIDATE_SDP_STRING_LEN];
    if (peer == NULL || !peer->agent ||
        juice_get_selected_candidates(ice_agent(peer), local, sizeof(local),
                                     remote, sizeof(remote)) != JUICE_ERR_SUCCESS) {
        return "unknown";
    }
    return strstr(local, " typ relay") || strstr(remote, " typ relay")
               ? "relay" : "direct";
}

bool nativeNetplayIce_GetSelectedAddresses(const NativeNetplayIcePeer* peer,
                                           char* local, size_t local_capacity,
                                           char* remote,
                                           size_t remote_capacity)
{
    return peer != NULL && peer->agent != 0 && local != NULL &&
           remote != NULL && local_capacity != 0 && remote_capacity != 0 &&
           juice_get_selected_addresses(ice_agent(peer), local, local_capacity,
                                        remote,
                                        remote_capacity) == JUICE_ERR_SUCCESS;
}

#endif
