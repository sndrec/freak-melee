#ifndef MELEE_GR_STAGE_STATE_INTERNAL_H
#define MELEE_GR_STAGE_STATE_INTERNAL_H
#include "types.h"
#include <sysdolphin/baselib/mtx.h>

// Actual shared stage storage. Graph payloads remain owned by their allocators.
enum { STAGE_MAP_OBJECT_COUNT = 64, STAGE_CAMERA_COUNT = 4 };
extern StageInfo stage_info;
extern u8 stage_joint_updates[STAGE_MAP_OBJECT_COUNT];
extern s16 stage_joint_update_counter;
extern VecMtx stage_spawn_cache;
#endif
