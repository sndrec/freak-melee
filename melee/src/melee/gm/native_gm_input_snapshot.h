#ifndef MELEE_GM_NATIVE_GM_INPUT_SNAPSHOT_H
#define MELEE_GM_NATIVE_GM_INPUT_SNAPSHOT_H

#include <Runtime/platform.h>
#include <dolphin/pad.h>

struct gm_controller_map {
    u64 button, trigger, repeat, release, repeat2;
    s32 repeat_timer, held_frames;
};

// Actual mapped-controller state: five ports include the aggregate port.
// Callback is the resident repeat algorithm; no second mapping representation.
typedef struct NativeGmInputSnapshot {
    struct gm_controller_map ports[PAD_MAX_CONTROLLERS + 1];
    void (*repeat_callback)(int);
    u16 repeat_delay;
    u8 early_interval;
    u16 middle_threshold;
    u8 middle_interval;
    u16 fast_threshold;
    u8 fast_interval;
} NativeGmInputSnapshot;

bool nativeGmInput_Save(void*, size_t capacity, size_t* written);
bool nativeGmInput_ValidateSnapshot(const void*, size_t bytes);
bool nativeGmInput_Load(const void*, size_t bytes);

#endif
