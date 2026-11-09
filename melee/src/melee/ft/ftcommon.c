#include "ftcommon.h"
#include <melee/ft/native_callback_lifetime.h>
#ifdef MELEE_NATIVE
#include "native_script_fighter.h"
#include "native_fighter_lifetime.h"
#endif
#include <melee/ft/native_fighter_runtime.h>

#include <melee/it/forward.h>
#include <melee/mp/forward.h>

#include <math.h>
#include <stdbool.h>

#include "fighter.h"
#include "forward.h"
#include "ft_081B.h"
#include "ft_0877.h"
#include "ft_0881.h"
#include "ft_0892.h"
#include "ft_0C88.h"
#include "ft_0DF0.h"
#include "ftcamera.h"
#include "ftchangeparam.h"
#include "ftcolanim.h"
#include "ftcoll.h"
#ifdef MELEE_NATIVE
#include "ftdata.h"
#endif
#include "ftlib.h"
#include "ftmetal.h"
#include "ftparts.h"
#include "kinds/ftCommon/ftCo_Damage.h"
#include "kinds/ftCommon/ftCo_DamageFall.h"
#include "kinds/ftCommon/ftCo_Fall.h"
#include "kinds/ftCommon/ftCo_HammerWait.h"
#include "kinds/ftCommon/ftpickupitem.h"
#include "types.h"
#include <melee/ef/eflib.h>
#include <melee/gm/gm_unsplit.h>
#include <melee/it/it_26B1.h>
#include <melee/it/item.h>
#include <melee/it/native_held_article.h>
#include <melee/it/kinds/it_2E5A.h>
#include <melee/it/kinds/itfflowerflame.h>
#include <melee/it/kinds/itpeachparasol.h>
#include <melee/it/kinds/itrabbitc.h>
#include <melee/it/kinds/itsword.h>
#include <melee/lb/lb_013B.h>
#include <melee/mp/mplib.h>
#include <melee/pl/player.h>
#include <melee/pl/plbonuslib.h>
#include <melee/sfx/crowdsfx.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/rumble.h>

const Vec3 ftCo_803B74A0 = { 0 };

bool ftCommon_CheckFallFast(Fighter* fp)
{
    const NativeActionStick* movement = &fp->action_buffer.movement;
    const bool buffered_down =
        movement->frames && !fp->action_buffer.direction.frames &&
        movement->position.y <= -p_ftCommonData->x88 &&
        movement->tilt_y < p_ftCommonData->x8C;
    if (!fp->fall_fast && fp->self_vel.y < 0 &&
        ((fp->input.lstick[0].y <= -p_ftCommonData->x88 &&
          fp->x671_timer_lstick_tilt_y < p_ftCommonData->x8C) || buffered_down))
    {
        fp->fall_fast = true;
        fp->action_buffer.movement.frames = 0;
        fp->x671_timer_lstick_tilt_y = 0xFE;
        ft_PlaySFX(fp, 0x96, 0x7F, 0x40);
        return true;
    }
    return false;
}

void ftCommon_UnlockECB(Fighter* fp)
{
    fp->ecb_lock = 0;
    fp->coll_data.x130_flags &= ~CollData_X130_Locked;
}

void ftCommon_8007D5D4(Fighter* fp)
{
    fp->ground_or_air = GA_Air;
    fp->gr_vel = 0;
    fp->x98_atk_shield_kb.z = 0;
    fp->cur_pos.z = 0;
    fp->x74_self_accel.y = 0;
    fp->x1968_jumpsUsed = 1;
    fp->ecb_lock = 10;
    fp->coll_data.x130_flags |= CollData_X130_Locked;
}

void ftCommon_8007D60C(Fighter* fp)
{
    ftCo_DatAttrs* ca = &fp->co_attrs;
    if (fp->x2227_b0 && fp->x1968_jumpsUsed <= 1) {
        pl_8003FC44(fp->player_id, fp->x221F_b4);
    }
    fp->ground_or_air = GA_Air;
    fp->gr_vel = 0;
    fp->x74_self_accel.y = 0;
    fp->x1968_jumpsUsed = ca->max_jumps;
    fp->ecb_lock = 5;
    fp->coll_data.x130_flags |= CollData_X130_Locked;
}

void ftCommon_UseAllJumps(Fighter* fp)
{
    fp->x1968_jumpsUsed = fp->co_attrs.max_jumps;
}

void ftCommon_8007D6A4(Fighter* fp)
{
#ifdef MELEE_NATIVE
    bool just_landed = fp->ground_or_air == GA_Air;
#endif
    if (fp->x594_b0) {
        fp->self_vel.x = fp->x6A4_transNOffset.z * fp->facing_dir;
    }
    ftCommon_ClampGroundVel(fp, fp->co_attrs.ground_max_horizontal_velocity);
    fp->ground_or_air = GA_Ground;
    fp->gr_vel = fp->self_vel.x;
    fp->x1968_jumpsUsed = 0;
    fp->x1969_walljumpUsed = 0;
    fp->x2227_b0 = 0;
    ftCommon_UnlockECB(fp);
    if (!ft_80084A18(fp->gobj)) {
        OSReport("fighter ground no under Id! %d %d\n", fp->player_id,
                 fp->motion_id);
        HSD_ASSERT(686, 0);
    }
#ifdef MELEE_NATIVE
    if(just_landed && nativeScriptFighter_IsAttached(fp->gobj)) {
        (void)nativeScriptFighter_Run(fp->gobj,NativeScriptFighter_Landing,NULL,0, NULL,NULL);
        return;
    }
    if (just_landed && fp->kind >= Ft_Kind_CustomFirst &&
        fp->kind < Ft_Kind_RuntimeCapacity && ftData_OnLanding[fp->kind] != NULL)
    {
        ftData_OnLanding[fp->kind](fp->gobj);
    }
#endif
}

void ftCommon_8007D780(Fighter* fp)
{
    if (fp->ground_or_air != GA_Air) {
        return;
    }

    if (un_803224DC(fp->x8_spawnNum, fp->cur_pos.x,
                    fp->dmg.x18A4_knockbackMagnitude))
    {
        fp->dmg.x18A4_knockbackMagnitude = 0;
    }
    if (fp->x2227_b0 && fp->x1968_jumpsUsed <= 1) {
        pl_8003FC44(fp->player_id, fp->x221F_b4);
    }
}

void ftCommon_8007D7FC(Fighter* fp)
{
    if (fp->ground_or_air == GA_Air) {
        if (un_803224DC(fp->x8_spawnNum, fp->cur_pos.x,
                        fp->dmg.x18A4_knockbackMagnitude))
        {
            fp->dmg.x18A4_knockbackMagnitude = 0;
        }
        if (fp->x2227_b0 && fp->x1968_jumpsUsed <= 1) {
            pl_8003FC44(fp->player_id, fp->x221F_b4);
        }
    }
    ftCommon_8007D6A4(fp);
}

void ftCommon_8007D92C(HSD_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    if (fp->ground_or_air == GA_Air) {
        ftCo_Fall_Enter(gobj);
    } else {
        ft_8008A2BC(gobj);
    }
}

float ftCo_GetLStickAngle(Fighter* fp)
{
    return atan2f(fp->input.lstick[0].y, ABS(fp->input.lstick[0].x));
}

float ftCo_GetCStickAngle(Fighter* fp)
{
    return atan2f(fp->input.cstick[0].y, ABS(fp->input.cstick[0].x));
}

float ftCommon_8007D9D4(Fighter* fp)
{
    return atan2f(fp->input.lstick[0].y, fp->input.lstick[0].x);
}

void ftCommon_UpdateFacing(Fighter* fp)
{
    float dir;
    if (fp->input.lstick[0].x >= 0) {
        dir = +1;
    } else {
        dir = -1;
    }
    fp->facing_dir = dir;
}

void ftCommon_8007DA24(Fighter* fp)
{
    if (ABS(fp->input.lstick[0].x) > p_ftCommonData->horizontal_stick_deadzone)
    {
        ftCommon_UpdateFacing(fp);
    }
}

float ftCommon_CalcHitlag(int dmg, FtMotionId msid, float mul)
{
    int tmp = dmg * p_ftCommonData->x198 + p_ftCommonData->x19C;
    float result = (int) (tmp * mul);
    if ((unsigned) msid - ftCo_MS_Squat <= 1) {
        result = (int) (result * p_ftCommonData->x1A0);
    }
    return result;
}

void ftCommon_8007DB24(HSD_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    fp->x2219_b0 = 0;
    efLib_DestroyAll(gobj);
}

void ftCommon_8007DB58(HSD_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    const uint32_t callback_generation = (uint32_t)fp->x8_spawnNum;
    ft_80088770(fp);
    ft_800887CC(fp);
#ifdef MELEE_NATIVE
    if (nativeScriptFighter_CallbackBound(gobj,NativeScriptFighterCallback_Damage)) {
        if (!nativeScriptFighter_RunCallback(gobj,NativeScriptFighterCallback_Damage)) return;
        if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
    } else
#endif
    if (fp->take_dmg_cb != NULL) {
        fp->take_dmg_cb(gobj);
        if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
    }
#ifdef MELEE_NATIVE
    if (nativeScriptFighter_CallbackBound(gobj,NativeScriptFighterCallback_Death1)) {
        if (!nativeScriptFighter_RunCallback(gobj,NativeScriptFighterCallback_Death1)) return;
        if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
    } else
#endif
    if (fp->death1_cb != NULL) {
        fp->death1_cb(gobj);
    }
}

void ftCommon_InitGrab(Fighter* fp, bool arg1, float timer)
{
    fp->grab_timer = timer;
    fp->x1A51 = 0;
    fp->x1A50 = 0;
    fp->x2224_b6 = arg1;
    if (fp->x2224_b6) {
        fp->x1A52 = 0;
        fp->x1A53 = Fighter_GrabMashShake->x4;
    }
}

bool ftCommon_GrabMash(Fighter* fp, float arg1)
{
    bool result = false;
    if (fp->input.pressed_buttons & (HSD_PAD_AB | HSD_PAD_XY | HSD_PAD_LR)) {
        fp->grab_timer -= arg1;
        result = true;
    }
    {
        s8 r5 = fp->x1A50;
        s8 r6 = fp->x1A51;
        if (fp->input.lstick[0].x < -p_ftCommonData->x308) {
            fp->x1A50 = -1;
        }
        if (fp->input.lstick[0].x > p_ftCommonData->x308) {
            fp->x1A50 = 1;
        }
        if (fp->input.lstick[0].y < -p_ftCommonData->x308) {
            fp->x1A51 = -1;
        }
        if (fp->input.lstick[0].y > p_ftCommonData->x308) {
            fp->x1A51 = 1;
        }
        if (r5 != fp->x1A50 || r6 != fp->x1A51) {
            fp->grab_timer -= arg1;
            result = true;
        }
    }
    if (result && fp->x2224_b6) {
        fp->x2224_b5 = true;
        fp->x1A52 += 1;
        if (fp->x1A52 >= fp->x1A53) {
            fp->x1A52 = 0;
        }
    } else {
        fp->x2224_b5 = false;
    }
    pl_800402D0(fp->player_id, fp->x221F_b4, result);
    return result;
}

void ftCommon_8007DD7C(HSD_GObj* gobj, Vec3* v)
{
    u8 unused0[4];

    Vec2* temp_r31;
    Vec3 sp24;
    Fighter* arg_ft;
    Fighter* cur_ft;
    float temp_f0;
    s32 arg_gnd;
    s32 cur_gnd;
    HSD_GObj* cur;
    bool phi_r28;
    Vec2* vtmp;

    u8 unused1[20];

    arg_ft = gobj->user_data;
    phi_r28 = false;
    for (cur = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER]; cur != NULL;
         cur = cur->next)
    {
        temp_r31 = &arg_ft->x2C4;
        if (cur != gobj && !ftLib_80086FD4(cur, gobj)) {
            cur_ft = cur->user_data;
            if (cur_ft->x221F_b3 || cur_ft->ground_or_air != GA_Ground ||
                cur_ft->victim_gobj != NULL || cur_ft->x221F_b4)
            {
                continue;
            }
            arg_gnd = arg_ft->coll_data.floor.index;
            cur_gnd = cur_ft->coll_data.floor.index;
            if (arg_gnd == cur_gnd || cur_gnd == mpLineGetNext(arg_gnd) ||
                cur_gnd == mpLineGetPrev(arg_gnd))
            {
                ftCommon_8007F8B4(cur_ft, &sp24);
                vtmp = &cur_ft->x2C4;
                temp_f0 = (temp_r31->x * arg_ft->facing_dir + v->x) -
                          (cur_ft->facing_dir * vtmp->x + sp24.x);
                if (ABS(temp_f0) < temp_r31->y + vtmp->y) {
                    if (temp_f0) {
                        arg_ft->xF8_playerNudgeVel.x +=
                            temp_f0 < 0 ? -p_ftCommonData->x450
                                        : p_ftCommonData->x450;
                    } else {
                        arg_ft->xF8_playerNudgeVel.x +=
                            phi_r28 ? -p_ftCommonData->x450
                                    : p_ftCommonData->x450;
                    }
                    if (v->z - sp24.z) {
                        arg_ft->xF8_playerNudgeVel.y +=
                            v->z - sp24.z < 0 ? -p_ftCommonData->x454
                                              : p_ftCommonData->x454;
                    } else if (temp_f0) {
                        arg_ft->xF8_playerNudgeVel.y +=
                            temp_f0 < 0 ? -p_ftCommonData->x454
                                        : p_ftCommonData->x454;
                    } else {
                        arg_ft->xF8_playerNudgeVel.y +=
                            phi_r28 ? -p_ftCommonData->x454
                                    : p_ftCommonData->x454;
                    }
                }
            }
        } else {
            phi_r28 = true;
        }
    }
}

void ftCommon_8007DFD0(HSD_GObj* gobj, Vec3* arg1)
{
    u8 unused0[4];

    Vec3 sp1C;
    Vec2* temp_r31;
    HSD_GObj* new_var;
    s32 new_var2;
    Fighter* fp;
    Fighter* temp_r3;
    float temp_f1;
    s32 temp_r0;
    s32 temp_r30;
    Vec2* tmp;
    u8 unused1[12];
    fp = gobj->user_data;
    temp_r31 = &fp->x2C4;
    new_var = Player_GetEntity(fp->player_id);
    temp_r3 = new_var->user_data;
    if (!temp_r3->x221F_b3 && temp_r3->ground_or_air == GA_Ground) {
        temp_r0 = fp->coll_data.floor.index;
        temp_r30 = (new_var2 = temp_r3->coll_data.floor.index);
        if (temp_r0 == temp_r30 || temp_r30 == mpLineGetNext(temp_r0) ||
            temp_r30 == mpLineGetPrev(temp_r0))
        {
            ftCommon_8007F8B4(temp_r3, &sp1C);
            tmp = &temp_r3->x2C4;
            temp_f1 = (temp_r31->x * fp->facing_dir + arg1->x) -
                      (temp_r3->facing_dir * tmp->x + sp1C.x);
            if (ABS(temp_f1) < temp_r31->y + tmp->y) {
                fp->xF8_playerNudgeVel.y -= p_ftCommonData->x45C;
            }
        }
    }
    ftCommon_8007DD7C(gobj, arg1);
}

void ftCommon_8007E0E4(HSD_GObj* gobj)
{
    Vec3 sp10;
    Fighter* fp;

    u8 _[4];

    float phi_f0;
    float phi_f31;
    float phi_f30;

    fp = gobj->user_data;
    fp->xF8_playerNudgeVel.y = 0;
    fp->xF8_playerNudgeVel.x = 0;
    if (!fp->x2219_b1 && !fp->x2219_b5 && fp->ground_or_air == GA_Ground) {
        if (!fp->x221F_b4) {
            phi_f30 = p_ftCommonData->x454;
            phi_f31 = p_ftCommonData->x458;
        } else {
            phi_f30 = p_ftCommonData->x45C;
            phi_f31 = p_ftCommonData->x460;
        }
        ftCommon_8007F8B4(fp, &sp10);
        if (!fp->x221D_b5) {
            if (!fp->x221F_b4) {
                ftCommon_8007DD7C(gobj, &sp10);
            } else {
                ftCommon_8007DFD0(gobj, &sp10);
            }
        }
        if (!fp->xF8_playerNudgeVel.y && sp10.z) {
            if (sp10.z < 0) {
                phi_f0 = phi_f30;
            } else {
                phi_f0 = -phi_f30;
            }
            fp->xF8_playerNudgeVel.y = phi_f0;
        }
        if ((fp->xF8_playerNudgeVel.y > 0 && sp10.z < 0 &&
             sp10.z + fp->xF8_playerNudgeVel.y >= 0) ||
            (fp->xF8_playerNudgeVel.y < 0 && sp10.z > 0 &&
             sp10.z + fp->xF8_playerNudgeVel.y <= 0))
        {
            fp->xF8_playerNudgeVel.y = -sp10.z;
            sp10.z = 0;
        }
        if (sp10.z + fp->xF8_playerNudgeVel.y > phi_f31) {
            fp->xF8_playerNudgeVel.y = phi_f31 - sp10.z;
        } else if (sp10.z + fp->xF8_playerNudgeVel.y < -phi_f31) {
            fp->xF8_playerNudgeVel.y = -phi_f31 - sp10.z;
        }
        if (fp->x221F_b4 == 1) {
            fp->xF8_playerNudgeVel.x = 0;
        }
    }
}

HSD_GObj* ftCommon_8007E2A4(HSD_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    CollData* colldata = &fp->coll_data;
    fp->x98_atk_shield_kb.x =
        colldata->floor.normal.y * fp->xF4_ground_attacker_shield_kb_vel;
    fp->x98_atk_shield_kb.y =
        -colldata->floor.normal.x * fp->xF4_ground_attacker_shield_kb_vel;
    return gobj;
}

void ftCommon_8007E2D0(Fighter* fp, s16 arg1, HSD_GObjEvent grab_cb,
                       HSD_GObjEvent unk_cb,
                       void (*grabbed_cb)(HSD_GObj*, HSD_GObj*))
{
    fp->x221E_b6 = true;
    fp->x1A68 = arg1;
    fp->grab_cb = grab_cb;
    fp->grabbed_cb = grabbed_cb;
    fp->x2194 = unk_cb;
}

void ftCommon_8007E2F4(Fighter* fp, s16 val)
{
    fp->x1A6A = val;
}

static inline void _func_8007E2FC_inline(HSD_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    fp->xE4_ground_accel_1 = 0;
    fp->xE8_ground_accel_2 = 0;
    fp->x74_self_accel.z = 0;
    fp->x74_self_accel.y = 0;
    fp->x74_self_accel.x = 0;
    fp->gr_vel = 0;
    fp->self_vel.z = 0;
    fp->self_vel.y = 0;
    fp->self_vel.x = 0;
    fp->xF0_ground_kb_vel = 0;
    fp->x8c_kb_vel.z = 0;
    fp->x8c_kb_vel.y = 0;
    fp->x8c_kb_vel.x = 0;
    fp->xF4_ground_attacker_shield_kb_vel = 0;
    fp->x98_atk_shield_kb.z = 0;
    fp->x98_atk_shield_kb.y = 0;
    fp->x98_atk_shield_kb.x = 0;
    fp->xD4_unk_vel.z = 0;
    fp->xD4_unk_vel.y = 0;
    fp->xD4_unk_vel.x = 0;
}

void ftCommon_8007E2FC(HSD_GObj* gobj)
{
    _func_8007E2FC_inline(gobj);
}

void ftCommon_8007E358(HSD_GObj* gobj)
{
    HSD_JObj* jobj;
    Fighter* fp = gobj->user_data;
    jobj = fp->parts[ftParts_GetBoneIndex(fp, 4)].joint;
    HSD_JObjGetTranslation(jobj, &fp->x1A7C);
}

void ftCommon_8007E3EC(HSD_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    HSD_JObj* jobj = fp->parts[ftParts_GetBoneIndex(fp, 4)].joint;
    Vec3 sp10;

    u8 _[4];

    if (HSD_JObjMtxIsDirty(jobj)) {
        HSD_JObjGetTranslation(jobj, &sp10);
        sp10.x -= fp->x1A7C.x;
        sp10.y -= fp->x1A7C.y;
        sp10.z -= fp->x1A7C.z;
        fp->x1A7C.x += sp10.x * fp->x1A6C;
        fp->x1A7C.y += sp10.y * fp->x1A6C;
        fp->x1A7C.z += sp10.z * fp->x1A6C;
        HSD_JObjSetTranslate(jobj, &fp->x1A7C);
    }
}

void ftCommon_8007E5AC(Fighter* fp)
{
    Vec3* ground_normal = &fp->coll_data.floor.normal;
    float tmp = -atan2f(ground_normal->x, ground_normal->y);
    HSD_ASSERT(1146, fp->ground_or_air == GA_Ground);
    ftPartSetRotZ(fp, 0, tmp);
}

void ftCommon_SetAccessory(Fighter* fp, HSD_Joint* joint)
{
    if (fp->x20A0_accessory != NULL) {
        OSReport("fighter accessory already exsist!\n");
        HSD_ASSERT(0x486, 0);
    }
    fp->x20A0_accessory = HSD_JObjLoadJoint(joint);
}

void ftCommon_8007E690(Fighter* fp, HSD_AnimJoint* arg1)
{
    if (fp->x20A0_accessory != NULL) {
        HSD_JObjAddAnimAll(fp->x20A0_accessory, arg1, NULL, NULL);
        HSD_JObjReqAnimAll(fp->x20A0_accessory, 0);
    }
}

#ifndef MELEE_NATIVE
extern void (*ftData_OnItemDropExt[])(HSD_GObj*, s32);
extern void (*ftData_OnItemDrop[])(HSD_GObj*, s32); // OnItemDrop?
extern void (*ftData_OnItemPickup[])(HSD_GObj*, s32);
#endif

void ftCommon_8007E6DC(HSD_GObj* gobj, HSD_GObj* item_gobj, s32 arg2)
{
    Fighter* fp = gobj->user_data;
#ifdef MELEE_NATIVE
    NativeFighterLifetime lifetime;
    if(!nativeFighterLifetime_Capture(fp,&lifetime))return;
#else
    const uint32_t callback_generation = (uint32_t)fp->x8_spawnNum;
#endif
    if ((fp->x1978 != NULL) && (fp->x1978 == item_gobj)) {
        fp->x1978 = NULL;
        return;
    }
    if (ftCo_800C5240(gobj)) {
        ftCo_800C5500(gobj);
#ifdef MELEE_NATIVE
        if(!nativeFighterLifetime_Current(&lifetime))return;
#endif
    }
#ifdef MELEE_NATIVE
    if(nativeScriptFighter_IsAttached(gobj)) {
        NativeScriptValue argument={.type=NativeScriptType_Bool,.as.u64=arg2!=0};
        if(nativeScriptFighter_Run(gobj,NativeScriptFighter_ItemDropExt,&argument,1, NULL,NULL)!=NativeScriptVM_Ok)return;
    } else
#endif
    if (ftData_OnItemDropExt[fp->kind] != NULL) {
        ftData_OnItemDropExt[fp->kind](gobj, arg2);
#ifdef MELEE_NATIVE
        if(!nativeFighterLifetime_Current(&lifetime))return;
#else
        if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
#endif
    }
#ifdef MELEE_NATIVE
    if(!nativeFighterLifetime_Current(&lifetime))return;
#endif
    pl_8003EA08(fp->player_id, fp->x221F_b4);
#ifdef MELEE_NATIVE
    if(!nativeFighterLifetime_Current(&lifetime))return;
#endif
    fp->item_gobj = NULL;
}

void ftCommon_8007E79C(HSD_GObj* gobj, s32 arg1)
{
    Fighter* fp = gobj->user_data;
#ifdef MELEE_NATIVE
    if(nativeScriptFighter_IsAttached(gobj)) {
        NativeScriptValue argument={.type=NativeScriptType_Bool,.as.u64=arg1!=0};
        (void)nativeScriptFighter_Run(gobj,NativeScriptFighter_ItemDrop,&argument,1, NULL,NULL);return;
    }
#endif
    if (ftData_OnItemDrop[fp->kind] != NULL) {
        ftData_OnItemDrop[fp->kind](gobj, arg1);
    }
}

void ftCommon_8007E7E4(HSD_GObj* gobj, s32 arg1)
{
    Fighter* fp = gobj->user_data;
#ifdef MELEE_NATIVE
    if(nativeScriptFighter_IsAttached(gobj)) {
        NativeScriptValue argument={.type=NativeScriptType_Bool,.as.u64=arg1!=0};
        (void)nativeScriptFighter_Run(gobj,NativeScriptFighter_ItemPickup,&argument,1, NULL,NULL);return;
    }
#endif
    if (ftData_OnItemPickup[fp->kind] != NULL) {
        ftData_OnItemPickup[fp->kind](gobj, arg1);
    }
}

void ftCommon_8007E82C(HSD_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    fp->x1984_heldItemSpec = NULL;
}

void it_8028B718(HSD_GObj*, float);
void it_8028B780(HSD_GObj*, float);
void it_8028B7E8(HSD_GObj*, float);
void it_8028B850(HSD_GObj*, float);
void it_8028B648(HSD_GObj*, float);
void it_8028B6B0(HSD_GObj*, float);
void it_8028B618(HSD_GObj*, float);

static void (*parasol_table_1[7])(HSD_GObj*, float) = {
    it_8028B718, it_8028B780, it_8028B7E8, it_8028B850,
    it_8028B648, it_8028B6B0, it_8028B618,
};
static s32 parasol_table_2[7] = {
    7, 8, 9, 10, 5, 6, 4,
};
static void (*parasol_table_3[7])(HSD_GObj*, float) = {
    NULL, NULL, NULL, NULL, it_802BDD40, NULL, it_802BDDB4,
};
static s32 parasol_table_4[7] = {
    -1, -1, -1, -1, 1, -1, 2,
};

s32 it_8028B08C(void*, s32);

static const int FtParasol_None = -1;

void ftCommon_8007E83C(HSD_GObj* gobj, s32 arg1, float div)
{
    float val;
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(gobj, NativeCallbackSource_Fighter, &generation)) return;
    Fighter* fp = gobj->user_data;
    const FighterKind kind = fp->kind;
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(kind);
    int status = ftGetParasolStatus(gobj);
    if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter) ||
        fp->kind != kind || nativeFighterRuntime_Definition(kind) != definition) return;
    if (nativeHeldArticle_Request(gobj, arg1, fp->frame_speed_mul, div)) return;
    HSD_ASSERT(1276, status != FtParasol_None);

    if (div == 0) {
        val = fp->frame_speed_mul;
    } else if (itGetKind(fp->item_gobj) == It_Kind_Parasol) {
        val = fp->frame_speed_mul *
              (it_8028B08C(fp->item_gobj, parasol_table_2[arg1]) / div);
    } else {
        val = fp->frame_speed_mul *
              (it_802BDA40(fp->item_gobj, parasol_table_4[arg1]) / div);
    }
    if (itGetKind(fp->item_gobj) == It_Kind_Parasol) {
        parasol_table_1[arg1](fp->item_gobj, val);
    } else {
        parasol_table_3[arg1](fp->item_gobj, val);
    }
}

/// @was{ftGetParasolStatus}
s32 ftGetParasolStatus(HSD_GObj* gobj)
{
    int32_t status;
    if (nativeHeldArticle_Status(gobj, &status)) return status;
    Fighter* fp = gobj->user_data;
    if (fp->item_gobj != NULL && itGetKind(fp->item_gobj) == It_Kind_Parasol) {
        switch (itGetMotionId(fp->item_gobj)) {
        case 7:
            return 0;
        case 8:
            return 1;
        case 9:
            return 2;
        case 10:
            return 3;
        case 5:
            return 4;
        case 6:
            return 5;
        case 4:
            return 6;
        }
    }
    if (fp->item_gobj != NULL &&
        itGetKind(fp->item_gobj) == It_Kind_Peach_Parasol)
    {
        switch (itGetMotionId(fp->item_gobj)) {
        case 1:
            return 4;
        case 0:
        case 2:
            return 6;
        }
    }
    return FtParasol_None;
}

void ftCommon_8007EA90(Fighter* fp, s32 arg1)
{
    s32 temp_r31;
    float phi_f31;
    float phi_f30;
    Vec3 sp10;

    if (gm_8016B0B4() == 0) {
        return;
    }
    if (fp->dmg.kb_applied) {
        phi_f30 = fp->dmg.kb_applied;
        phi_f31 = ftCo_Damage_CalcAngle(fp, phi_f30);
        if (fp->dmg.facing_dir_1 > 0) {
            if (phi_f31 > M_PI) {
                phi_f31 = M_PI * 3 - phi_f31;
            } else {
                phi_f31 = M_PI - phi_f31;
            }
        }
        sp10.x = fp->dmg.x1854_collpos.x;
        sp10.y = fp->dmg.x1854_collpos.y;
        sp10.z = 0;
    } else {
        phi_f31 = M_PI / 2;
        phi_f30 = p_ftCommonData->x5C0;
        ftLib_800866DC(fp->gobj, &sp10);
    }
    temp_r31 = it_802E5F8C(fp->gobj, &sp10, arg1, 1, phi_f31, phi_f30);
    if (fp->dmg.x18c4_source_ply != 6) {
        Player_SetUnk98(fp->dmg.x18c4_source_ply,
                        temp_r31 + Player_GetUnk98(fp->dmg.x18c4_source_ply));
    }
}

void ftCommon_8007EBAC(Fighter* fp, u32 arg1, u32 arg2)
{
    if (Player_8003544C(fp->player_id, fp->x221F_b4) && !fp->x221F_b3 &&
        !fp->x2224_b2)
    {
        lb_80014574(fp->x618_player_id, arg1 + 2, arg1, arg2);
    }
}

void ftCommon_8007EC30(u32 arg0, u32 arg1)
{
    Fighter* fp;
    HSD_GObj* cur = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];

    u8 _[16];

    while (cur != NULL) {
        fp = cur->user_data;
        ftCommon_8007EBAC(fp, arg0, arg1);
        cur = cur->next;
    }
}

void ftCommon_8007ECD4(Fighter* fp, s32 arg1)
{
    if (Player_8003544C(fp->player_id, fp->x221F_b4)) {
        HSD_PadRumbleRemoveId(fp->x618_player_id, arg1 + 2);
    }
}

void ftCommon_8007ED2C(Fighter* fp)
{
    lb_800145C0(fp->x618_player_id);
}

void ftCommon_8007ED50(Fighter* fp, s32 arg1)
{
    float tmp = arg1 * p_ftCommonData->x138 + p_ftCommonData->x13C;
    s32 val2 = tmp;
    if ((s32) tmp < 1) {
        return;
    }
    ftCommon_8007EBAC(fp, 5, val2);
}

void ftCommon_8007EE0C(Fighter* fp, s32 arg1)
{
    float tmp = arg1 * p_ftCommonData->xEC + p_ftCommonData->xF0;
    s32 val2 = tmp;
    if ((s32) tmp < 1) {
        return;
    }
    ftCommon_8007EBAC(fp, 10, val2);
}

void ftCommon_8007EEC8(Fighter* fp, s32 arg1, s32 arg2)
{
    if (fp->item_gobj != NULL &&
        fp->item_gobj->classifier == HSD_GOBJ_CLASS_ITEM &&
        itGetKind(fp->item_gobj) == It_Kind_Sword)
    {
        float multiplier = 1.0 / 256;
        float tmp = arg2 * multiplier;
        it_80284FC4(fp->item_gobj, arg1, tmp);
    }
}

void ftCommon_8007EF5C(Fighter* fp, s32 arg1)
{
    if (fp->item_gobj != NULL &&
        fp->item_gobj->classifier == HSD_GOBJ_CLASS_ITEM &&
        itGetKind(fp->item_gobj) == It_Kind_Sword)
    {
        it_80285024(fp->item_gobj, arg1);
    }
}

void ftCommon_8007EFC0(Fighter* fp, u32 val)
{
    fp->x209A = val;
}


#ifndef MELEE_NATIVE
extern void (*ftData_OnItemInvisible[])(HSD_GObj*);
extern void (*ftData_OnItemVisible[])(HSD_GObj*);
extern void (*ftData_OnKnockbackEnter[])(HSD_GObj*);
extern void (*ftData_OnKnockbackExit[])(HSD_GObj*);
#endif

void ftCommon_8007F578(HSD_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    if (fp->item_gobj == NULL) {
        return;
    }
#ifdef MELEE_NATIVE
    if(nativeScriptFighter_IsAttached(gobj)) {
        (void)nativeScriptFighter_Run(gobj,NativeScriptFighter_ItemInvisible,NULL,0, NULL,NULL);return;
    }
#endif
    if (ftData_OnItemInvisible[fp->kind] != NULL) {
        ftData_OnItemInvisible[fp->kind](gobj);
    }
}

void ftCommon_8007F5CC(Item_GObj* gobj, bool arg1)
{
    Fighter* fp = gobj->user_data;
    const uint32_t callback_generation = (uint32_t)fp->x8_spawnNum;
    Item_GObj* item = fp->item_gobj;

    PAD_STACK(8);

    if (item != NULL && fp->x221E_b3 != arg1) {
        if (!arg1) {
            ftCommon_8007F578(gobj);
            if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
#ifdef MELEE_NATIVE
            if(nativeScriptFighter_IsAttached(gobj)&&nativeScriptFighter_Status(gobj)!=NativeScriptVM_Ok)return;
#endif
        } else if (item != NULL) {
#ifdef MELEE_NATIVE
            if(nativeScriptFighter_IsAttached(gobj)) {
                if(nativeScriptFighter_Run(gobj,NativeScriptFighter_ItemVisible,NULL,0, NULL,NULL)!=NativeScriptVM_Ok)return;
            } else
#endif
            if (ftData_OnItemVisible[fp->kind] != NULL) {
                ftData_OnItemVisible[fp->kind](gobj);
                if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
            }
        }
    }
    fp->x221E_b3 = arg1;
}

void ftCommon_8007F76C(HSD_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    fp->gr_vel = fp->facing_dir * ABS(fp->gr_vel);
    fp->self_vel.x = fp->facing_dir * ABS(fp->self_vel.x);
}

void ftCommon_8007F7B4(HSD_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    fp->gr_vel = -fp->facing_dir * ABS(fp->gr_vel);
    fp->self_vel.x = -fp->facing_dir * ABS(fp->self_vel.x);
}

MtxPtr ftCommon_8007F804(Fighter* fp)
{
    if (fp->x34_scale.z != 1) {
        return fp->x44_mtx;
    }
    return NULL;
}

void ftCommon_8007F824(HSD_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
#ifdef MELEE_NATIVE
    if(nativeScriptFighter_IsAttached(gobj)) {
        (void)nativeScriptFighter_Run(gobj,NativeScriptFighter_KnockbackEnter,NULL,0, NULL,NULL);return;
    }
#endif
    if (ftData_OnKnockbackEnter[fp->kind] != NULL) {
        ftData_OnKnockbackEnter[fp->kind](gobj);
    }
}

void ftCommon_8007F86C(HSD_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
#ifdef MELEE_NATIVE
    if(nativeScriptFighter_IsAttached(gobj)) {
        (void)nativeScriptFighter_Run(gobj,NativeScriptFighter_KnockbackExit,NULL,0, NULL,NULL);return;
    }
#endif
    if (ftData_OnKnockbackExit[fp->kind] != NULL) {
        ftData_OnKnockbackExit[fp->kind](gobj);
    }
}

void ftCommon_8007F8B4(Fighter* fp, Vec3* v)
{
    v->x = fp->cur_pos.x + fp->xD4_unk_vel.x;
    v->y = fp->cur_pos.y + fp->xD4_unk_vel.y;
    v->z = fp->cur_pos.z + fp->xD4_unk_vel.z;
}

void ftCommon_8007F8E8(HSD_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    if (fp->x197C == NULL || fp->x1980 == NULL) {
#ifdef MELEE_NATIVE
        if (nativeScriptFighter_IsAttached(gobj)) {
            (void)nativeScriptFighter_Run(gobj, NativeScriptFighter_AuxiliaryItemVisibilityReset,
                                          NULL, 0, NULL, NULL);
            return;
        }
        const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(fp->kind);
        if (definition && definition->on_auxiliary_item_visibility_reset) {
            definition->on_auxiliary_item_visibility_reset(gobj);
            return;
        }
#endif
        if (ftData_UnkMotionStates2[fp->kind] != NULL) {
            ftData_UnkMotionStates2[fp->kind](gobj);
        }
    }
}

static inline void ftCommon_AuxiliaryItemAttach(HSD_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    if (fp->x197C == NULL || fp->x1980 == NULL) {
#ifdef MELEE_NATIVE
        if (nativeScriptFighter_IsAttached(gobj)) {
            (void)nativeScriptFighter_Run(gobj, NativeScriptFighter_AuxiliaryItemAttach,
                                          NULL, 0, NULL, NULL);
            return;
        }
        const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(fp->kind);
        if (definition && definition->on_auxiliary_item_attach) {
            definition->on_auxiliary_item_attach(gobj);
            return;
        }
#endif
        if (ftData_UnkMotionStates1[fp->kind] != NULL) {
            ftData_UnkMotionStates1[fp->kind](gobj);
        }
    }
}

void ftCommon_8007F948(HSD_GObj* gobj, HSD_GObj* arg1, s32 arg2)
{
    Fighter* fp = gobj->user_data;
    fp->x197C = arg1;
    fp->x2014 = arg2;
    ftCommon_AuxiliaryItemAttach(gobj);
}

void ftCommon_8007F9B4(HSD_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    uint32_t generation=(uint32_t)fp->x8_spawnNum;
    fp->x197C = NULL;
    ftCo_800D105C(gobj);
    if(!nativeCallbackSource_Alive(gobj,generation,NativeCallbackSource_Fighter))return;
#ifdef MELEE_NATIVE
    if(nativeScriptFighter_IsAttached(gobj)&&nativeScriptFighter_Status(gobj)!=NativeScriptVM_Ok)return;
#endif
    ft_80081C88(gobj, fp->x34_scale.y);
}

void ftCommon_8007FA00(HSD_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    Vec3* vec = &fp->co_attrs.x114;

    u8 _[8];

    it_80294EB0(fp->x197C, &vec[1], &vec[0]);
    it_80294E78(fp->x197C, fp->x34_scale.y * vec[2].x);
}

void ftCommon_8007FA58(Fighter_GObj* gobj, Item_GObj* arg1)
{
    Fighter* fp = gobj->user_data;
    Vec3 vec;
#ifdef MELEE_NATIVE
    NativeFighterLifetime lifetime;
    uint32_t item_generation;
    if (!nativeFighterLifetime_Capture(fp, &lifetime) ||
        !nativeCallbackSource_Snapshot(arg1, NativeCallbackSource_Item, &item_generation)) return;
#endif

    /// @todo This is an inline.
    {
        Fighter* fp2 = fp;

        u8 _[24];

        if (fp2->x197C != NULL) {
            fp->x2014 = it_8026B54C(arg1);
            ft_PlaySFX(fp, 0x117, 0x7F, 0x40);
            ftCommon_8007EBAC(fp, 0x10, 0);
            Item_8026A8EC(arg1);
            return;
        }
    }

    vec = ftCo_803B74A0;
    it_8026BD0C(arg1);
#ifdef MELEE_NATIVE
    if (!nativeFighterLifetime_Current(&lifetime) ||
        !nativeCallbackSource_Alive(arg1, item_generation, NativeCallbackSource_Item)) return;
#endif
    Item_8026ABD8(arg1, &vec, 0);
#ifdef MELEE_NATIVE
    if (!nativeFighterLifetime_Current(&lifetime) ||
        !nativeCallbackSource_Alive(arg1, item_generation, NativeCallbackSource_Item)) return;
#endif
    Item_8026AB54(arg1, gobj, fp->ft_data->x8->x12);
#ifdef MELEE_NATIVE
    if (!nativeFighterLifetime_Current(&lifetime) ||
        !nativeCallbackSource_Alive(arg1, item_generation, NativeCallbackSource_Item)) return;
#endif
    ftCommon_8007F948(gobj, arg1, it_8026B54C(arg1));
#ifdef MELEE_NATIVE
    if (!nativeFighterLifetime_Current(&lifetime) || fp->x197C != arg1 ||
        !nativeCallbackSource_Alive(arg1, item_generation, NativeCallbackSource_Item)) return;
#endif
    ftCo_800D105C(gobj);
#ifdef MELEE_NATIVE
    if (!nativeFighterLifetime_Current(&lifetime) || fp->x197C != arg1 ||
        !nativeCallbackSource_Alive(arg1, item_generation, NativeCallbackSource_Item)) return;
#endif
    ft_80081C88(gobj, fp->x34_scale.y);
    ft_PlaySFX(fp, 0x117, 0x7F, 0x40);
    ftCommon_8007EBAC(fp, 0x10, 0);
    it_8026BCF4(arg1);
    ftCommon_8007FA00(gobj);
}

void ftCommon_8007FC7C(HSD_GObj* gobj, float arg8)
{
    HSD_GObj* item_gobj;
    Fighter* fp;
    s32 sp20;

    u8 _[4];

    fp = gobj->user_data;
#ifdef MELEE_NATIVE
    NativeFighterLifetime lifetime;
    if (!nativeFighterLifetime_Capture(fp, &lifetime)) return;
#endif
    sp20 = arg8 * p_ftCommonData->x704;
    if (fp->x1980 != NULL) {
        fp->x2018 += sp20;
        if (fp->x2018 > p_ftCommonData->x700) {
            fp->x2018 = p_ftCommonData->x700;
        }
        sp20 = fp->x2024;
        fp->x2024 = sp20 + arg8;
        ftCommon_8007FDA0(gobj);
    } else {
        item_gobj = it_8029A748(gobj, &fp->cur_pos, fp->ft_data->x8->x12,
                                fp->facing_dir);
#ifdef MELEE_NATIVE
        if (!nativeFighterLifetime_Current(&lifetime)) return;
#endif
        if (item_gobj != NULL) {
#ifdef MELEE_NATIVE
            uint32_t item_generation;
            if (!nativeCallbackSource_Snapshot(item_gobj, NativeCallbackSource_Item, &item_generation)) return;
#endif
            ftCommon_8007FE84(gobj, item_gobj, sp20, arg8);
#ifdef MELEE_NATIVE
            if (!nativeFighterLifetime_Current(&lifetime) || fp->x1980 != item_gobj ||
                !nativeCallbackSource_Alive(item_gobj, item_generation, NativeCallbackSource_Item)) return;
#endif
            ftCommon_8007FDA0(gobj);
        }
    }
    ft_PlaySFX(fp, 0x11F, 0x7F, 0x40);
}

static inline float my_fminf(float a, float b)
{
    float result = a;
    if (a > b) {
        result = b;
    }
    return result;
}

void ftCommon_8007FDA0(HSD_GObj* gobj)
{
    Vec3 sp20;
    Fighter* fp;
    float temp_f1;
    Vec3* temp_r30;
    float phi_f31;

    u8 _[16];

    fp = gobj->user_data;
    temp_r30 = &fp->co_attrs.x130;
    phi_f31 = my_fminf(p_ftCommonData->x710 * fp->x2024 + p_ftCommonData->x708,
                       p_ftCommonData->x70C);
    temp_f1 = 1.0f / phi_f31;
    sp20 = *temp_r30;
    sp20.x *= temp_f1;
    sp20.y *= temp_f1;
    sp20.z *= temp_f1;
    it_8029A8F4(fp->x1980, &sp20);
    it_8029A89C(fp->x1980, phi_f31 * temp_r30[1].x * fp->x34_scale.y);
}

void ftCommon_8007FE84(HSD_GObj* gobj, HSD_GObj* item_gobj, s32 arg2,
                       float arg3)
{
    u8 _[8];

    Fighter* fp = gobj->user_data;
    fp->x1980 = item_gobj;
    fp->x2018 = arg2;
    fp->x201C = p_ftCommonData->x6F8;
    fp->x2024 = arg3;
    fp->x2021 = 0;
    fp->x2020 = 0;
    ftCommon_8007EBAC(fp, 2, 0);
    ftCommon_AuxiliaryItemAttach(gobj);
}

void ftCommon_8007FF74(HSD_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    fp->x1980 = NULL;
    ftCommon_8007ECD4(fp, 2);
    pl_80040460(fp->player_id, fp->x221F_b4);
}

bool ftCommon_8007FFD8(Fighter* fp, float arg8)
{
    s32 phi_r31;
    s8 b0, b1;
    phi_r31 = false;
    if (fp->input.pressed_buttons & (HSD_PAD_AB | HSD_PAD_XY | HSD_PAD_LR)) {
        fp->x2018 -= arg8;
        phi_r31 = true;
    }
    b0 = fp->x2020;
    b1 = fp->x2021;
    if (fp->input.lstick[0].x < -p_ftCommonData->x308) {
        fp->x2020 = -1;
    }
    if (fp->input.lstick[0].x > p_ftCommonData->x308) {
        fp->x2020 = 1;
    }
    if (fp->input.lstick[0].y < -p_ftCommonData->x308) {
        fp->x2021 = -1;
    }
    if (fp->input.lstick[0].y > p_ftCommonData->x308) {
        fp->x2021 = 1;
    }
    if (b0 != fp->x2020 || b1 != fp->x2021) {
        fp->x2018 = fp->x2018 - arg8;
        phi_r31 = true;
    }
    pl_800402D0(fp->player_id, fp->x221F_b4, phi_r31);
    return phi_r31;
}

bool ftCommon_80080144(Fighter* fp)
{
    int kind = fp->kind;
    if ((kind == Ft_Kind_Popo || kind == Ft_Kind_Nana) &&
        fp->x619_costume_id >= 2)
    {
        return true;
    }
    return false;
}

/// https://decomp.me/scratch/Jjkwx
void ftCommon_80080174(Fighter* fp)
{
    float phi_f2;
    float phi_f3;
    Vec3* v;

    u8 _[16];

    if (fp->x197C != NULL) {
        it_80294E78(fp->x197C, fp->x34_scale.y * fp->co_attrs.x12C);
    }
    if (fp->x1980 != NULL) {
        v = &fp->co_attrs.x130;
        if ((phi_f2 = p_ftCommonData->x710 * fp->x2024 +
                      p_ftCommonData->x708) > (phi_f3 = p_ftCommonData->x70C))
        {
            phi_f2 = phi_f3;
        }
        it_8029A89C(fp->x1980, phi_f2 * v[1].x * fp->x34_scale.y);
    }
}

void ftCommon_8008021C(HSD_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    Vec2 shift;
    Vec2* result;

    u8 _[4];

    fp->dmg.x18BC = 0;
    fp->dmg.x18B8 = 0;
    if (ftCo_80090690(fp, &shift) != NULL) {
        fp->dmg.x18B8 += shift.x;
        fp->dmg.x18BC += shift.y;
    }
    if (fp->x2224_b5) {
        Vec2* temp_r3 = &Fighter_GrabMashShake->x0[fp->x1A52];
        shift.x = temp_r3->x;
        shift.y = temp_r3->y;
        result = &shift;
    } else {
        result = NULL;
    }
    if (result != NULL) {
        fp->dmg.x18B8 += shift.x;
        fp->dmg.x18BC += shift.y;
    }
    if (ftCo_800DEEE8(fp, &shift) != NULL) {
        fp->dmg.x18B8 += shift.x;
        fp->dmg.x18BC += shift.y;
    }
}

void ftCommon_8008031C(HSD_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    if ((ABS(fp->input.lstick[0].x) >= p_ftCommonData->x7B8 &&
         fp->x679_x < p_ftCommonData->x7C0) ||
        (ABS(fp->input.lstick[0].y) >= p_ftCommonData->x7B8 &&
         fp->x67A_y < p_ftCommonData->x7C0))
    {
        Player_UpdateJoystickCountByIndex((s32) fp->player_id, fp->x221F_b4);
        fp->x67A_y = 0xFE;
        fp->x679_x = 0xFE;
    }
    if (ABS(fp->input.triggers[0]) >= p_ftCommonData->x7BC) {
        if (fp->x67B < p_ftCommonData->x7C0) {
            Player_UpdateJoystickCountByIndex((s32) fp->player_id,
                                              fp->x221F_b4);
            fp->x67B = 0xFE;
        }
    }
}

void ftCommon_80080460(Fighter* fp)
{
    fp->x2225_b6 = true;
}

void ftCommon_80080474(Fighter* fp)
{
    fp->x2225_b6 = fp->x2225_b5;
}

void ftCommon_80080484(Fighter* fp)
{
    if (fp->smash_attrs.x2138_smashSinceHitbox == -1) {
        fp->smash_attrs.x2138_smashSinceHitbox = 0;
    }
}

void ftCommon_800804A0(Fighter* fp, float arg8)
{
    float temp_f1;
    float phi_f31 = arg8;
    if ((temp_f1 = ft_GetGroundFrictionMultiplier(fp)) < 1) {
        phi_f31 *= temp_f1;
    }
    fp->xE8_ground_accel_2 = phi_f31;
}

float ftCommon_800804EC(Fighter* fp)
{
    return fp->x40 * fp->x34_scale.x;
}

void ftCommon_800804FC(Fighter* fp)
{
    if (fp->ground_or_air == GA_Ground) {
        fp->dmg.x18c4_source_ply = 6;
        fp->dmg.x18C8 = -1;
    }
}
