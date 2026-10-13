#ifndef SCRIPT_TETHER_PREREQUISITES_FIXTURE_H
#define SCRIPT_TETHER_PREREQUISITES_FIXTURE_H
#include <melee/ft/native_source_package.h>
#include <sysdolphin/baselib/gobj.h>
extern NativeSourcePackage target_package;
extern unsigned guard_calls,mutate_at,mutation;
extern HSD_GObj* query_caller,*replacement;
void target_guard_probe(NativeScriptRuntimeContext*);
#endif
