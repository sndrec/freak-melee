#include "ftCo_Attack100.h"
#include "ftCo_CaptureWait.h"
#include <melee/ft/native_capture.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_script_receiver.h>
#include "ftCo_CaptureCut.h"
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftYoshi/ftyoshi.h>
#include <melee/it/kinds/itlinkhookshot.h>
#include <melee/it/kinds/itsamusgrapple.h>

bool fn_800DAD18(Fighter_GObj*);
static void fn_800DBBF8(Fighter_GObj*);

void ftCo_CaptureWait_Setup(Fighter_GObj* gobj)
{
    Fighter* caught = GET_FIGHTER(gobj);
    Fighter_GObj* holder_gobj = caught->victim_gobj;
    uint32_t holder_generation;
    if (!nativeCallbackSource_Snapshot(holder_gobj, NativeCallbackSource_Fighter, &holder_generation)) return;
    Fighter* holder = GET_FIGHTER(holder_gobj);
#ifdef MELEE_NATIVE
    const bool retained = nativeScriptFighter_IsAttached(gobj) ||
        (holder_gobj != NULL && nativeScriptFighter_IsAttached(holder_gobj));
    NativeFighterLifetime source_lifetime, peer_lifetime;
    if (retained) {
        uint32_t peer_generation;
        if (!nativeFighterLifetime_Capture(caught, &source_lifetime) ||
            (holder_gobj != NULL && (!nativeCallbackSource_Snapshot(holder_gobj, NativeCallbackSource_Fighter, &peer_generation) ||
                !nativeFighterLifetime_Capture(GET_FIGHTER(holder_gobj), &peer_lifetime) ||
                peer_lifetime.data->victim_gobj != gobj))) return;
    }
#endif

    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(holder->kind);
#ifdef MELEE_NATIVE
    if (nativeScriptFighter_IsAttached(holder_gobj) &&
        nativeScriptFighter_HasHook(holder_gobj, NativeScriptFighter_CaptureWaitSetup)) {
        NativeScriptValue other, result;
        if (!nativeScriptReceiver_CaptureExternal(gobj, NativeCallbackSource_Fighter,
                source_lifetime.reference.lifetime, &other, NULL) ||
            nativeScriptFighter_Run(holder_gobj, NativeScriptFighter_CaptureWaitSetup,
                &other, 1, NULL, &result) != NativeScriptVM_Ok) return;
    } else
#endif
    if (definition != NULL && definition->on_capture_wait_setup != NULL) {
        u32 caught_generation = caught->x8_spawnNum;
        u32 holder_generation = holder->x8_spawnNum;
        FighterKind holder_kind = holder->kind;
        FighterKind caught_kind = caught->kind;
        definition->on_capture_wait_setup(holder_gobj, gobj);
        if (!nativeCallbackSource_Alive(gobj, caught_generation, NativeCallbackSource_Fighter) ||
            !nativeCallbackSource_Alive(holder_gobj, holder_generation, NativeCallbackSource_Fighter)) return;
        caught = GET_FIGHTER(gobj);
        holder = GET_FIGHTER(holder_gobj);
        if (caught->kind != caught_kind || caught->victim_gobj != holder_gobj ||
            holder->kind != holder_kind || nativeFighterRuntime_Definition(holder_kind) != definition) return;
    } else if (holder->kind == Ft_Kind_Yoshi) {
        nativeCapture_EncloseCaught(holder_gobj, ftYs_Init_8012BAC0(holder));
    }
#ifdef MELEE_NATIVE
    if (retained && (!nativeFighterLifetime_Current(&source_lifetime) ||
        caught->victim_gobj != holder_gobj || (holder_gobj != NULL &&
            (!nativeFighterLifetime_Current(&peer_lifetime) || peer_lifetime.data->victim_gobj != gobj)))) return;
#endif
    ftCommon_8007E2F4(caught, 0x1FF);
}

void fn_800DB6C8(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    Fighter_GObj* holder_gobj = fp->victim_gobj;
    u32 generation = fp->x8_spawnNum;
    uint32_t holder_generation;
    if (!nativeCallbackSource_Snapshot(holder_gobj, NativeCallbackSource_Fighter, &holder_generation)) return;
    Fighter* victim_fp = GET_FIGHTER(holder_gobj);
#ifdef MELEE_NATIVE
    const bool retained = nativeScriptFighter_IsAttached(gobj) ||
        (holder_gobj != NULL && nativeScriptFighter_IsAttached(holder_gobj));
    NativeFighterLifetime source_lifetime, peer_lifetime;
    if (retained) {
        uint32_t peer_generation;
        if (!nativeFighterLifetime_Capture(fp, &source_lifetime) ||
            (holder_gobj != NULL && (!nativeCallbackSource_Snapshot(holder_gobj, NativeCallbackSource_Fighter, &peer_generation) ||
                !nativeFighterLifetime_Capture(GET_FIGHTER(holder_gobj), &peer_lifetime) ||
                peer_lifetime.data->victim_gobj != gobj))) return;
    }
#endif

    FighterKind kind = fp->kind;
    FighterKind holder_kind = victim_fp->kind;
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(holder_kind);

    if (fp->motion_id == ftCo_MS_CapturePulledHi) {
        fn_800DB790(gobj);
    } else {
        fn_800DBAE4(gobj);
    }

#ifdef MELEE_NATIVE
    if (retained && (!nativeFighterLifetime_Current(&source_lifetime) ||
        fp->victim_gobj != holder_gobj || (holder_gobj != NULL &&
            (!nativeFighterLifetime_Current(&peer_lifetime) || peer_lifetime.data->victim_gobj != gobj)))) return;
#endif
    if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter) ||
        !nativeCallbackSource_Alive(holder_gobj, holder_generation, NativeCallbackSource_Fighter)) return;
    fp = GET_FIGHTER(gobj);
    victim_fp = GET_FIGHTER(holder_gobj);
    if (fp->kind != kind || fp->victim_gobj != holder_gobj || victim_fp->kind != holder_kind ||
        nativeFighterRuntime_Definition(holder_kind) != definition) return;
#ifdef MELEE_NATIVE
    if (nativeScriptFighter_IsAttached(holder_gobj) && nativeScriptFighter_HasHook(holder_gobj, NativeScriptFighter_CaptureWaitEnter)) {
        NativeScriptValue other, result;
        if (!nativeScriptReceiver_CaptureExternal(gobj, NativeCallbackSource_Fighter, peer_lifetime.reference.lifetime, &other, NULL) ||
            nativeScriptFighter_Run(holder_gobj, NativeScriptFighter_CaptureWaitEnter, &other, 1, NULL, &result) != NativeScriptVM_Ok) return;
        if (retained && (!nativeFighterLifetime_Current(&source_lifetime) ||
            fp->victim_gobj != holder_gobj || (holder_gobj != NULL &&
                (!nativeFighterLifetime_Current(&peer_lifetime) || peer_lifetime.data->victim_gobj != gobj)))) return;
    } else
#endif
    if (definition != NULL && definition->on_capture_wait_enter != NULL) {
        definition->on_capture_wait_enter(holder_gobj, gobj);
        if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter) ||
            !nativeCallbackSource_Alive(holder_gobj, holder_generation, NativeCallbackSource_Fighter)) return;
        fp = GET_FIGHTER(gobj);
        victim_fp = GET_FIGHTER(holder_gobj);
        if (fp->kind != kind || fp->victim_gobj != holder_gobj || victim_fp->kind != holder_kind ||
            nativeFighterRuntime_Definition(holder_kind) != definition) return;
    } else switch (victim_fp->kind) {
    case Ft_Kind_Yoshi:
        ftCo_800DB368(victim_fp, fp);
        break;
    case Ft_Kind_Link:
    case Ft_Kind_CLink:
        if (victim_fp->u.lk.xC != NULL) {
            it_802A7840((HSD_GObj*) victim_fp->u.lk.xC);
        }
        break;
    case Ft_Kind_Samus:
        if (victim_fp->u.ss.x223C != NULL) {
            it_802BAA94(victim_fp->u.ss.x223C);
        }
        break;
    default:
        break;
    }

    ftCommon_8007EBAC(fp, 3, 0);
}

void fn_800DB790(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    u32 generation = fp->x8_spawnNum;
    FighterKind kind = fp->kind;
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(kind);
    Fighter_GObj* holder = fp->victim_gobj;
    uint32_t holder_generation = 0;
#ifdef MELEE_NATIVE
    const bool retained = nativeScriptFighter_IsAttached(gobj) ||
        (holder != NULL && nativeScriptFighter_IsAttached(holder));
    NativeFighterLifetime source_lifetime, peer_lifetime;
    if (retained) {
        uint32_t peer_generation;
        if (!nativeFighterLifetime_Capture(fp, &source_lifetime) ||
            (holder != NULL && (!nativeCallbackSource_Snapshot(holder, NativeCallbackSource_Fighter, &peer_generation) ||
                !nativeFighterLifetime_Capture(GET_FIGHTER(holder), &peer_lifetime) ||
                peer_lifetime.data->victim_gobj != gobj))) return;
    }
#endif

    if (holder != NULL && !nativeCallbackSource_Snapshot(holder,
            NativeCallbackSource_Fighter, &holder_generation)) return;
    Fighter_ChangeMotionState(gobj, ftCo_MS_CaptureWaitHi, 0, 0.0F, 1.0F, 0.0F,
                              NULL);
    if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter)) return;
    fp = GET_FIGHTER(gobj);
    if (fp->kind != kind || nativeFighterRuntime_Definition(kind) != definition) return;
    if (holder != NULL && (!nativeCallbackSource_Alive(holder, holder_generation,
            NativeCallbackSource_Fighter) || fp->victim_gobj != holder)) return;
#ifdef MELEE_NATIVE
    if (retained && (!nativeFighterLifetime_Current(&source_lifetime) ||
        fp->victim_gobj != holder || (holder != NULL &&
            (!nativeFighterLifetime_Current(&peer_lifetime) || peer_lifetime.data->victim_gobj != gobj)))) return;
#endif
    ftCo_CaptureWait_Setup(gobj);
}

void fn_800DB8A4(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->mv.co.capturewait.x0 += 1.0;
    fp->grab_timer -= p_ftCommonData->grab_timer_decrement;
    fp->mv.co.capturewait.x8 = ftCommon_GrabMash(fp, p_ftCommonData->x3A8);
}

void ftCo_CaptureWaitHi_Anim(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);

    fp->mv.co.capturewait.x0 += 1.0F;
    fp->grab_timer -= p_ftCommonData->grab_timer_decrement;
    fp->mv.co.capturewait.x8 = ftCommon_GrabMash(fp, p_ftCommonData->x3A8);
    if (fp->grab_timer <= 0.0F) {
        ftCo_800DA698(fp->victim_gobj, false);
        if (fp->mv.co.capturewait.xC != 0 || fn_800DC044(gobj)) {
            fn_800DC070(gobj);
            return;
        }

        ftCo_CaptureCut_Enter(gobj);
        return;
    }

    if (fp->mv.co.capturewait.x4 != 0.0F) {
        fp->mv.co.capturewait.x4 -= 1.0F;
        if (fp->mv.co.capturewait.x4 <= 0.0F &&
            fp->mv.co.capturewait.x8 == 0)
        {
            ftAnim_SetAnimRate(gobj, 1.0F);
            fp->mv.co.capturewait.x4 = 0.0F;
        }
    }

    if (fp->mv.co.capturewait.x4 <= 0.0F && fp->mv.co.capturewait.x8 != 0) {
        fp->mv.co.capturewait.x4 = p_ftCommonData->x3B0;
        ftAnim_SetAnimRate(gobj, p_ftCommonData->shouldered_anim_rate);
    }
}

void ftCo_CaptureWaitHi_IASA(Fighter_GObj* gobj)
{
    fn_800DC014(gobj);
}

void ftCo_CaptureWaitHi_Phys(Fighter_GObj* gobj)
{
    fn_800DAD18(gobj);
}

void ftCo_CaptureWaitHi_Coll(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (!fp->x2226_b2) {
        ft_80083C00(gobj, fn_800DBAC4);
    }
}

void fn_800DBAC4(Fighter_GObj* gobj)
{
    fn_800DBBF8(gobj);
}

void fn_800DBAE4(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    u32 generation = fp->x8_spawnNum;
    FighterKind kind = fp->kind;
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(kind);
    Fighter_GObj* holder = fp->victim_gobj;
    uint32_t holder_generation = 0;
#ifdef MELEE_NATIVE
    const bool retained = nativeScriptFighter_IsAttached(gobj) ||
        (holder != NULL && nativeScriptFighter_IsAttached(holder));
    NativeFighterLifetime source_lifetime, peer_lifetime;
    if (retained) {
        uint32_t peer_generation;
        if (!nativeFighterLifetime_Capture(fp, &source_lifetime) ||
            (holder != NULL && (!nativeCallbackSource_Snapshot(holder, NativeCallbackSource_Fighter, &peer_generation) ||
                !nativeFighterLifetime_Capture(GET_FIGHTER(holder), &peer_lifetime) ||
                peer_lifetime.data->victim_gobj != gobj))) return;
    }
#endif

    if (holder != NULL && !nativeCallbackSource_Snapshot(holder,
            NativeCallbackSource_Fighter, &holder_generation)) return;
    Fighter_ChangeMotionState(gobj, ftCo_MS_CaptureWaitLw, 0, 0.0F, 1.0F, 0.0F,
                              NULL);
    if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter)) return;
    fp = GET_FIGHTER(gobj);
    if (fp->kind != kind || nativeFighterRuntime_Definition(kind) != definition) return;
    if (holder != NULL && (!nativeCallbackSource_Alive(holder, holder_generation,
            NativeCallbackSource_Fighter) || fp->victim_gobj != holder)) return;
#ifdef MELEE_NATIVE
    if (retained && (!nativeFighterLifetime_Current(&source_lifetime) ||
        fp->victim_gobj != holder || (holder != NULL &&
            (!nativeFighterLifetime_Current(&peer_lifetime) || peer_lifetime.data->victim_gobj != gobj)))) return;
#endif
    ftCo_CaptureWait_Setup(gobj);
}

static inline void fn_800DBBF8_noinline(Fighter_GObj* gobj1, Fighter* gobj2)
{
    Fighter_GObj* fighter = gobj1;
    fn_800DAA40(fighter, gobj2->victim_gobj);
}

static void fn_800DBBF8(Fighter_GObj* gobj)
{
    Fighter* fp_before = GET_FIGHTER(gobj);
    Fighter_GObj* holder = fp_before->victim_gobj;
#ifdef MELEE_NATIVE
    const bool retained = nativeScriptFighter_IsAttached(gobj) ||
        (holder != NULL && nativeScriptFighter_IsAttached(holder));
    NativeFighterLifetime source_lifetime, peer_lifetime;
    if (retained) {
        uint32_t peer_generation;
        if (!nativeFighterLifetime_Capture(fp_before, &source_lifetime) ||
            (holder != NULL && (!nativeCallbackSource_Snapshot(holder, NativeCallbackSource_Fighter, &peer_generation) ||
                !nativeFighterLifetime_Capture(GET_FIGHTER(holder), &peer_lifetime) ||
                peer_lifetime.data->victim_gobj != gobj))) return;
    }
#endif


    ftCommon_8007D7FC(fp_before);
    Fighter_ChangeMotionState(gobj, ftCo_MS_CaptureWaitLw, Ft_MF_UpdateCmd,
                              fp_before->cur_anim_frame, 1.0F, 0.0F, NULL);

#ifdef MELEE_NATIVE
    if (retained && (!nativeFighterLifetime_Current(&source_lifetime) ||
        fp_before->victim_gobj != holder || (holder != NULL &&
            (!nativeFighterLifetime_Current(&peer_lifetime) || peer_lifetime.data->victim_gobj != gobj)))) return;
#endif
    ftCo_CaptureWait_Setup(gobj);
#ifdef MELEE_NATIVE
    if (retained && (!nativeFighterLifetime_Current(&source_lifetime) ||
        fp_before->victim_gobj != holder || (holder != NULL &&
            (!nativeFighterLifetime_Current(&peer_lifetime) || peer_lifetime.data->victim_gobj != gobj)))) return;
#endif

    fn_800DBBF8_noinline(gobj, fp_before);
}

void ftCo_CaptureWaitLw_Anim(Fighter_GObj* gobj)
{
    ftCo_CaptureWaitHi_Anim(gobj);
}

void ftCo_CaptureWaitLw_IASA(Fighter_GObj* gobj)
{
    fn_800DC014(gobj);
}

static inline void ftCo_CaptureWaitLw_Phys_inline(Fighter_GObj* gobj)
{
    register Fighter* temp_r31;
    temp_r31 = GET_FIGHTER(gobj);
    Fighter_GObj* holder = temp_r31->victim_gobj;
#ifdef MELEE_NATIVE
    const bool retained = nativeScriptFighter_IsAttached(gobj) ||
        (holder != NULL && nativeScriptFighter_IsAttached(holder));
    NativeFighterLifetime source_lifetime, peer_lifetime;
    if (retained) {
        uint32_t peer_generation;
        if (!nativeFighterLifetime_Capture(temp_r31, &source_lifetime) ||
            (holder != NULL && (!nativeCallbackSource_Snapshot(holder, NativeCallbackSource_Fighter, &peer_generation) ||
                !nativeFighterLifetime_Capture(GET_FIGHTER(holder), &peer_lifetime) ||
                peer_lifetime.data->victim_gobj != gobj))) return;
    }
#endif


    ftCommon_8007D5D4(temp_r31);
    ftCommon_UnlockECB(temp_r31);
    Fighter_ChangeMotionState(gobj, ftCo_MS_CaptureWaitHi, Ft_MF_UpdateCmd,
                              temp_r31->cur_anim_frame, 1.0F, 0.0F, NULL);
#ifdef MELEE_NATIVE
    if (retained && (!nativeFighterLifetime_Current(&source_lifetime) ||
        temp_r31->victim_gobj != holder || (holder != NULL &&
            (!nativeFighterLifetime_Current(&peer_lifetime) || peer_lifetime.data->victim_gobj != gobj)))) return;
#endif
    ftCo_CaptureWait_Setup(gobj);
#ifdef MELEE_NATIVE
    if (retained && (!nativeFighterLifetime_Current(&source_lifetime) ||
        temp_r31->victim_gobj != holder || (holder != NULL &&
            (!nativeFighterLifetime_Current(&peer_lifetime) || peer_lifetime.data->victim_gobj != gobj)))) return;
#endif

    fn_800DAA40(gobj, temp_r31->victim_gobj);
    if (!GET_FIGHTER(gobj)->x2226_b2) {
        ft_80083C00(gobj, fn_800DBAC4);
    }
#ifdef MELEE_NATIVE
    if (retained && (!nativeFighterLifetime_Current(&source_lifetime) ||
        temp_r31->victim_gobj != holder || (holder != NULL &&
            (!nativeFighterLifetime_Current(&peer_lifetime) || peer_lifetime.data->victim_gobj != gobj)))) return;
#endif
    HSD_JObjSetTranslate(GET_JOBJ(gobj), &temp_r31->cur_pos);
}

void ftCo_CaptureWaitLw_Phys(Fighter_GObj* gobj)
{
    PAD_STACK(8);

    if (fn_800DAD18(gobj)) {
        ftCo_CaptureWaitLw_Phys_inline(gobj);
    }
}

void ftCo_CaptureWaitLw_Coll(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (!fp->x2226_b2) {
        ft_8008403C(gobj, fn_800DBED4);
    }
}

static inline void fn_800DBED4_inline(Fighter_GObj* gobj)
{
    Fighter* temp_r31 = GET_FIGHTER(gobj);
    Fighter_GObj* holder = temp_r31->victim_gobj;
#ifdef MELEE_NATIVE
    const bool retained = nativeScriptFighter_IsAttached(gobj) ||
        (holder != NULL && nativeScriptFighter_IsAttached(holder));
    NativeFighterLifetime source_lifetime, peer_lifetime;
    if (retained) {
        uint32_t peer_generation;
        if (!nativeFighterLifetime_Capture(temp_r31, &source_lifetime) ||
            (holder != NULL && (!nativeCallbackSource_Snapshot(holder, NativeCallbackSource_Fighter, &peer_generation) ||
                !nativeFighterLifetime_Capture(GET_FIGHTER(holder), &peer_lifetime) ||
                peer_lifetime.data->victim_gobj != gobj))) return;
    }
#endif


    ftCommon_8007D5D4(temp_r31);
    ftCommon_UnlockECB(temp_r31);
    Fighter_ChangeMotionState(gobj, ftCo_MS_CaptureWaitHi, Ft_MF_UpdateCmd,
                              temp_r31->cur_anim_frame, 1.0F, 0.0F, NULL);
#ifdef MELEE_NATIVE
    if (retained && (!nativeFighterLifetime_Current(&source_lifetime) ||
        temp_r31->victim_gobj != holder || (holder != NULL &&
            (!nativeFighterLifetime_Current(&peer_lifetime) || peer_lifetime.data->victim_gobj != gobj)))) return;
#endif
    ftCo_CaptureWait_Setup(gobj);
#ifdef MELEE_NATIVE
    if (retained && (!nativeFighterLifetime_Current(&source_lifetime) ||
        temp_r31->victim_gobj != holder || (holder != NULL &&
            (!nativeFighterLifetime_Current(&peer_lifetime) || peer_lifetime.data->victim_gobj != gobj)))) return;
#endif

    fn_800DAA40(gobj, temp_r31->victim_gobj);
    if (!GET_FIGHTER(gobj)->x2226_b2) {
        ft_80083C00(gobj, fn_800DBAC4);
    }
#ifdef MELEE_NATIVE
    if (retained && (!nativeFighterLifetime_Current(&source_lifetime) ||
        temp_r31->victim_gobj != holder || (holder != NULL &&
            (!nativeFighterLifetime_Current(&peer_lifetime) || peer_lifetime.data->victim_gobj != gobj)))) return;
#endif
    HSD_JObjSetTranslate(GET_JOBJ(gobj), &temp_r31->cur_pos);
}

void fn_800DBED4(Fighter_GObj* gobj)
{
    PAD_STACK(4);
    fn_800DBED4_inline(gobj);
}

void fn_800DC014(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->mv.co.capturewait.x0 < p_ftCommonData->x3AC) {
        if (fp->input.pressed_buttons & HSD_PAD_XY) {
            fp->mv.co.capturewait.xC = true;
        }
    }
}

bool fn_800DC044(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->input.lstick[0].y >= p_ftCommonData->tap_jump_threshold) {
        return true;
    }
    return false;
}
