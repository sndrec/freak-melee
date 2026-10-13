#include "fighter.h"
#include "ft_0892.h"
#include "ftcommon.h"
#include "ftdata.h"
#include "ftlib.h"
#include "inlines.h"
#include "native_fighter_runtime.h"
#include "native_callback_lifetime.h"
#include "native_fighter_lifetime.h"
#include "kinds/ftCLink/ftclink.h"
#include "kinds/ftCommon/ftCo_DownSpot.h"
#include "kinds/ftCommon/ftCo_HammerWait.h"
#include "kinds/ftCrazyHand/ftcrazyhandtagcancel.h"
#include "kinds/ftLink/ftlinkattackair.h"
#include "kinds/ftMasterHand/ftmasterhandwait12.h"
#include <melee/it/it_26B1.h>
#include <melee/it/kinds/itpeachparasol.h>
#include <melee/it/native_held_article.h>
#include <sysdolphin/baselib/gobj.h>

bool ft_8008A1FC(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);

    if ((fp->input.lstick[0].x * fp->facing_dir < 0.0F) ||
        (ABS(fp->input.lstick[0].x) < p_ftCommonData->walk_stick_threshold))
    {
        return true;
    }
    return false;
}

static inline bool ft_8008A244_inline(Fighter_GObj* gobj)
{
    Fighter* temp_r4 = GET_FIGHTER(gobj);
    f32 var_f2;

    var_f2 = temp_r4->input.lstick[0].x;
    if ((var_f2 * temp_r4->facing_dir < 0.0F) ||
        (ABS(var_f2) < p_ftCommonData->walk_stick_threshold))
    {
        return true;
    }
    return false;
}

bool ft_8008A244(Fighter_GObj* gobj)
{
    if (ft_8008A244_inline(gobj)) {
        ft_8008A2BC(gobj);
        return true;
    }

    return false;
}

/// Seems to be called to end many actions if no frames are remaining
void ft_8008A2BC(HSD_GObj* gobj)
{
    if (ftLib_GetKind(gobj) == Ft_Kind_MasterH) {
        ftMh_MS_389_80151018(gobj);
    } else if (ftLib_GetKind(gobj) == Ft_Kind_CrezyH) {
        ftCh_GrabUnk1_8015BC88(gobj);
    } else {
        ft_8008A348(gobj, 0);
    }
}

void ft_8008A324(HSD_GObj* gobj)
{
    ft_8008A348(gobj, -1.0f);
}

/// Transition state to ftCo_MS_Wait or otherwise neutral state
void ft_8008A348(Fighter_GObj* gobj, float anim_blend)
{
#ifdef MELEE_NATIVE
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(gobj, NativeCallbackSource_Fighter, &generation)) return;
#endif
    Fighter* fp = GET_FIGHTER(gobj);
#ifdef MELEE_NATIVE
    NativeFighterLifetime lifetime;
    if (!nativeFighterLifetime_Capture(fp, &lifetime)) return;
    const FighterKind source_kind = fp->kind;
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(source_kind);
#endif
    if (fp->x2224_b2) {
        ftCo_DownSpot_Enter(gobj);
        return;
    }
    if (ftCo_800C5240(gobj)) {
        ftCo_800C4ED8(gobj);
        return;
    }
    if (fp->ground_or_air == GA_Air) {
        ftCommon_8007D7FC(fp);
    }
#ifdef MELEE_NATIVE
    if (!nativeFighterLifetime_Current(&lifetime)) return;
    if (lifetime.source_owned) {
        NativeScriptValue argument = {.type=NativeScriptType_F32,.as.f32=anim_blend}, result;
        if (nativeScriptFighter_Run(gobj, NativeScriptFighter_NeutralEnter,
                &argument, 1, NULL, &result) != NativeScriptVM_Ok ||
            !nativeFighterLifetime_Current(&lifetime)) return;
    } else if (definition != NULL && definition->on_neutral_enter != NULL) {
        if (!nativeCallbackDispatch_Enter()) return;
        definition->on_neutral_enter(gobj, anim_blend);
        nativeCallbackDispatch_Leave();
        if (!nativeFighterLifetime_Current(&lifetime)) return;
    } else
#endif
    if (fp->kind == Ft_Kind_Peach) {
        if (fp->item_gobj != NULL &&
            itGetKind(fp->item_gobj) == It_Kind_Peach_Parasol)
        {
            bool claimed;
            nativeHeldArticle_NotifyNeutral(gobj, &claimed);
            if (!claimed) it_802BDB94(fp->item_gobj);
        }
    }
#ifdef MELEE_NATIVE
    if (!nativeFighterLifetime_Current(&lifetime)) return;
#endif
    Fighter_ChangeMotionState(gobj, ftCo_MS_Wait, Ft_MF_None, 0.0f, 1.0f,
                              anim_blend, NULL);
#ifdef MELEE_NATIVE
    if (!nativeFighterLifetime_Current(&lifetime)) return;
#endif
    if (ftCo_8008A698(fp) && ftData_80085FD4(fp, ftCo_MS_DeadUpFall)->x8 != 0)
    {
        ftCo_8008A6D8(gobj, ftCo_MS_DeadUpFall);
    }
#ifdef MELEE_NATIVE
    if (!nativeFighterLifetime_Current(&lifetime)) return;
#endif
    ftCommon_8007EFC0(fp, p_ftCommonData->x5F0);
#ifdef MELEE_NATIVE
    if (!nativeFighterLifetime_Current(&lifetime)) return;
    if (lifetime.source_owned) {
        NativeScriptValue result;
        (void)nativeScriptFighter_Run(gobj, NativeScriptFighter_WaitEnter, NULL, 0, NULL, &result);
        return;
    }
    if (definition != NULL && definition->on_wait_enter != NULL) {
        definition->on_wait_enter(gobj);
        return;
    }
#endif
    switch (fp->kind) {
    case Ft_Kind_Link:
        ftLk_AttackAir_800EB3BC(gobj);
        return;
    case Ft_Kind_CLink:
        ftCl_Init_8014919C(gobj);
        return;
    default:
        return;
    }
}
