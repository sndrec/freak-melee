#ifndef SCRIPT_ARTICLE_LIFECYCLE_FIXTURE_H
#define SCRIPT_ARTICLE_LIFECYCLE_FIXTURE_H
#include <melee/ft/native_source_package.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobj.h>
extern NativeSourcePackage lifecycle_package;
extern unsigned lifecycle_mode,lifecycle_pickups,lifecycle_tails,lifecycle_calls;
extern HSD_GObj* lifecycle_creator;
extern HSD_GObj* (*lifecycle_reuse)(void);
void lifecycle_mutation(HSD_GObj*);
#endif
