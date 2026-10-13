#ifndef MELEE_EF_EFSYNC_H
#define MELEE_EF_EFSYNC_H

#include <melee/ef/types.h>

EF_SpawnResult efSync_Spawn(s32 gfx_id, HSD_GObj* arg_gobj, ...);
const EF_ModelRecipe* efSync_ModelRecipe(s32 gfx_id);

#endif
