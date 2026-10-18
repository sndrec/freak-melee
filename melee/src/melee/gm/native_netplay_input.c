#include "native_netplay_input.h"
#include "native_pad_wire.h"

#ifdef MELEE_NATIVE

#include <string.h>

enum {
    INPUT_PACKET_HEADER_SIZE = 40,
    GAMEPLAY_PORT_MASK = (1U << NATIVE_ROLLBACK_INPUT_PORTS) - 1,
};

static const uint8_t input_packet_magic[4] = { 'F', 'M', 'I', 'N' };

static void write_u64(uint8_t* output, uint64_t value)
{
    for (uint32_t byte = 0; byte < 8; ++byte) {
        output[byte] = (uint8_t) (value >> (byte * 8));
    }
}

static uint64_t read_u64(const uint8_t* input)
{
    uint64_t result = 0;
    for (uint32_t byte = 0; byte < 8; ++byte) {
        result |= (uint64_t) input[byte] << (byte * 8);
    }
    return result;
}

static uint32_t port_count(uint8_t port_mask)
{
    uint32_t result = 0;
    for (uint32_t port = 0; port < NATIVE_ROLLBACK_INPUT_PORTS; ++port) {
        result += (port_mask >> port) & 1U;
    }
    return result;
}

static size_t packet_size(uint8_t port_mask, uint8_t frame_count)
{
    return INPUT_PACKET_HEADER_SIZE + (size_t) port_count(port_mask) *
                                          frame_count * NATIVE_PAD_WIRE_BYTES;
}

static bool valid_batch_shape(const NativeNetplayInputBatch* batch)
{
    return batch != NULL && batch->sender_peer < 4 && batch->port_mask != 0 &&
           (batch->port_mask & ~GAMEPLAY_PORT_MASK) == 0 &&
           batch->frame_count > 0 &&
           batch->frame_count <= NATIVE_NETPLAY_INPUT_MAX_FRAMES &&
           batch->start_frame <=
               UINT64_MAX - (uint64_t) batch->frame_count + 1;
}

size_t nativeNetplayInput_Encode(const NativeNetplayInputBatch* batch,
                                 uint8_t* packet, size_t packet_capacity)
{
    if (!valid_batch_shape(batch) || packet == NULL) {
        return 0;
    }
    size_t size = packet_size(batch->port_mask, batch->frame_count);
    if (size > packet_capacity || size > NATIVE_NETPLAY_INPUT_PACKET_CAPACITY)
    {
        return 0;
    }
    memcpy(packet, input_packet_magic, sizeof(input_packet_magic));
    write_u64(packet + 4, batch->session_id);
    write_u64(packet + 12, batch->start_frame);
    write_u64(packet + 20, batch->checksum_frame);
    write_u64(packet + 28, batch->checksum);
    packet[36] = batch->sender_peer;
    packet[37] = batch->port_mask;
    packet[38] = batch->frame_count;
    packet[39] = batch->has_checksum ? 1 : 0;

    uint8_t* output = packet + INPUT_PACKET_HEADER_SIZE;
    for (uint32_t frame = 0; frame < batch->frame_count; ++frame) {
        for (uint32_t port = 0; port < NATIVE_ROLLBACK_INPUT_PORTS; ++port) {
            if ((batch->port_mask & (1U << port)) == 0) {
                continue;
            }
            nativePadWire_Write(output, &batch->pads[frame][port]);
            output += NATIVE_PAD_WIRE_BYTES;
        }
    }
    return size;
}

bool nativeNetplayInput_Decode(NativeNetplayInputBatch* batch,
                               const uint8_t* packet, size_t packet_size_value)
{
    if (batch == NULL || packet == NULL ||
        packet_size_value < INPUT_PACKET_HEADER_SIZE ||
        memcmp(packet, input_packet_magic, sizeof(input_packet_magic)) != 0)
    {
        return false;
    }
    NativeNetplayInputBatch decoded = {
        .session_id = read_u64(packet + 4),
        .start_frame = read_u64(packet + 12),
        .checksum_frame = read_u64(packet + 20),
        .checksum = read_u64(packet + 28),
        .sender_peer = packet[36],
        .port_mask = packet[37],
        .frame_count = packet[38],
        .has_checksum = packet[39] == 1,
    };
    if (packet[39] > 1 || !valid_batch_shape(&decoded) ||
        packet_size(decoded.port_mask, decoded.frame_count) !=
            packet_size_value)
    {
        return false;
    }

    const uint8_t* input = packet + INPUT_PACKET_HEADER_SIZE;
    for (uint32_t frame = 0; frame < decoded.frame_count; ++frame) {
        for (uint32_t port = 0; port < NATIVE_ROLLBACK_INPUT_PORTS; ++port) {
            if ((decoded.port_mask & (1U << port)) == 0) {
                continue;
            }
            nativePadWire_Read(&decoded.pads[frame][port], input);
            input += NATIVE_PAD_WIRE_BYTES;
        }
    }
    *batch = decoded;
    return true;
}

bool nativeNetplayInput_SubmitBatch(NativeRollbackInputHistory* history,
                                    const NativeNetplayInputBatch* batch,
                                    uint64_t expected_session_id,
                                    uint8_t expected_sender_peer,
                                    uint8_t expected_port_mask)
{
    if (history == NULL || !valid_batch_shape(batch) ||
        batch->session_id != expected_session_id ||
        batch->sender_peer != expected_sender_peer ||
        batch->port_mask != expected_port_mask ||
        (batch->port_mask & history->local_mask) != 0)
    {
        return false;
    }
    NativeRollbackInputHistory inputs = *history;
    for (uint32_t frame = 0; frame < batch->frame_count; ++frame) {
        uint64_t frame_number = batch->start_frame + frame;
        if (frame_number < inputs.oldest_required_frame)
        {
            continue;
        }
        for (uint32_t port = 0; port < NATIVE_ROLLBACK_INPUT_PORTS; ++port) {
            if ((batch->port_mask & (1U << port)) != 0 &&
                !nativeRollbackInput_SubmitRemote(&inputs, frame_number, port,
                                                  &batch->pads[frame][port]))
            {
                return false;
            }
        }
    }
    *history = inputs;
    return true;
}

#endif
