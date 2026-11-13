#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct PADStatus;
void nativeFeedbackContext_Boundary(int scene, int64_t scene_tick, uint64_t rollback_tick,
                                    bool resimulating, bool online);
void nativeFeedbackContext_RecordPads(const struct PADStatus* pads);
#ifdef __cplusplus
}
#endif
