#ifndef HSD_NATIVE_RANDOM_SNAPSHOT_H
#define HSD_NATIVE_RANDOM_SNAPSHOT_H

#include <Runtime/platform.h>
#include <stdint.h>

// The native game's only seed binding is random.c's own seed. Reject any
// external binding instead of silently dropping its mutable target value.
typedef struct NativeRandomSnapshot {
    u32 seed, reserved;
} NativeRandomSnapshot;

bool nativeRandom_Save(void*, size_t capacity, size_t* written);
bool nativeRandom_ValidateSnapshot(const void*, size_t bytes);
bool nativeRandom_Load(const void*, size_t bytes);
// Host diagnostic context, not game state. Never changes random results.
void nativeRandom_TraceFrame(uint64_t frame, bool enabled, bool replaying);
void nativeRandom_TraceBytes(const char* name,const void* data,size_t bytes);
void nativeRandom_TraceEvent(const char* name,void* caller,const void* object,int value);

#endif
