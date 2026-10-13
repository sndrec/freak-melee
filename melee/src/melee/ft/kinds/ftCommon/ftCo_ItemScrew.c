#include "ftCo_ItemScrew.h"

#include <Runtime/platform.h>

#include <melee/ft/forward.h>

#include <stdbool.h>

#include "forward.h"
#include "ftCo_Fall.h"
#include "ftCo_FallAerial.h"
#include "ftCo_FallSpecial.h"
#include "ftCo_Jump.h"
#include "ftCo_JumpAerial.h"
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/inlines.h>
#ifdef MELEE_NATIVE
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_script_fighter.h>
#endif
#include <melee/ft/types.h>

void ftCo_ItemScrew_Enter(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    ftCommon_8007D5D4(fp);
    Fighter_ChangeMotionState(gobj, ftCo_MS_ItemScrew, 0, 0.0F, 1.0F, 0.0F,
                              NULL);
    ftCo_800CB110(gobj, false, p_ftCommonData->x800);
    fp->x2227_b0 = true;
}

void ftCo_ItemScrew_Anim(Fighter_GObj* gobj)
{
    if (!ftAnim_IsFramesRemaining(gobj)) {
        ftCo_Fall_Enter(gobj);
    }
}

void ftCo_ItemScrew_IASA(Fighter_GObj* gobj)
{
    ftCo_Jump_IASA(gobj);
}

void ftCo_ItemScrew_Phys(Fighter_GObj* gobj)
{
    ftCo_Jump_Phys_Inner(gobj);
}

void ftCo_ItemScrew_Coll(Fighter_GObj* gobj)
{
    ft_800831CC(gobj, ftCo_80096CC8, ft_80082B1C);
}

void ft_800D2E7C(Fighter_GObj* gobj, Vec3* arg1)
{
    u8 _[8];
    Fighter* fp = GET_FIGHTER(gobj);
#ifdef MELEE_NATIVE
    NativeFighterLifetime lifetime;
    bool retained = nativeFighterLifetime_Capture(fp, &lifetime);
    if (!retained && nativeScriptFighter_Program(fp->kind)) return;
#endif
    arg1->x *= p_ftCommonData->x800;
    arg1->y *= p_ftCommonData->x800;
    *(volatile f32*) &arg1->z;
    ftCommon_8007D5D4(fp);
    bool worn_item_sound = fp->kind != Ft_Kind_Mewtwo && fp->kind != Ft_Kind_Ness;
#ifdef MELEE_NATIVE
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(fp->kind);
    if (retained && !nativeFighterLifetime_Current(&lifetime)) return;
    if (nativeScriptFighter_HasHook(gobj, NativeScriptFighter_WornItemJumpSound)) {
        NativeScriptValue result;
        if (!retained || nativeScriptFighter_Run(gobj, NativeScriptFighter_WornItemJumpSound,
                NULL, 0, NULL, &result) != NativeScriptVM_Ok ||
            result.type != NativeScriptType_Bool || !nativeFighterLifetime_Current(&lifetime)) return;
        worn_item_sound = result.as.u64 != 0;
    } else if (definition != NULL && definition->on_worn_item_jump_sound != NULL) {
        worn_item_sound = definition->on_worn_item_jump_sound(gobj);
        if (retained && !nativeFighterLifetime_Current(&lifetime)) return;
    }
#endif
    ftCo_JumpAerial_Begin(gobj, ftCo_MS_ItemScrewAir, arg1, true, false,
                          worn_item_sound);
}

void ftCo_ItemScrewAir_Anim(Fighter_GObj* gobj)
{
    if (!ftAnim_IsFramesRemaining(gobj)) {
        ftCo_FallAerial_Enter(gobj);
    }
}

void ftCo_ItemScrewAir_IASA(Fighter_GObj* gobj)
{
    ftCo_JumpAerial_IASA(gobj);
}

void ftCo_ItemScrewAir_Phys(Fighter_GObj* gobj)
{
    ft_80084DB0(gobj);
}

void ftCo_ItemScrewAir_Coll(Fighter_GObj* gobj)
{
    ft_800831CC(gobj, ftCo_80096CC8, ft_80082B1C);
}
