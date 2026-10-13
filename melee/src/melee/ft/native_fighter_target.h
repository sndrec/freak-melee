#ifndef MELEE_FT_NATIVE_FIGHTER_TARGET_H
#define MELEE_FT_NATIVE_FIGHTER_TARGET_H
#include <melee/it/forward.h>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#include <stdbool.h>
#include <stdint.h>
#include <sysdolphin/baselib/forward.h>

// Retained script targets use fighter creation numbers, which are restored
// with rollback. No GObj address is stored in authored scalar state.
uint32_t nativeFighterTarget_Contact(Item_GObj*);
bool nativeFighterTarget_ControllerColor(uint32_t target, GXColor* color);
uint32_t nativeFighterTarget_Id(HSD_GObj*);
uint32_t nativeFighterTarget_CurrentOwner(Item_GObj*);
uint32_t nativeFighterTarget_Reflector(Item_GObj*);
bool nativeFighterTarget_CameraPosition(uint32_t, Vec3*);
// Current live fighter position; a failed ID/output query publishes nothing.
bool nativeFighterTarget_Position(uint32_t, Vec3*);
bool nativeFighterTarget_PositionDelta(uint32_t, Vec3*);
float nativeFighterTarget_Damage(uint32_t);
float nativeFighterTarget_Facing(uint32_t);
bool nativeFighterTarget_Sleeping(uint32_t);
int32_t nativeFighterTarget_FloorLine(uint32_t);
float nativeFighterTarget_CpuBodyHalfWidth(uint32_t);
// Raw imported skeleton part; failed queries leave the output unchanged.
bool nativeFighterTarget_BonePosition(uint32_t, unsigned part, Vec3*);
bool nativeFighterTarget_IsFighter(uint32_t);
bool nativeFighterTarget_Airborne(uint32_t);
bool nativeFighterTarget_SmashChargeActive(uint32_t);
int32_t nativeFighterTarget_Motion(uint32_t);
// Observe the current held object, not an article's creator or saved handle.
int32_t nativeFighterTarget_HeldId(uint32_t);
int32_t nativeFighterTarget_HeldKind(uint32_t);
int32_t nativeFighterTarget_HeldMotion(uint32_t);
float nativeFighterTarget_HeldFrame(uint32_t);
bool nativeFighterTarget_GuardActive(uint32_t);
float nativeFighterTarget_ModelScale(uint32_t);
float nativeFighterTarget_InstanceScaleY(uint32_t);
float nativeFighterTarget_Shield(uint32_t, Vec3* center);
#endif
