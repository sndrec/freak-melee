#ifndef SCRIPT_ITEM_ACTOR_FIXTURE_BOUNDARY_H
#define SCRIPT_ITEM_ACTOR_FIXTURE_BOUNDARY_H
#include <melee/it/native_script_item_actor.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobj.h>
extern bool fixture_remove,fixture_transition;
extern unsigned fixture_native_frees,fixture_destroy_writes;
extern int32_t fixture_destroy_count;
extern bool fixture_destroy_marked,fixture_deferred_alive;
void fixture_userdata_remove(void*);
#endif
