#include "gronett.h"

#include <Runtime/platform.h>

#include <placeholder.h>

#include "grdatfiles.h"
#include "grdisplay.h"
#include "grlib.h"
#include "grmaterial.h"
#include "grzakogenerator.h"
#include "inlines.h"
#include "types.h"
#include <melee/cm/camera.h>
#include <melee/cm/types.h>
#include <melee/ft/ftlib.h>
#include <melee/gm/gmvs.h>
#include <melee/if/ifhazard.h>
#include <melee/it/it_2725.h>
#include <melee/it/item.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lb_00F9.h>
#include <melee/lb/types.h>
#include <melee/mp/mplib.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjproc.h>

/* 1E40E4 */ static void
grOnett_OnBuildingCollision(void* user_data, int joint_id, CollData* coll,
                            int coll_x50, mpLib_GroundEnum ground_kind,
                            float delta_y);
/* 1E54B4 */ static void
grOnett_OnAwningCollision(void* user_data, int joint_id, CollData* coll,
                          int coll_x50, mpLib_GroundEnum ground_kind,
                          float delta_y);

/* 3E27E0 */ static StageCallbacks grOt_803E27E0[] = {
    { 0 },
    { 0 },
    {
        grOnett_801E38DC,
        grOnett_801E3920,
        grOnett_801E3928,
        grOnett_801E392C,
        0,
    },
    {
        grOnett_InitTraffic,
        grOnett_801E43D8,
        grOnett_UpdateTraffic,
        grOnett_801E502C,
        0,
    },
    {
        grOnett_InitBuilding,
        grOnett_801E3D98,
        grOnett_UpdateBuilding,
        grOnett_801E40E0,
        0,
    },
    {
        grOnett_InitAwnings,
        grOnett_801E3C58,
        grOnett_UpdateAwnings,
        grOnett_801E3CE0,
        (1 << 30) | (1 << 31),
    },
};

/* 3E2858 */ StageData grOt_StageData = {
    Gr_Kind_Onett,
    grOt_803E27E0,
    "/GrOt",
    grOnett_801E3738,
    grOnett_801E3734,
    grOnett_801E37C4,
    grOnett_801E37C8,
    grOnett_801E37EC,
    grOnett_801E5760,
    grOnett_801E5768,
    (1 << 0),
    NULL,
    0,
};

/// Onett stage yakumono parameters
struct grOnett_StageParam {
    /* 0x00 */ f32 awning_collision_velocity;
    /* 0x04 */ f32 max_velocity;
    /* 0x08 */ f32 vel_threshold;
    /* 0x0C */ f32 pos_threshold;
    /* 0x10 */ f32 damping;
    /* 0x14 */ f32 spring_force;
    /* 0x18 */ f32 spring_constant;
    /* 0x1C */ f32 max_displacement;
    /* 0x20 */ f32 awning_contact_displacement;
    /* 0x24 */ f32 ambient_hidden_frames_min;
    /* 0x28 */ f32 ambient_hidden_frames_max;
    /* 0x2C */ f32 ambient_visible_frames_min;
    /* 0x30 */ f32 ambient_visible_frames_max;
    /* 0x34 */ f32 building_first_break_hits;
    /* 0x38 */ f32 building_second_break_hits;
    /* 0x3C */ f32 building_rebuild_delay_min;
    /* 0x40 */ f32 building_rebuild_delay_max;
    /* 0x44 */ f32 traffic_warning_frames;
    /* 0x48 */ f32 traffic_approach_frames;
    /* 0x4C */ f32 traffic_rank_delay_frames;
    /* 0x50 */ f32 traffic_last_place_delay_frames;
    /* 0x54 */ f32 traffic_base_speed;
    /* 0x58 */ f32 traffic_random_speed;
    /* 0x5C */ f32 damaged_car_spin_rate;
    /* 0x60 */ f32 hazard_indicator_frames;
    /* 0x64 */ f32 hazard_indicator_camera_offset;
};

static struct grOnett_StageParam* onett_params;

typedef enum OnettBuildingState {
    OnettBuildingState_Intact = 0,
    OnettBuildingState_FirstHit = 1,
    OnettBuildingState_FirstCollapse = 2,
    OnettBuildingState_Damaged = 3,
    OnettBuildingState_SecondHit = 4,
    OnettBuildingState_SecondCollapse = 5,
    OnettBuildingState_RebuildDelay = 6,
    OnettBuildingState_Rebuilding = 7,
} OnettBuildingState;

typedef enum OnettTrafficState {
    OnettTrafficState_SelectCar = 0,
    OnettTrafficState_Warning = 1,
    OnettTrafficState_Approach = 2,
    OnettTrafficState_RankDelay = 3,
    OnettTrafficState_Damaged = 4,
    OnettTrafficState_Driving = 5,
    OnettTrafficState_HitWithoutSpin = 6,
} OnettTrafficState;

typedef enum OnettTrafficProgress {
    OnettTrafficProgress_None = 0,
    OnettTrafficProgress_IndicatorShown = 1,
    OnettTrafficProgress_PassingSoundPlayed = 2,
    OnettTrafficProgress_Complete = 4,
} OnettTrafficProgress;

void grOnett_801E3734(int arg) {}

void grOnett_801E3738(void)
{
    onett_params = Ground_GetYakumonoParam();
    stage_info.unk8C.b4 = false;
    stage_info.unk8C.b5 = true;
    grOnett_801E37F4(0);
    grOnett_801E37F4(2);
    grOnett_801E37F4(5);
    grOnett_801E37F4(4);
    grOnett_801E37F4(3);
    un_802FD8A0(grOnett_GetHazardIndicatorModel());
    un_802FD65C();
    Ground_801C39C0();
    Ground_801C3BB4();
}

void grOnett_801E37C4(void) {}

void grOnett_801E37C8(void)
{
    grZakoGenerator_801CAE04(NULL);
}

bool grOnett_801E37EC(void)
{
    return false;
}

HSD_GObj* grOnett_801E37F4(int gobj_id)
{
    HSD_GObj* gobj;
    StageCallbacks* callbacks = &grOt_803E27E0[gobj_id];

    gobj = Ground_GetStageGObj(gobj_id);

    if (gobj != NULL) {
        Ground* gp = GET_GROUND(gobj);
        gp->x8_callback = NULL;
        gp->xC_callback = NULL;
        GObj_SetupGXLink(gobj, grDisplay_801C5DB0, 3, 0);
        if (callbacks->callback3 != NULL) {
            gp->x1C_callback = callbacks->callback3;
        }
        if (callbacks->on_init != NULL) {
            callbacks->on_init(gobj);
        }
        if (callbacks->gobj_proc != NULL) {
            HSD_GObj_SetupProc(gobj, callbacks->gobj_proc, 4);
        }
    } else {
        OSReport("%s:%d: couldn t get gobj(id=%d)\n", __FILE__, 235, gobj_id);
    }

    return gobj;
}

void grOnett_801E38DC(Ground_GObj* gobj)
{
    Ground* gp = GET_GROUND(gobj);
    grAnime_801C8138(gobj, gp->map_id, 0);
    gp->x11_flags.b012 = 2;
}

bool grOnett_801E3920(Ground_GObj* gobj)
{
    return false;
}

void grOnett_801E3928(Ground_GObj* gobj) {}

void grOnett_801E392C(Ground_GObj* gobj) {}

void grOnett_SetupAwningCollision(Ground_GObj* gobj)
{
    Ground* gp = GET_GROUND(gobj);

    mpJointSetCb1(0, gp, grOnett_OnAwningCollision);
    mpJointSetCb1(1, gp, grOnett_OnAwningCollision);
    gp->u.onett.camera_subject = Camera_80029020();
    HSD_ASSERTMSG(331, gp->u.onett.camera_subject, "gp->u.map.subject");
    gp->u.onett.camera_subject->target_ext.h.x = -40.0f;
    gp->u.onett.camera_subject->target_ext.h.y = 40.0f;
    gp->u.onett.camera_subject->target_ext.v.x = 20.0f;
    gp->u.onett.camera_subject->target_ext.v.y = -20.0f;
    gp->u.onett.camera_subject->target_ext.v.z = 1.0f;
    gp->u.onett.camera_subject->pos.x = 0.0f;
    gp->u.onett.camera_subject->pos.y = 0.0f;
    gp->u.onett.camera_subject->pos.z = 0.0f;
    gp->u.onett.camera_subject->bone_pos = gp->u.onett.camera_subject->pos;
    gp->u.onett.camera_subject->state = CmSubjectState_Inactive;
}

void grOnett_InitAwnings(Ground_GObj* gobj)
{
    HSD_JObj* jobj;
    Ground* gp;
    PAD_STACK(8);
    gp = GET_GROUND(gobj);
    jobj = GET_JOBJ(gobj);

    Ground_801C2ED0(jobj, gp->map_id);
    grAnime_801C8138(gobj, gp->map_id, 0);
    gp->x10_flags.b5 = 1;

    gp->u.onett.camera_subject = NULL;

    gp->u.onett.awnings[0].velocity = 0.0f;
    gp->u.onett.awnings[0].joint = Ground_801C3FA4(gobj, 0xE);
    gp->u.onett.awnings[0].rest_y =
        HSD_JObjGetTranslationY(gp->u.onett.awnings[0].joint);
    gp->u.onett.awnings[0].contact_displacement = 0.0f;
    gp->u.onett.awnings[0].displacement = 0.0f;
    gp->u.onett.awnings[0].previous_contact_count = 0;
    gp->u.onett.awnings[0].contact_count = 0;
    gp->u.onett.awnings[0].landed_this_frame = 0;
    gp->u.onett.awnings[0].impact_effect_cooldown = 0;

    gp->u.onett.awnings[1].velocity = 0.0f;
    gp->u.onett.awnings[1].joint = Ground_801C3FA4(gobj, 0xF);
    gp->u.onett.awnings[1].rest_y =
        HSD_JObjGetTranslationY(gp->u.onett.awnings[1].joint);
    gp->u.onett.awnings[1].contact_displacement = 0.0f;
    gp->u.onett.awnings[1].displacement = 0.0f;
    gp->u.onett.awnings[1].previous_contact_count = 0;
    gp->u.onett.awnings[1].contact_count = 0;
    gp->u.onett.awnings[1].landed_this_frame = 0;
    gp->u.onett.awnings[1].impact_effect_cooldown = 0;

    gp->u.onett.ambient_effect_timer =
        rand_range(onett_params->ambient_hidden_frames_max,
                   onett_params->ambient_hidden_frames_min);
    gp->u.onett.ambient_effect = NULL;

    Ground_801C4E70(Ground_801C3FA4(gobj, 16), Ground_801C3FA4(gobj, 17),
                    Ground_801C3FA4(gobj, 19), Ground_801C3FA4(gobj, 20),
                    Ground_801C3FA4(gobj, 18), Ground_801C3FA4(gobj, 21));

    Ground_801C10B8(gobj, grOnett_SetupAwningCollision);
    Ground_801C2FE0(gobj);
}

bool grOnett_801E3C58(Ground_GObj* gobj)
{
    return false;
}

void grOnett_UpdateAwnings(Ground_GObj* gobj)
{
    Ground* gp = GET_GROUND(gobj);
    PAD_STACK(8);

    grOnett_UpdateAwningPhysics(gobj);
    grOnett_UpdateAmbientEffect(gobj);
    if (gp->u.onett.camera_subject != NULL) {
        if (Ground_801C5794() != 0) {
            gp->u.onett.camera_subject->state = CmSubjectState_Active;
        } else {
            gp->u.onett.camera_subject->state = CmSubjectState_Inactive;
        }
    }
    lb_800115F4();
    Ground_801C2FE0(gobj);
}

void grOnett_801E3CE0(Ground_GObj* gobj) {}

void grOnett_InitBuilding(Ground_GObj* gobj)
{
    Ground* gp = GET_GROUND(gobj);
    PAD_STACK(8);

    Ground_801C2ED0(GET_JOBJ(gobj), gp->map_id);
    grAnime_801C7FF8(gobj, 0, 7, 0, 0.0f, 0.0f);

    gp->u.onett_building.current_state = -1;
    gp->u.onett_building.requested_state = OnettBuildingState_Intact;
    gp->u.onett_building.hit_count = 0;
    gp->u.onett_building.state_frame = 0;

    gp->x8_callback = NULL;
    gp->xC_callback = NULL;
    gp->x10_flags.b5 = 1;

    mpJointSetCb1(3, gp, grOnett_OnBuildingCollision);
    mpJointSetCb1(4, gp, grOnett_OnBuildingCollision);
}

bool grOnett_801E3D98(Ground_GObj* gobj)
{
    return false;
}

void grOnett_UpdateBuilding(Ground_GObj* gobj)
{
    Ground* gp = GET_GROUND(gobj);
    HSD_JObj* jobj = GET_JOBJ(gobj);
    s32 tmp;

    gp->u.onett_building.state_frame += 1;

    switch (gp->u.onett_building.current_state) {
    case OnettBuildingState_Rebuilding:
        if (gp->u.onett_building.state_timer & 2) {
            HSD_JObjSetFlagsAll(Ground_801C3FA4(gobj, 5), JOBJ_HIDDEN);
            HSD_JObjSetFlagsAll(Ground_801C3FA4(gobj, 2), JOBJ_HIDDEN);
            HSD_JObjSetFlagsAll(Ground_801C3FA4(gobj, 4), JOBJ_HIDDEN);
        } else {
            HSD_JObjClearFlagsAll(jobj, JOBJ_HIDDEN);
        }
        tmp = gp->u.onett_building.state_timer;
        gp->u.onett_building.state_timer = tmp - 1;
        if (tmp < 0) {
            HSD_JObjClearFlagsAll(jobj, JOBJ_HIDDEN);
            gp->u.onett_building.requested_state = OnettBuildingState_Intact;
        }
        break;
    case OnettBuildingState_FirstHit:
        if (grAnime_801C83D0(gobj, 0, 7)) {
            gp->u.onett_building.requested_state = OnettBuildingState_Intact;
        }
        break;
    case OnettBuildingState_FirstCollapse:
        if (gp->u.onett_building.state_frame == 0x8C) {
            Ground_801C5440(gp, 0, 0x5F373);
        }
        if (grAnime_801C83D0(gobj, 0, 7)) {
            gp->u.onett_building.requested_state = OnettBuildingState_Damaged;
        }
        break;
    case OnettBuildingState_SecondHit:
        if (grAnime_801C83D0(gobj, 0, 7)) {
            gp->u.onett_building.requested_state = OnettBuildingState_Damaged;
        }
        break;
    case OnettBuildingState_SecondCollapse:
        if (gp->u.onett_building.state_frame == 0x46 ||
            gp->u.onett_building.state_frame == 0x82)
        {
            Ground_801C5440(gp, 0, 0x5F374);
        }
        if (grAnime_801C83D0(gobj, 0, 7)) {
            f32 rand = HSD_Randf();
            f32 lo = onett_params->building_rebuild_delay_min;
            f32 range = onett_params->building_rebuild_delay_max - lo;
            gp->u.onett_building.state_timer = (s32) (range * rand + lo);
            gp->u.onett_building.requested_state =
                OnettBuildingState_RebuildDelay;
        }
        if (gp->u.onett_building.state_frame == 0x14) {
            mpLib_80057BC0(3);
        } else if (gp->u.onett_building.state_frame == 0x5A) {
            mpLib_80057BC0(4);
        }
        break;
    case OnettBuildingState_RebuildDelay:
        tmp = gp->u.onett_building.state_timer;
        gp->u.onett_building.state_timer = tmp - 1;
        if (tmp < 0) {
            gp->u.onett_building.requested_state =
                OnettBuildingState_Rebuilding;
            gp->u.onett_building.state_timer = 0x3C;
        }
        break;
    }

    if (gp->u.onett_building.current_state !=
        gp->u.onett_building.requested_state)
    {
        gp->u.onett_building.current_state =
            gp->u.onett_building.requested_state;
        switch (gp->u.onett_building.current_state) {
        case OnettBuildingState_Rebuilding:
            grAnime_801C7FF8(gobj, 0, 7, 0, 0.0f, 1.0f);
            mpJointListAdd(3);
            mpJointListAdd(4);
            break;
        case OnettBuildingState_FirstHit:
            grAnime_801C7FF8(gobj, 0, 7, 1, 0.0f, 1.0f);
            break;
        case OnettBuildingState_FirstCollapse:
            grAnime_801C7FF8(gobj, 0, 7, 3, 0.0f, 1.0f);
            break;
        case OnettBuildingState_SecondHit:
            grAnime_801C7FF8(gobj, 0, 7, 4, 0.0f, 1.0f);
            break;
        case OnettBuildingState_SecondCollapse:
            grAnime_801C7FF8(gobj, 0, 7, 5, 0.0f, 1.0f);
            break;
        case OnettBuildingState_RebuildDelay:
            gp->u.onett_building.hit_count = 0;
            break;
        }
        gp->u.onett_building.state_frame = 0;
    }

    Ground_801C2FE0(gobj);
}

void grOnett_801E40E0(Ground_GObj* gobj) {}

/// @copydoc mpLib_JointCollisionCallback
void grOnett_OnBuildingCollision(void* user_data, int joint_id, CollData* coll,
                                 int coll_x50, mpLib_GroundEnum ground_kind,
                                 float delta_y)
{
    Ground* gp = user_data;

    if (ground_kind == 1) {
        gp->u.onett_building.hit_count += 1;
        switch (gp->u.onett_building.current_state) {
        case OnettBuildingState_SecondHit:
        case OnettBuildingState_SecondCollapse:
            break;
        case OnettBuildingState_Intact:
            if (gp->u.onett_building.hit_count <
                onett_params->building_first_break_hits)
            {
                gp->u.onett_building.requested_state =
                    OnettBuildingState_FirstHit;
                return;
            }
            gp->u.onett_building.requested_state =
                OnettBuildingState_FirstCollapse;
            return;
        case OnettBuildingState_Damaged:
            if (gp->u.onett_building.hit_count <
                onett_params->building_second_break_hits)
            {
                gp->u.onett_building.requested_state =
                    OnettBuildingState_SecondHit;
                return;
            }
            gp->u.onett_building.requested_state =
                OnettBuildingState_SecondCollapse;
            break;
        }
    }
}

void grOnett_StartTraffic(HSD_GObj* gobj)
{
    GET_GROUND(gobj)->u.onettcar.startup_pending = false;
}

void grOnett_InitTraffic(Ground_GObj* gobj)
{
    Ground* gp = GET_GROUND(gobj);
    int i;
    PAD_STACK(16);

    grAnime_801C8138(gobj, gp->map_id, 0);
    gp->x8_callback = NULL;
    gp->xC_callback = NULL;

    gp->u.onettcar.car_joints[0] = Ground_801C3FA4(gobj, 0xB);
    gp->u.onettcar.car_joints[1] = Ground_801C3FA4(gobj, 0x1);
    gp->u.onettcar.car_joints[2] = Ground_801C3FA4(gobj, 0x6);
    gp->u.onettcar.car_joints[3] = Ground_801C3FA4(gobj, 0x10);

    gp->u.onettcar.damage_effect_joints[0] = Ground_801C3FA4(gobj, 0xF);
    gp->u.onettcar.damage_effect_joints[1] = Ground_801C3FA4(gobj, 0x5);
    gp->u.onettcar.damage_effect_joints[2] = Ground_801C3FA4(gobj, 0xA);
    gp->u.onettcar.damage_effect_joints[3] = Ground_801C3FA4(gobj, 0x14);

    for (i = 0; i < 4; i++) {
        HSD_JObjSetTranslateX(gp->u.onettcar.car_joints[i], 726.0f);
        HSD_JObjSetFlagsAll(gp->u.onettcar.car_joints[i], JOBJ_HIDDEN);
        gp->u.onettcar.hazard_items[i] =
            grMaterial_801C8CFC(0, 0, gp, gp->u.onettcar.car_joints[i],
                                grOnett_OnTrafficCarHit, NULL, NULL);
        grMaterial_801C8E08(gp->u.onettcar.hazard_items[i]);
        it_80274C60(gp->u.onettcar.hazard_items[i]);
    }

    gp->u.onettcar.left_to_right_car = 4;
    gp->u.onettcar.right_to_left_car = 4;
    gp->u.onettcar.left_to_right_state = OnettTrafficState_SelectCar;
    gp->u.onettcar.right_to_left_state = OnettTrafficState_SelectCar;

    Ground_801C10B8(gobj, grOnett_StartTraffic);
    gp->u.onettcar.startup_pending = true;
}

bool grOnett_801E43D8(Ground_GObj* gobj)
{
    return false;
}

static void grOnett_PrepareLeftToRightCar(Ground* controller)
{
    s8 previous_car = controller->u.onettcar.left_to_right_car;

    while (controller->u.onettcar.left_to_right_car == previous_car ||
           controller->u.onettcar.left_to_right_car ==
               controller->u.onettcar.right_to_left_car)
    {
        controller->u.onettcar.left_to_right_car = HSD_Randi(4);
    }

    HSD_JObj* car_joint =
        controller->u.onettcar
            .car_joints[controller->u.onettcar.left_to_right_car];
    HSD_JObjSetTranslateX(car_joint, -642.0f);
    HSD_JObjSetTranslateZ(car_joint, 56.0f);
    HSD_JObjSetRotationY(car_joint->child, M_PI);
    controller->u.onettcar.left_to_right_state = OnettTrafficState_Warning;
    controller->u.onettcar.left_to_right_timer =
        onett_params->traffic_warning_frames;
}

static void grOnett_UpdateRightToLeftWarning(Ground* controller, s8 car_index)
{
    s32 remaining_frames = controller->u.onettcar.right_to_left_timer;

    if (remaining_frames != 0) {
        controller->u.onettcar.right_to_left_timer = remaining_frames - 1;
    } else {
        controller->u.onettcar.right_to_left_timer =
            onett_params->traffic_approach_frames;
        controller->u.onettcar.right_to_left_speed =
            onett_params->traffic_random_speed * HSD_Randf() +
            onett_params->traffic_base_speed;
        if (controller->u.onettcar.right_to_left_speed > 0.0f) {
            controller->u.onettcar.right_to_left_speed *= -1.0f;
        }
        controller->u.onettcar.right_to_left_state =
            OnettTrafficState_Approach;
        Item_80268E5C(controller->u.onettcar.hazard_items[car_index],
                      car_index + 2, ITEM_ANIM_UPDATE);
    }
}

void grOnett_UpdateTraffic(Ground_GObj* gobj)
{
    Ground* gp = GET_GROUND(gobj);
    s8 right_to_left_car = gp->u.onettcar.right_to_left_car;
    Vec3 fighter_position = { 0.0f, 0.0f, 0.0f };
    f32 camera_x, camera_y, camera_z;
    PAD_STACK(32);

    if (!gp->u.onettcar.startup_pending) {
        HSD_JObj* active_car_joint;
        HSD_JObj* damage_effect_joint;
        HSD_GObj* fighter;

        Camera_800307D0(&camera_x, &camera_y, &camera_z);

        active_car_joint = gp->u.onettcar.car_joints[right_to_left_car];
        damage_effect_joint =
            gp->u.onettcar.damage_effect_joints[right_to_left_car];

        switch ((s8) gp->u.onettcar.right_to_left_state) {
        case OnettTrafficState_SelectCar: {
            s32 old_car = gp->u.onettcar.right_to_left_car;

            while (gp->u.onettcar.right_to_left_car == old_car ||
                   gp->u.onettcar.right_to_left_car ==
                       gp->u.onettcar.left_to_right_car)
            {
                gp->u.onettcar.right_to_left_car = HSD_Randi(4);
            }

            active_car_joint =
                gp->u.onettcar.car_joints[gp->u.onettcar.right_to_left_car];
            HSD_JObjSetTranslateX(active_car_joint, 726.0f);
            HSD_JObjSetTranslateZ(active_car_joint, 30.0f);
            HSD_JObjSetRotationY(active_car_joint->child, 0.0f);
            gp->u.onettcar.right_to_left_state = OnettTrafficState_Warning;
            gp->u.onettcar.right_to_left_timer =
                onett_params->traffic_warning_frames;
            Ground_801C5784(0);
            break;
        }
        case OnettTrafficState_Warning:
            grOnett_UpdateRightToLeftWarning(gp, right_to_left_car);
            break;
        case OnettTrafficState_Approach:
            if (gp->u.onettcar.right_to_left_timer != 0) {
                int fighter_count = 0;
                fighter = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];
                while (fighter != NULL) {
                    fighter = fighter->next;
                    fighter_count++;
                }
                gp->u.onettcar.right_to_left_timer -= 1;
                fighter = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];
                while (fighter != NULL) {
                    ftLib_80086644(fighter, &fighter_position);
                    if (fighter_position.y <= 1.0f) {
                        s32 rank = gm_8016C6C0(fighter);
                        if (fighter_count == 1 || rank == 0) {
                            gp->u.onettcar.right_to_left_timer = 0;
                            HSD_JObjClearFlagsAll(active_car_joint,
                                                  JOBJ_HIDDEN);
                            gp->u.onettcar.right_to_left_state =
                                OnettTrafficState_Driving;
                        } else if (fighter_count == rank + 1) {
                            gp->u.onettcar.right_to_left_timer =
                                onett_params->traffic_last_place_delay_frames;
                            gp->u.onettcar.right_to_left_state =
                                OnettTrafficState_RankDelay;
                        } else {
                            gp->u.onettcar.right_to_left_timer =
                                onett_params->traffic_rank_delay_frames;
                            gp->u.onettcar.right_to_left_state =
                                OnettTrafficState_RankDelay;
                        }
                        gp->u.onettcar.right_to_left_progress =
                            OnettTrafficProgress_None;
                    }
                    fighter = fighter->next;
                }
            } else {
                gp->u.onettcar.right_to_left_timer = 0;
                gp->u.onettcar.right_to_left_progress =
                    OnettTrafficProgress_None;
                HSD_JObjClearFlagsAll(active_car_joint, JOBJ_HIDDEN);
                gp->u.onettcar.right_to_left_state = OnettTrafficState_Driving;
                Ground_801C5784(1);
            }
            break;
        case OnettTrafficState_RankDelay: {
            s32 t = gp->u.onettcar.right_to_left_timer;
            if (t != 0) {
                gp->u.onettcar.right_to_left_timer = t - 1;
            } else {
                gp->u.onettcar.right_to_left_timer = 0;
                gp->u.onettcar.right_to_left_progress =
                    OnettTrafficProgress_None;
                HSD_JObjClearFlagsAll(active_car_joint, JOBJ_HIDDEN);
                gp->u.onettcar.right_to_left_state = OnettTrafficState_Driving;
                Ground_801C5784(1);
            }
            break;
        }
        case OnettTrafficState_Damaged:
            HSD_JObjAddRotationY(active_car_joint->child,
                                 0.017453292f *
                                     onett_params->damaged_car_spin_rate);
            grLib_801C9808(0x2C, 0, damage_effect_joint);
            HSD_JObjAddTranslationX(active_car_joint,
                                    gp->u.onettcar.right_to_left_speed);
            if (HSD_JObjGetTranslationX(active_car_joint) <= -642.0f) {
                HSD_JObjSetFlagsAll(active_car_joint, JOBJ_HIDDEN);
                gp->u.onettcar.right_to_left_state =
                    OnettTrafficState_SelectCar;
                Item_80268E5C(gp->u.onettcar.hazard_items[right_to_left_car],
                              0, ITEM_ANIM_UPDATE);
                return;
            }
            break;
        case OnettTrafficState_Driving:
        case OnettTrafficState_HitWithoutSpin:
            HSD_JObjAddTranslationX(active_car_joint,
                                    gp->u.onettcar.right_to_left_speed);
            switch (gp->u.onettcar.right_to_left_progress) {
            case OnettTrafficProgress_None:
                if (HSD_JObjGetTranslationX(active_car_joint) <= 580.0f) {
                    grOnett_PlayTrafficSfx(
                        gp, gp->u.onettcar.right_to_left_car, 5);
                    un_802FD604(onett_params->hazard_indicator_frames);
                    gp->u.onettcar.right_to_left_progress =
                        OnettTrafficProgress_IndicatorShown;
                }
                break;
            case OnettTrafficProgress_IndicatorShown:
                if (HSD_JObjGetTranslationX(active_car_joint) <=
                    camera_z + onett_params->hazard_indicator_camera_offset)
                {
                    fighter = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];
                    while (fighter != NULL) {
                        ftLib_80086644(fighter, &fighter_position);
                        if (fighter_position.y <= 1.0f) {
                            HSD_Randi(3);
                            grOnett_PlayPassingCarSfx(right_to_left_car);
                            gp->u.onettcar.right_to_left_progress =
                                OnettTrafficProgress_PassingSoundPlayed;
                            Ground_801C5784(2);
                            break;
                        }
                        fighter = fighter->next;
                    }
                }
                /* fallthrough */
            case OnettTrafficProgress_PassingSoundPlayed:
                if (HSD_JObjGetTranslationX(active_car_joint) <=
                    100.0f + camera_y)
                {
                    gp->u.onettcar.right_to_left_progress =
                        OnettTrafficProgress_Complete;
                }
                break;
            case OnettTrafficProgress_Complete:
                break;
            }
            if (HSD_JObjGetTranslationX(active_car_joint) <= -642.0f) {
                HSD_JObjSetFlagsAll(active_car_joint, JOBJ_HIDDEN);
                gp->u.onettcar.right_to_left_state =
                    OnettTrafficState_SelectCar;
                Item_80268E5C(gp->u.onettcar.hazard_items[right_to_left_car],
                              0, ITEM_ANIM_UPDATE);
                return;
            }
            break;
        }

        /* State machine B: left-to-right car */
        {
            s8 state = gp->u.onettcar.left_to_right_state;
            s8 car_index = gp->u.onettcar.left_to_right_car;
            active_car_joint = gp->u.onettcar.car_joints[car_index];

            switch (state) {
            case OnettTrafficState_SelectCar:
                grOnett_PrepareLeftToRightCar(gp);
                return;
            case OnettTrafficState_Warning: {
                s32 t = gp->u.onettcar.left_to_right_timer;
                if (t != 0) {
                    gp->u.onettcar.left_to_right_timer = t - 1;
                    return;
                }
                gp->u.onettcar.left_to_right_timer =
                    onett_params->traffic_approach_frames;
                gp->u.onettcar.left_to_right_speed =
                    onett_params->traffic_random_speed * HSD_Randf() +
                    onett_params->traffic_base_speed;
                gp->u.onettcar.left_to_right_state =
                    OnettTrafficState_Approach;
                return;
            }
            case OnettTrafficState_Approach: {
                s32 t = gp->u.onettcar.left_to_right_timer;
                if (t != 0) {
                    gp->u.onettcar.left_to_right_timer = t - 1;
                    return;
                }
                gp->u.onettcar.left_to_right_timer = 0;
                HSD_JObjClearFlagsAll(active_car_joint, JOBJ_HIDDEN);
                gp->u.onettcar.left_to_right_progress =
                    OnettTrafficProgress_None;
                gp->u.onettcar.left_to_right_state = OnettTrafficState_Driving;
                Item_80268E5C(gp->u.onettcar.hazard_items[car_index], 0,
                              ITEM_ANIM_UPDATE);
                return;
            }
            case OnettTrafficState_Driving:
                HSD_JObjAddTranslationX(active_car_joint,
                                        gp->u.onettcar.left_to_right_speed);
                switch (gp->u.onettcar.left_to_right_progress) {
                case OnettTrafficProgress_None:
                    if (HSD_JObjGetTranslationX(active_car_joint) >= -513.0f) {
                        grOnett_PlayTrafficSfx(
                            gp, gp->u.onettcar.left_to_right_car, 4);
                        gp->u.onettcar.left_to_right_progress =
                            OnettTrafficProgress_Complete;
                    }
                    break;
                case OnettTrafficProgress_Complete:
                    break;
                }
                if (HSD_JObjGetTranslationX(active_car_joint) >= 726.0f) {
                    HSD_JObjSetFlagsAll(active_car_joint, JOBJ_HIDDEN);
                    gp->u.onettcar.left_to_right_state =
                        OnettTrafficState_SelectCar;
                }
                break;
            }
        }
    }
}

void grOnett_801E502C(Ground_GObj* gobj) {}

void grOnett_OnTrafficCarHit(Item_GObj* igobj, Ground* gp)
{
    if ((int) gp->u.onettcar.right_to_left_state == OnettTrafficState_Damaged)
    {
        s32 s = HSD_Randi(2) != 0 ? 0x5F37C : 0x5F37D;
        Ground_801C53EC(s);
    } else if (HSD_Randi(2)) {
        u32 sfx;
        f32 rand = HSD_Randf();
        gp->u.onettcar.right_to_left_speed += 0.5f + rand;
        if (ABS(gp->u.onettcar.right_to_left_speed) < 0.5f) {
            gp->u.onettcar.right_to_left_speed = 0.5f;
        }
        gp->u.onettcar.right_to_left_state = OnettTrafficState_Damaged;
        gp->u.onettcar.right_to_left_progress = OnettTrafficProgress_Complete;
        gp->u.onettcar.right_to_left_timer = 1;
        if (HSD_Randi(2) != 0) {
            sfx = 0x5F37C;
        } else {
            sfx = 0x5F37D;
        }
        Ground_801C5544(gp, 5);
        Ground_801C53EC(sfx);
    } else {
        gp->u.onettcar.right_to_left_state = OnettTrafficState_HitWithoutSpin;
    }
}

void grOnett_PlayPassingCarSfx(s32 idx)
{
    u32 table[] = { 0x5F370, 0x5F370, 0x5F371, 0x5F372 };
    Ground_801C53EC(table[idx]);
}

static s32 grOnett_GetTrafficSfx(s32 (*table)[2], s32 car_index,
                                 s32 sound_type)
{
    return table[car_index][sound_type - 4];
}

void grOnett_PlayTrafficSfx(Ground* gp, s32 car_idx, s32 sound_type)
{
    s32 sfx_table[4][2] = {
        { 0x5F377, 0x5F376 },
        { 0x5F377, 0x5F376 },
        { 0x5F379, 0x5F378 },
        { 0x5F37B, 0x5F37A },
    };
    Ground_801C5440(gp, sound_type,
                    grOnett_GetTrafficSfx(sfx_table, car_idx, sound_type));
}

void grOnett_UpdateAwningPhysics(Ground_GObj* gobj)
{
    Ground* gp = gobj->user_data;
    for (int i = 0; i < ARRAY_SIZE(gp->u.onett.awnings); i++) {
        struct grOnett_AwningData* awning = &gp->u.onett.awnings[i];

        if (awning->contact_displacement < -onett_params->max_displacement) {
            awning->contact_displacement = -onett_params->max_displacement;
        }

        f32 target_displacement = awning->contact_displacement;
        f32 displacement_error = target_displacement - awning->displacement;
        f32 next_velocity = awning->velocity + onett_params->spring_constant;

        f32 normalized_error =
            ABS(displacement_error) / onett_params->max_displacement;

        if (displacement_error < 0.0) {
            next_velocity = -(onett_params->spring_force * normalized_error -
                              next_velocity);
        } else {
            next_velocity =
                onett_params->spring_force * normalized_error + next_velocity;
        }

        next_velocity -= onett_params->spring_constant;
        next_velocity *= 1.0f - onett_params->damping;

        if (next_velocity > onett_params->max_velocity) {
            next_velocity = onett_params->max_velocity;
        } else if (next_velocity < -onett_params->max_velocity) {
            next_velocity = -onett_params->max_velocity;
        }

        if ((ABS(awning->displacement - target_displacement) <
             onett_params->pos_threshold) &&
            (ABS(next_velocity) < onett_params->vel_threshold))
        {
            awning->displacement = target_displacement;
            awning->velocity = 0.0f;
        } else {
            awning->velocity = next_velocity;
            awning->displacement += next_velocity;
            f32 displacement_limit = onett_params->max_displacement;
            if (awning->displacement > displacement_limit) {
                awning->displacement = displacement_limit;
            } else if (awning->displacement < -displacement_limit) {
                awning->displacement = -displacement_limit;
            }
        }

        HSD_JObjSetTranslateY(
            awning->joint,
            awning->displacement +
                (awning->rest_y + awning->contact_displacement));

        if (awning->impact_effect_cooldown == 0 &&
            (awning->previous_contact_count != awning->contact_count ||
             awning->landed_this_frame != 0))
        {
            grLib_801C9808(0x7533, 0x1E, awning->joint);
            Ground_801C5440(gp, 0, 0x5F375);
            awning->impact_effect_cooldown = 0xA;
        }

        awning->contact_displacement = 0.0f;
        awning->previous_contact_count = awning->contact_count;
        awning->contact_count = 0;
        awning->landed_this_frame = 0;

        if (awning->impact_effect_cooldown != 0) {
            awning->impact_effect_cooldown--;
        }
    }
}

/// @brief Updates awning collision tracking data
/// @copydetails mpLib_JointCollisionCallback
void grOnett_OnAwningCollision(void* user_data, int joint_id, CollData* coll,
                               int coll_x50, mpLib_GroundEnum ground_kind,
                               float delta_y)
{
    Ground* gp = user_data;
    int temp = coll->x34_flags.b1234;
    int idx;

    if (temp != 1 && temp != 2 && temp != 3) {
        return;
    }

    idx = (joint_id == 0) ? 0 : 1;
    if (ground_kind == 1) {
        gp->u.onett.awnings[idx].landed_this_frame = 1;
        gp->u.onett.awnings[idx].velocity =
            onett_params->awning_collision_velocity;
    }

    gp->u.onett.awnings[idx].contact_displacement +=
        onett_params->awning_contact_displacement;
    gp->u.onett.awnings[idx].contact_count++;
}

void grOnett_UpdateAmbientEffect(Ground_GObj* gobj)
{
    Ground* gp = GET_GROUND(gobj);
    s32 remaining_frames = gp->u.onett.ambient_effect_timer--;
    if (remaining_frames < 0) {
        if (gp->u.onett.ambient_effect != NULL) {
            gp->u.onett.ambient_effect_timer =
                rand_range(onett_params->ambient_hidden_frames_max,
                           onett_params->ambient_hidden_frames_min);
            grLib_801C9834(gp->u.onett.ambient_effect);
            gp->u.onett.ambient_effect = NULL;
            return;
        }
        gp->u.onett.ambient_effect_timer =
            rand_range(onett_params->ambient_visible_frames_max,
                       onett_params->ambient_visible_frames_min);
        {
            HSD_JObj* jobj = Ground_801C3FA4(gobj, 6);
            if (jobj != NULL) {
                Vec3 pos;
                Vec3 rot = { 15.75f, 40.0f, 28.25f };
                lb_8000B1CC(jobj, &rot, &pos);
                gp->u.onett.ambient_effect =
                    grLib_801C96F8(0x7530, 0x1E, &pos);
            }
        }
    }
}

DynamicModelDesc* grOnett_GetHazardIndicatorModel(void)
{
    UnkArchiveStruct* archive = grDatFiles_801C6330(0x1);
    UnkStageDat* dat;
    HSD_ASSERT(1319, archive);
    dat = archive->unk4;
    if (dat != NULL) {
        return (DynamicModelDesc*) &dat->unk8[1];
    }
    return NULL;
}

lbColl_80008D30_arg1* grOnett_801E5760(enum_t arg)
{
    return false;
}

bool grOnett_801E5768(Vec3* a, int b, HSD_JObj* jobj)
{
    Vec3 vec;
    lb_8000B1CC(jobj, NULL, &vec);
    if (a->y > vec.y) {
        return true;
    } else {
        return false;
    }
}
