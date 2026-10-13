#include "native_script_constants.h"
#include "native_script_fighter.h"
#include "native_fighter_runtime.h"
#include "ftcmdscript.h"
#include <melee/ft/forward.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/it/forward.h>
#include <melee/lb/forward.h>
#include <melee/mp/forward.h>
#include <melee/db/db.h>
#include <melee/gr/stage_events.h>
#include <stdio.h>
#include <string.h>

// Deliberately enumerated native declarations. Adding a similarly spelled
// identifier never makes it accepted. Values use authoritative enum symbols;
// static MotionFlags constants are read by value only while loading content.
typedef struct CatalogEntry { NativeScriptConstant value; uint16_t native_flag; } CatalogEntry;
static const CatalogEntry catalog[] = {
    {{"AnimationFamily.none", "tools/native_fighter_script_primitives.c:Property; native declaration Ft_Kind_None", (uint64_t)(int64_t)(Ft_Kind_None), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"AnimationFamily.peach", "tools/native_fighter_script_primitives.c:Property; native declaration Ft_Kind_Peach", (uint64_t)(int64_t)(Ft_Kind_Peach), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"AnimationFamily.zelda", "tools/native_fighter_script_primitives.c:Property; native declaration Ft_Kind_Zelda", (uint64_t)(int64_t)(Ft_Kind_Zelda), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Axis.x", "tools/native_fighter_script_primitives.c:Property; native declaration 1", (uint64_t)(int64_t)(1), NativeScriptType_I32, NativeScriptConstant_Axis}, 0},
    {{"Axis.y", "tools/native_fighter_script_primitives.c:Property; native declaration 2", (uint64_t)(int64_t)(2), NativeScriptType_I32, NativeScriptConstant_Axis}, 0},
    {{"Axis.z", "tools/native_fighter_script_primitives.c:Property; native declaration 4", (uint64_t)(int64_t)(4), NativeScriptType_I32, NativeScriptConstant_Axis}, 0},
    {{"Callback.accessory_before", "native_script_fighter.h: instance callback slots", (uint64_t)NativeScriptFighterCallback_AccessoryBefore, NativeScriptType_I32, NativeScriptConstant_Callback}, 0},
    {{"Callback.accessory_hitlag", "native_script_fighter.h: instance callback slots", (uint64_t)NativeScriptFighterCallback_AccessoryHitlag, NativeScriptType_I32, NativeScriptConstant_Callback}, 0},
    {{"Callback.damage", "native_script_fighter.h: instance callback slots", (uint64_t)NativeScriptFighterCallback_Damage, NativeScriptType_I32, NativeScriptConstant_Callback}, 0},
    {{"Callback.deal_damage", "native_script_fighter.h: instance callback slots", (uint64_t)NativeScriptFighterCallback_DealDamage, NativeScriptType_I32, NativeScriptConstant_Callback}, 0},
    {{"Callback.death1", "native_script_fighter.h: instance callback slots", (uint64_t)NativeScriptFighterCallback_Death1, NativeScriptType_I32, NativeScriptConstant_Callback}, 0},
    {{"Callback.death2", "native_script_fighter.h: instance callback slots", (uint64_t)NativeScriptFighterCallback_Death2, NativeScriptType_I32, NativeScriptConstant_Callback}, 0},
    {{"Callback.death3", "native_script_fighter.h: instance callback slots", (uint64_t)NativeScriptFighterCallback_Death3, NativeScriptType_I32, NativeScriptConstant_Callback}, 0},
    {{"Callback.hurtbox_detect", "native_script_fighter.h: instance callback slots", (uint64_t)NativeScriptFighterCallback_HurtboxDetect, NativeScriptType_I32, NativeScriptConstant_Callback}, 0},
    {{"Callback.post_hitlag", "native_script_fighter.h: instance callback slots", (uint64_t)NativeScriptFighterCallback_PostHitlag, NativeScriptType_I32, NativeScriptConstant_Callback}, 0},
    {{"Callback.pre_hitlag", "native_script_fighter.h: instance callback slots", (uint64_t)NativeScriptFighterCallback_PreHitlag, NativeScriptType_I32, NativeScriptConstant_Callback}, 0},
    {{"Callback.reflect", "native_script_fighter.h: instance callback slots", (uint64_t)NativeScriptFighterCallback_Reflect, NativeScriptType_I32, NativeScriptConstant_Callback}, 0},
    {{"Callback.reflect_hit", "native_script_fighter.h: instance callback slots", (uint64_t)NativeScriptFighterCallback_ReflectHit, NativeScriptType_I32, NativeScriptConstant_Callback}, 0},
    {{"Callback.secondary_damage", "native_script_fighter.h: instance callback slots", (uint64_t)NativeScriptFighterCallback_SecondaryDamage, NativeScriptType_I32, NativeScriptConstant_Callback}, 0},
    {{"Callback.shield_hit", "native_script_fighter.h: instance callback slots", (uint64_t)NativeScriptFighterCallback_ShieldHit, NativeScriptType_I32, NativeScriptConstant_Callback}, 0},
    {{"Callback.state_change", "native_script_fighter.h: instance callback slots", (uint64_t)NativeScriptFighterCallback_StateChange, NativeScriptType_I32, NativeScriptConstant_Callback}, 0},
    {{"Collide_CeilingHug", "src/melee/mp/forward.h:Collide_CeilingHug", (uint64_t)(int64_t)(Collide_CeilingHug), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Collide_CeilingMask", "src/melee/mp/forward.h:Collide_CeilingMask", (uint64_t)(int64_t)(Collide_CeilingMask), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Collide_CeilingPush", "src/melee/mp/forward.h:Collide_CeilingPush", (uint64_t)(int64_t)(Collide_CeilingPush), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Collide_Edge", "src/melee/mp/forward.h:Collide_Edge", (uint64_t)(int64_t)(Collide_Edge), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Collide_FloorHug", "src/melee/mp/forward.h:Collide_FloorHug", (uint64_t)(int64_t)(Collide_FloorHug), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Collide_FloorMask", "src/melee/mp/forward.h:Collide_FloorMask", (uint64_t)(int64_t)(Collide_FloorMask), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Collide_FloorPush", "src/melee/mp/forward.h:Collide_FloorPush", (uint64_t)(int64_t)(Collide_FloorPush), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Collide_LedgeGrabMask", "src/melee/mp/forward.h:Collide_LedgeGrabMask", (uint64_t)(int64_t)(Collide_LedgeGrabMask), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Collide_LeftEdge", "src/melee/mp/forward.h:Collide_LeftEdge", (uint64_t)(int64_t)(Collide_LeftEdge), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Collide_LeftLedgeGrab", "src/melee/mp/forward.h:Collide_LeftLedgeGrab", (uint64_t)(int64_t)(Collide_LeftLedgeGrab), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Collide_LeftLedgeSlip", "src/melee/mp/forward.h:Collide_LeftLedgeSlip", (uint64_t)(int64_t)(Collide_LeftLedgeSlip), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Collide_LeftWallHug", "src/melee/mp/forward.h:Collide_LeftWallHug", (uint64_t)(int64_t)(Collide_LeftWallHug), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Collide_LeftWallMask", "src/melee/mp/forward.h:Collide_LeftWallMask", (uint64_t)(int64_t)(Collide_LeftWallMask), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Collide_LeftWallPush", "src/melee/mp/forward.h:Collide_LeftWallPush", (uint64_t)(int64_t)(Collide_LeftWallPush), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Collide_RightEdge", "src/melee/mp/forward.h:Collide_RightEdge", (uint64_t)(int64_t)(Collide_RightEdge), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Collide_RightLedgeGrab", "src/melee/mp/forward.h:Collide_RightLedgeGrab", (uint64_t)(int64_t)(Collide_RightLedgeGrab), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Collide_RightLedgeSlip", "src/melee/mp/forward.h:Collide_RightLedgeSlip", (uint64_t)(int64_t)(Collide_RightLedgeSlip), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Collide_RightWallHug", "src/melee/mp/forward.h:Collide_RightWallHug", (uint64_t)(int64_t)(Collide_RightWallHug), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Collide_RightWallMask", "src/melee/mp/forward.h:Collide_RightWallMask", (uint64_t)(int64_t)(Collide_RightWallMask), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Collide_RightWallPush", "src/melee/mp/forward.h:Collide_RightWallPush", (uint64_t)(int64_t)(Collide_RightWallPush), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Collide_WallMask", "src/melee/mp/forward.h:Collide_WallMask", (uint64_t)(int64_t)(Collide_WallMask), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Collision.ceiling_mask", "tools/native_fighter_script_primitives.c:Property; native declaration Collide_CeilingMask", (uint64_t)(int64_t)(Collide_CeilingMask), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Collision.floor_mask", "tools/native_fighter_script_primitives.c:Property; native declaration Collide_FloorMask", (uint64_t)(int64_t)(Collide_FloorMask), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Collision.left_wall_mask", "tools/native_fighter_script_primitives.c:Property; native declaration Collide_LeftWallMask", (uint64_t)(int64_t)(Collide_LeftWallMask), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Collision.right_wall_mask", "tools/native_fighter_script_primitives.c:Property; native declaration Collide_RightWallMask", (uint64_t)(int64_t)(Collide_RightWallMask), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Collision.wall_mask", "tools/native_fighter_script_primitives.c:Property; native declaration Collide_WallMask", (uint64_t)(int64_t)(Collide_WallMask), NativeScriptType_I32, NativeScriptConstant_CollisionMask}, 0},
    {{"Cpu.cstick_x", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_SetCstickX), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.cstick_y", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_SetCstickY), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.done", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_Done), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.forward", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_LstickXForward), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.forward_clamped", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_LstickForwardClamped), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.press_a", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_PressA), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.press_a_for", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_PressAFor), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.press_b", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_PressB), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.press_b_for", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_PressBFor), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.press_down", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_PressDown), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.press_l", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_PressL), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.press_left", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_PressLeft), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.press_r", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_PressR), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.press_right", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_PressRight), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.press_start", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_PressStart), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.press_up", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_PressUp), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.press_x", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_PressX), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.press_x_for", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_PressXFor), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.press_y", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_PressY), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.press_y_for", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_PressYFor), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.press_z", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_PressZ), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.release_a", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_ReleaseA), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.release_a_for", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_ReleaseAFor), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.release_all", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_ReleaseAll), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.release_b", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_ReleaseB), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.release_b_for", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_ReleaseBFor), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.release_down", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_ReleaseDown), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.release_l", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_ReleaseL), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.release_left", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_ReleaseLeft), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.release_r", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_ReleaseR), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.release_right", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_ReleaseRight), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.release_start", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_ReleaseStart), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.release_up", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_ReleaseUp), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.release_x", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_ReleaseX), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.release_x_for", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_ReleaseXFor), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.release_y", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_ReleaseY), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.release_y_for", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_ReleaseYFor), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.release_z", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_ReleaseZ), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.set_behavior", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_Unk0x93), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.stick_x", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_SetLstickX), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.stick_y", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_SetLstickY), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.toward_destination", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_LstickTowardDestination), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.toward_destination_clamped", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_LstickTowardDestinationClamped), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.toward_target", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_LstickTowardFighter), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.trigger_l", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_SetLtrigger), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.trigger_r", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_SetRtrigger), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.wait", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_WaitFor), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.wait_if_motion", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_WaitIfMotionId), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.x_toward_destination", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_LstickXTowardDestination), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.x_toward_destination_clamped", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_LstickXTowardDestinationClamped), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"Cpu.x_toward_target", "src/melee/ft/ftcmdscript.h:CPUCommand; tools/native_fighter_script_cpu.c:queue", (uint64_t)(int64_t)(CpuCmd_LstickXTowardFighter), NativeScriptType_I32, NativeScriptConstant_CPUCommand}, 0},
    {{"DbLKind_DebugDevelop", "src/melee/db/db.h:DbLKind", (uint64_t)(int64_t)(DbLKind_DebugDevelop), NativeScriptType_I32, NativeScriptConstant_DebugLevel}, 0},
    {{"DbLKind_DebugRom", "src/melee/db/db.h:DbLKind", (uint64_t)(int64_t)(DbLKind_DebugRom), NativeScriptType_I32, NativeScriptConstant_DebugLevel}, 0},
    {{"DbLKind_Develop", "src/melee/db/db.h:DbLKind", (uint64_t)(int64_t)(DbLKind_Develop), NativeScriptType_I32, NativeScriptConstant_DebugLevel}, 0},
    {{"DbLKind_Master", "src/melee/db/db.h:DbLKind", (uint64_t)(int64_t)(DbLKind_Master), NativeScriptType_I32, NativeScriptConstant_DebugLevel}, 0},
    {{"DbLKind_NoDebugRom", "src/melee/db/db.h:DbLKind", (uint64_t)(int64_t)(DbLKind_NoDebugRom), NativeScriptType_I32, NativeScriptConstant_DebugLevel}, 0},
    {{"DebugLevel.debug_develop", "tools/native_fighter_script_primitives.c:Property; native declaration DbLKind_DebugDevelop", (uint64_t)(int64_t)(DbLKind_DebugDevelop), NativeScriptType_I32, NativeScriptConstant_DebugLevel}, 0},
    {{"DebugLevel.debug_rom", "tools/native_fighter_script_primitives.c:Property; native declaration DbLKind_DebugRom", (uint64_t)(int64_t)(DbLKind_DebugRom), NativeScriptType_I32, NativeScriptConstant_DebugLevel}, 0},
    {{"DebugLevel.develop", "tools/native_fighter_script_primitives.c:Property; native declaration DbLKind_Develop", (uint64_t)(int64_t)(DbLKind_Develop), NativeScriptType_I32, NativeScriptConstant_DebugLevel}, 0},
    {{"DebugLevel.master", "tools/native_fighter_script_primitives.c:Property; native declaration DbLKind_Master", (uint64_t)(int64_t)(DbLKind_Master), NativeScriptType_I32, NativeScriptConstant_DebugLevel}, 0},
    {{"DebugLevel.no_debug_rom", "tools/native_fighter_script_primitives.c:Property; native declaration DbLKind_NoDebugRom", (uint64_t)(int64_t)(DbLKind_NoDebugRom), NativeScriptType_I32, NativeScriptConstant_DebugLevel}, 0},
    {{"FtPart_109", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_109), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_56", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_56), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_BustN", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_BustN), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_HeadN", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_HeadN), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_HipN", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_HipN), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_L1stNa", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_L1stNa), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_L1stNb", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_L1stNb), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_L2ndNa", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_L2ndNa), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_L2ndNb", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_L2ndNb), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_L3rdNa", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_L3rdNa), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_L3rdNb", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_L3rdNb), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_L4thNa", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_L4thNa), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_L4thNb", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_L4thNb), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_LArmJ", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_LArmJ), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_LFootJ", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_LFootJ), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_LFootJA", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_LFootJA), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_LHandN", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_LHandN), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_LHandNb", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_LHandNb), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_LKneeJ", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_LKneeJ), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_LLegJ", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_LLegJ), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_LLegJA", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_LLegJA), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_LShoulderJ", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_LShoulderJ), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_LShoulderJA", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_LShoulderJA), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_LShoulderN", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_LShoulderN), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_LThumbNa", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_LThumbNa), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_LThumbNb", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_LThumbNb), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_NeckN", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_NeckN), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_R1stNa", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_R1stNa), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_R1stNb", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_R1stNb), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_R2ndNa", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_R2ndNa), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_R2ndNb", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_R2ndNb), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_R3rdNa", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_R3rdNa), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_R3rdNb", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_R3rdNb), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_R4thNa", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_R4thNa), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_R4thNb", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_R4thNb), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_RArmJ", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_RArmJ), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_RFootJ", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_RFootJ), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_RFootJA", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_RFootJA), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_RHandN", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_RHandN), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_RHandNb", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_RHandNb), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_RKneeJ", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_RKneeJ), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_RLegJ", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_RLegJ), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_RLegJA", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_RLegJA), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_RShoulderJ", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_RShoulderJ), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_RShoulderJA", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_RShoulderJA), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_RShoulderN", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_RShoulderN), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_RThumbNa", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_RThumbNa), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_RThumbNb", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_RThumbNb), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_ThrowN", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_ThrowN), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_TopN", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_TopN), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_TransN", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_TransN), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_TransN2", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_TransN2), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_WaistN", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_WaistN), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_XRotN", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_XRotN), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"FtPart_YRotN", "src/melee/ft/forward.h:Fighter_Part", (uint64_t)(int64_t)(FtPart_YRotN), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"Ft_Kind_Boy", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Boy), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_CLink", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_CLink), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Captain", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Captain), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_CrezyH", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_CrezyH), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_CustomFirst", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_CustomFirst), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Donkey", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Donkey), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_DrMario", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_DrMario), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Emblem", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Emblem), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Falco", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Falco), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Fox", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Fox), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_GKoops", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_GKoops), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_GameWatch", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_GameWatch), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Ganon", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Ganon), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Girl", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Girl), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Kirby", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Kirby), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Koopa", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Koopa), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Link", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Link), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Luigi", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Luigi), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Mario", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Mario), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Mars", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Mars), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_MasterH", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_MasterH), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Mewtwo", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Mewtwo), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Nana", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Nana), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Ness", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Ness), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_None", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_None), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Peach", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Peach), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Pichu", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Pichu), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Pikachu", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Pikachu), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Popo", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Popo), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Purin", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Purin), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_RuntimeCapacity", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_RuntimeCapacity), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Samus", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Samus), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Sandbag", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Sandbag), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Seak", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Seak), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_StockCount", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_StockCount), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Yoshi", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Yoshi), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_Kind_Zelda", "src/melee/ft/forward.h:FighterKind", (uint64_t)(int64_t)(Ft_Kind_Zelda), NativeScriptType_I32, NativeScriptConstant_AnimationFamily}, 0},
    {{"Ft_MF_FreezeState", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 23},
    {{"Ft_MF_KeepAccessory", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 15},
    {{"Ft_MF_KeepColAnimHitStatus", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 4},
    {{"Ft_MF_KeepColAnimPartHitStatus", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 18},
    {{"Ft_MF_KeepFastFall", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 2},
    {{"Ft_MF_KeepGfx", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 3},
    {{"Ft_MF_KeepSfx", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 11},
    {{"Ft_MF_KeepSwordTrail", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 19},
    {{"Ft_MF_None", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 1},
    {{"Ft_MF_SkipAnim", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 31},
    {{"Ft_MF_SkipAnimVel", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 7},
    {{"Ft_MF_SkipAttackCount", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 27},
    {{"Ft_MF_SkipColAnim", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 14},
    {{"Ft_MF_SkipHit", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 5},
    {{"Ft_MF_SkipHitStun", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 30},
    {{"Ft_MF_SkipItemVis", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 20},
    {{"Ft_MF_SkipMatAnim", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 9},
    {{"Ft_MF_SkipMetalB", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 25},
    {{"Ft_MF_SkipModel", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 6},
    {{"Ft_MF_SkipModelFlags", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 28},
    {{"Ft_MF_SkipModelPartVis", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 24},
    {{"Ft_MF_SkipNametagVis", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 17},
    {{"Ft_MF_SkipParasol", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 12},
    {{"Ft_MF_SkipRumble", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 13},
    {{"Ft_MF_SkipThrowException", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 10},
    {{"Ft_MF_Unk06", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 8},
    {{"Ft_MF_Unk19", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 21},
    {{"Ft_MF_Unk24", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 26},
    {{"Ft_MF_Unk27", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 29},
    {{"Ft_MF_Unk30", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 32},
    {{"Ft_MF_Unk31", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 33},
    {{"Ft_MF_UnkUpdatePhys", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 22},
    {{"Ft_MF_UpdateCmd", "src/melee/ft/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 16},
    {{"Hip", "src/melee/ft/forward.h:Fighter_Part; tools/native_fighter_script_code.c:BoneSymbol", (uint64_t)(int64_t)(FtPart_HipN), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"HitCapsule_Disabled", "src/melee/lb/forward.h:HitCapsuleState", (uint64_t)(int64_t)(HitCapsule_Disabled), NativeScriptType_I32, NativeScriptConstant_HitStatus}, 0},
    {{"HitCapsule_Enabled", "src/melee/lb/forward.h:HitCapsuleState", (uint64_t)(int64_t)(HitCapsule_Enabled), NativeScriptType_I32, NativeScriptConstant_HitStatus}, 0},
    {{"HitCapsule_Max", "src/melee/lb/forward.h:HitCapsuleState", (uint64_t)(int64_t)(HitCapsule_Max), NativeScriptType_I32, NativeScriptConstant_HitStatus}, 0},
    {{"HitCapsule_Unk2", "src/melee/lb/forward.h:HitCapsuleState", (uint64_t)(int64_t)(HitCapsule_Unk2), NativeScriptType_I32, NativeScriptConstant_HitStatus}, 0},
    {{"HitCapsule_Unk3", "src/melee/lb/forward.h:HitCapsuleState", (uint64_t)(int64_t)(HitCapsule_Unk3), NativeScriptType_I32, NativeScriptConstant_HitStatus}, 0},
    {{"HitCapsule_Unk4", "src/melee/lb/forward.h:HitCapsuleState", (uint64_t)(int64_t)(HitCapsule_Unk4), NativeScriptType_I32, NativeScriptConstant_HitStatus}, 0},
    {{"Hitbox.disabled", "tools/native_fighter_script_primitives.c:Property; native declaration HitCapsule_Disabled", (uint64_t)(int64_t)(HitCapsule_Disabled), NativeScriptType_I32, NativeScriptConstant_HitStatus}, 0},
    {{"Hitbox.enabled", "tools/native_fighter_script_primitives.c:Property; native declaration HitCapsule_Enabled", (uint64_t)(int64_t)(HitCapsule_Enabled), NativeScriptType_I32, NativeScriptConstant_HitStatus}, 0},
    {{"Hurt.height_high", "src/melee/lb/forward.h:HurtHeight_High", (uint64_t)(int64_t)(HurtHeight_High), NativeScriptType_I32, NativeScriptConstant_HitStatus}, 0},
    {{"Hurt.height_low", "src/melee/lb/forward.h:HurtHeight_Low", (uint64_t)(int64_t)(HurtHeight_Low), NativeScriptType_I32, NativeScriptConstant_HitStatus}, 0},
    {{"Hurt.height_mid", "tools/native_fighter_script_primitives.c:Property; native declaration HurtHeight_Mid", (uint64_t)(int64_t)(HurtHeight_Mid), NativeScriptType_I32, NativeScriptConstant_HitStatus}, 0},
    {{"Hurt.intangible", "tools/native_fighter_script_primitives.c:Property; native declaration HurtCapsule_Intangible", (uint64_t)(int64_t)(HurtCapsule_Intangible), NativeScriptType_I32, NativeScriptConstant_HitStatus}, 0},
    {{"Hurt.invincible", "tools/native_fighter_script_primitives.c:Property; native declaration HurtCapsule_Disabled", (uint64_t)(int64_t)(HurtCapsule_Disabled), NativeScriptType_I32, NativeScriptConstant_HitStatus}, 0},
    {{"Hurt.normal", "tools/native_fighter_script_primitives.c:Property; native declaration HurtCapsule_Enabled", (uint64_t)(int64_t)(HurtCapsule_Enabled), NativeScriptType_I32, NativeScriptConstant_HitStatus}, 0},
    {{"HurtCapsule_Disabled", "src/melee/lb/forward.h:HurtCapsuleState", (uint64_t)(int64_t)(HurtCapsule_Disabled), NativeScriptType_I32, NativeScriptConstant_HitStatus}, 0},
    {{"HurtCapsule_Enabled", "src/melee/lb/forward.h:HurtCapsuleState", (uint64_t)(int64_t)(HurtCapsule_Enabled), NativeScriptType_I32, NativeScriptConstant_HitStatus}, 0},
    {{"HurtCapsule_Intangible", "src/melee/lb/forward.h:HurtCapsuleState", (uint64_t)(int64_t)(HurtCapsule_Intangible), NativeScriptType_I32, NativeScriptConstant_HitStatus}, 0},
    {{"HurtHeight_High", "src/melee/lb/forward.h:HurtHeight", (uint64_t)(int64_t)(HurtHeight_High), NativeScriptType_I32, NativeScriptConstant_HitStatus}, 0},
    {{"HurtHeight_Low", "src/melee/lb/forward.h:HurtHeight", (uint64_t)(int64_t)(HurtHeight_Low), NativeScriptType_I32, NativeScriptConstant_HitStatus}, 0},
    {{"HurtHeight_Mid", "src/melee/lb/forward.h:HurtHeight", (uint64_t)(int64_t)(HurtHeight_Mid), NativeScriptType_I32, NativeScriptConstant_HitStatus}, 0},
    {{"ITEM_ANIM_UPDATE", "src/melee/it/forward.h:Item_StateChangeFlags", (uint64_t)(int64_t)(ITEM_ANIM_UPDATE), NativeScriptType_I32, NativeScriptConstant_TransitionFlag}, 0},
    {{"ITEM_CMD_UPDATE", "src/melee/it/forward.h:Item_StateChangeFlags", (uint64_t)(int64_t)(ITEM_CMD_UPDATE), NativeScriptType_I32, NativeScriptConstant_TransitionFlag}, 0},
    {{"ITEM_COLANIM_PRESERVE", "src/melee/it/forward.h:Item_StateChangeFlags", (uint64_t)(int64_t)(ITEM_COLANIM_PRESERVE), NativeScriptType_I32, NativeScriptConstant_TransitionFlag}, 0},
    {{"ITEM_DROP_UPDATE", "src/melee/it/forward.h:Item_StateChangeFlags", (uint64_t)(int64_t)(ITEM_DROP_UPDATE), NativeScriptType_I32, NativeScriptConstant_TransitionFlag}, 0},
    {{"ITEM_HIT_PRESERVE", "src/melee/it/forward.h:Item_StateChangeFlags", (uint64_t)(int64_t)(ITEM_HIT_PRESERVE), NativeScriptType_I32, NativeScriptConstant_TransitionFlag}, 0},
    {{"ITEM_HOLD_0", "src/melee/it/forward.h:Item_HoldKinds", (uint64_t)(int64_t)(ITEM_HOLD_0), NativeScriptType_I32, NativeScriptConstant_ItemCategory}, 0},
    {{"ITEM_HOLD_1", "src/melee/it/forward.h:Item_HoldKinds", (uint64_t)(int64_t)(ITEM_HOLD_1), NativeScriptType_I32, NativeScriptConstant_ItemCategory}, 0},
    {{"ITEM_HOLD_10", "src/melee/it/forward.h:Item_HoldKinds", (uint64_t)(int64_t)(ITEM_HOLD_10), NativeScriptType_I32, NativeScriptConstant_ItemCategory}, 0},
    {{"ITEM_HOLD_11", "src/melee/it/forward.h:Item_HoldKinds", (uint64_t)(int64_t)(ITEM_HOLD_11), NativeScriptType_I32, NativeScriptConstant_ItemCategory}, 0},
    {{"ITEM_HOLD_12", "src/melee/it/forward.h:Item_HoldKinds", (uint64_t)(int64_t)(ITEM_HOLD_12), NativeScriptType_I32, NativeScriptConstant_ItemCategory}, 0},
    {{"ITEM_HOLD_2", "src/melee/it/forward.h:Item_HoldKinds", (uint64_t)(int64_t)(ITEM_HOLD_2), NativeScriptType_I32, NativeScriptConstant_ItemCategory}, 0},
    {{"ITEM_HOLD_3", "src/melee/it/forward.h:Item_HoldKinds", (uint64_t)(int64_t)(ITEM_HOLD_3), NativeScriptType_I32, NativeScriptConstant_ItemCategory}, 0},
    {{"ITEM_HOLD_4", "src/melee/it/forward.h:Item_HoldKinds", (uint64_t)(int64_t)(ITEM_HOLD_4), NativeScriptType_I32, NativeScriptConstant_ItemCategory}, 0},
    {{"ITEM_HOLD_5", "src/melee/it/forward.h:Item_HoldKinds", (uint64_t)(int64_t)(ITEM_HOLD_5), NativeScriptType_I32, NativeScriptConstant_ItemCategory}, 0},
    {{"ITEM_HOLD_6", "src/melee/it/forward.h:Item_HoldKinds", (uint64_t)(int64_t)(ITEM_HOLD_6), NativeScriptType_I32, NativeScriptConstant_ItemCategory}, 0},
    {{"ITEM_HOLD_7", "src/melee/it/forward.h:Item_HoldKinds", (uint64_t)(int64_t)(ITEM_HOLD_7), NativeScriptType_I32, NativeScriptConstant_ItemCategory}, 0},
    {{"ITEM_HOLD_8", "src/melee/it/forward.h:Item_HoldKinds", (uint64_t)(int64_t)(ITEM_HOLD_8), NativeScriptType_I32, NativeScriptConstant_ItemCategory}, 0},
    {{"ITEM_HOLD_9", "src/melee/it/forward.h:Item_HoldKinds", (uint64_t)(int64_t)(ITEM_HOLD_9), NativeScriptType_I32, NativeScriptConstant_ItemCategory}, 0},
    {{"ITEM_MODEL_UPDATE", "src/melee/it/forward.h:Item_StateChangeFlags", (uint64_t)(int64_t)(ITEM_MODEL_UPDATE), NativeScriptType_I32, NativeScriptConstant_TransitionFlag}, 0},
    {{"ITEM_SFX_PRESERVE", "src/melee/it/forward.h:Item_StateChangeFlags", (uint64_t)(int64_t)(ITEM_SFX_PRESERVE), NativeScriptType_I32, NativeScriptConstant_TransitionFlag}, 0},
    {{"ITEM_UNK_0x1", "src/melee/it/forward.h:Item_StateChangeFlags", (uint64_t)(int64_t)(ITEM_UNK_0x1), NativeScriptType_I32, NativeScriptConstant_TransitionFlag}, 0},
    {{"ITEM_UNK_7", "src/melee/it/forward.h:Item_UnkKinds", (uint64_t)(int64_t)(ITEM_UNK_7), NativeScriptType_I32, NativeScriptConstant_ItemCategory}, 0},
    {{"ITEM_UNK_ENEMY", "src/melee/it/forward.h:Item_UnkKinds", (uint64_t)(int64_t)(ITEM_UNK_ENEMY), NativeScriptType_I32, NativeScriptConstant_ItemCategory}, 0},
    {{"ITEM_UNK_LOCKON", "src/melee/it/forward.h:Item_UnkKinds", (uint64_t)(int64_t)(ITEM_UNK_LOCKON), NativeScriptType_I32, NativeScriptConstant_ItemCategory}, 0},
    {{"ITEM_UNK_MATO", "src/melee/it/forward.h:Item_UnkKinds", (uint64_t)(int64_t)(ITEM_UNK_MATO), NativeScriptType_I32, NativeScriptConstant_ItemCategory}, 0},
    {{"ITEM_UNK_UPDATE", "src/melee/it/forward.h:Item_StateChangeFlags", (uint64_t)(int64_t)(ITEM_UNK_UPDATE), NativeScriptType_I32, NativeScriptConstant_TransitionFlag}, 0},
    {{"It_Common_End", "src/melee/it/forward.h:It_Common_End", (uint64_t)(int64_t)(It_Common_End), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Common_Start", "src/melee/it/forward.h:It_Common_Start", (uint64_t)(int64_t)(It_Common_Start), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Arwing_Laser", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Arwing_Laser), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Bat", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Bat), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_BombHei", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_BombHei), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Box", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Box), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_CLink_Arrow", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_CLink_Arrow), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_CLink_Bomb", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_CLink_Bomb), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_CLink_Boomerang", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_CLink_Boomerang), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_CLink_Bow", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_CLink_Bow), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_CLink_HShot", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_CLink_HShot), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_CLink_Milk", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_CLink_Milk), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Capsule", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Capsule), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Chicorita_Leaf", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Chicorita_Leaf), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Coin", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Coin), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_CrazyHand_Bomb", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_CrazyHand_Bomb), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_CrazyHand_Bullet", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_CrazyHand_Bullet), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_CrazyHand_Laser", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_CrazyHand_Laser), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_DKinoko", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_DKinoko), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Dosei", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Dosei), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_DrMario_Sheet", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_DrMario_Sheet), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_DrMario_Vitamin", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_DrMario_Vitamin), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Egg", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Egg), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_EvYoshiEgg", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_EvYoshiEgg), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_F_Flower", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_F_Flower), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_F_Flower_Flame", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_F_Flower_Flame), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Falco_Blaster", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Falco_Blaster), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Falco_Laser", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Falco_Laser), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Falco_Phantasm", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Falco_Phantasm), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Flipper", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Flipper), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Foods", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Foods), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Fox_Blaster", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Fox_Blaster), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Fox_Illusion", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Fox_Illusion), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Fox_Laser", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Fox_Laser), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Freeze", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Freeze), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_G_Shell", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_G_Shell), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_GameWatch_Breath", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_GameWatch_Breath), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_GameWatch_Chef", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_GameWatch_Chef), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_GameWatch_Fire", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_GameWatch_Fire), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_GameWatch_Greenhouse", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_GameWatch_Greenhouse), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_GameWatch_Judge", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_GameWatch_Judge), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_GameWatch_Manhole", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_GameWatch_Manhole), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_GameWatch_Panic", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_GameWatch_Panic), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_GameWatch_Parachute", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_GameWatch_Parachute), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_GameWatch_Rescue", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_GameWatch_Rescue), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_GameWatch_Turtle", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_GameWatch_Turtle), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_GreatFox_Laser", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_GreatFox_Laser), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Hammer", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Hammer), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Hammer_Head", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Hammer_Head), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Harisen", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Harisen), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Heart", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Heart), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Heiho", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Heiho), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Hinoarashi_Flame", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Hinoarashi_Flame), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Hitodeman_Star", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Hitodeman_Star), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Houou_SacredFire", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Houou_SacredFire), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_IceClimber_Blizzard", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_IceClimber_Blizzard), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_IceClimber_GumStrings", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_IceClimber_GumStrings), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_IceClimber_Ice", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_IceClimber_Ice), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Invalid1", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Invalid1), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Invalid2", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Invalid2), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Invalid3", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Invalid3), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Invalid4", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Invalid4), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Invalid5", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Invalid5), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Invalid6", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Invalid6), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Invalid7", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Invalid7), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Invalid8", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Invalid8), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kamex_HydroPump", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kamex_HydroPump), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kinoko", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kinoko), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_CBeam", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_CBeam), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_CLinkArrow", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_CLinkArrow), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_CLinkBow", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_CLinkBow), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_DrMarioVitamin", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_DrMarioVitamin), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_FalcoBlaster", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_FalcoBlaster), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_FalcoLaser", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_FalcoLaser), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_FoxBlaster", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_FoxBlaster), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_FoxLaser", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_FoxLaser), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_GameWatchChef", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_GameWatchChef), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_GameWatchChefPan", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_GameWatchChefPan), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_Hammer", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_Hammer), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_IceClimberIce", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_IceClimberIce), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_KoopaFlame", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_KoopaFlame), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_LinkArrow", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_LinkArrow), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_LinkBow", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_LinkBow), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_LuigiFire", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_LuigiFire), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_MarioFire", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_MarioFire), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_MewtwoShadowBall", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_MewtwoShadowBall), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_NessPKFlush", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_NessPKFlush), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_NessPKFlush_Explode", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_NessPKFlush_Explode), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_PeachToad", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_PeachToad), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_PeachToadSpore", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_PeachToadSpore), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_PichuTJolt_Air", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_PichuTJolt_Air), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_PichuTJolt_Ground", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_PichuTJolt_Ground), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_PikachuTJolt_Air", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_PikachuTJolt_Air), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_PikachuTJolt_Ground", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_PikachuTJolt_Ground), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_SamusCharge", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_SamusCharge), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_SeakNeedleHeld", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_SeakNeedleHeld), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_SeakNeedleThrow", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_SeakNeedleThrow), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kirby_YoshiEggLay", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kirby_YoshiEggLay), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Klap", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Klap), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Koopa_Flame", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Koopa_Flame), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kuriboh", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kuriboh), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kusudama", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kusudama), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kyasarin", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kyasarin), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Kyasarin_Egg", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Kyasarin_Egg), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_L_Gun", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_L_Gun), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_L_Gun_Beam", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_L_Gun_Beam), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_L_Gun_Ray", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_L_Gun_Ray), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Leadead", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Leadead), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Likelike", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Likelike), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Link_Arrow", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Link_Arrow), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Link_Bomb", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Link_Bomb), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Link_Boomerang", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Link_Boomerang), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Link_Bow", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Link_Bow), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Link_HShot", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Link_HShot), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_LipStick", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_LipStick), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_LipStick_Spore", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_LipStick_Spore), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Lizardon_Flame1", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Lizardon_Flame1), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Lizardon_Flame2", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Lizardon_Flame2), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Lizardon_Flame3", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Lizardon_Flame3), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Lizardon_Flame4", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Lizardon_Flame4), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Lucky_Egg", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Lucky_Egg), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Lugia_Aeroblast", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Lugia_Aeroblast), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Lugia_Aeroblast2", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Lugia_Aeroblast2), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Lugia_Aeroblast3", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Lugia_Aeroblast3), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Luigi_Fire", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Luigi_Fire), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_MSBomb", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_MSBomb), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_M_Ball", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_M_Ball), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Mario_Cape", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Mario_Cape), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Mario_Fire", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Mario_Fire), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_MasterHand_Bullet", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_MasterHand_Bullet), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_MasterHand_Laser", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_MasterHand_Laser), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Matadogas_Gas1", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Matadogas_Gas1), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Matadogas_Gas2", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Matadogas_Gas2), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Mato", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Mato), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_MetalB", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_MetalB), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Mewtwo_Disable", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Mewtwo_Disable), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Mewtwo_ShadowBall", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Mewtwo_ShadowBall), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_NativeHoldable", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_NativeHoldable), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Ness_Bat", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Ness_Bat), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Ness_PKFire", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Ness_PKFire), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Ness_PKFire_Flame", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Ness_PKFire_Flame), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Ness_PKFlush", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Ness_PKFlush), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Ness_PKFlush_Explode", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Ness_PKFlush_Explode), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Ness_PKThunder", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Ness_PKThunder), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Ness_PKThunder1", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Ness_PKThunder1), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Ness_PKThunder2", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Ness_PKThunder2), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Ness_PKThunder3", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Ness_PKThunder3), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Ness_PKThunder4", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Ness_PKThunder4), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Ness_Yoyo", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Ness_Yoyo), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Nokonoko", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Nokonoko), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_None", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_None), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Octarock", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Octarock), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Octarock_Stone", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Octarock_Stone), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Old_Kuri", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Old_Kuri), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Old_Lead", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Old_Lead), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Old_Octa", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Old_Octa), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Old_Otto", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Old_Otto), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Ottosea", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Ottosea), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Parasol", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Parasol), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Patapata", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Patapata), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Peach_Explode", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Peach_Explode), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Peach_Parasol", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Peach_Parasol), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Peach_Toad", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Peach_Toad), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Peach_ToadSpore", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Peach_ToadSpore), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Peach_Turnip", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Peach_Turnip), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Pichu_TJolt_Air", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Pichu_TJolt_Air), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Pichu_TJolt_Ground", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Pichu_TJolt_Ground), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Pichu_Thunder", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Pichu_Thunder), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Pikachu_TJolt_Air", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Pikachu_TJolt_Air), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Pikachu_TJolt_Ground", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Pikachu_TJolt_Ground), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Pikachu_Thunder", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Pikachu_Thunder), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Pokemon_Unk", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Pokemon_Unk), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_R_Shell", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_R_Shell), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_RabbitC", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_RabbitC), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_S_Scope", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_S_Scope), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_S_Scope_Beam", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_S_Scope_Beam), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Samus_Bomb", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Samus_Bomb), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Samus_Charge", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Samus_Charge), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Samus_GBeam", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Samus_GBeam), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Samus_Missile", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Samus_Missile), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_ScBall", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_ScBall), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Seak_Chain", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Seak_Chain), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Seak_NeedleHeld", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Seak_NeedleHeld), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Seak_NeedleThrow", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Seak_NeedleThrow), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Seak_Vanish", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Seak_Vanish), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Spycloak", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Spycloak), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Star", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Star), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_StarRod", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_StarRod), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_StarRod_Star", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_StarRod_Star), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Sword", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Sword), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Taru", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Taru), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_TaruCann", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_TaruCann), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Tincle", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Tincle), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Tomato", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Tomato), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Tools", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Tools), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Unk1", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Unk1), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Unk2", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Unk2), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Unk4", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Unk4), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Unknown_Swarm", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Unknown_Swarm), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_WStar", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_WStar), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_WhispyApple", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_WhispyApple), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_WhispyHealApple", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_WhispyHealApple), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Whitebea", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Whitebea), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Yoshi_EggLay", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Yoshi_EggLay), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Yoshi_EggThrow", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Yoshi_EggThrow), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Yoshi_Star", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Yoshi_Star), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_ZGShell", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_ZGShell), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_ZRShell", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_ZRShell), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Zelda_DinFire", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Zelda_DinFire), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_Kind_Zelda_DinFire_Explode", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_Kind_Zelda_DinFire_Explode), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Cerebi", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Cerebi), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Chicorita", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Chicorita), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Entei", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Entei), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Fire", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Fire), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Freezer", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Freezer), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Fushigibana", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Fushigibana), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Hassam", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Hassam), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Hinoarashi", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Hinoarashi), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Hitodeman", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Hitodeman), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Houou", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Houou), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Kabigon", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Kabigon), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Kamex", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Kamex), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Kireihana", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Kireihana), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Lizardon", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Lizardon), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Lucky", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Lucky), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Lugia", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Lugia), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Maril", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Maril), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Marumine", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Marumine), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Matadogas", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Matadogas), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Metamon", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Metamon), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Mew", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Mew), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Pippi", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Pippi), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Porygon2", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Porygon2), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Raikou", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Raikou), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Random", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Random), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Sonans", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Sonans), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Start", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Start), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Suikun", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Suikun), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Terminate", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Terminate), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Thunder", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Thunder), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Togepy", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Togepy), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Tosakinto", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Tosakinto), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"It_PKind_Unknown", "src/melee/it/forward.h:ItemKind", (uint64_t)(int64_t)(It_PKind_Unknown), NativeScriptType_I32, NativeScriptConstant_Item}, 0},
    {{"LeftFoot", "src/melee/ft/forward.h:Fighter_Part; tools/native_fighter_script_code.c:BoneSymbol", (uint64_t)(int64_t)(FtPart_LFootJA), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"LeftHand", "src/melee/ft/forward.h:Fighter_Part; tools/native_fighter_script_code.c:BoneSymbol", (uint64_t)(int64_t)(FtPart_L1stNb), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"LeftShoulder", "src/melee/ft/forward.h:Fighter_Part; tools/native_fighter_script_code.c:BoneSymbol", (uint64_t)(int64_t)(FtPart_LShoulderN), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"LeftThumb", "src/melee/ft/forward.h:Fighter_Part; tools/native_fighter_script_code.c:BoneSymbol", (uint64_t)(int64_t)(FtPart_LThumbNb), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"Motion.captured_damage_air", "tools/native_fighter_script_primitives.c:Property; native declaration ftCo_MS_CaptureDamageKoopaAir", (uint64_t)(int64_t)(ftCo_MS_CaptureDamageKoopaAir), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"Motion.captured_damage_ground", "tools/native_fighter_script_primitives.c:Property; native declaration ftCo_MS_CaptureDamageKoopa", (uint64_t)(int64_t)(ftCo_MS_CaptureDamageKoopa), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"Motion.captured_throw_air_back", "tools/native_fighter_script_primitives.c:Property; native declaration ftCo_MS_ThrownKoopaAirB", (uint64_t)(int64_t)(ftCo_MS_ThrownKoopaAirB), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"Motion.captured_throw_air_forward", "tools/native_fighter_script_primitives.c:Property; native declaration ftCo_MS_ThrownKoopaAirF", (uint64_t)(int64_t)(ftCo_MS_ThrownKoopaAirF), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"Motion.captured_throw_back", "tools/native_fighter_script_primitives.c:Property; native declaration ftCo_MS_ThrownKoopaB", (uint64_t)(int64_t)(ftCo_MS_ThrownKoopaB), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"Motion.captured_throw_forward", "tools/native_fighter_script_primitives.c:Property; native declaration ftCo_MS_ThrownKoopaF", (uint64_t)(int64_t)(ftCo_MS_ThrownKoopaF), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"Motion.catch_dash", "tools/native_fighter_script_primitives.c:Property; native declaration ftCo_MS_CatchDash", (uint64_t)(int64_t)(ftCo_MS_CatchDash), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"Motion.fall_aerial", "tools/native_fighter_script_primitives.c:Property; native declaration ftCo_MS_FallAerial", (uint64_t)(int64_t)(ftCo_MS_FallAerial), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"Motion.fall_special", "tools/native_fighter_script_primitives.c:Property; native declaration ftCo_MS_FallSpecial", (uint64_t)(int64_t)(ftCo_MS_FallSpecial), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"Motion.held_fall", "tools/native_fighter_script_primitives.c:Property; native declaration ftCo_MS_ItemParasolFall", (uint64_t)(int64_t)(ftCo_MS_ItemParasolFall), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"Motion.held_fall_special", "tools/native_fighter_script_primitives.c:Property; native declaration ftCo_MS_ItemParasolFallSpecial", (uint64_t)(int64_t)(ftCo_MS_ItemParasolFallSpecial), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"Motion.held_open", "tools/native_fighter_script_primitives.c:Property; native declaration ftCo_MS_ItemParasolOpen", (uint64_t)(int64_t)(ftCo_MS_ItemParasolOpen), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"Motion.held_throw_forward", "tools/native_fighter_script_primitives.c:Property; native declaration ftCo_MS_ThrownFF", (uint64_t)(int64_t)(ftCo_MS_ThrownFF), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"Motion.held_turn", "tools/native_fighter_script_primitives.c:Property; native declaration ftCo_MS_ShoulderedTurn", (uint64_t)(int64_t)(ftCo_MS_ShoulderedTurn), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"Motion.held_wait", "tools/native_fighter_script_primitives.c:Property; native declaration ftCo_MS_ShoulderedWait", (uint64_t)(int64_t)(ftCo_MS_ShoulderedWait), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"Motion.throw_back", "tools/native_fighter_script_primitives.c:Property; native declaration ftCo_MS_ThrowB", (uint64_t)(int64_t)(ftCo_MS_ThrowB), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"Motion.throw_down", "tools/native_fighter_script_primitives.c:Property; native declaration ftCo_MS_ThrowLw", (uint64_t)(int64_t)(ftCo_MS_ThrowLw), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"Motion.throw_forward", "tools/native_fighter_script_primitives.c:Property; native declaration ftCo_MS_ThrowF", (uint64_t)(int64_t)(ftCo_MS_ThrowF), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"Motion.throw_up", "tools/native_fighter_script_primitives.c:Property; native declaration ftCo_MS_ThrowHi", (uint64_t)(int64_t)(ftCo_MS_ThrowHi), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"Motion.thrown_down_alternate", "tools/native_fighter_script_primitives.c:Property; native declaration ftCo_MS_ThrownlwWomen", (uint64_t)(int64_t)(ftCo_MS_ThrownlwWomen), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"Motion.up_smash", "tools/native_fighter_script_primitives.c:Property; native declaration ftCo_MS_AttackHi4", (uint64_t)(int64_t)(ftCo_MS_AttackHi4), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"RightFoot", "src/melee/ft/forward.h:Fighter_Part; tools/native_fighter_script_code.c:BoneSymbol", (uint64_t)(int64_t)(FtPart_RFootJA), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"RightHand", "src/melee/ft/forward.h:Fighter_Part; tools/native_fighter_script_code.c:BoneSymbol", (uint64_t)(int64_t)(FtPart_R1stNb), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"RightShoulder", "src/melee/ft/forward.h:Fighter_Part; tools/native_fighter_script_code.c:BoneSymbol", (uint64_t)(int64_t)(FtPart_RShoulderN), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"RightThumb", "src/melee/ft/forward.h:Fighter_Part; tools/native_fighter_script_code.c:BoneSymbol", (uint64_t)(int64_t)(FtPart_RThumbNb), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"Shield.hit", "native_fighter_runtime.h:NativeFighterShieldTransition", NativeFighterShield_Hit, NativeScriptType_I32, NativeScriptConstant_ShieldTransition}, 0},
    {{"Shield.hold", "native_fighter_runtime.h:NativeFighterShieldTransition", NativeFighterShield_Hold, NativeScriptType_I32, NativeScriptConstant_ShieldTransition}, 0},
    {{"Shield.reflect_continue", "native_fighter_runtime.h:NativeFighterShieldTransition", NativeFighterShield_ReflectContinue, NativeScriptType_I32, NativeScriptConstant_ShieldTransition}, 0},
    {{"Shield.reflect_start", "native_fighter_runtime.h:NativeFighterShieldTransition", NativeFighterShield_ReflectStart, NativeScriptType_I32, NativeScriptConstant_ShieldTransition}, 0},
    {{"Shield.release", "native_fighter_runtime.h:NativeFighterShieldTransition", NativeFighterShield_Release, NativeScriptType_I32, NativeScriptConstant_ShieldTransition}, 0},
    {{"Shield.start", "native_fighter_runtime.h:NativeFighterShieldTransition", NativeFighterShield_Start, NativeScriptType_I32, NativeScriptConstant_ShieldTransition}, 0},
    {{"Smash.charging", "forward.h: SmashState; Ness manual charge uses state 4", (uint64_t)(SmashState_Charging), NativeScriptType_I32, NativeScriptConstant_Smash}, 0},
    {{"Smash.manual_charge", "forward.h: SmashState; Ness manual charge uses state 4", (uint64_t)(4), NativeScriptType_I32, NativeScriptConstant_Smash}, 0},
    {{"Smash.none", "forward.h: SmashState; Ness manual charge uses state 4", (uint64_t)(SmashState_None), NativeScriptType_I32, NativeScriptConstant_Smash}, 0},
    {{"Smash.precharge", "forward.h: SmashState; Ness manual charge uses state 4", (uint64_t)(SmashState_PreCharge), NativeScriptType_I32, NativeScriptConstant_Smash}, 0},
    {{"Smash.release", "forward.h: SmashState; Ness manual charge uses state 4", (uint64_t)(SmashState_Release), NativeScriptType_I32, NativeScriptConstant_Smash}, 0},
    {{"StageEvent_Conversation_active", "src/melee/gr/stage_events.h: public stage channel", StageEvent_Conversation_active, NativeScriptType_U32, NativeScriptConstant_StageChannel}, 0},
    {{"StageEvent_Conversation_available", "src/melee/gr/stage_events.h: public stage channel", StageEvent_Conversation_available, NativeScriptType_U32, NativeScriptConstant_StageChannel}, 0},
    {{"StageEvent_Conversation_cancel", "src/melee/gr/stage_events.h: public stage channel", StageEvent_Conversation_cancel, NativeScriptType_U32, NativeScriptConstant_StageChannel}, 0},
    {{"StageEvent_Conversation_start", "src/melee/gr/stage_events.h: public stage channel", StageEvent_Conversation_start, NativeScriptType_U32, NativeScriptConstant_StageChannel}, 0},
    {{"Top", "src/melee/ft/forward.h:Fighter_Part; tools/native_fighter_script_code.c:BoneSymbol", (uint64_t)(int64_t)(FtPart_TopN), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"TransN", "src/melee/ft/forward.h:Fighter_Part; tools/native_fighter_script_code.c:BoneSymbol", (uint64_t)(int64_t)(FtPart_TransN), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"TransN2", "src/melee/ft/forward.h:Fighter_Part; tools/native_fighter_script_code.c:BoneSymbol", (uint64_t)(int64_t)(FtPart_TransN2), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"Transition.common_ground_air", "tools/native_fighter_script_primitives.c:Property; native declaration ftCommon_GroundAirColl_MF", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 34},
    {{"Transition.keep_hurt_status", "tools/native_fighter_script_primitives.c:Property; native declaration Ft_MF_KeepColAnimHitStatus", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 4},
    {{"Transition.skip_attack_count", "tools/native_fighter_script_primitives.c:Property; native declaration Ft_MF_SkipAttackCount", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 27},
    {{"Transition.skip_hitboxes", "tools/native_fighter_script_primitives.c:Property; native declaration Ft_MF_SkipHit", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 5},
    {{"Transition.skip_model", "tools/native_fighter_script_primitives.c:Property; native declaration Ft_MF_SkipModel", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 6},
    {{"XRotN", "src/melee/ft/forward.h:Fighter_Part; tools/native_fighter_script_code.c:BoneSymbol", (uint64_t)(int64_t)(FtPart_XRotN), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"YRotN", "src/melee/ft/forward.h:Fighter_Part; tools/native_fighter_script_code.c:BoneSymbol", (uint64_t)(int64_t)(FtPart_YRotN), NativeScriptType_I32, NativeScriptConstant_Joint}, 0},
    {{"ftCo_MF_2_5_6", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 36},
    {{"ftCo_MF_3_5_6", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 49},
    {{"ftCo_MF_5_6", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 35},
    {{"ftCo_MF_9_10", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 41},
    {{"ftCo_MF_Appeal", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 40},
    {{"ftCo_MF_Attack", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 55},
    {{"ftCo_MF_Attack1", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 72},
    {{"ftCo_MF_Attack100", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 76},
    {{"ftCo_MF_Attack11", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 73},
    {{"ftCo_MF_Attack12", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 74},
    {{"ftCo_MF_Attack13", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 75},
    {{"ftCo_MF_Attack4", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 68},
    {{"ftCo_MF_AttackAir", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 62},
    {{"ftCo_MF_AttackAirB", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 65},
    {{"ftCo_MF_AttackAirF", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 64},
    {{"ftCo_MF_AttackAirHi", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 66},
    {{"ftCo_MF_AttackAirLw", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 67},
    {{"ftCo_MF_AttackAirN", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 63},
    {{"ftCo_MF_AttackBase", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 54},
    {{"ftCo_MF_AttackDash", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 57},
    {{"ftCo_MF_AttackHi3", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 59},
    {{"ftCo_MF_AttackHi4", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 70},
    {{"ftCo_MF_AttackLw3", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 60},
    {{"ftCo_MF_AttackLw4", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 71},
    {{"ftCo_MF_AttackS3", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 58},
    {{"ftCo_MF_AttackS4", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 69},
    {{"ftCo_MF_Attack_2", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 56},
    {{"ftCo_MF_BatSwing1", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 81},
    {{"ftCo_MF_BatSwing3", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 82},
    {{"ftCo_MF_BatSwing4", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 110},
    {{"ftCo_MF_BatSwingDash", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 83},
    {{"ftCo_MF_Capture", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 165},
    {{"ftCo_MF_CaptureAir", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 166},
    {{"ftCo_MF_Catch", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 158},
    {{"ftCo_MF_CatchAttack", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 160},
    {{"ftCo_MF_CatchBase", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 157},
    {{"ftCo_MF_CatchWait", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 156},
    {{"ftCo_MF_CliffAction", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 151},
    {{"ftCo_MF_CliffAction_4_5", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 152},
    {{"ftCo_MF_CliffAttackQuick", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 61},
    {{"ftCo_MF_CliffAttackSlow", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 154},
    {{"ftCo_MF_CliffCatch", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 153},
    {{"ftCo_MF_CliffWait", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 155},
    {{"ftCo_MF_Damage", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 135},
    {{"ftCo_MF_DamageScrew", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 136},
    {{"ftCo_MF_DamageScrewAir", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 137},
    {{"ftCo_MF_Dash", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 38},
    {{"ftCo_MF_Dazed", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 134},
    {{"ftCo_MF_Dead", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 172},
    {{"ftCo_MF_Down", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 138},
    {{"ftCo_MF_DownAttack", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 143},
    {{"ftCo_MF_DownAttackD", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 145},
    {{"ftCo_MF_DownAttackU", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 144},
    {{"ftCo_MF_DownD", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 140},
    {{"ftCo_MF_DownDamageD", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 142},
    {{"ftCo_MF_DownDamageU", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 141},
    {{"ftCo_MF_DownU", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 139},
    {{"ftCo_MF_FireFlowerShoot", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 130},
    {{"ftCo_MF_FireFlowerShootAir", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 132},
    {{"ftCo_MF_Guard", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 53},
    {{"ftCo_MF_GuardReflect", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 52},
    {{"ftCo_MF_Hammer", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 98},
    {{"ftCo_MF_HammerBase", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 97},
    {{"ftCo_MF_HammerFall", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 104},
    {{"ftCo_MF_HammerJump", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 123},
    {{"ftCo_MF_HammerMove", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 117},
    {{"ftCo_MF_HammerTurn", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 118},
    {{"ftCo_MF_HammerWalk", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 119},
    {{"ftCo_MF_HarisenSwing1", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 87},
    {{"ftCo_MF_HarisenSwing3", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 88},
    {{"ftCo_MF_HarisenSwing4", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 112},
    {{"ftCo_MF_HarisenSwingDash", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 89},
    {{"ftCo_MF_ItemFall", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 120},
    {{"ftCo_MF_ItemScope", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 77},
    {{"ftCo_MF_ItemScopeAir", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 107},
    {{"ftCo_MF_ItemScopeAirRapid", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 133},
    {{"ftCo_MF_ItemScopeFire", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 103},
    {{"ftCo_MF_ItemScopeRapid", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 131},
    {{"ftCo_MF_ItemScope_5_6", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 100},
    {{"ftCo_MF_ItemScrew", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 122},
    {{"ftCo_MF_ItemScrewAir", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 124},
    {{"ftCo_MF_ItemScrewBase", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 121},
    {{"ftCo_MF_ItemThrow", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 101},
    {{"ftCo_MF_ItemThrow4", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 115},
    {{"ftCo_MF_ItemThrowAir", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 105},
    {{"ftCo_MF_ItemThrowAir4", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 116},
    {{"ftCo_MF_Jump", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 50},
    {{"ftCo_MF_JumpAir", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 51},
    {{"ftCo_MF_LGunShoot", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 102},
    {{"ftCo_MF_LGunShootAir", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 106},
    {{"ftCo_MF_LandingAirB", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 44},
    {{"ftCo_MF_LandingAirF", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 43},
    {{"ftCo_MF_LandingAirHi", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 45},
    {{"ftCo_MF_LandingAirLw", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 46},
    {{"ftCo_MF_LandingAirN", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 42},
    {{"ftCo_MF_LiftMove", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 126},
    {{"ftCo_MF_LiftTurn", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 127},
    {{"ftCo_MF_LiftWait", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 125},
    {{"ftCo_MF_LiftWalk", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 128},
    {{"ftCo_MF_LipstickSwing1", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 93},
    {{"ftCo_MF_LipstickSwing3", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 94},
    {{"ftCo_MF_LipstickSwing4", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 114},
    {{"ftCo_MF_LipstickSwingDash", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 95},
    {{"ftCo_MF_OttottoWait", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 150},
    {{"ftCo_MF_ParasolFall", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 129},
    {{"ftCo_MF_ParasolOpen", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 96},
    {{"ftCo_MF_ParasolSwing1", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 84},
    {{"ftCo_MF_ParasolSwing3", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 85},
    {{"ftCo_MF_ParasolSwing4", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 111},
    {{"ftCo_MF_ParasolSwingDash", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 86},
    {{"ftCo_MF_Pass", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 149},
    {{"ftCo_MF_Passive", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 147},
    {{"ftCo_MF_PassiveWall", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 146},
    {{"ftCo_MF_Rebirth", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 170},
    {{"ftCo_MF_Run", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 39},
    {{"ftCo_MF_Shouldered", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 169},
    {{"ftCo_MF_Sleep", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 173},
    {{"ftCo_MF_Special", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 174},
    {{"ftCo_MF_Squat", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 37},
    {{"ftCo_MF_StarRodSwing1", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 90},
    {{"ftCo_MF_StarRodSwing3", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 91},
    {{"ftCo_MF_StarRodSwing4", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 113},
    {{"ftCo_MF_StarRodSwingDash", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 92},
    {{"ftCo_MF_StopWall", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 148},
    {{"ftCo_MF_Swing4", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 108},
    {{"ftCo_MF_SwordSwing1", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 78},
    {{"ftCo_MF_SwordSwing3", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 79},
    {{"ftCo_MF_SwordSwing4", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 109},
    {{"ftCo_MF_SwordSwingDash", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 80},
    {{"ftCo_MF_Throw", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 159},
    {{"ftCo_MF_ThrowB", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 162},
    {{"ftCo_MF_ThrowF", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 161},
    {{"ftCo_MF_ThrowHi", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 163},
    {{"ftCo_MF_ThrowLw", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 164},
    {{"ftCo_MF_Thrown", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 167},
    {{"ftCo_MF_ThrownAir", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 168},
    {{"ftCo_MF_ThrownStar", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 171},
    {{"ftCo_MF_Turn", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 47},
    {{"ftCo_MF_Walk", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 48},
    {{"ftCo_MF_WarpStarFall", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 99},
    {{"ftCo_MS_AppealSL", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_AppealSL), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_AppealSR", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_AppealSR), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Attack100End", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Attack100End), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Attack100Loop", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Attack100Loop), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Attack100Start", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Attack100Start), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Attack11", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Attack11), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Attack12", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Attack12), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Attack13", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Attack13), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_AttackAirB", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_AttackAirB), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_AttackAirF", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_AttackAirF), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_AttackAirHi", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_AttackAirHi), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_AttackAirLw", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_AttackAirLw), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_AttackAirN", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_AttackAirN), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_AttackDash", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_AttackDash), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_AttackHi3", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_AttackHi3), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_AttackHi4", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_AttackHi4), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_AttackLw3", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_AttackLw3), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_AttackLw4", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_AttackLw4), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_AttackS3Hi", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_AttackS3Hi), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_AttackS3HiS", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_AttackS3HiS), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_AttackS3Lw", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_AttackS3Lw), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_AttackS3LwS", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_AttackS3LwS), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_AttackS3S", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_AttackS3S), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_AttackS4Hi", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_AttackS4Hi), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_AttackS4HiS", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_AttackS4HiS), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_AttackS4Lw", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_AttackS4Lw), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_AttackS4LwS", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_AttackS4LwS), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_AttackS4S", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_AttackS4S), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Barrel", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Barrel), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_BarrelWait", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_BarrelWait), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_BatSwing1", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_BatSwing1), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_BatSwing3", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_BatSwing3), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_BatSwing4", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_BatSwing4), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_BatSwingDash", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_BatSwingDash), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Bury", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Bury), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_BuryJump", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_BuryJump), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_BuryWait", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_BuryWait), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureCaptain", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureCaptain), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureCrazyHand", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureCrazyHand), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureCut", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureCut), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureDamageCrazyHand", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureDamageCrazyHand), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureDamageHi", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureDamageHi), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureDamageKoopa", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureDamageKoopa), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureDamageKoopaAir", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureDamageKoopaAir), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureDamageLw", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureDamageLw), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureDamageMasterHand", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureDamageMasterHand), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureFoot", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureFoot), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureJump", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureJump), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureKirby", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureKirby), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureKirbyYoshi", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureKirbyYoshi), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureKoopa", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureKoopa), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureKoopaAir", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureKoopaAir), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureLeadead", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureLeadead), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureLikelike", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureLikelike), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureMasterHand", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureMasterHand), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureMewtwo", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureMewtwo), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureMewtwoAir", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureMewtwoAir), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureNeck", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureNeck), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CapturePulledHi", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CapturePulledHi), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CapturePulledLw", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CapturePulledLw), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureWaitCrazyHand", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureWaitCrazyHand), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureWaitHi", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureWaitHi), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureWaitKirby", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureWaitKirby), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureWaitKoopa", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureWaitKoopa), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureWaitKoopaAir", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureWaitKoopaAir), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureWaitLw", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureWaitLw), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureWaitMasterHand", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureWaitMasterHand), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CaptureYoshi", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CaptureYoshi), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Catch", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Catch), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CatchAttack", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CatchAttack), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CatchCut", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CatchCut), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CatchDash", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CatchDash), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CatchDashPull", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CatchDashPull), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CatchPull", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CatchPull), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CatchWait", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CatchWait), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CliffAttackQuick", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CliffAttackQuick), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CliffAttackSlow", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CliffAttackSlow), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CliffCatch", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CliffCatch), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CliffClimbQuick", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CliffClimbQuick), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CliffClimbSlow", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CliffClimbSlow), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CliffEscapeQuick", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CliffEscapeQuick), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CliffEscapeSlow", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CliffEscapeSlow), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CliffJumpQuick1", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CliffJumpQuick1), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CliffJumpQuick2", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CliffJumpQuick2), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CliffJumpSlow1", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CliffJumpSlow1), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CliffJumpSlow2", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CliffJumpSlow2), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_CliffWait", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_CliffWait), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Count", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Count), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageAir1", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageAir1), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageAir2", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageAir2), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageAir3", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageAir3), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageBind", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageBind), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageFall", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageFall), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageFlyHi", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageFlyHi), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageFlyLw", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageFlyLw), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageFlyN", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageFlyN), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageFlyRoll", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageFlyRoll), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageFlyTop", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageFlyTop), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageHi1", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageHi1), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageHi2", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageHi2), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageHi3", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageHi3), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageIce", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageIce), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageIceJump", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageIceJump), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageLw1", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageLw1), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageLw2", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageLw2), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageLw3", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageLw3), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageN1", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageN1), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageN2", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageN2), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageN3", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageN3), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageScrew", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageScrew), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageScrewAir", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageScrewAir), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageSong", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageSong), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageSongRv", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageSongRv), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DamageSongWait", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DamageSongWait), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Dash", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Dash), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DeadDown", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DeadDown), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DeadLeft", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DeadLeft), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DeadRight", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DeadRight), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DeadUp", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DeadUp), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DeadUpFall", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DeadUpFall), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DeadUpFallHitCamera", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DeadUpFallHitCamera), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DeadUpFallHitCameraFlat", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DeadUpFallHitCameraFlat), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DeadUpFallHitCameraIce", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DeadUpFallHitCameraIce), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DeadUpFallIce", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DeadUpFallIce), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DeadUpStar", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DeadUpStar), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DeadUpStarIce", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DeadUpStarIce), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DownAttackD", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DownAttackD), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DownAttackU", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DownAttackU), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DownBackD", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DownBackD), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DownBackU", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DownBackU), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DownBoundD", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DownBoundD), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DownBoundU", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DownBoundU), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DownDamageD", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DownDamageD), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DownDamageU", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DownDamageU), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DownFowardD", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DownFowardD), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DownFowardU", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DownFowardU), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DownReflect", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DownReflect), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DownSpotD", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DownSpotD), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DownSpotU", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DownSpotU), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DownStandD", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DownStandD), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DownStandU", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DownStandU), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DownWaitD", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DownWaitD), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_DownWaitU", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_DownWaitU), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Entry", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Entry), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_EntryEnd", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_EntryEnd), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_EntryStart", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_EntryStart), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_EscapeAir", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_EscapeAir), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_EscapeB", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_EscapeB), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_EscapeF", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_EscapeF), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_EscapeN", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_EscapeN), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Fall", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Fall), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_FallAerial", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_FallAerial), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_FallAerialB", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_FallAerialB), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_FallAerialF", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_FallAerialF), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_FallB", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_FallB), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_FallF", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_FallF), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_FallSpecial", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_FallSpecial), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_FallSpecialB", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_FallSpecialB), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_FallSpecialF", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_FallSpecialF), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_FireFlowerShoot", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_FireFlowerShoot), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_FireFlowerShootAir", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_FireFlowerShootAir), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_FlyReflectCeil", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_FlyReflectCeil), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_FlyReflectWall", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_FlyReflectWall), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Furafura", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Furafura), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Guard", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Guard), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_GuardOff", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_GuardOff), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_GuardOn", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_GuardOn), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_GuardReflect", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_GuardReflect), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_GuardSetOff", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_GuardSetOff), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_HammerFall", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_HammerFall), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_HammerJump", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_HammerJump), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_HammerKneeBend", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_HammerKneeBend), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_HammerLanding", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_HammerLanding), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_HammerTurn", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_HammerTurn), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_HammerWait", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_HammerWait), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_HammerWalk", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_HammerWalk), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_HarisenSwing1", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_HarisenSwing1), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_HarisenSwing3", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_HarisenSwing3), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_HarisenSwing4", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_HarisenSwing4), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_HarisenSwingDash", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_HarisenSwingDash), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_HeavyGet", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_HeavyGet), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_HeavyThrowB", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_HeavyThrowB), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_HeavyThrowB4", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_HeavyThrowB4), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_HeavyThrowF", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_HeavyThrowF), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_HeavyThrowF4", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_HeavyThrowF4), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_HeavyThrowHi", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_HeavyThrowHi), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_HeavyThrowHi4", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_HeavyThrowHi4), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_HeavyThrowLw", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_HeavyThrowLw), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_HeavyThrowLw4", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_HeavyThrowLw4), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ItemParasolDamageFall", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ItemParasolDamageFall), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ItemParasolFall", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ItemParasolFall), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ItemParasolFallSpecial", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ItemParasolFallSpecial), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ItemParasolOpen", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ItemParasolOpen), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ItemScopeAirEnd", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ItemScopeAirEnd), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ItemScopeAirEndEmpty", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ItemScopeAirEndEmpty), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ItemScopeAirFire", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ItemScopeAirFire), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ItemScopeAirFireEmpty", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ItemScopeAirFireEmpty), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ItemScopeAirRapid", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ItemScopeAirRapid), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ItemScopeAirRapidEmpty", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ItemScopeAirRapidEmpty), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ItemScopeAirStart", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ItemScopeAirStart), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ItemScopeAirStartEmpty", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ItemScopeAirStartEmpty), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ItemScopeEnd", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ItemScopeEnd), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ItemScopeEndEmpty", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ItemScopeEndEmpty), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ItemScopeFire", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ItemScopeFire), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ItemScopeFireEmpty", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ItemScopeFireEmpty), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ItemScopeRapid", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ItemScopeRapid), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ItemScopeRapidEmpty", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ItemScopeRapidEmpty), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ItemScopeStart", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ItemScopeStart), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ItemScopeStartEmpty", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ItemScopeStartEmpty), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ItemScrew", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ItemScrew), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ItemScrewAir", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ItemScrewAir), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_JumpAerialB", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_JumpAerialB), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_JumpAerialF", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_JumpAerialF), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_JumpB", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_JumpB), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_JumpF", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_JumpF), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_KinokoGiantEnd", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_KinokoGiantEnd), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_KinokoGiantEndAir", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_KinokoGiantEndAir), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_KinokoGiantStart", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_KinokoGiantStart), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_KinokoGiantStartAir", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_KinokoGiantStartAir), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_KinokoSmallEnd", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_KinokoSmallEnd), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_KinokoSmallEndAir", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_KinokoSmallEndAir), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_KinokoSmallStart", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_KinokoSmallStart), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_KinokoSmallStartAir", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_KinokoSmallStartAir), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_KirbyYoshiEgg", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_KirbyYoshiEgg), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_KneeBend", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_KneeBend), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LGunShoot", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LGunShoot), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LGunShootAir", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LGunShootAir), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LGunShootAirEmpty", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LGunShootAirEmpty), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LGunShootEmpty", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LGunShootEmpty), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Landing", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Landing), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LandingAirB", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LandingAirB), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LandingAirF", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LandingAirF), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LandingAirHi", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LandingAirHi), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LandingAirLw", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LandingAirLw), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LandingAirN", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LandingAirN), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LandingFallSpecial", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LandingFallSpecial), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LiftTurn", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LiftTurn), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LiftWait", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LiftWait), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LiftWalk1", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LiftWalk1), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LiftWalk2", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LiftWalk2), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LightGet", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LightGet), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LightThrowAirB", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LightThrowAirB), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LightThrowAirB4", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LightThrowAirB4), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LightThrowAirF", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LightThrowAirF), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LightThrowAirF4", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LightThrowAirF4), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LightThrowAirHi", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LightThrowAirHi), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LightThrowAirHi4", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LightThrowAirHi4), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LightThrowAirLw", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LightThrowAirLw), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LightThrowAirLw4", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LightThrowAirLw4), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LightThrowB", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LightThrowB), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LightThrowB4", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LightThrowB4), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LightThrowDash", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LightThrowDash), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LightThrowDrop", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LightThrowDrop), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LightThrowF", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LightThrowF), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LightThrowF4", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LightThrowF4), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LightThrowHi", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LightThrowHi), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LightThrowHi4", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LightThrowHi4), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LightThrowLw", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LightThrowLw), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LightThrowLw4", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LightThrowLw4), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LipstickSwing1", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LipstickSwing1), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LipstickSwing3", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LipstickSwing3), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LipstickSwing4", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LipstickSwing4), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_LipstickSwingDash", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_LipstickSwingDash), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_MissFoot", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_MissFoot), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_None", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_None), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Ottotto", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Ottotto), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_OttottoWait", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_OttottoWait), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ParasolSwing1", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ParasolSwing1), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ParasolSwing3", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ParasolSwing3), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ParasolSwing4", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ParasolSwing4), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ParasolSwingDash", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ParasolSwingDash), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Pass", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Pass), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Passive", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Passive), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_PassiveCeil", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_PassiveCeil), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_PassiveStandB", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_PassiveStandB), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_PassiveStandF", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_PassiveStandF), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_PassiveWall", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_PassiveWall), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_PassiveWallJump", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_PassiveWallJump), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Rebirth", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Rebirth), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_RebirthWait", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_RebirthWait), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Rebound", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Rebound), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ReboundStop", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ReboundStop), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Run", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Run), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_RunBrake", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_RunBrake), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_RunDirect", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_RunDirect), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ShieldBreakDownD", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ShieldBreakDownD), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ShieldBreakDownU", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ShieldBreakDownU), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ShieldBreakFall", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ShieldBreakFall), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ShieldBreakFly", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ShieldBreakFly), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ShieldBreakStandD", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ShieldBreakStandD), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ShieldBreakStandU", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ShieldBreakStandU), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ShoulderedTurn", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ShoulderedTurn), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ShoulderedWait", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ShoulderedWait), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ShoulderedWalkFast", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ShoulderedWalkFast), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ShoulderedWalkMiddle", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ShoulderedWalkMiddle), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ShoulderedWalkSlow", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ShoulderedWalkSlow), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Sleep", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Sleep), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Squat", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Squat), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_SquatRv", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_SquatRv), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_SquatWait", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_SquatWait), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_StarRodSwing1", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_StarRodSwing1), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_StarRodSwing3", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_StarRodSwing3), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_StarRodSwing4", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_StarRodSwing4), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_StarRodSwingDash", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_StarRodSwingDash), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_StopCeil", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_StopCeil), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_StopWall", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_StopWall), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_SwordSwing1", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_SwordSwing1), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_SwordSwing3", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_SwordSwing3), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_SwordSwing4", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_SwordSwing4), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_SwordSwingDash", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_SwordSwingDash), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ThrowB", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ThrowB), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ThrowF", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ThrowF), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ThrowHi", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ThrowHi), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ThrowLw", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ThrowLw), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ThrownB", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ThrownB), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ThrownCopyStar", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ThrownCopyStar), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ThrownCrazyHand", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ThrownCrazyHand), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ThrownF", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ThrownF), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ThrownFB", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ThrownFB), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ThrownFF", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ThrownFF), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ThrownFHi", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ThrownFHi), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ThrownFLw", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ThrownFLw), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ThrownHi", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ThrownHi), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ThrownKirby", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ThrownKirby), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ThrownKirbyStar", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ThrownKirbyStar), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ThrownKoopaAirB", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ThrownKoopaAirB), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ThrownKoopaAirF", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ThrownKoopaAirF), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ThrownKoopaB", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ThrownKoopaB), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ThrownKoopaF", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ThrownKoopaF), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ThrownLw", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ThrownLw), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ThrownMasterHand", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ThrownMasterHand), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ThrownMewtwo", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ThrownMewtwo), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ThrownMewtwoAir", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ThrownMewtwoAir), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_ThrownlwWomen", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_ThrownlwWomen), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Turn", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Turn), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_TurnRun", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_TurnRun), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_Wait", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_Wait), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_WalkFast", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_WalkFast), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_WalkMiddle", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_WalkMiddle), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_WalkSlow", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_WalkSlow), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_WarpStarFall", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_WarpStarFall), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_WarpStarJump", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_WarpStarJump), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCo_MS_YoshiEgg", "src/melee/ft/kinds/ftCommon/forward.h:ftCommon_MotionState", (uint64_t)(int64_t)(ftCo_MS_YoshiEgg), NativeScriptType_I32, NativeScriptConstant_CommonMotion}, 0},
    {{"ftCommon_GroundAirColl_MF", "src/melee/ft/kinds/ftCommon/forward.h:MotionFlags", 0, NativeScriptType_U32, NativeScriptConstant_TransitionFlag}, 34},
};

static uint32_t flag_value(uint16_t id)
{
    switch (id) {
    case 1: return Ft_MF_None;
    case 2: return Ft_MF_KeepFastFall;
    case 3: return Ft_MF_KeepGfx;
    case 4: return Ft_MF_KeepColAnimHitStatus;
    case 5: return Ft_MF_SkipHit;
    case 6: return Ft_MF_SkipModel;
    case 7: return Ft_MF_SkipAnimVel;
    case 8: return Ft_MF_Unk06;
    case 9: return Ft_MF_SkipMatAnim;
    case 10: return Ft_MF_SkipThrowException;
    case 11: return Ft_MF_KeepSfx;
    case 12: return Ft_MF_SkipParasol;
    case 13: return Ft_MF_SkipRumble;
    case 14: return Ft_MF_SkipColAnim;
    case 15: return Ft_MF_KeepAccessory;
    case 16: return Ft_MF_UpdateCmd;
    case 17: return Ft_MF_SkipNametagVis;
    case 18: return Ft_MF_KeepColAnimPartHitStatus;
    case 19: return Ft_MF_KeepSwordTrail;
    case 20: return Ft_MF_SkipItemVis;
    case 21: return Ft_MF_Unk19;
    case 22: return Ft_MF_UnkUpdatePhys;
    case 23: return Ft_MF_FreezeState;
    case 24: return Ft_MF_SkipModelPartVis;
    case 25: return Ft_MF_SkipMetalB;
    case 26: return Ft_MF_Unk24;
    case 27: return Ft_MF_SkipAttackCount;
    case 28: return Ft_MF_SkipModelFlags;
    case 29: return Ft_MF_Unk27;
    case 30: return Ft_MF_SkipHitStun;
    case 31: return Ft_MF_SkipAnim;
    case 32: return Ft_MF_Unk30;
    case 33: return Ft_MF_Unk31;
    case 34: return ftCommon_GroundAirColl_MF;
    case 35: return ftCo_MF_5_6;
    case 36: return ftCo_MF_2_5_6;
    case 37: return ftCo_MF_Squat;
    case 38: return ftCo_MF_Dash;
    case 39: return ftCo_MF_Run;
    case 40: return ftCo_MF_Appeal;
    case 41: return ftCo_MF_9_10;
    case 42: return ftCo_MF_LandingAirN;
    case 43: return ftCo_MF_LandingAirF;
    case 44: return ftCo_MF_LandingAirB;
    case 45: return ftCo_MF_LandingAirHi;
    case 46: return ftCo_MF_LandingAirLw;
    case 47: return ftCo_MF_Turn;
    case 48: return ftCo_MF_Walk;
    case 49: return ftCo_MF_3_5_6;
    case 50: return ftCo_MF_Jump;
    case 51: return ftCo_MF_JumpAir;
    case 52: return ftCo_MF_GuardReflect;
    case 53: return ftCo_MF_Guard;
    case 54: return ftCo_MF_AttackBase;
    case 55: return ftCo_MF_Attack;
    case 56: return ftCo_MF_Attack_2;
    case 57: return ftCo_MF_AttackDash;
    case 58: return ftCo_MF_AttackS3;
    case 59: return ftCo_MF_AttackHi3;
    case 60: return ftCo_MF_AttackLw3;
    case 61: return ftCo_MF_CliffAttackQuick;
    case 62: return ftCo_MF_AttackAir;
    case 63: return ftCo_MF_AttackAirN;
    case 64: return ftCo_MF_AttackAirF;
    case 65: return ftCo_MF_AttackAirB;
    case 66: return ftCo_MF_AttackAirHi;
    case 67: return ftCo_MF_AttackAirLw;
    case 68: return ftCo_MF_Attack4;
    case 69: return ftCo_MF_AttackS4;
    case 70: return ftCo_MF_AttackHi4;
    case 71: return ftCo_MF_AttackLw4;
    case 72: return ftCo_MF_Attack1;
    case 73: return ftCo_MF_Attack11;
    case 74: return ftCo_MF_Attack12;
    case 75: return ftCo_MF_Attack13;
    case 76: return ftCo_MF_Attack100;
    case 77: return ftCo_MF_ItemScope;
    case 78: return ftCo_MF_SwordSwing1;
    case 79: return ftCo_MF_SwordSwing3;
    case 80: return ftCo_MF_SwordSwingDash;
    case 81: return ftCo_MF_BatSwing1;
    case 82: return ftCo_MF_BatSwing3;
    case 83: return ftCo_MF_BatSwingDash;
    case 84: return ftCo_MF_ParasolSwing1;
    case 85: return ftCo_MF_ParasolSwing3;
    case 86: return ftCo_MF_ParasolSwingDash;
    case 87: return ftCo_MF_HarisenSwing1;
    case 88: return ftCo_MF_HarisenSwing3;
    case 89: return ftCo_MF_HarisenSwingDash;
    case 90: return ftCo_MF_StarRodSwing1;
    case 91: return ftCo_MF_StarRodSwing3;
    case 92: return ftCo_MF_StarRodSwingDash;
    case 93: return ftCo_MF_LipstickSwing1;
    case 94: return ftCo_MF_LipstickSwing3;
    case 95: return ftCo_MF_LipstickSwingDash;
    case 96: return ftCo_MF_ParasolOpen;
    case 97: return ftCo_MF_HammerBase;
    case 98: return ftCo_MF_Hammer;
    case 99: return ftCo_MF_WarpStarFall;
    case 100: return ftCo_MF_ItemScope_5_6;
    case 101: return ftCo_MF_ItemThrow;
    case 102: return ftCo_MF_LGunShoot;
    case 103: return ftCo_MF_ItemScopeFire;
    case 104: return ftCo_MF_HammerFall;
    case 105: return ftCo_MF_ItemThrowAir;
    case 106: return ftCo_MF_LGunShootAir;
    case 107: return ftCo_MF_ItemScopeAir;
    case 108: return ftCo_MF_Swing4;
    case 109: return ftCo_MF_SwordSwing4;
    case 110: return ftCo_MF_BatSwing4;
    case 111: return ftCo_MF_ParasolSwing4;
    case 112: return ftCo_MF_HarisenSwing4;
    case 113: return ftCo_MF_StarRodSwing4;
    case 114: return ftCo_MF_LipstickSwing4;
    case 115: return ftCo_MF_ItemThrow4;
    case 116: return ftCo_MF_ItemThrowAir4;
    case 117: return ftCo_MF_HammerMove;
    case 118: return ftCo_MF_HammerTurn;
    case 119: return ftCo_MF_HammerWalk;
    case 120: return ftCo_MF_ItemFall;
    case 121: return ftCo_MF_ItemScrewBase;
    case 122: return ftCo_MF_ItemScrew;
    case 123: return ftCo_MF_HammerJump;
    case 124: return ftCo_MF_ItemScrewAir;
    case 125: return ftCo_MF_LiftWait;
    case 126: return ftCo_MF_LiftMove;
    case 127: return ftCo_MF_LiftTurn;
    case 128: return ftCo_MF_LiftWalk;
    case 129: return ftCo_MF_ParasolFall;
    case 130: return ftCo_MF_FireFlowerShoot;
    case 131: return ftCo_MF_ItemScopeRapid;
    case 132: return ftCo_MF_FireFlowerShootAir;
    case 133: return ftCo_MF_ItemScopeAirRapid;
    case 134: return ftCo_MF_Dazed;
    case 135: return ftCo_MF_Damage;
    case 136: return ftCo_MF_DamageScrew;
    case 137: return ftCo_MF_DamageScrewAir;
    case 138: return ftCo_MF_Down;
    case 139: return ftCo_MF_DownU;
    case 140: return ftCo_MF_DownD;
    case 141: return ftCo_MF_DownDamageU;
    case 142: return ftCo_MF_DownDamageD;
    case 143: return ftCo_MF_DownAttack;
    case 144: return ftCo_MF_DownAttackU;
    case 145: return ftCo_MF_DownAttackD;
    case 146: return ftCo_MF_PassiveWall;
    case 147: return ftCo_MF_Passive;
    case 148: return ftCo_MF_StopWall;
    case 149: return ftCo_MF_Pass;
    case 150: return ftCo_MF_OttottoWait;
    case 151: return ftCo_MF_CliffAction;
    case 152: return ftCo_MF_CliffAction_4_5;
    case 153: return ftCo_MF_CliffCatch;
    case 154: return ftCo_MF_CliffAttackSlow;
    case 155: return ftCo_MF_CliffWait;
    case 156: return ftCo_MF_CatchWait;
    case 157: return ftCo_MF_CatchBase;
    case 158: return ftCo_MF_Catch;
    case 159: return ftCo_MF_Throw;
    case 160: return ftCo_MF_CatchAttack;
    case 161: return ftCo_MF_ThrowF;
    case 162: return ftCo_MF_ThrowB;
    case 163: return ftCo_MF_ThrowHi;
    case 164: return ftCo_MF_ThrowLw;
    case 165: return ftCo_MF_Capture;
    case 166: return ftCo_MF_CaptureAir;
    case 167: return ftCo_MF_Thrown;
    case 168: return ftCo_MF_ThrownAir;
    case 169: return ftCo_MF_Shouldered;
    case 170: return ftCo_MF_Rebirth;
    case 171: return ftCo_MF_ThrownStar;
    case 172: return ftCo_MF_Dead;
    case 173: return ftCo_MF_Sleep;
    case 174: return ftCo_MF_Special;
    default: return 0;
    }
}

static const char* const namespaces[] = {
    "Motion", "Transition", "Hurt", "Hitbox", "DebugLevel", "AnimationFamily", "Collision", "Axis", "Callback", "Cpu", "Shield", "Smash",
};

uint32_t nativeScriptConstants_Count(void) { return (uint32_t)(sizeof(catalog)/sizeof(*catalog)); }

bool nativeScriptConstants_At(uint32_t index, NativeScriptConstant* output)
{
    if (!output || index>=nativeScriptConstants_Count()) return false;
    NativeScriptConstant value=catalog[index].value;
    if (catalog[index].native_flag) value.bits=flag_value(catalog[index].native_flag);
    *output=value;return true;
}

bool nativeScriptConstants_Lookup(const char* name, uint32_t length, NativeScriptConstant* output)
{
    if (!name || !output || !length) return false;
    uint32_t first=0,last=nativeScriptConstants_Count();
    while (first<last) {
        uint32_t middle=first+(last-first)/2;
        const char* candidate=catalog[middle].value.name;
        size_t candidate_length=strlen(candidate),n=length<candidate_length ? length : candidate_length;
        int compared=memcmp(name,candidate,n);
        if (!compared) compared=(length>candidate_length)-(length<candidate_length);
        if (!compared) return nativeScriptConstants_At(middle,output);
        if (compared<0) last=middle;else first=middle+1;
    }
    return false;
}

static int invalid(NativeScriptDiagnostic* diagnostic, NativeScriptSourceSpan span,const char* message)
{
    if (diagnostic) {diagnostic->source=span;snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message);}
    return -1;
}

static bool text_valid(const NativeScriptIRArena* arena,const NativeScriptIRNode* node)
{
    return arena && arena->source && node && node->text_offset<=arena->source_bytes &&
        node->text_length<=arena->source_bytes-node->text_offset;
}

int nativeScriptConstants_LookupNode(const NativeScriptIRArena* arena,const NativeScriptIRNode* node,
    NativeScriptConstant* scalar,NativeScriptIRResolution* space,NativeScriptDiagnostic* diagnostic)
{
    if (!arena || !node) return invalid(diagnostic,(NativeScriptSourceSpan){0},"Missing constant lookup source/node.");
    if (node->kind==NativeScriptIR_Name) {
        if (!text_valid(arena,node)) return invalid(diagnostic,node->source,"Constant identifier exceeds source bounds.");
        const char* name=arena->source+node->text_offset;
        for (uint32_t i=0;i<sizeof(namespaces)/sizeof(*namespaces);++i) {
            if (strlen(namespaces[i])==node->text_length && !memcmp(name,namespaces[i],node->text_length)) {
                if (!space) return invalid(diagnostic,node->source,"Missing constant namespace destination.");
                *space=(NativeScriptIRResolution){.type=NativeScriptType_Namespace,.semantic=NATIVE_SCRIPT_CONSTANT_NAMESPACE+i+1};return 2;
            }
        }
        if (!scalar) return invalid(diagnostic,node->source,"Missing constant literal destination.");
        return nativeScriptConstants_Lookup(name,node->text_length,scalar) ? 1 : 0;
    }
    if (node->a==NATIVE_SCRIPT_NODE_NONE || node->a>=arena->node_count || !arena->nodes) return 0;
    const NativeScriptIRNode* parent=&arena->nodes[node->a];
    uint32_t namespace_count=(uint32_t)(sizeof(namespaces)/sizeof(*namespaces));
    if (parent->type!=NativeScriptType_Namespace || parent->semantic<=NATIVE_SCRIPT_CONSTANT_NAMESPACE ||
        parent->semantic>NATIVE_SCRIPT_CONSTANT_NAMESPACE+namespace_count) return 0;
    if (node->kind!=NativeScriptIR_Member) return invalid(diagnostic,node->source,"Constant namespace permits only a named member.");
    if (!text_valid(arena,node)) return invalid(diagnostic,node->source,"Constant member exceeds source bounds.");
    const char* prefix=namespaces[parent->semantic-NATIVE_SCRIPT_CONSTANT_NAMESPACE-1];
    char name[96];size_t prefix_length=strlen(prefix);
    if (node->text_length>=sizeof(name)-prefix_length-1) return invalid(diagnostic,node->source,"Unknown constant member.");
    memcpy(name,prefix,prefix_length);name[prefix_length]='.';
    memcpy(name+prefix_length+1,arena->source+node->text_offset,node->text_length);
    if (!scalar) return invalid(diagnostic,node->source,"Missing constant literal destination.");
    if (!nativeScriptConstants_Lookup(name,(uint32_t)prefix_length+1+node->text_length,scalar))
        return invalid(diagnostic,node->source,"Unknown constant member.");
    return 1;
}

int nativeScriptConstants_TryResolve(void* unused,const NativeScriptIRArena* arena,
    const NativeScriptIRNode* node,NativeScriptIRResolution* output,NativeScriptDiagnostic* diagnostic)
{
    (void)unused;
    if (!output) return invalid(diagnostic,node ? node->source : (NativeScriptSourceSpan){0},"Missing constant resolution destination.");
    // Closed destinations are not integer enum aliases in the scalar catalog.
    const uint32_t pose_namespace=NATIVE_SCRIPT_CONSTANT_NAMESPACE+UINT32_C(0x100);
    if(node&&node->kind==NativeScriptIR_Name&&text_valid(arena,node)&&node->text_length==4&&
       !memcmp(arena->source+node->text_offset,"Pose",4)) {
        *output=(NativeScriptIRResolution){.type=NativeScriptType_Namespace,.semantic=pose_namespace};return 1;
    }
    if(node&&node->a!=NATIVE_SCRIPT_NODE_NONE&&node->a<arena->node_count&&arena->nodes&&
       arena->nodes[node->a].type==NativeScriptType_Namespace&&arena->nodes[node->a].semantic==pose_namespace) {
        if(node->kind!=NativeScriptIR_Member||!text_valid(arena,node))return invalid(diagnostic,node->source,"Pose destination requires a named member");
        uint64_t destination;
        if(node->text_length==4&&!memcmp(arena->source+node->text_offset,"live",4))destination=0;
        else if(node->text_length==5&&!memcmp(arena->source+node->text_offset,"blend",5))destination=1;
        else return invalid(diagnostic,node->source,"Unknown closed Pose destination");
        *output=(NativeScriptIRResolution){.type=NativeScriptType_PoseDestination,.semantic=NATIVE_SCRIPT_SEMANTIC_NONE,
            .flags=NativeScriptFlag_Literal,.literal_bits=destination};return 1;
    }
    NativeScriptConstant scalar;
    NativeScriptIRResolution space;
    int found=nativeScriptConstants_LookupNode(arena,node,&scalar,&space,diagnostic);
    if (found==1) *output=(NativeScriptIRResolution){.type=scalar.type,
        .semantic=NATIVE_SCRIPT_SEMANTIC_NONE,.flags=NativeScriptFlag_Literal,.literal_bits=scalar.bits};
    else if (found==2) *output=space;
    return found==2 ? 1 : found;
}
