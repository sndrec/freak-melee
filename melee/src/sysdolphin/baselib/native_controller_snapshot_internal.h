#ifndef HSD_NATIVE_CONTROLLER_SNAPSHOT_INTERNAL_H
#define HSD_NATIVE_CONTROLLER_SNAPSHOT_INTERNAL_H

#include "controller.h"

// Actual native input callback and activity, not parallel controller state.
extern HSD_PadRawFilter nativeControllerRawFilter;
extern u32 nativeControllerInputDepth;
void nativeController_Bind(u8 queue_count, HSD_PadData* queue,
                           u16 rumble_count, HSD_PadRumbleListData* rumble);

#endif
