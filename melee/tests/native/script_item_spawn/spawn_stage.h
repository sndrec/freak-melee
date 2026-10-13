#ifndef SCRIPT_ITEM_SPAWN_STAGE_H
#define SCRIPT_ITEM_SPAWN_STAGE_H
enum { StageConstruct=1,StageSpawned,StageState,StageAppearance,StageCreated,StageDebug,
       StageUpdate,StageLaunched,StagePickup,StageInventory };
static void stage(unsigned value);
#endif
