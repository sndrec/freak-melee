#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <melee/gm/native_netplay_input.h>

static void require(bool condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

static PADStatus pad(uint16_t button, int8_t stick_x, int8_t stick_y,
                     uint8_t trigger, uint32_t extension)
{
    return (PADStatus){
        .button = button,
        .stickX = stick_x,
        .stickY = stick_y,
        .substickX = (int8_t) -stick_x,
        .substickY = (int8_t) -stick_y,
        .triggerLeft = trigger,
        .triggerRight = (uint8_t) (trigger + 1),
        .analogA = (uint8_t) (trigger + 2),
        .analogB = (uint8_t) (trigger + 3),
        .err = PAD_ERR_NONE,
        .extButton = extension,
    };
}

static bool pads_equal(const PADStatus* first, const PADStatus* second)
{
    return first->button == second->button &&
           first->stickX == second->stickX &&
           first->stickY == second->stickY &&
           first->substickX == second->substickX &&
           first->substickY == second->substickY &&
           first->triggerLeft == second->triggerLeft &&
           first->triggerRight == second->triggerRight &&
           first->analogA == second->analogA &&
           first->analogB == second->analogB && first->err == second->err &&
           first->extButton == second->extButton;
}

int main(void)
{
    NativeNetplayInputBatch sent = {
        .session_id = UINT64_C(0xFEDCBA9876543210),
        .start_frame = 42,
        .checksum_frame = 40,
        .checksum = UINT64_C(0x0123456789ABCDEF),
        .sample_frame = 47,
        .observed_frames = { 45, 49, UINT64_MAX, 44 },
        .sender_peer = 2,
        .port_mask = (1U << 0) | (1U << 2),
        .frame_count = 3,
        .has_checksum = true,
    };
    for (uint32_t frame = 0; frame < sent.frame_count; ++frame) {
        sent.pads[frame][0] =
            pad((uint16_t) (PAD_BUTTON_A << frame), (int8_t) (-20 + frame),
                (int8_t) (30 - frame), (uint8_t) (50 + frame),
                0xA0000000U + frame);
        sent.pads[frame][2] =
            pad((uint16_t) (PAD_BUTTON_X >> frame), (int8_t) (40 - frame),
                (int8_t) (-50 + frame), (uint8_t) (70 + frame),
                0xC0000000U + frame);
    }

    uint8_t packet[NATIVE_NETPLAY_INPUT_PACKET_CAPACITY];
    size_t packet_size =
        nativeNetplayInput_Encode(&sent, packet, sizeof(packet));
    require(packet_size == 80 + 3 * 2 * 15,
            "encode compact explicit controller fields");
    NativeNetplayInputBatch received;
    require(nativeNetplayInput_Decode(&received, packet, packet_size),
            "decode input batch");
    require(received.session_id == sent.session_id &&
                received.start_frame == sent.start_frame &&
                received.checksum_frame == sent.checksum_frame &&
                received.checksum == sent.checksum &&
                received.sample_frame == sent.sample_frame &&
                memcmp(received.observed_frames, sent.observed_frames,
                       sizeof(sent.observed_frames)) == 0 &&
                received.sender_peer == sent.sender_peer &&
                received.port_mask == sent.port_mask &&
                received.frame_count == sent.frame_count &&
                received.has_checksum,
            "round-trip packet metadata");
    for (uint32_t frame = 0; frame < sent.frame_count; ++frame) {
        require(pads_equal(&received.pads[frame][0], &sent.pads[frame][0]) &&
                    pads_equal(&received.pads[frame][2], &sent.pads[frame][2]),
                "round-trip each owned controller sample");
    }

    NativeRollbackInputHistory session;
    require(nativeRollbackInput_Init(&session, (1U << 1) | (1U << 3)),
            "initialize packet receiver input history");
    require(nativeNetplayInput_SubmitBatch(&session, &received,
                                           sent.session_id, sent.sender_peer,
                                           sent.port_mask),
            "submit validated peer-owned input batch");
    PADStatus local[4] = { 0 };
    PADStatus resolved[4];
    require(
        nativeRollbackInput_PrepareFrame(&session, 42, local, resolved),
        "prepare frame from queued network input");
    require(pads_equal(&resolved[0], &sent.pads[0][0]) &&
                pads_equal(&resolved[2], &sent.pads[0][2]),
            "queued batch confirms both peer-owned fighter slots");
    require(!nativeNetplayInput_SubmitBatch(
                &session, &received, sent.session_id + 1, sent.sender_peer,
                sent.port_mask) &&
                !nativeNetplayInput_SubmitBatch(
                    &session, &received, sent.session_id,
                    (uint8_t) (sent.sender_peer + 1), sent.port_mask) &&
                !nativeNetplayInput_SubmitBatch(
                    &session, &received, sent.session_id, sent.sender_peer,
                    (uint8_t) (1U << 0)),
            "reject the wrong session, peer, and port assignment");

    NativeRollbackInputHistory atomic;
    require(nativeRollbackInput_Init(&atomic, (1U << 1) | (1U << 3)),
            "initialize atomic receiver");
    PADStatus original_port_two = pad(PAD_BUTTON_Y, 1, 2, 3, 4);
    require(nativeRollbackInput_SubmitRemote(&atomic, 43, 2,
                                             &original_port_two),
            "queue an existing confirmation");
    NativeNetplayInputBatch contradiction = sent;
    contradiction.start_frame = 43;
    contradiction.frame_count = 1;
    contradiction.pads[0][0] = pad(PAD_BUTTON_B, 5, 6, 7, 8);
    contradiction.pads[0][2] = pad(PAD_BUTTON_A, 9, 10, 11, 12);
    require(!nativeNetplayInput_SubmitBatch(&atomic, &contradiction,
                                            sent.session_id, sent.sender_peer,
                                            sent.port_mask),
            "reject a batch containing a contradictory confirmation");
    require(
        nativeRollbackInput_PrepareFrame(&atomic, 43, local, resolved),
        "prepare after rejected batch");
    require(resolved[0].button == 0 &&
                pads_equal(&resolved[2], &original_port_two),
            "rejected batch commits none of its earlier samples");

    require(!nativeNetplayInput_Decode(&received, packet, packet_size - 1),
            "reject truncated packet");
    packet[packet_size] = 0;
    require(!nativeNetplayInput_Decode(&received, packet, packet_size + 1),
            "reject trailing packet data");
    packet[0] ^= 1;
    require(!nativeNetplayInput_Decode(&received, packet, packet_size),
            "reject the wrong packet type");
    sent.sample_frame = 43;
    require(nativeNetplayInput_Encode(&sent, packet, sizeof(packet)) == 0,
            "reject a sender clock behind its input batch");

    puts("Netplay input packets round-trip and commit atomically");
    return 0;
}
