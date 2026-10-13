#ifndef MELEE_LB_NATIVE_INPUT_CLOCK_SNAPSHOT_H
#define MELEE_LB_NATIVE_INPUT_CLOCK_SNAPSHOT_H

#include <Runtime/platform.h>

// Private in-memory state, not a replay/network format. Restore only at an
// idle game-thread boundary; the controller queue is a separate companion.
typedef struct NativeInputClockSnapshot {
    u64 game_period_ns, sample_period_ns, next_sample_ns, virtual_time_ns;
    u64 phase_ns[2];
    u8 update[2], sampled, scheduled, running;
    u8 reserved[3];
} NativeInputClockSnapshot;

bool nativeInputClock_Save(void*, size_t capacity, size_t* written);
bool nativeInputClock_ValidateSnapshot(const void*, size_t bytes);
bool nativeInputClock_Load(const void*, size_t bytes);

#endif
