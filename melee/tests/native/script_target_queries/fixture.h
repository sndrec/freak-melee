#ifndef SCRIPT_TARGET_QUERY_FIXTURE_H
#define SCRIPT_TARGET_QUERY_FIXTURE_H
#include <melee/ft/native_source_package.h>
#include <sysdolphin/baselib/gobj.h>
extern NativeSourcePackage target_package;
extern NativeSourcePackage target_foreign_package;
extern unsigned guard_calls,mutate_at,mutation;
extern HSD_GObj* query_caller,*query_target,*replacement;
extern HSD_GObj* (*target_reuse)(void);
void target_guard_probe(NativeScriptRuntimeContext*);
#endif
