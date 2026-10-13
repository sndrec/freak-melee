#ifndef SCRIPT_FIGHTER_SCHEDULER_BOUNDARY_H
#define SCRIPT_FIGHTER_SCHEDULER_BOUNDARY_H
#include <melee/it/native_script_item_actor.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobj.h>
extern unsigned scheduler_tail,scheduler_native,scheduler_frees,scheduler_models;
extern bool scheduler_remove,scheduler_nested,scheduler_fail_selector;
extern bool scheduler_fail_write;
extern bool scheduler_overwrite_landing;
extern bool scheduler_phase_transition;
extern HSD_GObj* scheduler_retire_target;
extern HSD_GObj* scheduler_remove_other;
extern HSD_GObj* scheduler_clear_owner;
extern HSD_GObj* (*scheduler_spawn_on_write)(void);
extern int32_t scheduler_destroyed;
extern const NativeScriptItemProgram* scheduler_spec;
void scheduler_userdata_remove(void*);
void scheduler_reset(void);
void fighter_scheduler_legacy(void);
extern HSD_GObj* scheduler_aux_seen_first;
extern HSD_GObj* scheduler_aux_seen_second;
extern HSD_GObj* scheduler_aux_spawn;
extern int scheduler_aux_seen_timer;
extern unsigned scheduler_aux_tail;
extern uint32_t scheduler_jab_flags;
void fighter_scheduler_aux_native(bool);
extern unsigned fighter_stock_calls,fighter_cpu_tail,fighter_bookkeeping,fighter_accessory_tail;
extern unsigned fighter_input_calls,fighter_input_stop;
extern unsigned holdable_animation_leaves,holdable_collision_response,holdable_collision_queries;
extern int holdable_contact;
extern bool holdable_retire_in_state,holdable_fail_in_state;
extern bool holdable_retire_in_contact,holdable_retire_in_response;
void holdable_legacy_recipe(const struct NativeItemDefinition*);
void holdable_legacy_pick(HSD_GObj*);void holdable_legacy_drop(HSD_GObj*);void holdable_legacy_throw(HSD_GObj*);
bool holdable_legacy_animation(HSD_GObj*);void holdable_legacy_physics(HSD_GObj*);bool holdable_legacy_collision(HSD_GObj*);
bool holdable_legacy_hit(HSD_GObj*);bool holdable_legacy_clank(HSD_GObj*);bool holdable_legacy_reflect(HSD_GObj*);bool holdable_legacy_shield(HSD_GObj*);
const ItemStateTable* holdable_legacy_states(void);
#endif
