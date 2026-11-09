#include "native_netplay_exchange.h"

#ifdef MELEE_NATIVE

#include <string.h>

enum {
    GAMEPLAY_PORT_MASK = (1U << NATIVE_ROLLBACK_INPUT_PORTS) - 1,
    REDUNDANT_FRAME_COUNT = NATIVE_NETPLAY_INPUT_MAX_FRAMES,
    LEAD_SAMPLE_MAX_AGE = 60,
    MAX_MEASURED_LEAD = 16000, // milliframes
};

bool nativeNetplayExchange_Init(NativeNetplayExchange* exchange,
                                uint64_t session_id, uint8_t local_peer,
                                uint8_t local_port_mask)
{
    if (exchange == NULL || local_peer >= 4 || local_port_mask == 0 ||
        (local_port_mask & ~GAMEPLAY_PORT_MASK) != 0)
    {
        return false;
    }
    *exchange = (NativeNetplayExchange){
        .session_id = session_id,
        .local_peer = local_peer,
        .local_port_mask = local_port_mask,
        .active = true,
    };
    return true;
}

bool nativeNetplayExchange_BeginPhase(NativeNetplayExchange* exchange,
                                      uint64_t session_id)
{
    if (exchange == NULL || !exchange->active ||
        session_id == exchange->session_id) {
        return false;
    }
    NativeNetplayExchangePeer peers[4];
    memcpy(peers, exchange->peers, sizeof(peers));
    uint8_t local_peer = exchange->local_peer;
    uint8_t local_port_mask = exchange->local_port_mask;
    uint8_t artificial_delay_frames = exchange->artificial_delay_frames;
    uint64_t prior_session_id = exchange->session_id;
    if (!nativeNetplayExchange_Init(exchange, session_id, local_peer,
                                    local_port_mask)) {
        return false;
    }
    for (uint32_t i = 0; i < 4; ++i) {
        if (peers[i].configured &&
            !nativeNetplayExchange_AddPeer(exchange, (uint8_t) i,
                                            peers[i].port_mask)) {
            return false;
        }
    }
    exchange->artificial_delay_frames = artificial_delay_frames;
    exchange->prior_session_id = prior_session_id;
    exchange->has_prior_session = true;
    return true;
}

bool nativeNetplayExchange_AddPeer(NativeNetplayExchange* exchange,
                                   uint8_t peer, uint8_t port_mask)
{
    if (exchange == NULL || !exchange->active || peer >= 4 ||
        peer == exchange->local_peer || port_mask == 0 ||
        (port_mask & ~GAMEPLAY_PORT_MASK) != 0 ||
        (port_mask & exchange->local_port_mask) != 0 ||
        exchange->peers[peer].configured)
    {
        return false;
    }
    for (uint32_t i = 0; i < 4; ++i) {
        if (exchange->peers[i].configured &&
            (exchange->peers[i].port_mask & port_mask) != 0)
        {
            return false;
        }
    }
    exchange->peers[peer] = (NativeNetplayExchangePeer){
        .port_mask = port_mask,
        .configured = true,
    };
    return true;
}

bool nativeNetplayExchange_SetArtificialDelay(NativeNetplayExchange* exchange,
                                              uint8_t frames)
{
    // Leave one tick of lead so peers can record/send the next input before
    // the confirmed-checkpoint window stops forward simulation.
    if (exchange == NULL || !exchange->active ||
        frames >= NativeConfirmedCheckpoint_MaxPrediction)
    {
        return false;
    }
    exchange->artificial_delay_frames = frames;
    return true;
}

bool nativeNetplayExchange_RecordLocal(NativeNetplayExchange* exchange,
                                       uint64_t frame,
                                       const PADStatus pads[4])
{
    if (exchange == NULL || !exchange->active || pads == NULL ||
        (exchange->has_newest_frame && frame != exchange->newest_frame + 1))
    {
        return false;
    }
    NativeNetplayExchangeFrame* saved =
        &exchange->frames[frame % NATIVE_ROLLBACK_INPUT_CAPACITY];
    *saved = (NativeNetplayExchangeFrame){
        .frame = frame,
        .valid = true,
    };
    for (uint32_t port = 0; port < NATIVE_ROLLBACK_INPUT_PORTS; ++port) {
        if ((exchange->local_port_mask & (1U << port)) != 0) {
            saved->pads[port] = pads[port];
        }
    }
    exchange->newest_frame = frame;
    exchange->has_newest_frame = true;
    for (unsigned i = 0; i < 4; ++i) {
        const NativeNetplayExchangePeer* peer = &exchange->peers[i];
        exchange->observed_frames[i] =
            peer->has_sample ? peer->sample_frame : UINT64_MAX;
    }
    return true;
}

int32_t nativeNetplayExchange_MaxLead(const NativeNetplayExchange* exchange)
{
    int32_t lead = 0;
    for (unsigned i = 0; i < 4; ++i) {
        const NativeNetplayExchangePeer* peer = &exchange->peers[i];
        if (peer->has_lead && peer->lead_milliframes > lead &&
            exchange->newest_frame - peer->lead_sample_at < LEAD_SAMPLE_MAX_AGE) {
            lead = peer->lead_milliframes;
        }
    }
    return lead;
}

static void observe_peer_sample(const NativeNetplayExchange* exchange,
                                NativeNetplayExchangePeer* peer,
                                const NativeNetplayInputBatch* batch)
{
    // Redundant and reordered datagrams carry useful input, but must not
    // update the clock estimate more than once per sender sample.
    uint64_t remote = batch->sample_frame;
    if (peer->has_sample && remote <= peer->sample_frame) {
        return;
    }
    peer->sample_frame = remote;
    peer->has_sample = true;

    uint64_t local = exchange->newest_frame;
    uint64_t echoed = batch->observed_frames[exchange->local_peer];
    uint64_t gap = local > remote ? local - remote : remote - local;
    if (!exchange->has_newest_frame || echoed > local ||
        local - echoed >= LEAD_SAMPLE_MAX_AGE || gap >= LEAD_SAMPLE_MAX_AGE) {
        return;
    }

    // The local samples span a round trip. Compare the remote sample to
    // their midpoint to cancel symmetric travel time in frame units.
    int32_t separation = local >= remote ? (int32_t) gap : -(int32_t) gap;
    int32_t lead = 1000 * separation - 500 * (int32_t) (local - echoed);
    if (lead > MAX_MEASURED_LEAD) lead = MAX_MEASURED_LEAD;
    if (lead < -MAX_MEASURED_LEAD) lead = -MAX_MEASURED_LEAD;
    if (peer->has_lead && local - peer->lead_sample_at < LEAD_SAMPLE_MAX_AGE) {
        peer->lead_milliframes += (lead - peer->lead_milliframes) / 8;
    } else {
        peer->lead_milliframes = lead;
    }
    peer->lead_sample_at = local;
    peer->has_lead = true;
}

bool nativeNetplayExchange_SetChecksum(NativeNetplayExchange* exchange,
                                      uint64_t frame, uint64_t checksum)
{
    if (exchange == NULL || !exchange->active ||
        (exchange->has_checksum && frame <= exchange->checksum_frame))
    {
        return false;
    }
    exchange->checksum_frame = frame;
    exchange->checksum = checksum;
    exchange->has_checksum = true;
    return true;
}

bool nativeNetplayExchange_PreparePacket(const NativeNetplayExchange* exchange,
                                         uint8_t* packet,
                                         size_t packet_capacity,
                                         size_t* packet_size)
{
    if (exchange == NULL || !exchange->active || packet == NULL ||
        packet_size == NULL || !exchange->has_newest_frame)
    {
        return false;
    }
    *packet_size = 0;
    uint64_t send_through = exchange->newest_frame;
    if (send_through != 0 &&
        send_through <= exchange->artificial_delay_frames)
    {
        return true;
    }
    if (send_through != 0) {
        send_through -= exchange->artificial_delay_frames;
    }
    uint64_t start_frame = send_through + 1 > REDUNDANT_FRAME_COUNT
                               ? send_through + 1 - REDUNDANT_FRAME_COUNT
                               : 0;
    while (start_frame < send_through) {
        const NativeNetplayExchangeFrame* saved =
            &exchange->frames[start_frame % NATIVE_ROLLBACK_INPUT_CAPACITY];
        if (saved->valid && saved->frame == start_frame) {
            break;
        }
        ++start_frame;
    }
    NativeNetplayInputBatch batch = {
        .session_id = exchange->session_id,
        .start_frame = start_frame,
        .sender_peer = exchange->local_peer,
        .port_mask = exchange->local_port_mask,
        .frame_count = (uint8_t) (send_through - start_frame + 1),
        .checksum_frame = exchange->checksum_frame,
        .checksum = exchange->checksum,
        .sample_frame = exchange->newest_frame,
        .has_checksum = exchange->has_checksum,
    };
    memcpy(batch.observed_frames, exchange->observed_frames,
           sizeof(batch.observed_frames));
    for (uint32_t frame = 0; frame < batch.frame_count; ++frame) {
        const NativeNetplayExchangeFrame* saved =
            &exchange->frames[(start_frame + frame) %
                              NATIVE_ROLLBACK_INPUT_CAPACITY];
        if (!saved->valid || saved->frame != start_frame + frame) {
            return false;
        }
        memcpy(batch.pads[frame], saved->pads, sizeof(saved->pads));
    }
    *packet_size =
        nativeNetplayInput_Encode(&batch, packet, packet_capacity);
    return *packet_size != 0;
}

bool nativeNetplayExchange_AcceptPacket(NativeNetplayExchange* exchange,
                                       NativeRollbackInputHistory* inputs,
                                       uint8_t source_peer,
                                       const uint8_t* packet,
                                       size_t packet_size)
{
    if (exchange == NULL || inputs == NULL || !exchange->active ||
        source_peer >= 4 || packet == NULL)
    {
        return false;
    }
    NativeNetplayInputBatch batch;
    NativeNetplayExchangePeer* peer = &exchange->peers[source_peer];
    if (!peer->configured ||
        !nativeNetplayInput_Decode(&batch, packet, packet_size))
    {
        ++exchange->rejected_packets;
        ++exchange->rejected_header_packets;
        return false;
    }
    if (exchange->has_prior_session &&
        batch.session_id == exchange->prior_session_id)
    {
        ++exchange->stale_packets;
        return true;
    }
    NativeNetplayPendingChecksum* pending = NULL;
    if (batch.has_checksum) {
        pending = &peer->pending_checksums[
            batch.checksum_frame % NATIVE_ROLLBACK_INPUT_CAPACITY];
        if ((pending->valid && pending->frame == batch.checksum_frame &&
             pending->value != batch.checksum) ||
            (peer->has_checksum &&
             batch.checksum_frame == peer->checksum_frame &&
             batch.checksum != peer->checksum))
        {
            ++exchange->rejected_packets;
            ++exchange->rejected_checksum_packets;
            return false;
        }
        if ((!peer->has_checksum ||
             batch.checksum_frame > peer->checksum_frame) &&
            pending->valid && pending->frame != batch.checksum_frame)
        {
            exchange->checksum_overflow = true;
            ++exchange->rejected_packets;
            ++exchange->rejected_checksum_packets;
            return false;
        }
    }
    if (!nativeNetplayInput_SubmitBatch(inputs, &batch, exchange->session_id,
                                        source_peer, peer->port_mask))
    {
        ++exchange->rejected_packets;
        ++exchange->rejected_input_packets;
        return false;
    }
    if (batch.has_checksum &&
        (!peer->has_checksum || batch.checksum_frame > peer->checksum_frame))
    {
        *pending = (NativeNetplayPendingChecksum){
            .frame = batch.checksum_frame,
            .value = batch.checksum,
            .valid = true,
        };
        peer->checksum_frame = batch.checksum_frame;
        peer->checksum = batch.checksum;
        peer->has_checksum = true;
    }
    observe_peer_sample(exchange, peer, &batch);
    ++exchange->received_packets;
    return true;
}

#endif
