#include "grgarden.h"

#include <Runtime/platform.h>

#include "granime.h"
#include "grlib.h"
#include "grmaterial.h"
#include "ground.h"
#include "grzakogenerator.h"
#include "inlines.h"
#include "stage.h"
#include "types.h"
#include <melee/ft/ftdevice.h>
#include <melee/ft/ftlib.h>
#include <melee/gm/gmvs.h>
#include <melee/it/item.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lb_00F9.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/psstructs.h>
#include <sysdolphin/baselib/random.h>

struct grGarden_YakumonoParam {
    float water_surface_y;
    float water_current_x;
    int cranky_idle_random;
    int cranky_pose_random;
    int klaptrap_wait_base;
    int klaptrap_wait_random;
    float klaptrap_min_x;
    float klaptrap_max_x;
};

StageCallbacks grGd_StageCallbacks[] = {
    { grGarden_80202D60, grGarden_80202D8C, grGarden_80202D94,
      grGarden_80202D98, 0 },
    { grGarden_InitCranky, grGarden_80203090, grGarden_UpdateCranky,
      grGarden_802031A0, 0 }, // cranky kong
    { grGarden_80202DC8, grGarden_80202E40, grGarden_80202E48,
      grGarden_80202E68, 0xC0000000 }, // main stage
    { grGarden_InitKlaptrap, grGarden_80203248, grGarden_UpdateKlaptrap,
      grGarden_80203494, 0 }, // klaptrap
    { grGarden_80202E6C, grGarden_80202F34, grGarden_80202F3C,
      grGarden_80202F40, 0 }, // background
    { grGarden_80202F44, grGarden_80202F94, grGarden_80202F9C,
      grGarden_80202FA0, 0 }, // birds
    { grGarden_80202FA4, grGarden_80202FF4, grGarden_80202FFC,
      grGarden_80203000, 0 } // water
};

StageData grGd_StageData = {
    Gr_Kind_Garden,
    grGd_StageCallbacks,
    "/GrGd.dat",
    grGarden_80202B70,
    grGarden_80202B6C,
    grGarden_80202C24,
    grGarden_80202C4C,
    grGarden_80202C70,
    grGarden_802035C8,
    grGarden_802035D0,
    1,
    0,
    0,
};

static struct grGarden_YakumonoParam* yakumono_param;

void grGarden_80202B6C(int arg) {}

void grGarden_80202B70(void)
{
    yakumono_param = Ground_GetYakumonoParam();
    stage_info.unk8C.b4 = false;
    stage_info.unk8C.b5 = true;
    grGarden_80202C78(0);
    grGarden_80202C78(4);
    grGarden_80202C78(5);
    grGarden_80202C78(6);
    grGarden_80202C78(1);
    grGarden_80202C78(3);
    grGarden_80202C78(2);
    Ground_801C39C0();
    Ground_801C3BB4();
    if ((Stage_80225194() != 62) && gm_8016B238() == 0) {
        Ground_801C53EC(0x64194);
    }
}

void grGarden_80202C24(void)
{
    Ground_801C53EC(0x64193);
}

void grGarden_80202C4C(void)
{
    grZakoGenerator_801CAE04(NULL);
}

bool grGarden_80202C70(void)
{
    return false;
}

HSD_GObj* grGarden_80202C78(int gobj_id)
{
    HSD_GObj* gobj;
    StageCallbacks* callbacks = &grGd_StageCallbacks[gobj_id];

    gobj = Ground_GetStageGObj(gobj_id);

    if (gobj != NULL) {
        Ground_SetupStageCallbacks(gobj, callbacks);
    } else {
        OSReport("%s:%d: couldn t get gobj(id=%d)\n", __FILE__, 0xE3, gobj_id);
    }

    return gobj;
}

void grGarden_80202D60(Ground_GObj* gobj)
{
    Ground* gp = GET_GROUND(gobj);
    grAnime_801C8138(gobj, gp->map_id, 0);
}

bool grGarden_80202D8C(Ground_GObj* arg)
{
    return false;
}

void grGarden_80202D94(Ground_GObj* arg) {}

void grGarden_80202D98(Ground_GObj* arg) {}

void grGarden_80202D9C(Ground_GObj* arg)
{
    ftCo_800C06E8(arg, 4, grGarden_CheckWaterCollision);
}

void grGarden_80202DC8(Ground_GObj* gobj)
{
    Ground* gp = GET_GROUND(gobj);
    grAnime_801C8138(gobj, gp->map_id, 0);
    gp->x8_callback = NULL;
    gp->xC_callback = NULL;
    Ground_801C10B8(gobj, grGarden_80202D9C);
    gp->x11_flags.b012 = 1;
    gp->x10_flags.b5 = 1;
}

bool grGarden_80202E40(Ground_GObj* arg)
{
    return false;
}

void grGarden_80202E48(Ground_GObj* arg)
{
    lb_800115F4();
}

void grGarden_80202E68(Ground_GObj* arg) {}

void grGarden_80202E6C(Ground_GObj* gobj)
{
    Ground* gp = GET_GROUND(gobj);
    grAnime_801C8138(gobj, gp->map_id, 0);
    gp->x8_callback = NULL;
    gp->xC_callback = NULL;
    gp->x11_flags.b012 = 1;
    Ground_801C4E70(Ground_801C3FA4(gobj, 4), Ground_801C3FA4(gobj, 3),
                    Ground_801C3FA4(gobj, 7), Ground_801C3FA4(gobj, 6),
                    Ground_801C3FA4(gobj, 10), Ground_801C3FA4(gobj, 9));
}

bool grGarden_80202F34(Ground_GObj* arg)
{
    return false;
}

void grGarden_80202F3C(Ground_GObj* arg) {}

void grGarden_80202F40(Ground_GObj* arg) {}

void grGarden_80202F44(Ground_GObj* gobj)
{
    Ground* gp = GET_GROUND(gobj);
    grAnime_801C8138(gobj, gp->map_id, 0);
    gp->x8_callback = NULL;
    gp->xC_callback = NULL;
    gp->x11_flags.b012 = 1;
}

bool grGarden_80202F94(Ground_GObj* arg)
{
    return false;
}

void grGarden_80202F9C(Ground_GObj* arg) {}

void grGarden_80202FA0(Ground_GObj* arg) {}

void grGarden_80202FA4(Ground_GObj* gobj)
{
    Ground* gp = GET_GROUND(gobj);
    grAnime_801C8138(gobj, gp->map_id, 0);
    gp->x8_callback = NULL;
    gp->xC_callback = NULL;
    gp->x11_flags.b012 = 2;
}

bool grGarden_80202FF4(Ground_GObj* arg)
{
    return false;
}

void grGarden_80202FFC(Ground_GObj* arg) {}

void grGarden_80203000(Ground_GObj* arg) {}

void grGarden_InitCranky(Ground_GObj* gobj)
{
    int random_wait;
    Ground* gp = GET_GROUND(gobj);
    gp->u.garden_cranky.animation_state = 0;
    random_wait = yakumono_param->cranky_idle_random;
    random_wait = random_wait != 0 ? HSD_Randi(random_wait) : 0;
    gp->u.garden_cranky.wait_cycles = random_wait + 1;
    grAnime_801C8138(gobj, gp->map_id, gp->u.garden_cranky.animation_state);
    gp->x8_callback = NULL;
    gp->xC_callback = NULL;
    gp->x11_flags.b012 = 1;
}

bool grGarden_80203090(Ground_GObj* arg)
{
    return false;
}

void grGarden_UpdateCranky(Ground_GObj* gobj)
{
    int random_wait;
    Ground* gp = GET_GROUND(gobj);
    if (grAnime_801C83D0(gobj, 0, 7) || grAnime_801C84A4(gobj, 0, 7)) {
        if (gp->u.garden_cranky.wait_cycles == 0) {
            gp->u.garden_cranky.animation_state++;
            if (3 < gp->u.garden_cranky.animation_state) {
                gp->u.garden_cranky.animation_state = 0;
            }
            if (gp->u.garden_cranky.animation_state == 0) {
                if (yakumono_param->cranky_idle_random != 0) {
                    random_wait =
                        HSD_Randi(yakumono_param->cranky_idle_random);
                } else {
                    random_wait = 0;
                }
                gp->u.garden_cranky.wait_cycles = random_wait + 1;
            } else if (gp->u.garden_cranky.animation_state == 2) {
                if (yakumono_param->cranky_pose_random != 0) {
                    random_wait =
                        HSD_Randi(yakumono_param->cranky_pose_random);
                } else {
                    random_wait = 0;
                }
                gp->u.garden_cranky.wait_cycles = random_wait + 1;
            }
            grAnime_801C8138(gobj, gp->map_id,
                             gp->u.garden_cranky.animation_state);
        } else {
            gp->u.garden_cranky.wait_cycles--;
        }
    }
}

void grGarden_802031A0(Ground_GObj* arg) {}

void grGarden_InitKlaptrap(Ground_GObj* gobj)
{
    int random_wait;
    Ground* gp = GET_GROUND(gobj);
    gp->u.garden_klaptrap.article =
        grMaterial_801C8CFC(0, 0, gp, Ground_801C3FA4(gobj, 8),
                            grGarden_80203498, 0, 0);
    if (yakumono_param->klaptrap_wait_random != 0) {
        random_wait = HSD_Randi(yakumono_param->klaptrap_wait_random);
    } else {
        random_wait = 0;
    }
    gp->u.garden_klaptrap.timer =
        yakumono_param->klaptrap_wait_base + random_wait;
    gp->u.garden_klaptrap.state = 0;
    gp->x8_callback = NULL;
    gp->xC_callback = NULL;
    gp->x11_flags.b012 = 1;
}

bool grGarden_80203248(Ground_GObj* arg)
{
    return false;
}

void grGarden_UpdateKlaptrap(Ground_GObj* gobj)
{
    int random_value;
    int frame;
    float start_x;
    Ground* gp = GET_GROUND(gobj);
    HSD_JObj* jobj = Ground_801C3FA4(gobj, 0);

    switch (gp->u.garden_klaptrap.state) {
    case 0:
        if (gp->u.garden_klaptrap.timer == 0) {
            Item_80268E5C(gp->u.garden_klaptrap.article, 2, 2);
            grAnime_801C8138(gobj, gp->map_id, 0);
            float range = yakumono_param->klaptrap_max_x -
                          yakumono_param->klaptrap_min_x;
            start_x = yakumono_param->klaptrap_min_x;
            random_value = range < 0.0F ? -range : range;

            if (random_value != 0) {
                random_value = HSD_Randi(random_value);
            } else {
                random_value = 0;
            }
            start_x += random_value;
            HSD_JObjSetTranslateX(jobj, start_x);
            gp->u.garden_klaptrap.timer = 0;
            gp->u.garden_klaptrap.state = 1;
        } else {
            gp->u.garden_klaptrap.timer--;
        }
        break;
    case 1:
        frame = gp->u.garden_klaptrap.timer;
        if (frame == 38 || frame == 148) {
            Ground_801C53EC(410001);
        } else if (frame == 110 || frame == 210) {
            Ground_801C53EC(410002);
        }
        if (grAnime_801C83D0(gobj, 0, 7)) {
            Item_80268E5C(gp->u.garden_klaptrap.article, 0, 2);
            HSD_JObjRemoveAnimAll(jobj);
            if (yakumono_param->klaptrap_wait_random != 0) {
                random_value =
                    HSD_Randi(yakumono_param->klaptrap_wait_random);
            } else {
                random_value = 0;
            }
            gp->u.garden_klaptrap.timer =
                yakumono_param->klaptrap_wait_base + random_value;
            gp->u.garden_klaptrap.state = 0;
        }
        gp->u.garden_klaptrap.timer++;
        break;
    }
}

void grGarden_80203494(Ground_GObj* arg) {}

void grGarden_80203498(Item_GObj* item_gobj, Ground* gp) {}

bool grGarden_CheckWaterCollision(Ground_GObj* stage, HSD_GObj* player,
                                  Vec3* current_velocity)
{
    float surface_y;
    Vec3 current;
    Vec3 previous;
    (void) stage;

    surface_y = yakumono_param->water_surface_y;
    ftLib_80086644(player, &current);
    ftLib_80086684(player, &previous);
    if (current.y < surface_y) {
        if (previous.y > surface_y) {
            float splash_scale = ftLib_80086B80(player) / 10.0f;
            current.y = -30.0f;
            grGarden_SpawnWaterSplash(splash_scale, &current);
            Ground_801C53EC(410000);
        }
        current_velocity->x = yakumono_param->water_current_x;
        current_velocity->y = 0.0f;
        current_velocity->z = 0.0f;
        return true;
    }
    return false;
}

void grGarden_SpawnWaterSplash(float scale, Vec3* position)
{
    HSD_Generator* gen = grLib_801C96F8(0x753A, 0x1E, position);
    if (gen != NULL) {
        HSD_psAppSRT* srt = gen->appsrt;
        srt->scale.x *= scale;
        srt->scale.y *= scale;
        srt->scale.z *= scale;
    }
}

lbColl_80008D30_arg1* grGarden_802035C8(enum_t arg)
{
    return NULL;
}

bool grGarden_802035D0(Vec3* a, int b, HSD_JObj* jobj)
{
    Vec3 vec;
    lb_8000B1CC(jobj, NULL, &vec);
    if (a->y > vec.y) {
        return true;
    } else {
        return false;
    }
}

f32 grGarden_80203624(void)
{
    return -20.0f;
}
