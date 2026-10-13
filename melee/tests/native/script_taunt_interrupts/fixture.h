#ifndef SCRIPT_TARGET_QUERY_FIXTURE_H
#define SCRIPT_TARGET_QUERY_FIXTURE_H
#include <melee/ft/native_source_package.h>
#include <sysdolphin/baselib/gobj.h>
extern NativeSourcePackage target_package;
extern unsigned guard_calls,mutate_at,mutation;
extern HSD_GObj* query_caller,*query_target,*replacement;
extern HSD_GObj* (*target_reuse)(void);
void target_guard_probe(NativeScriptRuntimeContext*);
void taunt_change(Fighter_GObj*,FtMotionId,MotionFlags,float,float,float,Fighter_GObj*);
bool input_check(Fighter_GObj*,unsigned);
void reference_taunt_inputs(Fighter_GObj*);
#endif
