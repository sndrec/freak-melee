#ifndef SCRIPT_TARGET_EXPORT_FIXTURE_H
#define SCRIPT_TARGET_EXPORT_FIXTURE_H
#include <melee/ft/native_source_package.h>
#include <sysdolphin/baselib/gobj.h>
extern NativeSourcePackage target_packages[3];
void target_guard_probe(NativeScriptRuntimeContext*);
void target_write_probe(NativeScriptRuntimeContext*);
#endif
