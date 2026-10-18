#include "native_netplay_connection.h"
#include "native_netplay_connection_wire.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>

static const NativeSignalingParticipant*
member(const NativeNetplayConnection* c, const char* peer)
{
    if (!c || !c->active || c->failed || !c->has_presence || !peer ||
        !strcmp(peer, c->local_peer_id))
    {
        return NULL;
    }
    for (unsigned i = 0; i < c->presence.participant_count; ++i) {
        const NativeSignalingParticipant* p = &c->presence.participants[i];
        if (!strcmp(peer, p->peer_id) && !strcmp(p->build, c->local_build)) {
            return p;
        }
    }
    return NULL;
}
static NativeContentChannel* route(NativeNetplayConnection* c,
                                   const char* peer, uint32_t serial,
                                   bool local_owner)
{
    if (!c || !peer || !serial) {
        return NULL;
    }
    for (unsigned i = 0; i < NATIVE_CONTENT_CHANNELS; ++i) {
        NativeContentChannel* r = &c->content.routes[i];
        if (r->serial == serial && r->local_owner == local_owner &&
            !strcmp(r->peer_id, peer))
        {
            return r;
        }
    }
    return NULL;
}
static void close_route(NativeContentChannel* r)
{
    nativeNetplayIce_Shutdown(&r->ice);
    memset(r, 0, sizeof(*r));
}
bool nativeNetplayConnection_ReserveContent(NativeNetplayConnection* c,
                                            const char* peer, uint32_t serial,
                                            bool local_owner)
{
    const NativeSignalingParticipant* person = member(c, peer);
    if (!person || !serial) {
        return false;
    }
    NativeContentChannel* r = route(c, peer, serial, local_owner);
    if (r) {
        return !r->failed && r->join_order == person->join_order;
    }
    for (unsigned i = 0; i < NATIVE_CONTENT_CHANNELS; ++i) {
        r = &c->content.routes[i];
        if (r->serial) {
            continue;
        }
        r->serial = serial;
        r->local_owner = local_owner;
        r->join_order = person->join_order;
        memcpy(r->peer_id, person->peer_id, sizeof(r->peer_id));
        return true;
    }
    return false;
}
bool nativeNetplayConnection_StartContent(NativeNetplayConnection* c,
                                          const char* peer, uint32_t serial,
                                          bool local_owner)
{
    NativeContentChannel* r = route(c, peer, serial, local_owner);
    if (!r || r->failed || !member(c, peer)) {
        return false;
    }
    if (r->started) {
        return true;
    }
    r->started = true;
    r->started_ms = GetTickCount64();
    if (!nativeNetplayIce_Init(&r->ice, &c->ice_config) ||
        !nativeNetplayIce_StartGathering(&r->ice))
    {
        r->failed = true;
        return false;
    }
    if (r->remote_description[0]) {
        r->remote_description_applied =
            nativeNetplayIce_SetRemoteDescription(&r->ice,
                                                  r->remote_description) &&
            nativeNetplayIce_SetRemoteGatheringDone(&r->ice);
        r->failed = !r->remote_description_applied;
    }
    return !r->failed;
}
void nativeNetplayConnection_ReleaseContent(NativeNetplayConnection* c,
                                            const char* peer, uint32_t serial,
                                            bool local_owner)
{
    NativeContentChannel* r = route(c, peer, serial, local_owner);
    if (r) {
        close_route(r);
    }
}
NativeContentChannelState
nativeNetplayConnection_ContentState(NativeNetplayConnection* c,
                                     const char* peer, uint32_t serial,
                                     bool local_owner)
{
    NativeContentChannel* r = route(c, peer, serial, local_owner);
    if (!r) {
        return NativeContentChannel_None;
    }
    if (r->failed) {
        return NativeContentChannel_Failed;
    }
    if (!r->started) {
        return NativeContentChannel_Reserved;
    }
    NativeNetplayIceState state = nativeNetplayIce_State(&r->ice);
    if (state == NativeNetplayIceState_Failed ||
        ((state != NativeNetplayIceState_Connected &&
          state != NativeNetplayIceState_Completed) &&
         GetTickCount64() - r->started_ms >= 30000))
    {
        r->failed = true;
        return NativeContentChannel_Failed;
    }
    return state == NativeNetplayIceState_Connected ||
                   state == NativeNetplayIceState_Completed
               ? NativeContentChannel_Connected
               : NativeContentChannel_Connecting;
}
bool nativeNetplayConnection_SendContent(NativeNetplayConnection* c,
                                         const char* peer, uint32_t serial,
                                         bool local_owner,
                                         const uint8_t* bytes, size_t size)
{
    NativeContentChannel* r = route(c, peer, serial, local_owner);
    return r && !r->failed && r->started &&
           nativeNetplayIce_Send(&r->ice, bytes, size);
}
bool nativeNetplayConnection_PollContent(NativeNetplayConnection* c,
                                         NativeContentDatagram* out)
{
    if (!c || !out) {
        return false;
    }
    for (unsigned i = 0; i < NATIVE_CONTENT_CHANNELS; ++i) {
        unsigned index =
            (c->content.poll_cursor + i) % NATIVE_CONTENT_CHANNELS;
        NativeContentChannel* r = &c->content.routes[index];
        if (!r->serial || !r->started || r->failed ||
            !nativeNetplayIce_PollDatagram(&r->ice, &out->message.datagram))
        {
            continue;
        }
        memcpy(out->message.peer_id, r->peer_id, sizeof(out->message.peer_id));
        out->serial = r->serial;
        out->local_owner = r->local_owner;
        c->content.poll_cursor = (index + 1) % NATIVE_CONTENT_CHANNELS;
        return true;
    }
    return false;
}
bool nativeNetplayConnection_SendControl(NativeNetplayConnection* c,
                                         const char* peer,
                                         const uint8_t* bytes, size_t size)
{
    if (!member(c, peer)) {
        return false;
    }
    uint8_t packet[NATIVE_NETPLAY_INPUT_PACKET_CAPACITY + 8];
    size_t length = nativeNetplayConnectionWire_EncodeControl(
        bytes, size, packet, sizeof(packet));
    return length && nativeSignalingClient_QueueSignal(
                         &c->signaling, peer, packet, (uint16_t) length);
}
void nativeNetplayContentChannels_Control(NativeNetplayConnection* c,
                                          const char* peer,
                                          const uint8_t* bytes, size_t size)
{
    if (!member(c, peer) || !size ||
        size > NATIVE_NETPLAY_INPUT_PACKET_CAPACITY ||
        c->content.write - c->content.read >= NATIVE_CONTENT_CONTROL_QUEUE)
    {
        return;
    }
    NativeContentControl* slot =
        &c->content
             .controls[c->content.write++ % NATIVE_CONTENT_CONTROL_QUEUE];
    memcpy(slot->peer_id, peer, strlen(peer) + 1);
    slot->datagram.size = (uint16_t) size;
    memcpy(slot->datagram.bytes, bytes, size);
}
bool nativeNetplayConnection_PollControl(NativeNetplayConnection* c,
                                         NativeContentControl* out)
{
    if (!c || !out) {
        return false;
    }
    while (c->content.read != c->content.write) {
        NativeContentControl* slot =
            &c->content
                 .controls[c->content.read++ % NATIVE_CONTENT_CONTROL_QUEUE];
        if (!member(c, slot->peer_id)) {
            continue;
        }
        *out = *slot;
        return true;
    }
    return false;
}
void nativeNetplayContentChannels_Description(
    NativeNetplayConnection* c, const char* peer, uint32_t serial,
    bool sender_owner, const uint8_t* bytes, size_t size)
{
    NativeContentChannel* r = route(c, peer, serial, !sender_owner);
    if (!r || !member(c, peer) || r->remote_description_applied || r->failed ||
        !size || size >= sizeof(r->remote_description))
    {
        return;
    }
    memcpy(r->remote_description, bytes, size);
    r->remote_description[size] = 0;
    if (r->started) {
        r->remote_description_applied =
            nativeNetplayIce_SetRemoteDescription(&r->ice,
                                                  r->remote_description) &&
            nativeNetplayIce_SetRemoteGatheringDone(&r->ice);
        r->failed = !r->remote_description_applied;
    }
}
void nativeNetplayContentChannels_Reconcile(NativeNetplayConnection* c)
{
    for (unsigned i = 0; i < NATIVE_CONTENT_CHANNELS; ++i) {
        NativeContentChannel* r = &c->content.routes[i];
        if (!r->serial) {
            continue;
        }
        const NativeSignalingParticipant* p = member(c, r->peer_id);
        if (!p || r->join_order != p->join_order) {
            close_route(r);
        }
    }
}
void nativeNetplayContentChannels_Pump(NativeNetplayConnection* c)
{
    uint8_t packet[NATIVE_NETPLAY_ICE_DESCRIPTION_CAPACITY + 13];
    char description[NATIVE_NETPLAY_ICE_DESCRIPTION_CAPACITY];
    for (unsigned i = 0; i < NATIVE_CONTENT_CHANNELS; ++i) {
        NativeContentChannel* r = &c->content.routes[i];
        if (!r->serial || !r->started || r->failed ||
            r->local_description_sent ||
            !nativeNetplayIce_GatheringDone(&r->ice))
        {
            continue;
        }
        if (!nativeNetplayIce_GetLocalDescription(&r->ice, description,
                                                  sizeof(description)))
        {
            r->failed = true;
            continue;
        }
        size_t length = nativeNetplayConnectionWire_EncodeContentDescription(
            description, r->serial, r->local_owner, packet, sizeof(packet));
        if (length &&
            nativeSignalingClient_QueueSignal(&c->signaling, r->peer_id,
                                              packet, (uint16_t) length))
        {
            r->local_description_sent = true;
        }
    }
}
void nativeNetplayContentChannels_Clear(NativeNetplayConnection* c)
{
    for (unsigned i = 0; i < NATIVE_CONTENT_CHANNELS; ++i) {
        close_route(&c->content.routes[i]);
    }
    c->content.read = c->content.write = c->content.poll_cursor = 0;
}
