#ifndef SCRIPT_ARTICLE_COMBAT_FIXTURE_H
#define SCRIPT_ARTICLE_COMBAT_FIXTURE_H
#include <melee/ft/native_source_package.h>
#include <melee/pl/types.h>
#include <sysdolphin/baselib/gobj.h>
extern NativeSourcePackage combat_package;
extern StaleMoveTable combat_stale;
extern unsigned combat_probe_calls,combat_mutation;
extern HSD_GObj* combat_source,*combat_owner;
extern HSD_GObj* (*combat_reuse)(void);
void combat_native_completed(HSD_GObj*);
#endif
