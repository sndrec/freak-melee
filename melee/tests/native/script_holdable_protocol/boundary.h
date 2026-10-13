#ifndef SCRIPT_HOLDABLE_PROTOCOL_BOUNDARY_H
#define SCRIPT_HOLDABLE_PROTOCOL_BOUNDARY_H
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
void scheduler_reset(void);
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
