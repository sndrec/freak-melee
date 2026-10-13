#include "ftCo_YoshiEgg.h"

#include "ftCo_Bury.h"
#include "ftCo_CaptureKoopa.h"
#include "ftCo_Fall.h"
#include <melee/ef/efasync.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0877.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/native_fighter_capture_rig.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/types.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>

void ftCo_800BBC88(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->invisible = true;
    ftColl_8007B62C(gobj, 2);
}

static void enclosure_shrink(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    NativeCaptureEnclosureState* state = &fp->capture_enclosure;
    if (state->shrink_remaining <= 0) {
        Fighter_UpdateModelScale(gobj);
        fp->accessory4_cb = NULL;
        return;
    }
    state->shrink_remaining -= 1;
    float amount = (state->settings.shrink_frames - state->shrink_remaining) /
                   state->settings.shrink_frames;
    amount *= state->settings.shrink_fraction;
    float factor = (1 - state->settings.shrink_fraction) + amount;
    Vec3 root = { factor, factor, factor };
    Vec3 accessory = root;
    root.x = root.x * state->root_scale.x;
    root.y *= state->root_scale.y;
    root.z *= state->root_scale.z;
    HSD_JObjSetScale(GET_JOBJ(gobj), &root);
    accessory.x = accessory.x * state->accessory_scale.x;
    accessory.y *= state->accessory_scale.y;
    accessory.z *= state->accessory_scale.z;
    HSD_JObjSetScale(fp->x20A0_accessory, &accessory);
}

static void enclosure_hurtbox(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    ftCo_DatAttrs_xBC_t* attrs = &fp->co_attrs.xBC;
    ftHurtboxInit hurt;
    ftColl_8007B0C0(gobj, HurtCapsule_Intangible);
    hurt.bone_idx = ftParts_GetBoneIndex(fp, FtPart_TransN);
    hurt.height = HurtHeight_Mid;
    hurt.is_grabbable = false;
    hurt.a_offset = attrs->x4;
    hurt.b_offset = attrs->x10;
    hurt.scale = attrs->x1C;
    ftColl_HurtboxInit(fp, fp->hurt_capsules, &hurt);
}

bool ftCo_CaptureEnclosure_Enter(Fighter_GObj* gobj, Fighter_GObj* holder,
                                FtMotionId motion,
                                const NativeCaptureEnclosureSettings* settings,
                                HSD_Joint* model, float facing)
{
    NativeMatchPoolReference physical;
    NativeFighterLifetime victim, owner;
    if (!settings || !model || gobj == holder ||
        !nativeMatchPool_Reference(&nativeGObjPool, gobj, &physical) ||
        gobj->classifier != HSD_GOBJ_CLASS_FIGHTER ||
        !nativeFighterLifetime_Capture(gobj->user_data, &victim) ||
        !nativeMatchPool_Reference(&nativeGObjPool, holder, &physical) ||
        holder->classifier != HSD_GOBJ_CLASS_FIGHTER ||
        !nativeFighterLifetime_Capture(holder->user_data, &owner) ||
        victim.reference.lifetime != owner.reference.lifetime ||
        (motion != ftCo_MS_YoshiEgg && motion != ftCo_MS_KirbyYoshiEgg) ||
        (s32)motion >= victim.data->x18) return false;
    HSD_JObj* attachment;
    if (!nativeFighterCapture_Joint(&victim, FtPart_TransN, &attachment)) return false;
    Fighter* fp = victim.data;
    NativeCaptureEnclosureState* state = &fp->capture_enclosure;
    // Cache by value before motion change can run callbacks or clear links.
    NativeCaptureEnclosureSettings snapshot = *settings;
    bool copied = motion == ftCo_MS_KirbyYoshiEgg;
    state->settings = snapshot;
    state->mashed = false;
    state->restore_model_on_escape = !copied;
    state->subtract_damage_form = copied;
    if (fp->ground_or_air == GA_Ground) ftCommon_8007D5D4(fp);
    if (!nativeFighterLifetime_Current(&victim) || !nativeFighterLifetime_Current(&owner)) return false;
    Fighter_ChangeMotionState(gobj, motion, Ft_MF_Unk06, 0, 1, 0, holder);
    // State exit and motion selection may execute authored callbacks. Retain
    // the original identities, then recheck the rig before touching its parts.
    if (!nativeFighterLifetime_Current(&victim) || !nativeFighterLifetime_Current(&owner) ||
        !nativeFighterCapture_Joint(&victim, FtPart_TransN, &attachment) || fp->x20A0_accessory) return false;
    fp->take_dmg_cb = ftCo_800BC438;
    ftCommon_8007E2F4(fp, 0x1FF);
    fp->invisible = true;
    Vec3 scale;
    scale.x = scale.y = scale.z = fp->co_attrs.xBC.size;
    ftCommon_SetAccessory(fp, model);
    if (!nativeFighterLifetime_Current(&victim) || !nativeFighterLifetime_Current(&owner) ||
        !fp->x20A0_accessory || !HSD_JObjGetChild(fp->x20A0_accessory)) return false;
    HSD_JObjSetScale(fp->x20A0_accessory, &scale);
    if (!copied) state->accessory_scale = scale;
    lb_8000C2F8(HSD_JObjGetChild(fp->x20A0_accessory),
                attachment);
    if (copied) state->accessory_scale = scale;
    enclosure_hurtbox(gobj);
    fp->self_vel = snapshot.launch_velocity;
    fp->facing_dir = facing;
    fp->dmg.x182c_behavior = snapshot.damage_behavior;
    // The original Captain alias addresses this timer on the console.
    state->mash_pause_remaining = 0;
    state->holder = holder;
    fp->take_dmg_2_cb = ftCo_CaptureEnclosure_Damage;
    ftCommon_8007EFC0(fp, 1);
    // The original walk alias addresses shrink duration on the console.
    state->shrink_remaining = snapshot.shrink_frames;
    ftCommon_InitGrab(fp, 0, snapshot.lifetime);
    HSD_JObjGetScale(GET_JOBJ(gobj), &state->root_scale);
    fp->accessory4_cb = enclosure_shrink;
    return nativeFighterLifetime_Current(&victim) && nativeFighterLifetime_Current(&owner);
}

void ftCo_CaptureEnclosure_Anim(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    NativeCaptureEnclosureState* state = &fp->capture_enclosure;
    fp->grab_timer -= state->settings.timer_decay;
    state->mashed = ftCommon_GrabMash(fp, state->settings.mash_reduction);
    if (fp->grab_timer <= 0) {
        ft_PlaySFX(fp, 280088, 127, 64);
        efAsync_Spawn(gobj, &fp->x60C, 4, 1231,
                      fp->parts[FtPart_TopN].joint, &fp->co_attrs.xBC);
        fp->self_vel = state->settings.escape_velocity;
        ftCommon_8007D5D4(fp);
        if (state->restore_model_on_escape) Fighter_UpdateModelScale(gobj);
        ftColl_8007B760(gobj, state->settings.escape_invincibility_frames);
        ftCo_Fall_Enter_YoshiEgg(gobj);
        return;
    }
    if (state->mash_pause_remaining) {
        state->mash_pause_remaining -= 1;
        if (state->mash_pause_remaining <= 0 && !state->mashed) {
            ftAnim_SetAnimRate(gobj, 1);
            state->mash_pause_remaining = 0;
        }
    }
    if (state->mash_pause_remaining <= 0 && state->mashed) {
        state->mash_pause_remaining = state->settings.mash_pause_frames;
        ftAnim_SetAnimRate(gobj, state->settings.mash_animation_rate);
    }
}

void ftCo_CaptureEnclosure_IASA(Fighter_GObj* gobj) {}

void ftCo_CaptureEnclosure_Phys(Fighter_GObj* gobj)
{
    if (GET_FIGHTER(gobj)->ground_or_air == GA_Ground) ft_80084F3C(gobj);
    else ft_80084DB0(gobj);
}

static void enclosure_land(Fighter_GObj* gobj)
{
    ftCommon_8007D7FC(GET_FIGHTER(gobj));
}

static void enclosure_leave_ground(Fighter_GObj* gobj)
{
    ftCommon_8007D5D4(GET_FIGHTER(gobj));
}

void ftCo_CaptureEnclosure_Coll(Fighter_GObj* gobj)
{
    if (GET_FIGHTER(gobj)->ground_or_air == GA_Ground)
        ft_8008403C(gobj, enclosure_leave_ground);
    else ft_80082C74(gobj, enclosure_land);
}

void ftCo_CaptureEnclosure_Damage(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    NativeCaptureEnclosureState* state = &fp->capture_enclosure;
    if (state->subtract_damage_form)
        fp->grab_timer -= fp->dmg.x1838_percentTemp * state->settings.damage_time_scale;
    else
        fp->grab_timer = -(fp->dmg.x1838_percentTemp * state->settings.damage_time_scale - fp->grab_timer);
    if (fp->dmg.x18CC == 3 && ftCo_800C0C88(fp->dmg.x18D0)) fp->grab_timer = 0;
    fp->x1828 = 4;
}
