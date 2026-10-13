#include "native_netplay_input.h"

#ifdef MELEE_NATIVE

#include <string.h>

enum {
    INPUT_PACKET_HEADER_SIZE = 40,
    INPUT_PACKET_PAD_SIZE = 15,
    GAMEPLAY_PORT_MASK = (1U << NATIVE_ROLLBACK_INPUT_PORTS) - 1,
};

static const uint8_t input_packet_magic[4] = { 'F', 'M', 'I', 'N' };

static void write_u16(uint8_t* output, uint16_t value)
{
    output[0] = (uint8_t) value;
    output[1] = (uint8_t) (value >> 8);
}

static void write_u32(uint8_t* output, uint32_t value)
{
    for (uint32_t byte = 0; byte < 4; ++byte) {
        output[byte] = (uint8_t) (value >> (byte * 8));
    }
}

static void write_u64(uint8_t* output, uint64_t value)
{
    for (uint32_t byte = 0; byte < 8; ++byte) {
        output[byte] = (uint8_t) (value >> (byte * 8));
    }
}

static uint16_t read_u16(const uint8_t* input)
{
    return (uint16_t) input[0] | (uint16_t) input[1] << 8;
}

static uint32_t read_u32(const uint8_t* input)
{
    uint32_t result = 0;
    for (uint32_t byte = 0; byte < 4; ++byte) {
        result |= (uint32_t) input[byte] << (byte * 8);
    }
    return result;
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
                                          frame_count * INPUT_PACKET_PAD_SIZE;
}

static void encode_pad(uint8_t* output, const PADStatus* pad)
{
    write_u16(output, pad->button);
    output[2] = (uint8_t) pad->stickX;
    output[3] = (uint8_t) pad->stickY;
    output[4] = (uint8_t) pad->substickX;
    output[5] = (uint8_t) pad->substickY;
    output[6] = pad->triggerLeft;
    output[7] = pad->triggerRight;
    output[8] = pad->analogA;
    output[9] = pad->analogB;
    output[10] = (uint8_t) pad->err;
    write_u32(output + 11, pad->extButton);
}

static void decode_pad(PADStatus* pad, const uint8_t* input)
{
    *pad = (PADStatus){
        .button = read_u16(input),
        .stickX = (int8_t) input[2],
        .stickY = (int8_t) input[3],
        .substickX = (int8_t) input[4],
        .substickY = (int8_t) input[5],
        .triggerLeft = input[6],
        .triggerRight = input[7],
        .analogA = input[8],
        .analogB = input[9],
        .err = (int8_t) input[10],
        .extButton = read_u32(input + 11),
    };
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
            encode_pad(output, &batch->pads[frame][port]);
            output += INPUT_PACKET_PAD_SIZE;
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
            decode_pad(&decoded.pads[frame][port], input);
            input += INPUT_PACKET_PAD_SIZE;
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
