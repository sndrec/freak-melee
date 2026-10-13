#include "ftCo_Throw.h"
#include <melee/ft/native_fighter_target.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_fighter_capture.h>

#include <Runtime/platform.h>

#include <melee/ft/forward.h>

#include <placeholder.h>
#include <stdbool.h>

#include "ftCo_CargoWait.h"
#include "ftCo_Damage.h"
#include "ftCo_Fall.h"
#include "ftCo_Thrown.h"
#include <melee/ft/fighter.h>
#ifdef MELEE_NATIVE
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_script_fighter.h>
#endif
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0DF1.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcamera.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftFox/ftfoxspecialn.h>
#include <melee/ft/kinds/ftMewtwo/ftmewtwospecialn.h>
#include <melee/ft/kinds/ftSamus/ftsamus.h>
#include <melee/ft/types.h>
#include <melee/gm/gm_unsplit.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lbcollision.h>
#include <melee/mp/mpcoll.h>
#include <melee/mp/mplib.h>
#include <melee/mp/types.h>
#include <melee/pl/player.h>
#include <melee/pl/plbonuslib.h>
#include <sysdolphin/baselib/jobj.h>

void fn_800DD568(Fighter_GObj* gobj);
void fn_800DD5EC(Fighter_GObj* gobj);
void fn_800DD684(Fighter_GObj* gobj);
bool fn_800DD6E4(Fighter_GObj* gobj, int arg);

void ftCo_800DD100(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    Fighter_GObj* victim = fp->victim_gobj;
    if (victim != NULL) {
        if (fp->x221B_b5) {
            ftCo_800DC920(gobj, victim);
        } else {
            ftCo_800DC920(victim, gobj);
        }
        ftCommon_8007D92C(victim);
    }
}

void ftCo_800DD168(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    Fighter_GObj* victim = fp->victim_gobj;
    if (victim != NULL) {
        if (fp->x221B_b5) {
            ftCo_800DCFD4(victim);
            ftCo_800DC920(gobj, victim);
        } else {
            ftCo_800DC920(victim, gobj);
            ftCommon_8007D92C(victim);
        }
    }
}

static inline bool ftCo_800DD1E4_inline1(Fighter* fp);
static inline bool ftCo_800DD1E4_inline1(Fighter* fp)
{
    float thresh = p_ftCommonData->x98;
    if ((fp->input.lstick[1].x < thresh && fp->input.lstick[0].x >= thresh) ||
        (fp->input.lstick[1].x > -thresh && fp->input.lstick[0].x <= -thresh))
    {
        return true;
    }
    return false;
}

static inline bool ftCo_800DD1E4_inline2(Fighter* fp);
static inline bool ftCo_800DD1E4_inline2(Fighter* fp)
{
    float thresh = p_ftCommonData->attackhi3_stick_threshold_y;
    if (fp->input.lstick[1].y < thresh && fp->input.lstick[0].y >= thresh) {
        return true;
    }
    return false;
}

static inline bool ftCo_800DD1E4_inline3(Fighter* fp);
static inline bool ftCo_800DD1E4_inline3(Fighter* fp)
{
    float thresh = p_ftCommonData->xB0;
    if (fp->input.lstick[1].y > thresh && fp->input.lstick[0].y <= thresh) {
        return true;
    }
    return false;
}

bool ftCo_800DD1E4(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    FtMotionId msid = fp->motion_id;
    if (ftCo_800DD1E4_inline1(fp)) {
        if (fp->input.lstick[0].x * fp->facing_dir > 0.0f) {
            msid = 219;
        } else {
            msid = 220;
        }
    } else if (ftCo_800DF7F4(fp)) {
        if (fp->input.cstick[0].x * fp->facing_dir > 0.0f) {
            msid = 219;
        } else {
            msid = 220;
        }
    } else if (ftCo_800DD1E4_inline2(fp) || ftCo_800DF844(fp)) {
        msid = 221;
    } else if (ftCo_800DD1E4_inline3(fp) || ftCo_800DF878(fp)) {
        msid = 222;
    }
    if (msid != fp->motion_id) {
        ftCo_800DD4B0(gobj, msid);
        return true;
    }
    return false;
}

void ftCo_800DD398(Fighter_GObj* gobj, FtMotionId msid, FtMotionId victim_msid,
                   float anim_speed)
{
    Fighter* fp = GET_FIGHTER(gobj);
    Fighter_GObj* caught = fp->victim_gobj;
#ifdef MELEE_NATIVE
    const bool retained = nativeScriptFighter_IsAttached(gobj) ||
        (caught != NULL && nativeScriptFighter_IsAttached(caught));
    NativeFighterCapturePair pair;
    if (retained && (!nativeFighterCapture_Retain(gobj, &pair) ||
        pair.victim.object != caught || !caught ||
        !nativeFighterCapture_Attached(&pair))) return;
#endif

    const uint32_t callback_generation = (uint32_t)fp->x8_spawnNum;
    PAD_STACK(0x10);
    fp->cmd_vars[0] = 0;
    fp->throw_flags = 0;
    fp->mv.co.throw.x4 = 0;
    fp->mv.co.throw.x8 = 0;
    Fighter_ChangeMotionState(gobj, msid, 0, 0.0f, anim_speed, 0.0f, NULL);
#ifdef MELEE_NATIVE
    if (retained && !nativeFighterCapture_Attached(&pair)) return;
#endif
    if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
    ftAnim_8006EBA4(gobj);
#ifdef MELEE_NATIVE
    if (retained && !nativeFighterCapture_Attached(&pair)) return;
#endif
    ftCommon_8007E2F4(fp, 0x1FF);
    ftCo_800DE3FC(fp->victim_gobj, victim_msid, anim_speed);
#ifdef MELEE_NATIVE
    if (retained && !nativeFighterCapture_Attached(&pair)) return;
#endif
#ifdef MELEE_NATIVE
    if (nativeScriptFighter_IsAttached(gobj)) {
        // This is the stock accessory slot, after the victim's full setup.
        // The earlier throw_victim_setup hook deliberately remains separate.
        const NativeScriptValue arguments[] = {
            { .type=NativeScriptType_I32, .as.i64=msid },
            { .type=NativeScriptType_F32, .as.f32=anim_speed },
        };
        if (nativeScriptFighter_Run(gobj, NativeScriptFighter_ThrowPresentation,
                arguments, 2, NULL, NULL) != NativeScriptVM_Ok) return;
    } else
#endif
    switch (fp->kind) {
    case Ft_Kind_Kirby:
        if (msid == 221) {
            Fighter* victim = GET_FIGHTER(fp->victim_gobj);
            fp->x2219_b2 = victim->x2219_b2 = 1;
        }
        break;
    case Ft_Kind_Samus:
        ftSs_Init_CreateThrowGrappleBeam(gobj, msid, anim_speed);
        break;
    default:
        break;
    }
#ifdef MELEE_NATIVE
    if (retained && !nativeFighterCapture_Attached(&pair)) return;
#endif
    ftColl_8007B7A4(gobj, p_ftCommonData->x348);
}

void ftCo_800DD4B0(Fighter_GObj* gobj, FtMotionId msid)
{
    Fighter* fp = GET_FIGHTER(gobj);
    Fighter_GObj* caught = fp->victim_gobj;
#ifdef MELEE_NATIVE
    const bool retained = nativeScriptFighter_IsAttached(gobj) ||
        (caught != NULL && nativeScriptFighter_IsAttached(caught));
    NativeFighterLifetime source_lifetime, peer_lifetime;
    if (retained) {
        uint32_t peer_generation;
        if (!nativeFighterLifetime_Capture(fp, &source_lifetime) ||
            (caught != NULL && (!nativeCallbackSource_Snapshot(caught, NativeCallbackSource_Fighter, &peer_generation) ||
                !nativeFighterLifetime_Capture(GET_FIGHTER(caught), &peer_lifetime) ||
                peer_lifetime.data->victim_gobj != gobj))) return;
    }
#endif

    const uint32_t callback_generation = (uint32_t)fp->x8_spawnNum;
    Fighter* victim = GET_FIGHTER(fp->victim_gobj);
    int throw_index = msid - 219;
    FtMotionId victim_msid = throw_index + 239;
    float anim_speed;
    if (fp->ft_data->x0->weight_independent_throws_mask & (1 << throw_index)) {
        anim_speed = 1.0f;
    } else {
        anim_speed =
            1.0f / (victim->ft_data->x0->weight * p_ftCommonData->x37C);
    }
#ifdef MELEE_NATIVE
    const NativeFighterDefinition* authored = nativeFighterRuntime_Definition(fp->kind);
    if (nativeScriptFighter_HasHook(gobj, NativeScriptFighter_ThrowVictimMotion)) {
        NativeScriptValue arguments[2]={{.type=NativeScriptType_I32,.as.i64=msid},
            {.type=NativeScriptType_I32,.as.i64=victim_msid}}, result;
        if(nativeScriptFighter_Run(gobj,NativeScriptFighter_ThrowVictimMotion,
                arguments,2,NULL,&result)!=NativeScriptVM_Ok || result.type!=NativeScriptType_I32 ||
           !nativeFighterLifetime_Current(&source_lifetime) || fp->victim_gobj!=caught ||
           !nativeFighterLifetime_Current(&peer_lifetime) || peer_lifetime.data->victim_gobj!=gobj)return;
        if(result.as.i64>=0 && result.as.i64<ftCo_MS_Count)victim_msid=(FtMotionId)result.as.i64;
    } else if (authored != NULL && authored->on_throw_victim_motion != NULL) {
        int selected = authored->on_throw_victim_motion(gobj, msid, victim_msid);
        if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
        if (retained && (!nativeFighterLifetime_Current(&source_lifetime) ||
            fp->victim_gobj != caught || (caught != NULL &&
                (!nativeFighterLifetime_Current(&peer_lifetime) || peer_lifetime.data->victim_gobj != gobj)))) return;
        if (selected >= 0 && selected < ftCo_MS_Count) victim_msid = selected;
    } else
#endif
    switch (msid) {
    case 222:
        switch (fp->kind) {
        case Ft_Kind_GKoops:
        case Ft_Kind_Koopa:
        {
            // The victim's copied animation family owns the alternate pose;
            // an independent package has no stock fighter-kind identity.
            FighterKind victim_family = nativeFighterRuntime_AnimationFamily(victim);
            if (victim_family == Ft_Kind_Peach || victim_family == Ft_Kind_Zelda) {
                victim_msid = 243;
            }
            break;
        }
        default:
            break;
        }
    default:
        break;
    }
    ftCo_800DD398(gobj, msid, victim_msid, anim_speed);
}

void fn_800DD568(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    const uint32_t callback_generation = (uint32_t)fp->x8_spawnNum;
    ftAnim_SetAnimRate(gobj, 1.0f);
    ftAnim_8006EBA4(gobj);
    if (fp->anim_cb != NULL) {
        fp->anim_cb(gobj);
        if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
    }
    fp->mv.co.throw.x4 = 0;
    fp->mv.co.throw.x8 = 1;
    if (fp->victim_gobj != NULL) {
        ftCo_800DE974(fp->victim_gobj);
    }
    ftCommon_8007D7FC(fp);
}

void fn_800DD5EC(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    const uint32_t callback_generation = (uint32_t)fp->x8_spawnNum;
    while (fp->cmd_vars[0] == 0) {
        ftAnim_8006EBA4(gobj);
    }
    if (fp->anim_cb != NULL) {
        fp->anim_cb(gobj);
        if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
    }
    ftAnim_SetAnimRate(gobj, 1.0f);
    fp->mv.co.throw.x4 = 0;
    fp->mv.co.throw.x8 = 1;
    if (fp->victim_gobj != NULL) {
        ftCo_800DE974(fp->victim_gobj);
    }
    ftCommon_8007D7FC(fp);
}

void fn_800DD684(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    Fighter_GObj* victim = fp->victim_gobj;
    if (victim != NULL) {
        ftCo_800DC920(gobj, victim);
        ftCo_Fall_Enter(victim);
    }
    ftCo_Fall_Enter(gobj);
}

bool fn_800DD6E4(Fighter_GObj* gobj, int arg)
{
    if (arg != -1 && (mpLineGetFlags(arg) & LINE_FLAG_PLATFORM) == 0) {
        return true;
    }
    return false;
}

static inline bool ftCo_800DD724_inline1(Fighter* fp)
{
    if (fp->throw_flags_b4) {
        fp->throw_flags_b4 = 0;
        return true;
    }
    return false;
}

static inline bool ftCo_800DD724_inline2(Fighter* fp)
{
    if (fp->throw_flags_b3) {
        fp->throw_flags_b3 = 0;
        return true;
    }
    return false;
}

void ftCo_800DD724(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    NativeFighterCapturePair pair;
    if (!nativeFighterCapture_Retain(gobj, &pair) ||
        (pair.victim.object && !nativeFighterCapture_Attached(&pair))) return;
    PAD_STACK(8);
    if (ftCo_800DD724_inline1(fp)) {
        fp->facing_dir = -(fp->facing_dir);
    }
    if (ftCo_800DD724_inline2(fp)) {
        Fighter_GObj* victim = fp->victim_gobj;
        ftCommon_8007E2F4(fp, 0);
        if (victim != NULL) {
            pl_80040614(fp->player_id, fp->x221F_b4,
                        GET_FIGHTER(victim)->grab_timer);
            if (!nativeFighterCapture_Attached(&pair)) return;
            ftCo_800DE2A8(gobj, victim);
            if (!nativeFighterCapture_Current(&pair, false)) return;
            ftCo_800DE7C0(victim, gobj, fp->motion_id == 222);
            if (!nativeFighterCapture_Current(&pair, false) ||
                (fp->victim_gobj && !nativeFighterCapture_Attached(&pair))) return;
        }
    }
    if (fp->cmd_vars[0] != 0 && fp->mv.co.throw.x4 == 0) {
        fp->mv.co.throw.x4 = 1;
        fp->cmd_vars[0] = 0;
        fp->mv.co.throw.xC = fp->x6A4_transNOffset;
        ftAnim_SetAnimRate(gobj, 0.0f);
        if (!nativeFighterCapture_Current(&pair, false) ||
            (fp->victim_gobj && !nativeFighterCapture_Attached(&pair))) return;
        if (fp->victim_gobj != NULL) {
            ftCo_800DE920(fp->victim_gobj, fp->cur_anim_frame);
            if (!nativeFighterCapture_Attached(&pair)) return;
        }
        if (fp->kind == Ft_Kind_Kirby && fp->motion_id == 221) {
            fp->x2219_b2 = 0;
            if (fp->victim_gobj != NULL) {
                GET_FIGHTER(fp->victim_gobj)->x2219_b2 = 0;
            }
        }
    }
}

void ftCo_ThrowF_Anim(Fighter_GObj* gobj)
{
    const uint32_t callback_generation = (uint32_t)GET_FIGHTER(gobj)->x8_spawnNum;
    PAD_STACK(8);
#ifdef MELEE_NATIVE
    const NativeFighterDefinition* authored =
        nativeFighterRuntime_Definition(GET_FIGHTER(gobj)->kind);
    if(nativeScriptFighter_IsAttached(gobj)) {
        NativeFighterLifetime lifetime;NativeScriptValue result;
        if(!nativeFighterLifetime_Capture(GET_FIGHTER(gobj),&lifetime) ||
           nativeScriptFighter_Run(gobj,NativeScriptFighter_ThrowAnimation,
               NULL,0,NULL,&result)!=NativeScriptVM_Ok || result.type!=NativeScriptType_Void ||
           !nativeFighterLifetime_Current(&lifetime))return;
    } else if (authored != NULL && authored->on_throw_animation != NULL) {
        authored->on_throw_animation(gobj);
        if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
    } else
#endif
        ftMt_SpecialN_Shoot(gobj);
    ftCo_800DD724(gobj);
    if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
    if (ftAnim_IsFramesRemaining(gobj) == 0) {
#ifdef MELEE_NATIVE
        if(nativeScriptFighter_IsAttached(gobj)) {
            NativeFighterLifetime lifetime;NativeScriptValue result;
            if(!nativeFighterLifetime_Capture(GET_FIGHTER(gobj),&lifetime) ||
               nativeScriptFighter_Run(gobj,NativeScriptFighter_ThrowFinish,
                   NULL,0,NULL,&result)!=NativeScriptVM_Ok || result.type!=NativeScriptType_Bool ||
               !nativeFighterLifetime_Current(&lifetime))return;
            if(!result.as.u64)ftCommon_8007D92C(gobj);
        } else if (authored != NULL && authored->on_throw_finish != NULL) {
            bool handled = authored->on_throw_finish(gobj);
            if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
            if (!handled) ftCommon_8007D92C(gobj);
        } else
#endif
        if (GET_FIGHTER(gobj)->x2222_b0) {
            ftCo_8009B56C(gobj);
        } else {
            ftCommon_8007D92C(gobj);
        }
    }
}

void ftCo_ThrowF_IASA(Fighter_GObj* gobj)
{
    return;
}

void ftCo_ThrowF_Phys(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->ground_or_air == GA_Air) {
        if (fp->mv.co.throw.x4 != 0) {
            fp->self_vel.x = fp->mv.co.throw.xC.z * fp->facing_dir;
            fp->self_vel.y = fp->mv.co.throw.xC.y;
        } else {
            ft_80085134(gobj);
        }
    } else {
        ft_80085004(gobj);
    }
}

void ftCo_ThrowF_Coll(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->ground_or_air == GA_Air) {
        if (fp->mv.co.throw.x4 != 0) {
            ft_80083C00(gobj, fn_800DD568);
        } else if (fp->mv.co.throw.x8 == 0) {
            ft_80083CE4(gobj, fn_800DD6E4, fn_800DD5EC);
        } else {
            ft_80083B68(gobj);
        }
    } else {
        ft_800841B8(gobj, fn_800DD684);
    }
}

void ftCo_ThrowB_Anim(Fighter_GObj* gobj)
{
    const uint32_t callback_generation = (uint32_t)GET_FIGHTER(gobj)->x8_spawnNum;
#ifdef MELEE_NATIVE
    const NativeFighterDefinition* authored =
        nativeFighterRuntime_Definition(GET_FIGHTER(gobj)->kind);
    if(nativeScriptFighter_IsAttached(gobj)) {
        NativeFighterLifetime lifetime;NativeScriptValue result;
        if(!nativeFighterLifetime_Capture(GET_FIGHTER(gobj),&lifetime) ||
           nativeScriptFighter_Run(gobj,NativeScriptFighter_ThrowAnimation,
               NULL,0,NULL,&result)!=NativeScriptVM_Ok || result.type!=NativeScriptType_Void ||
           !nativeFighterLifetime_Current(&lifetime))return;
    } else if (authored != NULL && authored->on_throw_animation != NULL) {
        authored->on_throw_animation(gobj);
        if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
    } else
#endif
        ftFx_Throw_Anim(gobj);
    ftCo_800DD724(gobj);
    if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
    if (ftAnim_IsFramesRemaining(gobj) == 0) {
#ifdef MELEE_NATIVE
        if(nativeScriptFighter_IsAttached(gobj)) {
            NativeFighterLifetime lifetime;NativeScriptValue result;
            if(!nativeFighterLifetime_Capture(GET_FIGHTER(gobj),&lifetime) ||
               nativeScriptFighter_Run(gobj,NativeScriptFighter_ThrowFinish,
                   NULL,0,NULL,&result)!=NativeScriptVM_Ok || result.type!=NativeScriptType_Bool ||
               !nativeFighterLifetime_Current(&lifetime))return;
            if(result.as.u64)return;
        } else if (authored != NULL && authored->on_throw_finish != NULL) {
            bool handled = authored->on_throw_finish(gobj);
            if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
            if (handled) return;
        }
#endif
        ftCommon_8007D92C(gobj);
    }
}

void ftCo_ThrowB_IASA(Fighter_GObj* gobj)
{
    return;
}

void ftCo_ThrowB_Phys(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->ground_or_air == GA_Air) {
        if (fp->mv.co.throw.x4 != 0) {
            fp->self_vel.x = fp->mv.co.throw.xC.z * fp->facing_dir;
            fp->self_vel.y = fp->mv.co.throw.xC.y;
        } else {
            ft_80085134(gobj);
        }
    } else {
        ft_80085004(gobj);
    }
}

void ftCo_ThrowB_Coll(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->ground_or_air == GA_Air) {
        if (fp->mv.co.throw.x4 != 0) {
            ft_80083C00(gobj, fn_800DD568);
        } else if (fp->mv.co.throw.x8 == 0) {
            ft_80083CE4(gobj, fn_800DD6E4, fn_800DD5EC);
        } else {
            ft_80083B68(gobj);
        }
    } else {
        ft_800841B8(gobj, fn_800DD684);
    }
}

void ftCo_ThrowHi_Anim(Fighter_GObj* gobj)
{
    const uint32_t callback_generation = (uint32_t)GET_FIGHTER(gobj)->x8_spawnNum;
#ifdef MELEE_NATIVE
    const NativeFighterDefinition* authored =
        nativeFighterRuntime_Definition(GET_FIGHTER(gobj)->kind);
    if(nativeScriptFighter_IsAttached(gobj)) {
        NativeFighterLifetime lifetime;NativeScriptValue result;
        if(!nativeFighterLifetime_Capture(GET_FIGHTER(gobj),&lifetime) ||
           nativeScriptFighter_Run(gobj,NativeScriptFighter_ThrowAnimation,
               NULL,0,NULL,&result)!=NativeScriptVM_Ok || result.type!=NativeScriptType_Void ||
           !nativeFighterLifetime_Current(&lifetime))return;
    } else if (authored != NULL && authored->on_throw_animation != NULL) {
        authored->on_throw_animation(gobj);
        if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
    } else
#endif
        ftFx_Throw_Anim(gobj);
    ftCo_800DD724(gobj);
    if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
    if (ftAnim_IsFramesRemaining(gobj) == 0) {
#ifdef MELEE_NATIVE
        if(nativeScriptFighter_IsAttached(gobj)) {
            NativeFighterLifetime lifetime;NativeScriptValue result;
            if(!nativeFighterLifetime_Capture(GET_FIGHTER(gobj),&lifetime) ||
               nativeScriptFighter_Run(gobj,NativeScriptFighter_ThrowFinish,
                   NULL,0,NULL,&result)!=NativeScriptVM_Ok || result.type!=NativeScriptType_Bool ||
               !nativeFighterLifetime_Current(&lifetime))return;
            if(result.as.u64)return;
        } else if (authored != NULL && authored->on_throw_finish != NULL) {
            bool handled = authored->on_throw_finish(gobj);
            if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
            if (handled) return;
        }
#endif
        ftCommon_8007D92C(gobj);
    }
}

void ftCo_ThrowHi_IASA(Fighter_GObj* gobj)
{
    return;
}

void ftCo_ThrowHi_Phys(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->ground_or_air == GA_Air) {
        if (fp->mv.co.throw.x4 != 0) {
            fp->self_vel.x = fp->mv.co.throw.xC.z * fp->facing_dir;
            fp->self_vel.y = fp->mv.co.throw.xC.y;
        } else {
            ft_80085134(gobj);
        }
    } else {
        ft_80085004(gobj);
    }
}

void ftCo_ThrowHi_Coll(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->ground_or_air == GA_Air) {
        if (fp->mv.co.throw.x4 != 0) {
            ft_80083C00(gobj, fn_800DD568);
        } else if (fp->mv.co.throw.x8 == 0) {
            ft_80083CE4(gobj, fn_800DD6E4, fn_800DD5EC);
        } else {
            ft_80083B68(gobj);
        }
    } else {
        ft_800841B8(gobj, fn_800DD684);
    }
}

void ftCo_ThrowHi_Cam(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->kind == Ft_Kind_Kirby && fp->motion_id == 221) {
        ftCamera_800762F4(gobj);
    } else {
        ftCamera_UpdateCameraBox(gobj);
    }
}

void ftCo_ThrowLw_Anim(Fighter_GObj* gobj)
{
    const uint32_t callback_generation = (uint32_t)GET_FIGHTER(gobj)->x8_spawnNum;
#ifdef MELEE_NATIVE
    const NativeFighterDefinition* authored =
        nativeFighterRuntime_Definition(GET_FIGHTER(gobj)->kind);
    if(nativeScriptFighter_IsAttached(gobj)) {
        NativeFighterLifetime lifetime;NativeScriptValue result;
        if(!nativeFighterLifetime_Capture(GET_FIGHTER(gobj),&lifetime) ||
           nativeScriptFighter_Run(gobj,NativeScriptFighter_ThrowAnimation,
               NULL,0,NULL,&result)!=NativeScriptVM_Ok || result.type!=NativeScriptType_Void ||
           !nativeFighterLifetime_Current(&lifetime))return;
    } else if (authored != NULL && authored->on_throw_animation != NULL) {
        authored->on_throw_animation(gobj);
        if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
    } else
#endif
        ftFx_Throw_Anim(gobj);
    ftCo_800DD724(gobj);
    if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
    if (ftAnim_IsFramesRemaining(gobj) == 0) {
#ifdef MELEE_NATIVE
        if(nativeScriptFighter_IsAttached(gobj)) {
            NativeFighterLifetime lifetime;NativeScriptValue result;
            if(!nativeFighterLifetime_Capture(GET_FIGHTER(gobj),&lifetime) ||
               nativeScriptFighter_Run(gobj,NativeScriptFighter_ThrowFinish,
                   NULL,0,NULL,&result)!=NativeScriptVM_Ok || result.type!=NativeScriptType_Bool ||
               !nativeFighterLifetime_Current(&lifetime))return;
            if(result.as.u64)return;
        } else if (authored != NULL && authored->on_throw_finish != NULL) {
            bool handled = authored->on_throw_finish(gobj);
            if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
            if (handled) return;
        }
#endif
        ftCommon_8007D92C(gobj);
    }
}

void ftCo_ThrowLw_IASA(Fighter_GObj* gobj)
{
    return;
}

void ftCo_ThrowLw_Phys(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->ground_or_air == GA_Air) {
        if (fp->mv.co.throw.x4 != 0) {
            fp->self_vel.x = fp->mv.co.throw.xC.z * fp->facing_dir;
            fp->self_vel.y = fp->mv.co.throw.xC.y;
        } else {
            ft_80085134(gobj);
        }
    } else {
        ft_80085004(gobj);
    }
}

void ftCo_ThrowLw_Coll(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->ground_or_air == GA_Air) {
        if (fp->mv.co.throw.x4 != 0) {
            ft_80083C00(gobj, fn_800DD568);
        } else if (fp->mv.co.throw.x8 == 0) {
            ft_80083CE4(gobj, fn_800DD6E4, fn_800DD5EC);
        } else {
            ft_80083B68(gobj);
        }
    } else {
        ft_800841B8(gobj, fn_800DD684);
    }
}

static inline float ftCo_800DDDE4_inline3(Fighter* fp3)
{
    return 0.0f;
}

static inline float ftCo_800DDDE4_inline2(Fighter* fp3)
{
    return 0.5f * (fp3->coll_data.ecb.top.y + fp3->coll_data.ecb.bottom.y);
}

void ftCo_800DDDE4(Fighter_GObj* gobj, Fighter_GObj* gobj2, bool arg)
{
    Fighter* fp = GET_FIGHTER(gobj);
    Fighter* fp2 = GET_FIGHTER(gobj2);
    HitCapsule* hit = fp->xDF4;
    Fighter* fp3;
    Fighter* fp4;
    Vec vec;
    float damage;
    Vec vec2;
    CollData* cd;
    HSD_JObj* jobj;
    PAD_STACK(0x14);
    if (fp->x221B_b7) {
        fp3 = fp2;
        fp4 = fp;
    } else {
        fp3 = fp;
        fp4 = fp2;
    }
    fp->x1A5C = NULL;
    fp->native_hitlag_link_generation = nativeFighterTarget_Id(fp->x1A5C);
    fp->victim_gobj = NULL;
    fp->x221B_b5 = 0;
    fp->x221B_b7 = 0;
    ftColl_8007891C(gobj, gobj2, hit[0].damage);
    if (fp2->x1988 != 0) {
        ftColl_8007B62C(gobj2, 0);
    }
    lb_8000B1CC(fp3->parts[ftParts_GetBoneIndex(fp3, FtPart_TransN2)].joint,
                NULL, &vec);
    if (ftColl_8007B868(gobj2) == 0) {
        damage = hit[0].damage;
    } else {
        damage = 0.0f;
    }
    ftColl_80076640(fp2, &damage);
    fp2->dmg.kb_applied = ftColl_80079AB0(
        fp2, &hit[0], hit[0].unk_count, gm_8016B248(),
        Player_GetAttackRatio(fp->player_id),
        Player_GetDefenseRatio(fp2->player_id), p_ftCommonData->x10C);
    fp2->dmg.x1848_kb_angle = hit[0].kb_angle;
    fp2->dmg.facing_dir_1 = -(fp->facing_dir);
    fp2->dmg.x184c_damaged_hurtbox = 1;
    fp2->dmg.x1854_collpos = vec;
    fp2->dmg.x1860_element = hit[0].element;
    ftColl_80078710(gobj, gobj2, &fp2->dmg.facing_dir_1);
    ftCommon_8007EE0C(fp, hit[0].damage);
    ftCo_Damage_CalcKnockback(fp2);
    ftCommon_8007D5D4(fp4);
    if (fp4->x2226_b2) {
        HSD_JObj* jobj2 =
            fp4->parts[ftParts_GetBoneIndex(fp4, FtPart_XRotN)].joint;
        lb_8000C390(jobj2);
        fp4->x2226_b2 = 0;
        HSD_JObjSetTranslate(jobj2, &fp4->x2174);
        if (arg) {
            vec.x += fp4->facing_dir * (fp4->x1A70.z * fp4->x34_scale.y);
            vec.y += fp4->x1A70.y * fp4->x34_scale.y;
            vec.z = 0.0f;
        }
        cd = &fp4->coll_data;
        jobj = GET_JOBJ(fp4->gobj);
        vec2.x = fp3->cur_pos.x + ftCo_800DDDE4_inline3(fp3);
        vec2.y = fp3->cur_pos.y + ftCo_800DDDE4_inline2(fp3);
        vec2.z = fp3->cur_pos.z + ftCo_800DDDE4_inline3(fp3);
        fp4->coll_data.last_pos = vec2;
        mpColl_80043670(cd);
        cd->cur_pos = vec;
        HSD_JObjSetTranslate(jobj, &vec);
        ftCommon_UnlockECB(fp4);
        mpColl_800471F8(cd);
        fp4->cur_pos = cd->cur_pos;
        HSD_JObjSetTranslate(jobj, &fp4->cur_pos);
    }
    fp2->x1A5C = NULL;
    fp2->native_hitlag_link_generation = nativeFighterTarget_Id(fp2->x1A5C);
    fp2->victim_gobj = NULL;
    fp2->x221B_b7 = 0;
}

void ftCo_800DE2A8(Fighter_GObj* gobj, Fighter_GObj* other)
{
    ftCo_800DDDE4(gobj, other, true);
}

void ftCo_800DE2CC(Fighter_GObj* gobj, Fighter_GObj* other)
{
    ftCo_800DDDE4(gobj, other, false);
}

void ftCo_800DE2F0(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    ftCo_DatAttrs* attrs = &fp->co_attrs;
    HitCapsule hit;
    Vec vec;
    PAD_STACK(4);
    lbColl_80008D30(&hit, &p_ftCommonData->x380);
    ftColl_800788D4(gobj);
    lb_8000B1CC(fp->parts[ftParts_GetBoneIndex(fp, FtPart_TransN2)].joint,
                NULL, &vec);
    fp->dmg.kb_applied =
        ftColl_80079AB0(fp, &hit, hit.unk_count, gm_8016B248(), 1.0f,
                        Player_GetDefenseRatio(fp->player_id), attrs->weight);
    fp->dmg.x1848_kb_angle = hit.kb_angle;
    fp->dmg.facing_dir_1 = fp->facing_dir;
    fp->dmg.x184c_damaged_hurtbox = 1;
    fp->dmg.x1854_collpos = vec;
    fp->dmg.x1860_element = hit.element;
    Fighter_UnkTakeDamage_8006CC30(fp, hit.damage);
    ftCo_Damage_CalcKnockback(fp);
    ftCo_8008E908(gobj, 0.0f);
}
