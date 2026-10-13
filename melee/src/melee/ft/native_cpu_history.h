#ifndef MELEE_FT_NATIVE_CPU_HISTORY_H
#define MELEE_FT_NATIVE_CPU_HISTORY_H
#include <melee/ft/forward.h>
#include <dolphin/mtx.h>
#include <stdbool.h>
#include <stdint.h>

// Existing inline CPU history storage. IDs select an explicit live input
// source; history itself owns values and never retains that actor's pointer.
bool nativeCpuHistory_Record(Fighter_GObj* receiver, uint32_t input_source);
bool nativeCpuHistory_ReadPose(Fighter_GObj*, Vec3*, float* facing);
bool nativeCpuHistory_Apply(Fighter_GObj*);
#endif
