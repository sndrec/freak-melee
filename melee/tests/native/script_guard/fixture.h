#ifndef SCRIPT_GUARD_FIXTURE_H
#define SCRIPT_GUARD_FIXTURE_H
#include <melee/ft/native_source_package.h>
#include <sysdolphin/baselib/gobj.h>
extern NativeSourcePackage guard_packages[3];
void guard_capture_probe(NativeScriptRuntimeContext*);
void guard_write_probe(NativeScriptRuntimeContext*);
#endif
