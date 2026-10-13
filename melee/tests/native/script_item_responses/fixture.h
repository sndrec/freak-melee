#ifndef SCRIPT_ITEM_RESPONSES_FIXTURE_H
#define SCRIPT_ITEM_RESPONSES_FIXTURE_H
#include <melee/ft/native_source_package.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobj.h>
extern NativeSourcePackage target_packages[3];
extern ItemCommonData response_common;
extern unsigned response_sound,response_effect,response_damage,response_geometry;
extern uint32_t response_order;
void target_guard_probe(NativeScriptRuntimeContext*);
void response_kernel_probe(Item_GObj*);
void response_relation_probe(Item_GObj*);
void response_reference_bounce(Item_GObj*);
void response_reference_stage(Item_GObj*,s32);
#endif
