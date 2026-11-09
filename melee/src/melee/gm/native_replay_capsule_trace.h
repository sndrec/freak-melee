#ifndef MELEE_GM_NATIVE_REPLAY_CAPSULE_TRACE_H
#define MELEE_GM_NATIVE_REPLAY_CAPSULE_TRACE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

struct Fighter;

enum { NativeReplay_StateTraceFrames = 64 };

void nativeReplayCapsuleTrace_Capture(uint64_t frame, unsigned actor,
                                     const struct Fighter* fighter);
void nativeReplayCapsuleTrace_WriteColumns(FILE* output);
bool nativeReplayCapsuleTrace_WriteFrame(FILE* output, uint64_t frame,
                                        unsigned actor_count, size_t* remaining);

#endif
