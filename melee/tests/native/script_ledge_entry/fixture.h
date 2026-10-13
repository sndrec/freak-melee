#ifndef LEDGE_FIXTURE_H
#define LEDGE_FIXTURE_H
#include <melee/ft/native_source_package.h>
extern NativeSourcePackage ledge_package;
extern ftCommonData common_data;
extern bool valid_floor;
extern Vec3 effect_position;
void ledge_observe(HSD_GObj*,unsigned);
void reference_ftCliffCommon_80081370(Fighter_GObj*);
#endif
