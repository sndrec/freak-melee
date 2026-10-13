#ifndef DYNAMICS_FIXTURE_H
#define DYNAMICS_FIXTURE_H
#include <melee/ft/native_source_package.h>
extern NativeSourcePackage packages[3];
void dynamics_observe(HSD_GObj*);
void dynamics_step(DynamicsDesc*,void*,int,float,bool,Fighter_Part,int,bool);
#endif
