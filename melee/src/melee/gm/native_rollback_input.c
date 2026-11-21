#include "native_rollback_input.h"

#ifdef MELEE_NATIVE

#include <string.h>

enum {
    GAMEPLAY_PORT_MASK = (1U << NATIVE_ROLLBACK_INPUT_PORTS) - 1,
    TRIGGER_DEADZONE = 42,
};

static NativeRollbackInputFrame*
find_frame(NativeRollbackInputHistory* history, uint64_t frame, bool create)
{
    NativeRollbackInputFrame* input =
        &history->frames[frame % NATIVE_ROLLBACK_INPUT_CAPACITY];
    if (input->valid && input->frame == frame) {
        return input;
    }
    if (!create) {
        return NULL;
    }
    if (input->valid && input->frame > frame) {
        return NULL;
    }
    if (input->valid && input->frame>=history->oldest_required_frame) return NULL;
    *input = (NativeRollbackInputFrame){
        .frame = frame,
        .valid = true,
    };
    return input;
}

static const NativeRollbackInputFrame*
find_const_frame(const NativeRollbackInputHistory* history, uint64_t frame)
{
    const NativeRollbackInputFrame* input =
        &history->frames[frame % NATIVE_ROLLBACK_INPUT_CAPACITY];
    return input->valid && input->frame == frame ? input : NULL;
}

static bool trigger_equivalent(uint8_t first, uint8_t second)
{
    return first == second ||
           (first <= TRIGGER_DEADZONE && second <= TRIGGER_DEADZONE);
}

static bool input_equivalent(const PADStatus* first, const PADStatus* second)
{
    return first->button == second->button &&
           first->stickX == second->stickX &&
           first->stickY == second->stickY &&
           first->substickX == second->substickX &&
           first->substickY == second->substickY &&
           trigger_equivalent(first->triggerLeft, second->triggerLeft) &&
           trigger_equivalent(first->triggerRight, second->triggerRight) &&
           first->analogA == second->analogA &&
           first->analogB == second->analogB && first->err == second->err &&
           first->extButton == second->extButton;
}

static PADStatus predict_input(const NativeRollbackInputHistory* history,
                               uint64_t frame, uint32_t port)
{
    uint64_t available = frame < NATIVE_ROLLBACK_INPUT_CAPACITY
                             ? frame
                             : NATIVE_ROLLBACK_INPUT_CAPACITY;
    for (uint64_t distance = 1; distance <= available; ++distance) {
        const NativeRollbackInputFrame* previous =
            find_const_frame(history, frame - distance);
        if (previous != NULL && (previous->confirmed_mask & (1U << port))) {
            return previous->pads[port];
        }
    }
    return (PADStatus){ .err = PAD_ERR_NONE };
}

static void request_correction(NativeRollbackInputHistory* history,
                               uint64_t frame)
{
    if (!history->has_correction || frame < history->correction_frame) {
        history->correction_frame = frame;
        history->has_correction = true;
    }
}

static void record_confirmation(NativeRollbackInputHistory* history,
                                uint32_t port, uint64_t frame)
{
    uint8_t bit = 1U << port;
    if (!(history->has_latest_confirmed_mask & bit) ||
        frame > history->latest_confirmed_frame[port])
    {
        history->latest_confirmed_frame[port] = frame;
        history->has_latest_confirmed_mask |= bit;
    }
}

static void propagate_prediction(NativeRollbackInputHistory* history,
                                 uint64_t frame, uint32_t port,
                                 const PADStatus* input)
{
    if (!history->has_newest_frame || frame >= history->newest_frame) {
        return;
    }
    uint64_t end = history->newest_frame;
    if (end - frame >= NATIVE_ROLLBACK_INPUT_CAPACITY) {
        end = frame + NATIVE_ROLLBACK_INPUT_CAPACITY - 1;
    }
    for (uint64_t at = frame + 1; at <= end; ++at) {
        NativeRollbackInputFrame* later = find_frame(history, at, false);
        if (later == NULL || (later->confirmed_mask & (1U << port))) {
            break;
        }
        if (later->predicted_mask & (1U << port)) {
            later->pads[port] = *input;
        }
    }
}

bool nativeRollbackInput_Init(NativeRollbackInputHistory* history,
                              uint8_t local_mask)
{
    if (history == NULL || (local_mask & ~GAMEPLAY_PORT_MASK) != 0) {
        return false;
    }
    *history = (NativeRollbackInputHistory){
        .local_mask = local_mask,
    };
    return true;
}

bool nativeRollbackInput_PrepareFrame(NativeRollbackInputHistory* history,
                                      uint64_t frame,
                                      const PADStatus local_pads[4],
                                      PADStatus result[4])
{
    if (history == NULL || local_pads == NULL || result == NULL ||
        (history->has_newest_frame && frame > history->newest_frame + 1))
    {
        return false;
    }
    NativeRollbackInputFrame* input = find_frame(history, frame, true);
    if (input == NULL) {
        return false;
    }
    if (!input->prepared) {
        for (uint32_t port = 0; port < NATIVE_ROLLBACK_INPUT_PORTS; ++port) {
            uint8_t bit = 1U << port;
            if (history->local_mask & bit) {
                input->pads[port] = local_pads[port];
                input->confirmed_mask |= bit;
                record_confirmation(history, port, frame);
            } else if (!(input->confirmed_mask & bit)) {
                input->pads[port] = predict_input(history, frame, port);
                input->predicted_mask |= bit;
            }
        }
        input->prepared = true;
    }
    memcpy(result, input->pads, sizeof(input->pads));
    if (!history->has_newest_frame || frame > history->newest_frame) {
        history->newest_frame = frame;
        history->has_newest_frame = true;
    }
    return true;
}

bool nativeRollbackInput_SubmitRemote(NativeRollbackInputHistory* history,
                                      uint64_t frame, uint32_t port,
                                      const PADStatus* input)
{
    if (history == NULL || input == NULL ||
        frame < history->oldest_required_frame || frame == UINT64_MAX ||
        port >= NATIVE_ROLLBACK_INPUT_PORTS ||
        (history->local_mask & (1U << port)) ||
        (!history->has_newest_frame && frame >= NATIVE_ROLLBACK_INPUT_CAPACITY) ||
        (frame < history->newest_frame &&
         history->newest_frame - frame >= NATIVE_ROLLBACK_INPUT_CAPACITY) ||
        (frame > history->newest_frame &&
         frame - history->newest_frame > NATIVE_ROLLBACK_INPUT_CAPACITY))
    {
        return false;
    }
    NativeRollbackInputFrame* saved = find_frame(history, frame, true);
    if (saved == NULL) {
        return false;
    }
    uint8_t bit = 1U << port;
    if ((saved->confirmed_mask & bit) != 0 &&
        !input_equivalent(&saved->pads[port], input))
    {
        return false;
    }
    if (saved->prepared && (saved->predicted_mask & bit) &&
        !input_equivalent(&saved->pads[port], input))
    {
        request_correction(history, frame);
    }
    saved->pads[port] = *input;
    saved->confirmed_mask |= bit;
    saved->predicted_mask &= ~bit;
    record_confirmation(history, port, frame);
    propagate_prediction(history, frame, port, input);
    return true;
}

bool nativeRollbackInput_GetFrame(const NativeRollbackInputHistory* history,
                                  uint64_t frame, PADStatus result[4])
{
    if (history == NULL || result == NULL) {
        return false;
    }
    const NativeRollbackInputFrame* input = find_const_frame(history, frame);
    if (input == NULL || !input->prepared) {
        return false;
    }
    memcpy(result, input->pads, sizeof(input->pads));
    return true;
}

bool nativeRollbackInput_TakeCorrection(NativeRollbackInputHistory* history,
                                        uint64_t* frame)
{
    if (history == NULL || frame == NULL || !history->has_correction) {
        return false;
    }
    *frame = history->correction_frame;
    history->has_correction = false;
    return true;
}

uint8_t
nativeRollbackInput_ConfirmedMask(const NativeRollbackInputHistory* history,
                                  uint64_t frame)
{
    if (history == NULL) {
        return 0;
    }
    const NativeRollbackInputFrame* input = find_const_frame(history, frame);
    return input != NULL ? input->confirmed_mask : 0;
}

bool nativeRollbackInput_LatestConfirmedFrame(
    const NativeRollbackInputHistory* history, uint8_t port_mask,
    uint64_t* frame)
{
    if (history == NULL || frame == NULL || port_mask == 0 ||
        (port_mask & ~GAMEPLAY_PORT_MASK) != 0 ||
        (history->has_latest_confirmed_mask & port_mask) != port_mask)
    {
        return false;
    }
    uint64_t oldest = UINT64_MAX;
    for (uint32_t port = 0; port < NATIVE_ROLLBACK_INPUT_PORTS; ++port) {
        if ((port_mask & (1U << port)) != 0 &&
            history->latest_confirmed_frame[port] < oldest)
        {
            oldest = history->latest_confirmed_frame[port];
        }
    }
    *frame = oldest;
    return true;
}

bool nativeRollbackInput_ConfirmedBoundary(
    const NativeRollbackInputHistory* history, uint64_t start, uint64_t end,
    uint8_t port_mask, uint64_t* boundary)
{
    if (!history || !boundary || !port_mask ||
        (port_mask & ~GAMEPLAY_PORT_MASK) || end < start ||
        end - start > NATIVE_ROLLBACK_INPUT_CAPACITY) return false;
    uint64_t at = start;
    for (; at < end; ++at) {
        const NativeRollbackInputFrame* input = find_const_frame(history, at);
        // A hole in actual inputs stops confirmation. A missing simulated
        // record means necessary replay inputs have already been lost.
        if (!input || !input->prepared) return false;
        if ((input->confirmed_mask & port_mask) != port_mask) break;
    }
    *boundary = at;
    return true;
}

bool nativeRollbackInput_ReleaseBefore(NativeRollbackInputHistory* history,uint64_t boundary)
{
    if (!history || boundary<history->oldest_required_frame ||
        (boundary && (!history->has_newest_frame || history->newest_frame==UINT64_MAX ||
                      boundary>history->newest_frame+1)) ||
        (history->has_correction && history->correction_frame<boundary)) return false;
    history->oldest_required_frame=boundary;
    return true;
}

#endif
