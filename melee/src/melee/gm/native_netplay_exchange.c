#include "native_netplay_exchange.h"

#ifdef MELEE_NATIVE

#include <string.h>

enum {
    GAMEPLAY_PORT_MASK = (1U << NATIVE_ROLLBACK_INPUT_PORTS) - 1,
    REDUNDANT_FRAME_COUNT = NATIVE_NETPLAY_INPUT_MAX_FRAMES,
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
    return true;
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
        .has_checksum = exchange->has_checksum,
    };
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
    ++exchange->received_packets;
    return true;
}

#endif
