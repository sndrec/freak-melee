#include <stdio.h>
#include <stdlib.h>

#include <melee/gm/native_netplay_udp.h>

static void require(bool condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

static PADStatus pad(uint16_t button, int8_t stick_x)
{
    return (PADStatus){
        .button = button,
        .stickX = stick_x,
        .err = PAD_ERR_NONE,
    };
}

static void initialize_inputs(NativeRollbackInputHistory* inputs,
                              uint8_t local_mask)
{
    require(nativeRollbackInput_Init(inputs, local_mask), "initialize peer input history");
}

static void check_clock_samples(uint64_t session_id)
{
    NativeNetplayExchange local;
    NativeRollbackInputHistory inputs;
    require(nativeNetplayExchange_Init(&local, session_id, 0, 0xD) &&
                nativeNetplayExchange_AddPeer(&local, 1, 0x2),
            "initialize clock observation peer");
    initialize_inputs(&inputs, 0xD);

    PADStatus pads[4] = { 0 };
    require(nativeNetplayExchange_RecordLocal(&local, 24, pads),
            "sample the local clock at the return leg");
    NativeNetplayInputBatch sample = {
        .session_id = session_id,
        .start_frame = 18,
        .sample_frame = 19,
        .observed_frames = { 22, UINT64_MAX, UINT64_MAX, UINT64_MAX },
        .sender_peer = 1,
        .port_mask = 0x2,
        .frame_count = 1,
    };
    uint8_t packet[NATIVE_NETPLAY_INPUT_PACKET_CAPACITY];
    size_t size = nativeNetplayInput_Encode(&sample, packet, sizeof(packet));
    require(size && nativeNetplayExchange_AcceptPacket(&local, &inputs, 1, packet, size) &&
                nativeNetplayExchange_MaxLead(&local) == 4000,
            "measure a four-frame lead after removing the round-trip allowance");

    require(nativeNetplayExchange_RecordLocal(&local, 25, pads) &&
                nativeNetplayExchange_AcceptPacket(&local, &inputs, 1, packet, size) &&
                nativeNetplayExchange_MaxLead(&local) == 4000,
            "a redundant packet does not bias the clock");
    sample.sample_frame = 18;
    size = nativeNetplayInput_Encode(&sample, packet, sizeof(packet));
    require(size && nativeNetplayExchange_AcceptPacket(&local, &inputs, 1, packet, size) &&
                nativeNetplayExchange_MaxLead(&local) == 4000 &&
                local.peers[1].sample_frame == 19,
            "an out-of-order packet cannot move the peer clock backward");

    sample.has_checksum = true;
    sample.checksum_frame = sample.sample_frame;
    sample.session_id++;
    size = nativeNetplayInput_Encode(&sample, packet, sizeof(packet));
    require(size && !nativeNetplayExchange_AcceptPacket(&local, &inputs, 1, packet, size) &&
                !local.peers[1].has_checksum && local.peers[1].sample_frame == 19,
            "another session cannot publish a checksum or change the peer clock");
    sample.session_id = session_id;
    sample.start_frame = sample.sample_frame = 25 + NATIVE_ROLLBACK_INPUT_CAPACITY;
    size = nativeNetplayInput_Encode(&sample, packet, sizeof(packet));
    require(size && !nativeNetplayExchange_AcceptPacket(&local, &inputs, 1, packet, size) &&
                !local.peers[1].has_checksum && local.peers[1].sample_frame == 19,
            "use local time to bound incoming samples even before the first simulated tick");
}

static void check_delayed_samples(uint64_t session_id)
{
    NativeNetplayExchange sender;
    require(nativeNetplayExchange_Init(&sender, session_id, 0, 0xD) &&
                nativeNetplayExchange_AddPeer(&sender, 1, 0x2),
            "initialize buffered sender");
    sender.input_delay_frames = 2;
    PADStatus pads[4] = { 0 };
    pads[0] = pad(PAD_BUTTON_A, 23);
    pads[2].err = PAD_ERR_NO_CONTROLLER;
    require(nativeNetplayExchange_RecordLocal(&sender, 0, pads),
            "record first buffered controller sample");

    uint8_t packet[NATIVE_NETPLAY_INPUT_PACKET_CAPACITY];
    size_t size;
    NativeNetplayInputBatch batch;
    require(nativeNetplayExchange_PreparePacket(&sender, 1, false, packet, sizeof(packet), &size) &&
                nativeNetplayInput_Decode(&batch, packet, size) &&
                batch.sample_frame == 0 && batch.start_frame == 0 &&
                batch.frame_count == 3,
            "transmit the actual input two frames early without shifting the clock");
    require(batch.pads[0][0].button == 0 && batch.pads[1][0].stickX == 0 &&
                batch.pads[0][2].err == PAD_ERR_NO_CONTROLLER &&
                batch.pads[2][0].button == PAD_BUTTON_A &&
                batch.pads[2][0].stickX == 23,
            "send neutral startup frames followed by the captured input");
    PADStatus local[4];
    require(nativeNetplayExchange_GetLocal(&sender, 0, local) &&
                local[0].button == 0 &&
                nativeNetplayExchange_GetLocal(&sender, 2, local) &&
                local[0].button == batch.pads[2][0].button,
            "local simulation consumes the same buffered input as the wire");
    require(nativeNetplayExchange_BeginPhase(&sender, session_id + 1) &&
                sender.input_delay_frames == 2 &&
                !nativeNetplayExchange_GetLocal(&sender, 2, local),
            "preserve the configured buffer across phases and clear its inputs");
}

static void check_burst_recovery(uint64_t session_id)
{
    NativeNetplayExchange sender, receiver;
    NativeRollbackInputHistory sender_inputs, receiver_inputs;
    require(nativeNetplayExchange_Init(&sender, session_id, 0, 0xD) &&
                nativeNetplayExchange_AddPeer(&sender, 1, 0x2) &&
                nativeNetplayExchange_Init(&receiver, session_id, 1, 0x2) &&
                nativeNetplayExchange_AddPeer(&receiver, 0, 0xD),
            "initialize loss-recovery peers");
    initialize_inputs(&sender_inputs, 0xD);
    initialize_inputs(&receiver_inputs, 0x2);
    sender.input_delay_frames = receiver.input_delay_frames = 2;

    enum { LostStart = 20, Resume = 32, MissingInput = LostStart + 2 };
    uint8_t packet[NATIVE_NETPLAY_INPUT_PACKET_CAPACITY];
    uint8_t stale_ack[NATIVE_NETPLAY_INPUT_PACKET_CAPACITY];
    size_t size, stale_size = 0;
    PADStatus neutral[4] = { 0 };
    PADStatus resolved[4];
    for (uint64_t frame = 0; frame <= Resume; ++frame) {
        PADStatus local[4] = { 0 };
        if (frame == LostStart) local[0].button = PAD_BUTTON_A;
        require(nativeNetplayExchange_RecordLocal(&sender, frame, local) &&
                    nativeNetplayExchange_RecordLocal(&receiver, frame, neutral),
                "record both samples through a one-way burst");
        require(nativeNetplayExchange_PreparePacket(&sender, 1, false,
                    packet, sizeof(packet), &size) && size,
                "always prepare the newest inputs first");
        // Simulate twelve dropped sends (including any repair packets).
        if (frame < LostStart || frame == Resume) {
            require(nativeNetplayExchange_AcceptPacket(&receiver, &receiver_inputs,
                        0, packet, size), "receive the normal input stream");
        }
        require(nativeRollbackInput_PrepareFrame(&receiver_inputs, frame, neutral,
                    resolved), "keep predicting while input is missing");
        require(nativeNetplayExchange_PreparePacket(&receiver, 0, false,
                    packet, sizeof(packet), &size) && size &&
                    nativeNetplayExchange_AcceptPacket(&sender, &sender_inputs,
                        1, packet, size), "receive acknowledgements while the other direction is impaired");
        if (frame == LostStart - 1) {
            stale_size = size;
            for (size_t i = 0; i < size; ++i) stale_ack[i] = packet[i];
        }
    }

    const NativeNetplayInputAck before = sender.peers[1].acknowledged;
    uint64_t boundary;
    require(before.boundary == MissingInput && before.selective != 0 &&
                nativeRollbackInput_ConfirmedBoundary(&receiver_inputs, 0, Resume + 1,
                    0xD, &boundary) && boundary == MissingInput,
            "newer input is selectively acknowledged without skipping the lost frame");
    require(nativeNetplayExchange_AcceptPacket(&sender, &sender_inputs, 1,
                stale_ack, stale_size) &&
                sender.peers[1].acknowledged.selective == before.selective,
            "reordered acknowledgements do not erase newer confirmations");

    NativeNetplayInputBatch repair;
    require(nativeNetplayExchange_PreparePacket(&sender, 1, true,
                packet, sizeof(packet), &size) && size &&
                nativeNetplayInput_Decode(&repair, packet, size) &&
                repair.repair && repair.start_frame == MissingInput &&
                repair.frame_count == 5 &&
                nativeNetplayExchange_AcceptPacket(&receiver, &receiver_inputs, 0, packet, size),
            "repair the precise older interval omitted by the normal packet");
    uint64_t correction;
    require(nativeRollbackInput_ConfirmedBoundary(&receiver_inputs, 0, Resume + 1,
                0xD, &boundary) && boundary == Resume + 1 &&
                nativeRollbackInput_TakeCorrection(&receiver_inputs, &correction) &&
                correction == MissingInput &&
                nativeRollbackInput_GetFrame(&receiver_inputs, MissingInput, resolved) &&
                resolved[0].button == PAD_BUTTON_A,
            "repair frees the checkpoint boundary and restores the lost button edge");
    require(nativeNetplayExchange_PreparePacket(&receiver, 0, false,
                packet, sizeof(packet), &size) && size &&
                nativeNetplayExchange_AcceptPacket(&sender, &sender_inputs, 1, packet, size) &&
                sender.peers[1].acknowledged.boundary == Resume + 3 &&
                nativeNetplayExchange_PreparePacket(&sender, 1, true,
                    packet, sizeof(packet), &size) && size == 0,
            "a fresh acknowledgement retires the repair without another tick");
}

int main(void)
{
    const uint64_t session_id = UINT64_C(0x0123456789ABCDEF);
    check_clock_samples(session_id);
    check_delayed_samples(session_id);
    check_burst_recovery(session_id);
    NativeNetplayUdp first;
    NativeNetplayUdp second;
    require(
        nativeNetplayUdp_Init(&first, 0, session_id, 0, (1U << 0) | (1U << 2)),
        "open first nonblocking UDP endpoint");
    require(nativeNetplayUdp_Init(&second, 0, session_id, 1,
                                  (1U << 1) | (1U << 3)),
            "open second nonblocking UDP endpoint");
    require(nativeNetplayUdp_BoundPort(&first) != 0 &&
                nativeNetplayUdp_BoundPort(&second) != 0 &&
                nativeNetplayUdp_BoundPort(&first) !=
                    nativeNetplayUdp_BoundPort(&second),
            "operating system assigns distinct loopback ports");
    require(nativeNetplayUdp_AddPeer(&first, 1, "127.0.0.1",
                                     nativeNetplayUdp_BoundPort(&second),
                                     (1U << 1) | (1U << 3)) &&
                nativeNetplayUdp_AddPeer(&second, 0, "127.0.0.1",
                                         nativeNetplayUdp_BoundPort(&first),
                                         (1U << 0) | (1U << 2)),
            "assign each peer its remote fighter slots");
    require(!nativeNetplayUdp_AddPeer(&first, 2, "127.0.0.1",
                                      nativeNetplayUdp_BoundPort(&second),
                                      1U << 3),
            "reject overlapping remote fighter ownership");

    NativeRollbackInputHistory first_inputs;
    NativeRollbackInputHistory second_inputs;
    initialize_inputs(&first_inputs, (1U << 0) | (1U << 2));
    initialize_inputs(&second_inputs, (1U << 1) | (1U << 3));

    PADStatus first_pads[4] = {
        pad(PAD_BUTTON_A, 10),
        { 0 },
        pad(PAD_BUTTON_X, 30),
        { 0 },
    };
    PADStatus second_pads[4] = {
        { 0 },
        pad(PAD_BUTTON_B, 20),
        { 0 },
        pad(PAD_BUTTON_Y, 40),
    };
    require(nativeNetplayExchange_RecordLocal(&first.exchange, 0, first_pads) &&
                nativeNetplayExchange_RecordLocal(&second.exchange, 0,
                                                  second_pads),
            "record each PC's two local fighter slots");
    require(nativeNetplayExchange_SetChecksum(
                &first.exchange, 0, UINT64_C(0x123456789ABCDEF0)) &&
                !nativeNetplayExchange_SetChecksum(
                    &first.exchange, 0, UINT64_C(0x123456789ABCDEF0)),
            "publish each confirmed checksum frame only once");
    require(nativeNetplayUdp_Send(&first) && nativeNetplayUdp_Send(&second),
            "exchange first-frame datagrams");
    require(nativeNetplayUdp_Receive(&first, &first_inputs) &&
                nativeNetplayUdp_Receive(&second, &second_inputs),
            "poll both endpoints without blocking");
    require(first.exchange.received_packets == 1 &&
                second.exchange.received_packets == 1,
            "each endpoint accepts its configured peer");
    require(second.exchange.peers[0].has_checksum &&
                second.exchange.peers[0].checksum_frame == 0 &&
                second.exchange.peers[0].checksum ==
                    UINT64_C(0x123456789ABCDEF0),
            "receiver retains the peer's newest semantic checksum");
    PADStatus resolved[4];
    require(nativeRollbackInput_PrepareFrame(&first_inputs, 0,
                                             first_pads, resolved),
            "resolve first peer frame 0");
    require(resolved[0].button == PAD_BUTTON_A &&
                resolved[1].button == PAD_BUTTON_B &&
                resolved[2].button == PAD_BUTTON_X &&
                resolved[3].button == PAD_BUTTON_Y,
            "first peer combines two local and two remote slots");
    require(nativeRollbackInput_PrepareFrame(&second_inputs, 0,
                                             second_pads, resolved),
            "resolve second peer frame 0");
    require(resolved[0].button == PAD_BUTTON_A &&
                resolved[1].button == PAD_BUTTON_B &&
                resolved[2].button == PAD_BUTTON_X &&
                resolved[3].button == PAD_BUTTON_Y,
            "second peer resolves the same four-player input");

    first_pads[0] = pad(PAD_BUTTON_START, 11);
    first_pads[2] = pad(PAD_TRIGGER_Z, 31);
    require(nativeNetplayExchange_RecordLocal(&first.exchange, 1, first_pads),
            "record a deliberately unsent frame");
    first_pads[0] = pad(PAD_BUTTON_LEFT, 12);
    first_pads[2] = pad(PAD_BUTTON_RIGHT, 32);
    require(nativeNetplayExchange_RecordLocal(&first.exchange, 2, first_pads) &&
                nativeNetplayExchange_SetChecksum(&first.exchange, 1,
                    UINT64_C(0x0FEDCBA987654321)) &&
                nativeNetplayUdp_Send(&first) &&
                nativeNetplayUdp_Receive(&second, &second_inputs),
            "next datagram redundantly delivers the skipped frame");
    require(second.exchange.peers[0].pending_checksums[0].valid &&
                second.exchange.peers[0].pending_checksums[0].value ==
                    UINT64_C(0x123456789ABCDEF0) &&
                second.exchange.peers[0].pending_checksums[1].valid &&
                second.exchange.peers[0].pending_checksums[1].value ==
                    UINT64_C(0x0FEDCBA987654321),
            "retain both peer checksums until delayed simulation catches up");
    PADStatus neutral[4] = { 0 };
    require(nativeRollbackInput_PrepareFrame(&second_inputs, 1,
                                             neutral, resolved) &&
                resolved[0].button == PAD_BUTTON_START &&
                resolved[2].button == PAD_TRIGGER_Z,
            "redundancy recovers frame 1");
    require(nativeRollbackInput_PrepareFrame(&second_inputs, 2,
                                             neutral, resolved) &&
                resolved[0].button == PAD_BUTTON_LEFT &&
                resolved[2].button == PAD_BUTTON_RIGHT,
            "same datagram carries the newest frame");
    require(nativeNetplayExchange_SetArtificialDelay(
                &first.exchange, NativeConfirmedCheckpoint_MaxPrediction - 1) &&
                !nativeNetplayExchange_SetArtificialDelay(
                    &first.exchange, NativeConfirmedCheckpoint_MaxPrediction),
            "diagnostic delay remains inside the reliable prediction lead");

    require(nativeNetplayExchange_SetArtificialDelay(&first.exchange, 0),
            "remove diagnostic delay before the scene transition");
    uint8_t old_packet[NATIVE_NETPLAY_INPUT_PACKET_CAPACITY];
    size_t old_packet_size = 0;
    require(nativeNetplayExchange_PreparePacket(
                &first.exchange, 1, false, old_packet, sizeof(old_packet),
                &old_packet_size) && old_packet_size != 0,
            "retain one old-scene packet for phase isolation");
    const uint64_t sudden_death_id = session_id ^ UINT64_C(0x53554444454E0001);
    require(nativeNetplayExchange_BeginPhase(&first.exchange,
                                              sudden_death_id) &&
                nativeNetplayExchange_BeginPhase(&second.exchange,
                                                  sudden_death_id),
            "start a fresh input timeline on both existing transports");
    initialize_inputs(&first_inputs, (1U << 0) | (1U << 2));
    initialize_inputs(&second_inputs, (1U << 1) | (1U << 3));
    require(nativeNetplayExchange_AcceptPacket(
                &second.exchange, &second_inputs, 0, old_packet,
                old_packet_size) &&
                second.exchange.stale_packets == 1 &&
                second.exchange.rejected_packets == 0,
            "old-scene input cannot enter sudden-death rollback history");
    require(first.exchange.peers[1].configured &&
                second.exchange.peers[0].configured &&
                !first.exchange.has_newest_frame &&
                !second.exchange.has_newest_frame &&
                !second.exchange.peers[0].has_checksum,
            "phase reset keeps peer ownership but clears frame and checksum state");
    require(nativeNetplayExchange_RecordLocal(&first.exchange, 0,
                                              first_pads) &&
                nativeNetplayExchange_RecordLocal(&second.exchange, 0,
                                                  second_pads) &&
                nativeNetplayUdp_Send(&first) &&
                nativeNetplayUdp_Send(&second) &&
                nativeNetplayUdp_Receive(&first, &first_inputs) &&
                nativeNetplayUdp_Receive(&second, &second_inputs),
            "reuse the same UDP sockets for sudden-death frame zero");
    require(first.exchange.received_packets == 1 &&
                second.exchange.received_packets == 1,
            "both endpoints accept new-phase input after timeline reset");

    nativeNetplayUdp_Shutdown(&second);
    nativeNetplayUdp_Shutdown(&first);
    puts("Nonblocking UDP exchanges redundant shared-PC input batches");
    return 0;
}
