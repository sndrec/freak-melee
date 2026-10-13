#ifndef SCRIPT_TARGET_QUERY_FIXTURE_H
#define SCRIPT_TARGET_QUERY_FIXTURE_H
#include <melee/ft/native_source_package.h>
#include <sysdolphin/baselib/gobj.h>
extern NativeSourcePackage packages[2];
extern unsigned guard_calls,mutate_at,mutation;
extern bool ahead_teams;
extern HSD_GObj* query_caller,*query_target,*replacement;
extern HSD_GObj* (*target_reuse)(void);
void target_guard_probe(NativeScriptRuntimeContext*);
void bone_native_observe(void);
void bone_install_dummy(HSD_RObj*);
#endif
