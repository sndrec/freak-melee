#ifndef SCRIPT_WORLD_FEEDBACK_FIXTURE_H
#define SCRIPT_WORLD_FEEDBACK_FIXTURE_H
#include <melee/ft/native_source_package.h>
#include <melee/cm/types.h>
#include <melee/gr/types.h>
#include <melee/pl/types.h>
#include <melee/pl/player.h>
#include <sysdolphin/baselib/dat_rumble.h>
#include <sysdolphin/baselib/gobj.h>
extern NativeSourcePackage feedback_package;
extern Camera game_camera;
extern StageInfo stage_info;
extern StaticPlayer player_slots[Gm_Player_NumMax];
extern HSD_RumblePattern rumble_patterns[40];
extern unsigned model_loads,model_frees,animation_adds,animation_requests,animation_ticks,loop_flags,hardware_calls;
extern int last_controller,last_id,last_duration,last_priority;
extern const void* last_commands;
extern unsigned feedback_mutation;
extern HSD_GObj* feedback_source,*feedback_owner,*feedback_replacement;
extern HSD_GObj* (*feedback_reuse)(void);
void feedback_completed(void);
#endif
