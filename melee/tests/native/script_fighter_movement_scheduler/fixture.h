#ifndef MOVEMENT_FIXTURE_H
#define MOVEMENT_FIXTURE_H
#include <melee/ft/native_source_package.h>
extern NativeSourcePackage movement_package;
extern unsigned mutation_point,mutation_mode,write_count,grounded_tail,timer_tail,trace,drop_calls,float_calls,donor_calls,animation_calls;
extern bool invoke_grounded,dead_fall;
extern HSD_GObj* replacement;
void movement_mutate(HSD_GObj*,unsigned);
void movement_grounded(Fighter_GObj*);
void movement_timer(Fighter_GObj*);
#endif
