#include "ground.h"
#include "stage.h"
#include "stage_state_internal.h"

StageInfo stage_info;
u8 stage_joint_updates[STAGE_MAP_OBJECT_COUNT];
s16 stage_joint_update_counter;
VecMtx stage_spawn_cache;

float Ground_801C0498(void)
{
    if (stage_info.param != NULL) {
        return stage_info.param->y;
    } else {
        return 1.0F;
    }
}

