#ifndef MELEE_IT_NATIVE_HOLDABLE_H
#define MELEE_IT_NATIVE_HOLDABLE_H

#ifdef MELEE_NATIVE

#include <melee/it/kinds/types.h>
#include <melee/it/forward.h>
#include <melee/ft/forward.h>
#include <stdint.h>
#include <sysdolphin/baselib/forward.h>
#include <melee/ft/native_script_vm.h>

#ifdef __cplusplus
extern "C" {
#endif

extern ItemLogicTable nativeHoldable_Logic;
void nativeHoldable_Draw(HSD_GObj* gobj, int pass);

Item_GObj* nativeHoldable_Spawn(Fighter_GObj* fighter_gobj,
                               uint16_t definition_index);
Item_GObj* nativeHoldable_SpawnStockFood(Item_GObj* source);
// Universal held-item protocol for source-owned recipes. These are substantive
// shared engine operations; authored hooks execute through code IDs, not tables.
NativeScriptVMStatus nativeHoldable_Animation(Item_GObj*,NativeScriptValue*);
NativeScriptVMStatus nativeHoldable_Physics(Item_GObj*,NativeScriptValue*);
NativeScriptVMStatus nativeHoldable_Collision(Item_GObj*,NativeScriptValue*);
NativeScriptVMStatus nativeHoldable_PickedUp(Item_GObj*,NativeScriptValue*);
NativeScriptVMStatus nativeHoldable_Dropped(Item_GObj*,NativeScriptValue*);
NativeScriptVMStatus nativeHoldable_Thrown(Item_GObj*,NativeScriptValue*);
NativeScriptVMStatus nativeHoldable_Hit(Item_GObj*,NativeScriptValue*);
NativeScriptVMStatus nativeHoldable_Clanked(Item_GObj*,NativeScriptValue*);
NativeScriptVMStatus nativeHoldable_Reflected(Item_GObj*,NativeScriptValue*);
NativeScriptVMStatus nativeHoldable_ShieldBounced(Item_GObj*,NativeScriptValue*);

#ifdef __cplusplus
}
#endif

#endif
#endif
