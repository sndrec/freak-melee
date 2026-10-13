#include "ftCo_FallAerial.h"

#include <melee/ft/forward.h>

#include "forward.h"
#include "ftCo_Fall.h"
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#ifdef MELEE_NATIVE
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_lifetime.h>
#endif

void ftCo_FallAerial_Enter(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
#ifdef MELEE_NATIVE
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(gobj, NativeCallbackSource_Fighter,
                                       &generation)) return;
    NativeFighterLifetime lifetime;
    bool source_owned=nativeScriptFighter_IsAttached(gobj);
    if(source_owned&&!nativeFighterLifetime_Capture(fp,&lifetime))return;
#endif
    Fighter_ChangeMotionState(gobj, ftCo_MS_FallAerial, Ft_MF_None, 0.0F, 1.0F,
                              0.0F, NULL);
#ifdef MELEE_NATIVE
    if (!nativeCallbackSource_Alive(gobj, generation,
                                    NativeCallbackSource_Fighter)) return;
    if(source_owned&&!nativeFighterLifetime_Current(&lifetime))return;
#endif
    fp->mv.co.fallaerial.smid = ftCo_SM_FallAerial;
    fp->mv.co.fallaerial.x4 = 0.0F;
}

void ftCo_FallAerial_Anim(Fighter_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    ftCo_Fall_Anim_Inner(gobj, &fp->mv.co.fallaerial.x4, ftCo_SM_FallAerial,
                         ftCo_SM_FallAerialF, ftCo_SM_FallAerialB);
    ftCo_800CC988(gobj, fp->mv.co.fallaerial.x4);
}

void ftCo_FallAerial_IASA(Fighter_GObj* gobj)
{
    RETURN_IF(ftCo_Fall_IASA_Inner(gobj));
}

void ftCo_FallAerial_Phys(Fighter_GObj* gobj)
{
    ft_80084DB0(gobj);
}
