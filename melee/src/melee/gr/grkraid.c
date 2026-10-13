#include "grkraid.h"
#include "stage_hazard_state_internal.h"

#include "forward.h"
#include "grzakogenerator.h"
#include "inlines.h"
#include <melee/cm/camera.h>
#include <melee/ft/ftlib.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lb_00F9.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/random.h>

struct grKraid_YakumonoParam {
    u32 map_time_min;
    u32 map_time_max;
    s32 map_time_acl;
    float map_rot_spd_min;
    float map_rot_spd_max;
    u32 kraid_wait_time;
    u32 kraid_wait_time_add;
    float kraid_pos_x[6];
};

GrJoint grKr_803E4C78[] = { { 0, 3, 12 }, { 1, 3, 12 }, { 2, 3, 12 },
                            { 3, 3, 12 }, { 4, 3, 12 }, { 5, 3, 12 } };

StageCallbacks grKr_StageCallbacks[5] = {
    { grKraid_801FE1B0, grKraid_801FE1DC, grKraid_801FE1E4, grKraid_801FE1E8,
      0 },
    { grKraid_801FE1EC, grKraid_801FE2C8, grKraid_801FE2D0, grKraid_801FE35C,
      0 },
    { grKraid_801FE360, grKraid_801FE3A4, grKraid_801FE3AC, grKraid_801FE3B0,
      0 },
    { grKraid_InitRotation, grKraid_801FE438, grKraid_UpdateRotation,
      grKraid_801FE6D4, 0xC0000000 },
    { grKraid_InitActor, grKraid_801FE9F8, grKraid_UpdateActor,
      grKraid_801FF14C, 0 }
};

StageData grKr_StageData = {
    Gr_Kind_Kraid,
    grKr_StageCallbacks,
    "/GrKr.dat",
    grKraid_OnInit,
    grKraid_OnDemoInit,
    grKraid_OnLoad,
    grKraid_OnStart,
    grKraid_801FE0BC,
    grKraid_OnTouchLine,
    grKraid_OnCheckShadowRender,
    (1 << 0),
    grKr_803E4C78,
    6,
};




void grKraid_OnDemoInit(int unused)
{
    return;
}

void grKraid_OnInit(void)
{
    HSD_GObj* gobj;

    kraid_state.parameters = Ground_GetYakumonoParam();
    stage_info.unk8C.b4 = false;
    stage_info.unk8C.b5 = true;
    grKraid_801FE0C4(0);
    grKraid_801FE0C4(2);
    grKraid_801FE0C4(3);
    gobj = grKraid_801FE0C4(4);
    HSD_JObjSetFlagsAll(GET_JOBJ(gobj), JOBJ_HIDDEN);
    gobj = grKraid_801FE0C4(1);
    HSD_JObjSetFlagsAll(GET_JOBJ(gobj), JOBJ_HIDDEN);
    Ground_801C39C0();
    Ground_801C3BB4();
}

void grKraid_OnLoad(void) {}

void grKraid_OnStart(void)
{
    grZakoGenerator_801CAE04(NULL);
}

bool grKraid_801FE0BC(void)
{
    return false;
}

HSD_GObj* grKraid_801FE0C4(int gobj_id)
{
    HSD_GObj* gobj;
    StageCallbacks* callbacks = &grKr_StageCallbacks[gobj_id];

    gobj = Ground_GetStageGObj(gobj_id);

    if (gobj != NULL) {
        Ground_SetupStageCallbacks(gobj, callbacks);
    } else {
        OSReport("%s:%d: couldn t get gobj(id=%d)\n", __FILE__, 223, gobj_id);
    }

    return gobj;
}

void grKraid_801FE1B0(Ground_GObj* gobj)
{
    Ground* gp = GET_GROUND(gobj);
    grAnime_801C8138(gobj, gp->map_id, 0);
}

bool grKraid_801FE1DC(Ground_GObj* gobj)
{
    return false;
}

void grKraid_801FE1E4(Ground_GObj* gobj)
{
    return;
}

void grKraid_801FE1E8(Ground_GObj* gobj)
{
    return;
}

void grKraid_801FE1EC(Ground_GObj* gobj)
{
    Ground* gp = gobj->user_data;
    Vec3 scale;

    scale.x = scale.y = scale.z = Ground_801C0498();
    HSD_JObjSetScale(GET_JOBJ(gobj), &scale);
    gp->x11_flags.b012 = 1;
}

bool grKraid_801FE2C8(Ground_GObj* gobj)
{
    return false;
}

void grKraid_801FE2D0(Ground_GObj* gobj)
{
    HSD_PadStatus* master = HSD_PadMasterStatus;
    Ground* gp = GET_GROUND(gobj);
    if ((master[1].trigger & PAD_BUTTON_A) != 0) {
        kraid_state.debug_animation += 1;
        if (kraid_state.debug_animation > 6) {
            kraid_state.debug_animation = 0;
        }
        OSReport("*** Req Effect Anime %d\n", kraid_state.debug_animation);
        grAnime_801C8138(gobj, gp->map_id, kraid_state.debug_animation);
    }
}

void grKraid_801FE35C(Ground_GObj* gobj)
{
    return;
}

void grKraid_801FE360(Ground_GObj* gobj)
{
    Ground* gp = GET_GROUND(gobj);
    grAnime_801C8138(gobj, gp->map_id, 0);
    gp->x11_flags.b012 = 2;
}

bool grKraid_801FE3A4(Ground_GObj* gobj)
{
    return false;
}

void grKraid_801FE3AC(Ground_GObj* gobj)
{
    return;
}

void grKraid_801FE3B0(Ground_GObj* gobj)
{
    return;
}

void grKraid_InitRotation(Ground_GObj* gobj)
{
    Ground* gp = GET_GROUND(gobj);
    HSD_JObj* jobj = GET_JOBJ(gobj);

    Ground_801C2ED0(jobj, gp->map_id);
    gp->u.kraid_rotation.state = 0;
    gp->u.kraid_rotation.trigger_direction = 0.0f;
    gp->u.kraid_rotation.remaining_angle = 0.0f;
    gp->u.kraid_rotation.deceleration = 0.0f;
    grAnime_801C8138(gobj, gp->map_id, 0);
    grAnime_801C7FF8(gobj, 0xD, 0x7, 0x2, 0.0f, 1.0f);
}

bool grKraid_801FE438(Ground_GObj* gobj)
{
    return false;
}

const Vec grKr_803B8278 = { 0.0f, 0.0f, 0.0f };
void grKraid_UpdateRotation(Ground_GObj* gobj)
{
    float fVar2;
    float fVar3;
    float fVar4;
    Ground* gp = GET_GROUND(gobj);
    HSD_JObj* jobj = Ground_801C3FA4(gobj, 0);
    Vec pos = grKr_803B8278;
    switch (gp->u.kraid_rotation.state) {
    case 0:
        if (gp->u.kraid_rotation.trigger_direction) {
            /// @remark Explicit != 0.0f comparison leads to incorrect ordering
            gp->u.kraid_rotation.wait_timer =
                rand_range(kraid_state.parameters->map_time_max,
                           kraid_state.parameters->map_time_min);
            gp->u.kraid_rotation.state = 1;
            grAnime_801C7FF8(gobj, 18, 7, 1, 0.0f, 1.0f);
            Ground_801C53EC(420007);
            gp->u.kraid_rotation.effect_animation_active = 1;
        }
        break;
    case 1:
        if (gp->u.kraid_rotation.wait_timer == 0) {
            gp->u.kraid_rotation.remaining_angle -= gp->u.kraid_rotation.deceleration;
        } else {
            gp->u.kraid_rotation.wait_timer--;
        }
        fVar2 = gp->u.kraid_rotation.deceleration < 0.0f
                    ? -gp->u.kraid_rotation.deceleration
                    : gp->u.kraid_rotation.deceleration;
        fVar3 = gp->u.kraid_rotation.remaining_angle;
        if (fVar3 < 0.0f) {
            fVar4 = -fVar3;
        } else {
            fVar4 = fVar3;
        }
        if (fVar4 < fVar2) {
            gp->u.kraid_rotation.deceleration = 0.0f;
            gp->u.kraid_rotation.remaining_angle = 0.0f;
            gp->u.kraid_rotation.trigger_direction = 0.0f;
            gp->u.kraid_rotation.state = 0;
        } else {
            fVar3 = fVar3 * (3.14159265f / 180);
            HSD_JObjAddRotationZ(jobj, fVar3);
            Camera_RequestQuake(QuakeKind_Loop, &pos);
        }
        break;
    }
    if (gp->u.kraid_rotation.effect_animation_active != 0 &&
        grAnime_801C83D0(gobj, 18, 7) != 0)
    {
        grAnime_801C7FF8(gobj, 18, 7, 0, 0.0f, 1.0f);
        gp->u.kraid_rotation.effect_animation_active = 0;
    }
    lb_800115F4();
    Ground_801C2FE0(gobj);
    grKraid_801FF150(gobj);
}

void grKraid_801FE6D4(Ground_GObj* gobj)
{
    return;
}

void grKraid_RequestRotation(HSD_JObj* hand, float direction)
{
    Ground* map = Ground_GetMapGObj(3)->user_data;
    Vec handpos;
    f32 rot;
    f32 min;
    f32 max;
    if (map->u.kraid_rotation.trigger_direction == 0.0f) {
        map->u.kraid_rotation.trigger_direction = direction;
        lb_8000B1CC(hand, NULL, &handpos);
        max = kraid_state.parameters->map_rot_spd_max;
        min = kraid_state.parameters->map_rot_spd_min;
        rot = (max / min) / 132.0f;
        rot = rot * min;
        map->u.kraid_rotation.remaining_angle =
            rot * ((handpos.x < 0.0f) ? -handpos.x : handpos.x);
        if (map->u.kraid_rotation.remaining_angle <
            kraid_state.parameters->map_rot_spd_min)
        {
            map->u.kraid_rotation.remaining_angle =
                kraid_state.parameters->map_rot_spd_min;
        }
        if (map->u.kraid_rotation.remaining_angle >
            kraid_state.parameters->map_rot_spd_max)
        {
            map->u.kraid_rotation.remaining_angle =
                kraid_state.parameters->map_rot_spd_max;
        }
        map->u.kraid_rotation.remaining_angle *=
            map->u.kraid_rotation.trigger_direction;
        map->u.kraid_rotation.deceleration =
            map->u.kraid_rotation.remaining_angle /
            kraid_state.parameters->map_time_acl;
        Ground_801C53EC(420005);
        Camera_RequestQuake(QuakeKind_Medium, &handpos);
    }
}

void grKraid_InitActor(Ground_GObj* gobj)
{
    float fVar2;
    float fVar3;
    float fVar1;
    s8 iVar;

    Ground* gp = GET_GROUND(gobj);
    gp->u.kraid_actor.left_hand = Ground_801C3FA4(gobj, 31);
    gp->u.kraid_actor.right_hand = Ground_801C3FA4(gobj, 47);
    fVar1 = (fVar3 = 0.0f);
    gp->u.kraid_actor.rightmost_position_index = 0;
    gp->u.kraid_actor.leftmost_position_index = 0;

    fVar2 = kraid_state.parameters->kraid_pos_x[0];
    if (fVar2 < 0.0f) {
        gp->u.kraid_actor.leftmost_position_index = 0;
        fVar3 = fVar2;
    } else {
        if (fVar2 > fVar1) {
            fVar1 = fVar2;
            gp->u.kraid_actor.rightmost_position_index = 0;
        }
    }
    fVar2 = kraid_state.parameters->kraid_pos_x[1];
    if (fVar2 < fVar3) {
        gp->u.kraid_actor.leftmost_position_index = 1;
        fVar3 = fVar2;
    } else {
        if (fVar2 > fVar1) {
            fVar1 = fVar2;
            gp->u.kraid_actor.rightmost_position_index = 1;
        }
    }
    fVar2 = kraid_state.parameters->kraid_pos_x[2];
    if (fVar2 < fVar3) {
        gp->u.kraid_actor.leftmost_position_index = 2;
        fVar3 = fVar2;
    } else {
        if (fVar2 > fVar1) {
            fVar1 = fVar2;
            gp->u.kraid_actor.rightmost_position_index = 2;
        }
    }
    fVar2 = kraid_state.parameters->kraid_pos_x[3];
    if (fVar2 < fVar3) {
        gp->u.kraid_actor.leftmost_position_index = 3;
        fVar3 = fVar2;
    } else {
        if (fVar2 > fVar1) {
            fVar1 = fVar2;
            gp->u.kraid_actor.rightmost_position_index = 3;
        }
    }
    fVar2 = kraid_state.parameters->kraid_pos_x[4];
    if (fVar2 < fVar3) {
        gp->u.kraid_actor.leftmost_position_index = 4;
        fVar3 = fVar2;
    } else {
        if (fVar2 > fVar1) {
            fVar1 = fVar2;
            gp->u.kraid_actor.rightmost_position_index = 4;
        }
    }
    fVar2 = kraid_state.parameters->kraid_pos_x[5];
    if (fVar2 < fVar3) {
        gp->u.kraid_actor.leftmost_position_index = 5;
        fVar3 = fVar2;
    } else {
        if (fVar2 > fVar1) {
            fVar1 = fVar2;
            gp->u.kraid_actor.rightmost_position_index = 5;
        }
    }
    gp->u.kraid_actor.state = 0;
    gp->u.kraid_actor.timer =
        kraid_state.parameters->kraid_wait_time +
        ((s32) kraid_state.parameters->kraid_wait_time_add != 0
             ? HSD_Randi(kraid_state.parameters->kraid_wait_time_add)
             : 0);
    gp->u.kraid_actor.side = 0.0f;
    fVar2 = 0;
    iVar = (u8) fVar2;
    gp->u.kraid_actor.previous_position_index = iVar;
    gp->u.kraid_actor.animation_index = iVar;
    gp->u.kraid_actor.choose_random_position = 1;
}

bool grKraid_801FE9F8(Ground_GObj* gobj)
{
    return false;
}

void grKraid_UpdateActor(Ground_GObj* gobj)
{
    s8 cVar3;
    Ground* gp = GET_GROUND(gobj);
    HSD_JObj* jobj = GET_JOBJ(gobj);
    PAD_STACK(0x70);
    switch (gp->u.kraid_actor.state) {
    case 0:
        if (gp->u.kraid_actor.timer <= 0) {
            Ground* map = Ground_GetMapGObj(3)->user_data;
            if (map->u.kraid_rotation.trigger_direction == 0.0F) {
                grKraid_ChooseActorPosition(gobj);
                HSD_JObjClearFlagsAll(jobj, JOBJ_HIDDEN);
                HSD_JObjClearFlagsAll(Ground_GetMapGObj(1)->hsd_obj,
                                      JOBJ_HIDDEN);
                grKraid_PlayActorAnimation(gobj, 3);
                Ground_801C53EC(420001);
                Ground_801C53EC(420008);
                gp->u.kraid_actor.state = 1;
            }
        } else {
            gp->u.kraid_actor.timer += -1;
        }
        break;
    case 1:
        if (grAnime_801C83D0(gobj, 0, 7) != 0) {
            grKraid_PlayActorAnimation(gobj, HSD_Randi(3));
            gp->u.kraid_actor.timer = 0;
            gp->u.kraid_actor.state = 2;
        }
        break;
    case 2:
        cVar3 = gp->u.kraid_actor.animation_index;
        if (cVar3 == 0 && gp->u.kraid_actor.timer == 80) {
            Ground_801C53EC(420000);
        } else if (cVar3 == 1 && gp->u.kraid_actor.timer == 45) {
            Ground_801C53EC(420000);
        } else if (cVar3 == 2 && gp->u.kraid_actor.timer == 55) {
            Ground_801C5440(gp, 0, 420000);
        }
        cVar3 = gp->u.kraid_actor.animation_index;
        if (cVar3 == 2 && gp->u.kraid_actor.timer == 161) {
            Ground_801C5544(gp, 0);
            Ground_801C53EC(420004);
        } else if (cVar3 == 2 && gp->u.kraid_actor.timer == 246) {
            Ground_801C53EC(420004);
        }
        if (grAnime_801C83D0(gobj, 0, 7) != 0) {
            grKraid_PlayActorAnimation(gobj, gp->u.kraid_actor.side + 5.0f);
            gp->u.kraid_actor.timer = 0;
            gp->u.kraid_actor.state = 3;
        }
        gp->u.kraid_actor.timer++;
        break;
    case 3:
        if (gp->u.kraid_actor.animation_index == 5) {
            Ground* map = Ground_GetMapGObj(3)->user_data;
            if (map->u.kraid_rotation.trigger_direction == 0.0F &&
                gp->u.kraid_actor.timer >= 147 &&
                gp->u.kraid_actor.timer <= 169)
            {
                if (gp->u.kraid_actor.timer == 147) {
                    Ground_801C53EC(420003);
                }
                if (grKraid_ReachedHandStrikeFrame(gobj, 1)) {
                    gp->u.kraid_actor.choose_random_position = 1;
                    gp->u.kraid_actor.side = 1.0f;
                    grKraid_RequestRotation(gp->u.kraid_actor.right_hand,
                                            -1.0f);
                }
            }
            gp->u.kraid_actor.timer++;
        } else {
            Ground* map = Ground_GetMapGObj(3)->user_data;
            if (map->u.kraid_rotation.trigger_direction == 0.0F &&
                gp->u.kraid_actor.timer >= 123 &&
                gp->u.kraid_actor.timer <= 138)
            {
                if (gp->u.kraid_actor.timer == 123) {
                    Ground_801C53EC(420003);
                }
                if (grKraid_ReachedHandStrikeFrame(gobj, 0)) {
                    gp->u.kraid_actor.choose_random_position = 1;
                    gp->u.kraid_actor.side = 1.0f;
                    grKraid_RequestRotation(gp->u.kraid_actor.left_hand,
                                            1.0f);
                }
            }
            gp->u.kraid_actor.timer++;
        }
        if (grAnime_801C83D0(gobj, 0, 7) != 0) {
            grKraid_PlayActorAnimation(gobj, 4);
            Ground_801C53EC(420002);
            Ground_801C53EC(420008);
            gp->u.kraid_actor.state = 4;
        }
        break;
    case 4:
        if (grAnime_801C83D0(gobj, 0, 7) != 0) {
            gp->u.kraid_actor.timer = kraid_state.parameters->kraid_wait_time +
                              kraid_state.parameters->kraid_wait_time_add;
            HSD_JObjSetFlagsAll(jobj, JOBJ_HIDDEN);
            HSD_JObjSetFlagsAll(Ground_GetMapGObj(1)->hsd_obj, JOBJ_HIDDEN);
            gp->u.kraid_actor.timer =
                kraid_state.parameters->kraid_wait_time +
                ((s32) kraid_state.parameters->kraid_wait_time_add != 0
                     ? HSD_Randi(kraid_state.parameters->kraid_wait_time_add)
                     : 0);
            gp->u.kraid_actor.state = 0;
        }
        break;
    }
}

void grKraid_ChooseActorPosition(HSD_GObj* gobj)
{
    s32 iVar3;
    Ground* gp = GET_GROUND(gobj);
    HSD_JObj* jobj = GET_JOBJ(gobj);
    PAD_STACK(0x10);
    if (gp->u.kraid_actor.choose_random_position != 0) {
        do {
            iVar3 = HSD_Randi(6);
        } while (gp->u.kraid_actor.previous_position_index == iVar3);
    } else {
        iVar3 = HSD_Randi(2) != 0
                    ? gp->u.kraid_actor.leftmost_position_index
                    : gp->u.kraid_actor.rightmost_position_index;
    }
    gp->u.kraid_actor.choose_random_position = 0;
    gp->u.kraid_actor.previous_position_index = iVar3;
    HSD_JObjSetTranslateX(jobj, kraid_state.parameters->kraid_pos_x[iVar3]);
    HSD_JObjSetTranslateX(Ground_GetMapGObj(1)->hsd_obj,
                          kraid_state.parameters->kraid_pos_x[iVar3]);
    if (kraid_state.parameters->kraid_pos_x[iVar3] == 0.0f) {
        if (HSD_Randi(2) != 0) {
            iVar3 = 1;
        } else {
            iVar3 = 0;
        }
        gp->u.kraid_actor.side = iVar3;
    } else if (kraid_state.parameters->kraid_pos_x[iVar3] < 0.0f) {
        gp->u.kraid_actor.side = 0.0f;
    } else {
        gp->u.kraid_actor.side = 1.0f;
    }
}

void grKraid_PlayActorAnimation(HSD_GObj* gobj, int val)
{
    int temp;
    Ground* gp = GET_GROUND(gobj);
    PAD_STACK(8);

    gp->u.kraid_actor.animation_index = val;
    grAnime_801C8138(gobj, gp->map_id, gp->u.kraid_actor.animation_index);
    temp = gp->u.kraid_actor.animation_index;
    gobj = Ground_GetMapGObj(1);
    gp = GET_GROUND(gobj);
    gp->u.kraid_actor.animation_index = temp;
    grAnime_801C8138(gobj, gp->map_id, gp->u.kraid_actor.animation_index);
    gp->u.kraid_actor.state = 1;
}

bool grKraid_ReachedHandStrikeFrame(Ground_GObj* gobj, int right_hand)
{
    Ground* gp = GET_GROUND(gobj);
    int val;
    bool comp;

    if (right_hand == 1) {
        val = 0x9e;
    } else {
        val = 0x82;
    }
    comp = gp->u.kraid_actor.timer >= val ? true : false;
    if (comp != false) {
        ftLib_80086C9C(0xC, 0x0);
    }
    return comp;
}

void grKraid_801FF14C(Ground_GObj* gobj)
{
    return;
}

void grKraid_801FF150(Ground_GObj* gobj) {}

lbColl_80008D30_arg1* grKraid_OnTouchLine(enum_t unused)
{
    return false;
}

bool grKraid_OnCheckShadowRender(Vec3* a, int _, HSD_JObj* joint)
{
    return true;
}
