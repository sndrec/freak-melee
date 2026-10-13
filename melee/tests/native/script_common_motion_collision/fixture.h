#ifndef SCRIPT_COMMON_MOTION_COLLISION_FIXTURE_H
#define SCRIPT_COMMON_MOTION_COLLISION_FIXTURE_H
#include <melee/ft/native_source_package.h>
#include <sysdolphin/baselib/gobj.h>
extern NativeSourcePackage target_package;
bool collision_decision(Fighter_GObj*,unsigned);
void collision_fall(Fighter_GObj*);
void reference_nudge(Fighter_GObj*);
void reference_ground_to_air(Fighter*);
bool tether_geometry(CollData*);
bool tether_environment(Fighter_GObj*);
bool tether_hammer(Fighter_GObj*);
bool tether_hook(Fighter_GObj*);
void tether_motion(Fighter_GObj*,FtMotionId,MotionFlags,float,float,float,Fighter_GObj*);
void tether_neutral(Fighter_GObj*);
void tether_cleanup(Fighter*);
GroundOrAir reference_air_contact(Fighter_GObj*);
void reference_air_catch_hit(Fighter_GObj*);
#endif
