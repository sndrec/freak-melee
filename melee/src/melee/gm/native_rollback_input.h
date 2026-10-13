#ifndef MELEE_GM_NATIVE_ROLLBACK_INPUT_H
#define MELEE_GM_NATIVE_ROLLBACK_INPUT_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stdint.h>

#include <dolphin/pad.h>

enum {
    NATIVE_ROLLBACK_INPUT_PORTS = 4,
    // Network retention includes outbound delay/redundancy and a checksum's
    // round trip, not just the fifteen ticks that may need reconstruction.
    NATIVE_ROLLBACK_INPUT_CAPACITY = 64,
};

typedef struct NativeRollbackInputFrame {
    PADStatus pads[NATIVE_ROLLBACK_INPUT_PORTS];
    uint64_t frame;
    uint8_t confirmed_mask;
    uint8_t predicted_mask;
    bool valid;
    bool prepared;
} NativeRollbackInputFrame;

typedef struct NativeRollbackInputHistory {
    NativeRollbackInputFrame frames[NATIVE_ROLLBACK_INPUT_CAPACITY];
    uint64_t latest_confirmed_frame[NATIVE_ROLLBACK_INPUT_PORTS];
    uint64_t newest_frame;
    uint64_t correction_frame;
    uint64_t oldest_required_frame;
    uint8_t local_mask;
    uint8_t has_latest_confirmed_mask;
    bool has_newest_frame;
    bool has_correction;
} NativeRollbackInputHistory;

bool nativeRollbackInput_Init(NativeRollbackInputHistory* history,
                              uint8_t local_mask);
bool nativeRollbackInput_PrepareFrame(NativeRollbackInputHistory* history,
                                      uint64_t frame,
                                      const PADStatus local_pads[4],
                                      PADStatus result[4]);
bool nativeRollbackInput_SubmitRemote(NativeRollbackInputHistory* history,
                                      uint64_t frame, uint32_t port,
                                      const PADStatus* input);
bool nativeRollbackInput_GetFrame(const NativeRollbackInputHistory* history,
                                  uint64_t frame, PADStatus result[4]);
bool nativeRollbackInput_TakeCorrection(NativeRollbackInputHistory* history,
                                        uint64_t* frame);
uint8_t
nativeRollbackInput_ConfirmedMask(const NativeRollbackInputHistory* history,
                                  uint64_t frame);
bool nativeRollbackInput_LatestConfirmedFrame(
    const NativeRollbackInputHistory* history, uint8_t port_mask,
    uint64_t* frame);
// Scan only retained, already simulated inputs. Boundary C includes frames
// before C; later complete packets never bridge an earlier missing input.
bool nativeRollbackInput_ConfirmedBoundary(
    const NativeRollbackInputHistory*, uint64_t start, uint64_t end,
    uint8_t port_mask, uint64_t* boundary);
// The owning rollback policy releases inputs only after its saved boundary moves.
// Future packets cannot overwrite a still-required frame sharing their ring slot.
bool nativeRollbackInput_ReleaseBefore(NativeRollbackInputHistory*,uint64_t boundary);

#endif
#endif
