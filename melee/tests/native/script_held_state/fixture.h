#ifndef SCRIPT_HELD_STATE_FIXTURE_H
#define SCRIPT_HELD_STATE_FIXTURE_H
#include <melee/ft/native_source_package.h>
#include <melee/ft/forward.h>
extern NativeSourcePackage held_packages[3];
extern ftCommonData held_common;
void held_transition(Fighter_GObj*,FtMotionId,MotionFlags,float,float,float,Fighter_GObj*);
void held_animation(Fighter_GObj*);
void reference_enter(Fighter_GObj*,float);
#endif
