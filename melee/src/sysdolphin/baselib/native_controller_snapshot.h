#ifndef HSD_NATIVE_CONTROLLER_SNAPSHOT_H
#define HSD_NATIVE_CONTROLLER_SNAPSHOT_H

#include "controller.h"
#include <stdint.h>

// Private game-thread component. Queue/list backing and the raw-input filter
// remain fixed while history is retained. Command bytes referenced by rumble
// head/list/stack pointers belong to resident definitions, not this pool.
typedef struct NativeControllerSnapshotHeader {
    uint64_t epoch;
    PadLibData library;
    HSD_PadStatus master[4], copy[4], game[4];
    HSD_RumbleData rumble[4];
} NativeControllerSnapshotHeader;

size_t nativeController_SnapshotBytes(void);
bool nativeController_Save(void*, size_t capacity, size_t* written);
bool nativeController_ValidateSnapshot(const void*, size_t bytes);
bool nativeController_Load(const void*, size_t bytes);

#endif
