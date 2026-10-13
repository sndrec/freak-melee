#ifndef SCRIPT_ITEM_SCHEDULER_BOUNDARY_H
#define SCRIPT_ITEM_SCHEDULER_BOUNDARY_H
#include <melee/it/native_script_item_actor.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobj.h>
extern unsigned scheduler_tail,scheduler_native,scheduler_frees,scheduler_models;
extern bool scheduler_remove,scheduler_nested,scheduler_fail_selector;
extern bool scheduler_fail_write;
extern bool scheduler_phase_transition;
extern HSD_GObj* scheduler_retire_target;
extern HSD_GObj* scheduler_remove_other;
extern HSD_GObj* scheduler_clear_owner;
extern HSD_GObj* (*scheduler_spawn_on_write)(void);
extern int32_t scheduler_destroyed;
extern const NativeScriptItemProgram* scheduler_spec;
void scheduler_userdata_remove(void*);
void scheduler_animation(HSD_GObj*);
void scheduler_accessory(HSD_GObj*);
void scheduler_hitlag(HSD_GObj*,bool enter);
void scheduler_damage_pipeline(HSD_GObj*);
void scheduler_grab(HSD_GObj*);
void scheduler_jumped(HSD_GObj*);
void scheduler_damage(HSD_GObj*,uint32_t hook);
void scheduler_collision(HSD_GObj*);
void scheduler_physics(HSD_GObj*);
void scheduler_state(HSD_GObj*,int state);
bool scheduler_spawned(HSD_GObj*);
HSD_GObj* scheduler_construct(SpawnItem*);
void scheduler_reset(void);
extern unsigned scheduler_stock_assets;
#endif
