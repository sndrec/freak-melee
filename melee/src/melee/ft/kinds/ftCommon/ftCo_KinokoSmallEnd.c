#include "ftCo_KinokoSmallEnd.h"

#include <Runtime/platform.h>

#include <melee/ft/forward.h>

#include <stdbool.h>

#include "forward.h"
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ft_0D27.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/inlines.h>
#include <melee/ft/types.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>

/* 0D2530 */ static void fn_800D2530(Fighter_GObj* gobj);

bool Fighter_PoisonMushroomEnd(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (!fn_800D2818(fp)) {
        return false;
    }
    if (fp->ground_or_air == GA_Ground) {
        ftCo_800D2490(gobj, 0);
    } else {
        ftCo_800D2600(gobj, 0);
    }
    return true;
}

void ftCo_800D23F4(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fn_800D2BF8(gobj);
    fp->x2220_b6 = false;
    ftCo_800D2770(gobj, fp->x34_scale.x);
    if (fp->mv.co.mushroom.clear_velocity) {
        ftCommon_8007E2FC(gobj);
        fp->mv.co.mushroom.clear_velocity = 0;
    }
    if (fp->ground_or_air == GA_Ground) {
        ft_8008A324(gobj);
        return;
    }
    fp->mv.co.mushroom.resume_air(gobj);
}

static inline bool initialize_small_end(Fighter_GObj* gobj, int id, int arg1)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fn_800D2B30(gobj);
    if (!ftCo_Mushroom_SelectAirResume(gobj, id)) return false;
    fp->mv.co.mushroom.clear_velocity = arg1;
    fn_800D299C(gobj, 0, fp->x34_scale.y, fp->x34_scale.x);
    fn_800D2AD8(fp);
    fp->mv.co.mushroom.finish = ftCo_800D23F4;
    return true;
}

void ftCo_800D2490(Fighter_GObj* gobj, int arg1)
{
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(gobj, NativeCallbackSource_Fighter, &generation)) return;
    Fighter* fp = GET_FIGHTER(gobj);
    const FighterKind kind = fp->kind;
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(kind);
    int id = fp->motion_id;
    fn_800D290C(gobj);
    if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter) ||
        fp->kind != kind || nativeFighterRuntime_Definition(kind) != definition) return;
    Fighter_ChangeMotionState(gobj, ftCo_MS_KinokoSmallEnd, 0x91, 0.0F, 1.0F,
                              0.0F, NULL);
    if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter) ||
        fp->kind != kind || nativeFighterRuntime_Definition(kind) != definition) return;
    if (!initialize_small_end(gobj, id, arg1)) return;
    fp->take_dmg_cb = ftCo_800D23F4;
}

void fn_800D2530(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fn_800D2CA0(fp);
    Fighter_ChangeMotionState(gobj, ftCo_MS_KinokoSmallEnd, 0xC4C5081,
                              fp->cur_anim_frame, fp->frame_speed_mul, 0.0F,
                              NULL);
    fp->take_dmg_cb = ftCo_800D23F4;
}

#ifdef MUST_MATCH
#pragma push
#pragma dont_inline on
#endif
void ftCo_KinokoSmallEnd_Anim(Fighter_GObj* gobj)
{
    if (!fn_800D2A3C(gobj)) {
        ftCo_800D23F4(gobj);
    }
}
#ifdef MUST_MATCH
#pragma pop
#endif

void ftCo_KinokoSmallEnd_Coll(Fighter_GObj* gobj)
{
    ft_8008403C(gobj, fn_800D26A0);
}

void ftCo_800D2600(Fighter_GObj* gobj, int arg1)
{
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(gobj, NativeCallbackSource_Fighter, &generation)) return;
    Fighter* fp = GET_FIGHTER(gobj);
    const FighterKind kind = fp->kind;
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(kind);
    int id = fp->motion_id;
    fn_800D290C(gobj);
    if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter) ||
        fp->kind != kind || nativeFighterRuntime_Definition(kind) != definition) return;
    Fighter_ChangeMotionState(gobj, ftCo_MS_KinokoSmallEndAir, 0x91, 0.0F,
                              1.0F, 0.0F, NULL);
    if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter) ||
        fp->kind != kind || nativeFighterRuntime_Definition(kind) != definition) return;
    if (!initialize_small_end(gobj, id, arg1)) return;
    fp->take_dmg_cb = ftCo_800D23F4;
}

void fn_800D26A0(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fn_800D2CD4(fp);
    Fighter_ChangeMotionState(gobj, ftCo_MS_KinokoSmallEndAir, 0xC4C5081,
                              fp->cur_anim_frame, fp->frame_speed_mul, 0.0F,
                              NULL);
    fp->take_dmg_cb = ftCo_800D23F4;
}

#ifdef MUST_MATCH
#pragma push
#pragma dont_inline on
#endif
void ftCo_KinokoSmallEndAir_Anim(Fighter_GObj* gobj)
{
    u8 _[8];
    if (!fn_800D2A3C(gobj)) {
        ftCo_800D23F4(gobj);
    }
}
#ifdef MUST_MATCH
#pragma pop
#endif

void ftCo_KinokoSmallEndAir_Coll(Fighter_GObj* gobj)
{
    ft_80082C74(gobj, fn_800D2530);
}
