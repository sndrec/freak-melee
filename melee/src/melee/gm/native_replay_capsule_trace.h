#ifndef MELEE_GM_NATIVE_REPLAY_CAPSULE_TRACE_H
#define MELEE_GM_NATIVE_REPLAY_CAPSULE_TRACE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

struct Fighter;
struct HurtCapsule;

enum { NativeReplay_StateTraceFrames = 64 };

#ifdef NATIVE_NETPLAY_DIAGNOSTICS
typedef enum NativeHurtboxTransformSource {
    NativeHurtboxTransform_HitTest = 1,
    NativeHurtboxTransform_GrabTest,
    NativeHurtboxTransform_ContactTest,
    NativeHurtboxTransform_FighterBounds,
    NativeHurtboxTransform_DebugDraw,
} NativeHurtboxTransformSource;

void nativeReplayCapsuleTrace_BeginTick(uint64_t frame);
void nativeReplayCapsuleTrace_EndMatch(void);
void nativeReplayCapsuleTrace_RecordTransform(const struct HurtCapsule* capsule,
                                             NativeHurtboxTransformSource source);

void nativeReplayCapsuleTrace_Capture(uint64_t frame, unsigned actor,
                                     const struct Fighter* fighter);
void nativeReplayCapsuleTrace_WriteColumns(FILE* output);
bool nativeReplayCapsuleTrace_WriteFrame(FILE* output, uint64_t frame,
                                        unsigned actor_count, size_t* remaining);
#endif

#endif
