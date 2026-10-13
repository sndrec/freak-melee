#ifndef HELD_CONTINUATION_FIXTURE_H
#define HELD_CONTINUATION_FIXTURE_H
#include <melee/ft/native_source_package.h>
extern NativeSourcePackage lifecycle_package;
extern unsigned held_mode,native_tail,status_reads,read_trigger;
void held_mutation(HSD_GObj*);
void held_motion_block(Fighter_GObj*);
#endif
