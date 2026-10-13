#ifndef SCRIPT_ARTICLE_MOTION_FIXTURE_H
#define SCRIPT_ARTICLE_MOTION_FIXTURE_H
#include <melee/ft/native_source_package.h>
#include <sysdolphin/baselib/gobj.h>
extern NativeSourcePackage motion_package;
extern unsigned motion_mode,motion_path,motion_calls,read_probe_count,read_mutation;
extern bool collision_result;
extern int db_ShowItemCollisionBubbles;
extern HSD_GObj* motion_source,*motion_owner,*motion_replacement;
extern HSD_GObj* (*motion_reuse)(void);
void motion_native_completed(HSD_GObj*);
void motion_read_probe(NativeScriptRuntimeContext*);
bool motion_reflected_entry(HSD_GObj*);
#endif
