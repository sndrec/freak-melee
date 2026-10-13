#include "grvenom.h"
#include "stage_hazard_state_internal.h"

#include <Runtime/platform.h>

#include "grcorneria.h"
#include "grdisplay.h"
#include "grmaterial.h"
#include "grzakogenerator.h"
#include "inlines.h"
#include "stage.h"
#include "types.h"
#include <melee/if/ifcoget.h>
#include <melee/if/ifstatus.h>
#include <melee/it/kinds/itarwinglaser.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lb_00F9.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/mp/mplib.h>
#include <melee/pl/player.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/random.h>
#include <sysdolphin/baselib/sislib.h>

struct grVenom_YakumonoParam {
    f32 x0;
    f32 x4;
    f32 x8;
    f32 xC;
    f32 x10;
    char x14[0x2C - 0x14];
    f32 x2C;
    char x30[0x34 - 0x30];
    f32 x34;
    union ColorOverlay_x8_t* x38;
};

static GrJoint grVe_Joints[] = {
    { 0, 5, 0 }, { 1, 5, 0 }, { 2, 9, 0 }, { 4, 10, 0 }, { 3, 13, 0 },
};





StageCallbacks grVe_StageCallbacks[16] = {
    {
        grVenom_80203F98,
        grVenom_80203FC4,
        grVenom_80203FCC,
        grVenom_80203FD0,
        0,
    },
    {
        grVenom_80206B94,
        grVenom_80206BBC,
        grVenom_80206BC4,
        grVenom_80206BEC,
        0x20000000,
    },
    {
        grVenom_80204F20,
        grVenom_802052D8,
        grVenom_802053B0,
        grVenom_802056AC,
        0,
    },
    {
        grVenom_80205AD4,
        grVenom_80205DF0,
        grVenom_80205F30,
        grVenom_80206870,
        0,
    },
    {
        grVenom_80203FD4,
        grVenom_802040A4,
        grVenom_802040AC,
        grVenom_802040B0,
        0,
    },
    {
        grVenom_802040F0,
        grVenom_8020427C,
        grVenom_80204284,
        grVenom_80204424,
        0xC0000000,
    },
    {
        grVenom_80206874,
        grVenom_80206B68,
        grVenom_80206B70,
        grVenom_80206B90,
        0,
    },
    {
        grVenom_80204428,
        grVenom_80204544,
        grVenom_8020454C,
        grVenom_80204B84,
        0,
    },
    {
        grVenom_80204CE0,
        grVenom_80204CE4,
        grVenom_80204CEC,
        grVenom_80204DB0,
        0,
    },
    {
        grVenom_80204DD4,
        grVenom_80204EF4,
        grVenom_80204EFC,
        grVenom_80204F1C,
        0,
    },
    {
        grVenom_802056B0,
        grVenom_80205750,
        grVenom_80205758,
        grVenom_80205AD0,
        0,
    },
    {
        grVenom_802056B0,
        grVenom_80205750,
        grVenom_80205758,
        grVenom_80205AD0,
        0,
    },
    {
        grVenom_802056B0,
        grVenom_80205750,
        grVenom_80205758,
        grVenom_80205AD0,
        0,
    },
    {
        grVenom_802056B0,
        grVenom_80205750,
        grVenom_80205758,
        grVenom_80205AD0,
        0,
    },
    {
        grVenom_802056B0,
        grVenom_80205750,
        grVenom_80205758,
        grVenom_80205AD0,
        0,
    },
    {
        grVenom_802056B0,
        grVenom_80205750,
        grVenom_80205758,
        grVenom_80205AD0,
        0,
    },
};

StageData grVe_StageData = {
    Gr_Kind_Venom,
    grVe_StageCallbacks,
    "/GrVe",
    grVenom_80203B18,
    grVenom_80203B14,
    grVenom_80203DD0,
    grVenom_80203E80,
    grVenom_80203EA4,
    grVenom_80206D74,
    grVenom_80206D7C,
    1,
    grVe_Joints,
    ARRAY_SIZE(grVe_Joints),
    .on_event = grCorneria_ConversationEvent,
};







typedef struct grVe_Lighting {
    char pad[0xE0];
    u8 xE0;
} grVe_Lighting;

typedef struct grVe_GroundData {
    char pad[0x2C];
    grVe_Lighting* x2C;
} grVe_GroundData;

/// grVenom_8020362C

static inline s32 grVe_RandRange(f32 fmin, f32 fmax)
{
    s32 imin = fmin;
    s32 imax = fmax;
    if (imax > imin) {
        s32 range = imax - imin;
        imax = imin + (range != 0 ? HSD_Randi(range) : 0);
    } else if (imax < imin) {
        s32 range = imin - imax;
        imax += (range != 0 ? HSD_Randi(range) : 0);
    }
    return imax;
}

void grVenom_8020362C(void)
{
    PAD_STACK(0x20);

    if (!venom_state.multi_arwing_mode) {
        int region_mask;
        int new_type;
        int new_group;
        Ground* background;

        if (venom_state.arwings[0] != NULL) {
            venom_state.spawn_timer =
                grVe_RandRange(venom_state.parameters->x8, venom_state.parameters->xC);
            return;
        }
        if (--venom_state.spawn_timer > 0) {
            return;
        }

        background = Ground_GetMapGObj(7)->user_data;
        region_mask = 0;
        if (background->u.venom2.xE0_state.b0 ||
            background->u.venom2.xE0_state.b1 ||
            background->u.venom2.xE0_state.b2)
        {
            region_mask |= 1;
        }
        if (background->u.venom2.xE0_state.b3 ||
            background->u.venom2.xE0_state.b4 ||
            background->u.venom2.xE0_state.b5)
        {
            region_mask |= 2;
        }
        if (region_mask == 1 || region_mask == 3) {
            return;
        }

        new_type = venom_state.types[0];
        while (new_type == venom_state.types[0]) {
            new_type = HSD_Randi(region_mask == 2 ? 4 : 0xB) + 1;
        }
        if (venom_state.groups[0] == 4) {
            new_group = 1;
        } else {
            new_group = HSD_Randf() > venom_state.parameters->x10 ? 1 : 4;
        }

        venom_state.current_slot = 0;
        venom_state.types[0] = new_type;
        venom_state.groups[0] = new_group;
        venom_state.arwings[0] = grVenom_80203EAC(2);
        return;
    }

    {
        int free_slot = 0;
        int other_slot;
        bool has_active_near_arwing = false;

        while (free_slot < ARRAY_SIZE(venom_state.arwings) &&
               venom_state.arwings[free_slot] != NULL)
        {
            free_slot++;
        }
        if (free_slot == ARRAY_SIZE(venom_state.arwings)) {
            venom_state.spawn_timer = 10;
            return;
        }
        if (--venom_state.spawn_timer > 0) {
            return;
        }

        for (other_slot = 0; other_slot < ARRAY_SIZE(venom_state.arwings);
             other_slot++)
        {
            int type = venom_state.types[other_slot];
            if (other_slot != free_slot &&
                venom_state.arwings[other_slot] != NULL && type >= 1 && type <= 7)
            {
                has_active_near_arwing = true;
            }
        }

        if (!has_active_near_arwing) {
            int new_type = venom_state.types[free_slot];
            while (new_type == venom_state.types[0] ||
                   new_type == venom_state.types[1] ||
                   new_type == venom_state.types[2])
            {
                new_type = HSD_Randi(0xB) + 1;
            }
            venom_state.current_slot = free_slot;
            venom_state.types[free_slot] = new_type;
            venom_state.groups[free_slot] = free_slot + 1;
            venom_state.arwings[free_slot] = grVenom_80203EAC(2);
        } else {
            Ground* background = Ground_GetMapGObj(7)->user_data;
            bool first_region_active = background->u.venom2.xE0_state.b0 ||
                                       background->u.venom2.xE0_state.b1 ||
                                       background->u.venom2.xE0_state.b2;
            bool second_region_active = background->u.venom2.xE0_state.b3 ||
                                        background->u.venom2.xE0_state.b4 ||
                                        background->u.venom2.xE0_state.b5;

            if (!first_region_active && !second_region_active) {
                int new_type = venom_state.types[free_slot];
                while (new_type == venom_state.types[0] ||
                       new_type == venom_state.types[1] ||
                       new_type == venom_state.types[2])
                {
                    new_type = HSD_Randi(4) + 8;
                }
                venom_state.current_slot = free_slot;
                venom_state.types[free_slot] = new_type;
                venom_state.groups[free_slot] = free_slot + 1;
                venom_state.arwings[free_slot] = grVenom_80203EAC(2);
            }
        }
    }
}
void grVenom_80203B14(int arg) {}

static void grVe_ResetArwingSpawnTimer(void)
{
    venom_state.spawn_timer =
        grVe_RandRange(venom_state.parameters->x0, venom_state.parameters->x4);
}

/// Stage initialization function for Venom
void grVenom_80203B18(void)
{
    PAD_STACK(4);
    {
        Vec3 position;
        Ground_GObj* gobj;
        Ground* platform;
        bool multi_arwing_mode;

        venom_state.parameters = Ground_GetYakumonoParam();
        grVenom_80203EAC(4);
        stage_info.unk8C.b4 = false;
        stage_info.unk8C.b5 = true;
        gobj = grVenom_80203EAC(0);
        {
            int i;
            for (i = 0; i < 3; i++) {
                venom_state.arwings[i] = NULL;
                venom_state.types[i] = 0;
                venom_state.groups[i] = 0;
            }
        }
        multi_arwing_mode = Stage_80225194() == 0xE9;
        venom_state.multi_arwing_mode = multi_arwing_mode;
        if (multi_arwing_mode) {
            venom_state.spawn_timer = 0x78;
        } else {
            grVe_ResetArwingSpawnTimer();
        }
        {
            Ground_GObj* platform_gobj = grVenom_80203EAC(5);
            platform = GET_GROUND(platform_gobj);
            platform->u.venom_platform.target_gobj = gobj;
            grVenom_80203EAC(9);
            platform_gobj = grVenom_80203EAC(7);
            grAnime_801C8138(platform_gobj, 7, 0);
        }
        Ground_801C39C0();
        Ground_801C3BB4();
        {
            float zero;
            position.x = -1.0F;
            position.y = 1.0F;
            position.z = -1.0F;
            zero = 0.0F;
            lb_80011A50(&position, -1, 1.0F, zero, 1.0471976F, -100000.0F,
                        10000.0F, zero, -100000.0F);
        }
        {
            position.x = 1.0F;
            position.y = 1.0F;
            position.z = -1.0F;
            lb_80011A50(&position, -1, 1.0F, 0.0F, 1.0471976F, 0.0F, 10000.0F,
                        100000.0F, -100000.0F);
        }
        mpLib_80057BC0(3);
        mpLib_80057BC0(4);
        { /// @todo shared loop with #grVenom_8020454C
            HSD_GObj* gobj = Ground_801C498C();
            if (gobj != NULL) {
                HSD_LObj* lobj;
                if ((lobj = gobj->hsd_obj) != NULL) {
                    while (lobj != NULL) {
                        if (lobj->aobj != NULL) {
                            HSD_AObjSetFlags(lobj->aobj, AOBJ_LOOP);
                        }
                        if (lobj->position != NULL) {
                            HSD_ForeachAnim(lobj->position, WOBJ_TYPE,
                                            ALL_TYPE_MASK, HSD_AObjSetFlags,
                                            AOBJ_ARG_AU, AOBJ_LOOP);
                        }
                        if (lobj->interest != NULL) {
                            HSD_ForeachAnim(lobj->interest, WOBJ_TYPE,
                                            ALL_TYPE_MASK, HSD_AObjSetFlags,
                                            AOBJ_ARG_AU, AOBJ_LOOP);
                        }
                        lobj = HSD_LObjGetNext(lobj);
                    }
                }
            }
        }
    }
}

void grVenom_80203DD0(void)
{
    HSD_GObj* gobj;
    HSD_LObj* lobj;

    gobj = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_LIGHT];
    while (gobj != NULL) {
        if (HSD_GObjGetClassifier(gobj) == 0xC) {
            lobj = GET_LOBJ(gobj);
            while (lobj != NULL) {
                HSD_ForeachAnim(lobj, LOBJ_TYPE, ALL_TYPE_MASK,
                                HSD_AObjSetFlags, AOBJ_ARG_AU, AOBJ_LOOP);
                lobj = HSD_LObjGetNext(lobj);
            }
            return;
        }
        gobj = HSD_GObjGetNext(gobj);
    }
}

void grVenom_80203E80(void)
{
    grZakoGenerator_801CAE04(NULL);
}

bool grVenom_80203EA4(void)
{
    return false;
}

/// grVenom_80203EAC

Ground_GObj* grVenom_80203EAC(int gobj_id)
{
    Ground_GObj* gobj;
    Ground* gp;
    StageCallbacks* callbacks = &grVe_StageCallbacks[gobj_id];

    gobj = Ground_GetStageGObj(gobj_id);

    if (gobj != NULL) {
        gp = gobj->user_data;
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
        OSReport("%s:%d: couldn t get gobj(id=%d)\n", "grvenom.c", 0x23B,
                 gobj_id);
    }

    return gobj;
}

static int grVe_ArwingMapAnimationIds[12] = {
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
};

static const Vec3 grVe_ArwingPositionOffsets[12] = { 0 };

static int grVe_GroupLinkedMapGObjIds[5] = { 3, 3, 3, 3, 6 };

void grVenom_80203F98(Ground_GObj* gobj)
{
    Ground* gp = GET_GROUND(gobj);
    grAnime_801C8138(gobj, gp->map_id, 0);
}

bool grVenom_80203FC4(Ground_GObj* arg)
{
    return false;
}

void grVenom_80203FCC(Ground_GObj* arg) {}

void grVenom_80203FD0(Ground_GObj* arg) {}

void grVenom_80203FD4(Ground_GObj* gobj)
{
    HSD_JObj* jobj1;
    HSD_JObj* jobj4;
    HSD_JObj* jobj3;
    HSD_JObj* jobj2;
    HSD_JObj* jobj5;
    HSD_JObj* jobj6;
    Ground* gp = gobj->user_data;
    PAD_STACK(8);

    grAnime_801C8138(gobj, gp->map_id, 0);
    gp->x11_flags.b012 = 2;
    jobj6 = Ground_801C3FA4(gobj, 6);
    jobj5 = Ground_801C3FA4(gobj, 5);
    jobj2 = Ground_801C3FA4(gobj, 2);
    jobj3 = Ground_801C3FA4(gobj, 3);
    jobj4 = Ground_801C3FA4(gobj, 4);
    jobj1 = Ground_801C3FA4(gobj, 1);
    Ground_801C4E70(jobj1, jobj4, jobj3, jobj2, jobj5, jobj6);
    gp = gobj->user_data;
    gp->x10_flags.b2 = 0;
}

bool grVenom_802040A4(Ground_GObj* arg)
{
    return false;
}

void grVenom_802040AC(Ground_GObj* arg) {}

void grVenom_802040B0(Ground_GObj* arg) {}

void fn_802040B4(Ground_GObj* gobj)
{
    Ground* gp = GET_GROUND(gobj);
    if (Stage_80225194() == 0xE4) {
        gp->u.venom_platform.smash_taunt_timer = 1;
    }
}

/// grVenom_802040F0

void grVenom_802040F0(Ground_GObj* gobj)
{
    Vec3 pos1;
    Vec3 pos2;
    HSD_JObj* temp;
    Ground* new_var;
    Ground* gp = GET_GROUND(gobj);

    Ground_801C2ED0(gobj->hsd_obj, gp->map_id);
    grAnime_801C7FF8(gobj, 0, 7, 0, 0.0F, 1.0F);
    grAnime_801C8098(gobj, 0, 7, 1, 0.0F, 1.0F);
    grAnime_801C7FF8(gobj, 0xB, 7, 2, 0.0F, 1.0F);
    grAnime_801C7FF8(gobj, 0xD, 7, 2, 0.0F, 1.0F);
    grAnime_801C7FF8(gobj, 0xF, 7, 2, 0.0F, 1.0F);
    new_var = gp;
    grAnime_801C7FF8(gobj, 4, 7, 3, 0.0F, 1.0F);
    grAnime_801C7FF8(gobj, 7, 7, 3, 0.0F, 1.0F);
    Ground_801C10B8(gobj, (HSD_GObjEvent) fn_802040B4);
    gp->u.venom_platform.smash_taunt_timer = -1;
    gp->u.venom_platform.upper_jobj = Ground_801C3FA4(gobj, 2);
    gp->u.venom_platform.lower_jobj = Ground_801C3FA4(gobj, 3);
    lb_8000B1CC(gp->u.venom_platform.upper_jobj, NULL, &pos1);
    lb_8000B1CC(gp->u.venom_platform.lower_jobj, NULL, &pos2);
    if (pos2.y > pos1.y) {
        temp = gp->u.venom_platform.upper_jobj;
        gp->u.venom_platform.upper_jobj = new_var->u.venom_platform.lower_jobj;
        gp->u.venom_platform.lower_jobj = temp;
    }
    gp->x10_flags.b5 = 1;
}

bool grVenom_8020427C(Ground_GObj* arg)
{
    return false;
}

void grVenom_80204284(Ground_GObj* gobj)
{
    Ground* gp;
    Ground* tmp_gp;
    HSD_JObj* src_jobj;
    HSD_JObj* dst_jobj;
    u32 pad2;
    Ground_GObj* other_gobj;
    Vec3 pos;
    s32 timer;
    u32 pad;

    tmp_gp = GET_GROUND(gobj);
    gp = tmp_gp;
    src_jobj = GET_JOBJ(gobj);
    dst_jobj = gp->u.venom_platform.target_gobj->hsd_obj;

    HSD_JObjGetTranslation(src_jobj, &pos);
    HSD_JObjSetTranslate(dst_jobj, &pos);

    Ground_801C39C0();
    Ground_801C3BB4();

    timer = gp->u.venom_platform.smash_taunt_timer;
    if (timer > 0) {
        gp->u.venom_platform.smash_taunt_timer = timer + 1;
        if (gp->u.venom_platform.smash_taunt_timer >= 0x3C) {
            if (gp->u.venom_platform.smash_taunt_timer == 0x3C) {
                ifStatus_802F6898();
                un_802FF570();
                other_gobj = grVenom_80203EAC(1);
                if (other_gobj != NULL) {
                    Ground* other_gp = GET_GROUND(other_gobj);
                    grCorneria_801E25C4(other_gobj, &other_gp->u.smashtaunt, 4,
                                        6, 0x6B6CC);
                }
            } else {
                ifStatus_802F6898();
                un_802FF570();
                if (Ground_GetMapGObj(1) == NULL) {
                    ifStatus_802F68F0();
                    un_802FF620();
                    gp->u.venom_platform.smash_taunt_timer = -1;
                }
            }
        }
    }

    lb_800115F4();
    grVenom_8020362C();
    Ground_801C2FE0(gobj);
}

void grVenom_80204424(Ground_GObj* arg) {}

void grVenom_80204428(Ground_GObj* gobj)
{
    Ground* gp = GET_GROUND(gobj);
    PAD_STACK(8);

    gp->u.venom2.xC4 = Ground_801C3FA4(gobj, 4);
    gp->u.venom2.xC8 = Ground_801C3FA4(gobj, 0xE);
    gp->u.venom2.xCC = Ground_801C3FA4(gobj, 0x10);
    gp->u.venom2.xD0 = Ground_801C3FA4(gobj, 0x11);
    gp->u.venom2.xD4 = Ground_801C3FA4(gobj, 0x12);
    gp->u.venom2.xD8 = Ground_801C3FA4(gobj, 0x13);
    gp->u.venom2.xDC = Ground_801C3FA4(gobj, 0x15);

    gp->u.venom2.xE0_state.b0 = 0;
    gp->u.venom2.xE0_state.b1 = 0;
    gp->u.venom2.xE0_state.b2 = 0;
    gp->u.venom2.xE0_state.b3 = 0;
    gp->u.venom2.xE0_state.b4 = 0;
    gp->u.venom2.xE0_state.b5 = 0;
    gp->u.venom2.xE0_state.b6 = 0;

    gp->u.venom2.previous_frame = 0.0F;

    Ground_801C5440(gp, 0, 0x6B6C3);
    mpLib_80057BC0(2);
}

bool grVenom_80204544(Ground_GObj* arg)
{
    return false;
}

void grVenom_8020454C(Ground_GObj* gobj)
{
    PAD_STACK(4);
    {
        Vec3 position;
        bool visible;
        Ground* gp;
        gp = gobj->user_data;
        HSD_JObj* parts[] = {
            gp->u.venom2.xC4, gp->u.venom2.xC8, gp->u.venom2.xCC,
            gp->u.venom2.xD0, gp->u.venom2.xD4, gp->u.venom2.xD8,
            gp->u.venom2.xDC,
        };
        float lo = 1000.0F;
        float hi = -60000.0F;

        for (int i = 0; i < ARRAY_SIZE(parts); ++i) {
            HSD_JObj* part = parts[i];
            if (part != NULL) {
                lb_8000B1CC(part, NULL, &position);
                visible = true;
                if (!(position.z < lo) && !(position.z > hi)) {
                    visible = false;
                }
                if (HSD_JObjGetFlags(part) & JOBJ_HIDDEN) {
                    if (visible) {
                        HSD_JObjClearFlagsAll(part, JOBJ_HIDDEN);
                    }
                } else if (!visible) {
                    HSD_JObjSetFlagsAll(part, JOBJ_HIDDEN);
                }
            }
        }

        if (grAnime_801C84A4(gobj, 0, 7)) {
            Ground_GObj* lgobj = Ground_801C498C();
            if (lgobj != NULL) {
                HSD_LObj* lobj;
                if ((lobj = lgobj->hsd_obj) != NULL) {
                    while (lobj != NULL) {
                        if (lobj->aobj != NULL) {
                            HSD_AObjReqAnim(lobj->aobj, 0.0F);
                        }
                        if (lobj->position != NULL) {
                            HSD_ForeachAnim(lobj->position, WOBJ_TYPE,
                                            ALL_TYPE_MASK, HSD_AObjReqAnim,
                                            AOBJ_ARG_AF, 0.0);
                        }
                        if (lobj->interest != NULL) {
                            HSD_ForeachAnim(lobj->interest, WOBJ_TYPE,
                                            ALL_TYPE_MASK, HSD_AObjReqAnim,
                                            AOBJ_ARG_AF, 0.0);
                        }
                        lobj = HSD_LObjGetNext(lobj);
                    }
                }
            }
        }

        {
            HSD_AObj* a = grAnime_801C8318(gobj, 0, 7);
            if (a != NULL) {
                float frame = HSD_AObjGetCurrFrame(a);
                if ((gp->u.venom2.previous_frame < 1033.0F && 1033.0F <= frame) ||
                    (gp->u.venom2.previous_frame < 1086.0F && 1086.0F <= frame) ||
                    (gp->u.venom2.previous_frame < 1185.0F && 1185.0F <= frame) ||
                    (gp->u.venom2.previous_frame < 1238.0F && 1238.0F <= frame))
                {
                    Ground_801C53EC(0x6B6C5);
                }
                if ((gp->u.venom2.previous_frame < 1063.0F && 1063.0F <= frame) ||
                    (gp->u.venom2.previous_frame < 1116.0F && 1116.0F <= frame) ||
                    (gp->u.venom2.previous_frame < 1215.0F && 1215.0F <= frame) ||
                    (gp->u.venom2.previous_frame < 1268.0F && 1268.0F <= frame))
                {
                    Ground_801C53EC(0x6B6C6);
                }
                if ((gp->u.venom2.previous_frame < 2460.0F && 2460.0F <= frame) ||
                    (gp->u.venom2.previous_frame < 6440.0F && 6440.0F <= frame) ||
                    (gp->u.venom2.previous_frame < 9330.0F && 9330.0F <= frame))
                {
                    gp->u.venom2.xE0_state.b6 = true;
                }
                if ((gp->u.venom2.previous_frame < 2460.0F && 2460.0F <= frame) ||
                    (gp->u.venom2.previous_frame < 2747.0F && 2747.0F <= frame) ||
                    (gp->u.venom2.previous_frame < 3030.0F && 3030.0F <= frame) ||
                    (gp->u.venom2.previous_frame < 3393.0F && 3393.0F <= frame) ||
                    (gp->u.venom2.previous_frame < 3727.0F && 3727.0F <= frame) ||
                    (gp->u.venom2.previous_frame < 6440.0F && 6440.0F <= frame) ||
                    (gp->u.venom2.previous_frame < 6880.0F && 6880.0F <= frame) ||
                    (gp->u.venom2.previous_frame < 9330.0F && 9330.0F <= frame) ||
                    (gp->u.venom2.previous_frame < 9600.0F && 9600.0F <= frame) ||
                    (gp->u.venom2.previous_frame < 9835.0F && 9835.0F <= frame))
                {
                    Ground_801C53EC(0x6B6C7);
                }
                if ((gp->u.venom2.previous_frame < 4070.0F && 4070.0F <= frame) ||
                    (gp->u.venom2.previous_frame < 7220.0F && 7220.0F <= frame) ||
                    (gp->u.venom2.previous_frame > frame))
                {
                    gp->u.venom2.xE0_state.b6 = false;
                }
                if (gp->u.venom2.previous_frame < 4200 && 4200 <= frame) {
                    gp->u.venom2.xE0_state.b0 = true;
                }
                if (gp->u.venom2.previous_frame < 3800 && 3800 <= frame) {
                    gp->u.venom2.xE0_state.b1 = true;
                }
                if (gp->u.venom2.previous_frame < 4400 && 4400 <= frame) {
                    Ground_801C5440(gp, 0, 0x6B6C4);
                    gp->u.venom2.xE0_state.b2 = true;
                    mpJointListAdd(2);
                }
                if (gp->u.venom2.previous_frame < 6100 && 6100 <= frame) {
                    Ground_801C5440(gp, 0, 0x6B6C3);
                    gp->u.venom2.xE0_state.b0 = false;
                    gp->u.venom2.xE0_state.b1 = false;
                    gp->u.venom2.xE0_state.b2 = false;
                    mpLib_80057BC0(2);
                }
                if (gp->u.venom2.previous_frame < 7210 && 7210 <= frame) {
                    gp->u.venom2.xE0_state.b3 = true;
                }
                if (gp->u.venom2.previous_frame < 6810 && 6810 <= frame) {
                    gp->u.venom2.xE0_state.b4 = true;
                }
                if (gp->u.venom2.previous_frame < 7410 && 7410 <= frame) {
                    gp->u.venom2.xE0_state.b5 = true;
                }
                if (gp->u.venom2.previous_frame < 9200 && 9200 <= frame) {
                    gp->u.venom2.xE0_state.b3 = false;
                    gp->u.venom2.xE0_state.b4 = false;
                    gp->u.venom2.xE0_state.b5 = false;
                }
                gp->u.venom2.previous_frame = frame;
            }
        }

        lbAudioAx_80024D78(!gp->u.venom2.xE0_state.b2 ? 1 : 2);
        grVenom_80204B88(gobj);
    }
}

void grVenom_80204B84(Ground_GObj* arg) {}

/// grVenom_80204B88
/// Updates Venom stage lighting colors based on environment state.

static const GXColor grVe_804DB7D8 = { 0, 0, 0, 0 };
static const GXColor grVe_804DB7DC = { 190, 10, 0, 0 };
static const GXColor grVe_804DB7E0 = { 110, 60, 25, 0 };
static const GXColor grVe_804DB7E4 = { 110, 90, 0, 0 };

void grVenom_80204B88(Ground_GObj* gobj)
{
    Ground* gp = GET_GROUND(gobj);
    u8 env_flags;
    GXColor color_set1;
    GXColor color_set2;
    GXColor color_neutral1;
    GXColor color_neutral2;

    env_flags = *(u8*) &gp->u.venom2.xE0_state.xE0_state_pad;

    if ((env_flags >> 5) & 1) {
        if (gp->u.venom2.xE0_state.xE0_state_pad.state != 1) {
            color_set1 = grVe_804DB7D8;
            Ground_801C052C(&color_set1);
            Ground_801C05D4(&color_set1);
            Ground_801C0544(&color_set1);
            Ground_801C058C(&color_set1);
            Ground_801C05EC(&color_set1);
            Ground_801C05A4(&color_set1);
            Ground_801C055C(&color_set1);
            Ground_801C05BC(&color_set1);
            Ground_801C0574(&color_set1);
            gp->u.venom2.xE0_state.xE0_state_pad.state = 1;
        }
    } else if ((env_flags >> 1) & 1) {
        if (gp->u.venom2.xE0_state.xE0_state_pad.state != 2) {
            color_set2 = grVe_804DB7DC;
            Ground_801C05BC(&color_set2);
            gp->u.venom2.xE0_state.xE0_state_pad.state = 2;
        }
    } else {
        if (gp->u.venom2.xE0_state.xE0_state_pad.state != 0) {
            color_neutral1 = grVe_804DB7E0;
            color_neutral2 = grVe_804DB7E4;
            Ground_801C052C(&color_neutral1);
            Ground_801C05D4(&color_neutral1);
            Ground_801C0544(&color_neutral1);
            Ground_801C058C(&color_neutral1);
            Ground_801C05EC(&color_neutral1);
            Ground_801C05A4(&color_neutral1);
            Ground_801C055C(&color_neutral2);
            Ground_801C05BC(&color_neutral2);
            Ground_801C0574(&color_neutral2);
            gp->u.venom2.xE0_state.xE0_state_pad.state = 0;
        }
    }
}

void grVenom_80204CE0(Ground_GObj* arg) {}

bool grVenom_80204CE4(Ground_GObj* arg)
{
    return false;
}

void grVenom_80204CEC(Ground_GObj* gobj)
{
    Ground* gp = GET_GROUND(gobj);
    Ground_UpdateStarFoxSequence(gobj, gp, 1, grVenom_80203EAC);
}

void grVenom_80204DB0(Ground_GObj* gobj)
{
    ifStatus_802F68F0();
    un_802FF620();
}

/// grVenom_80204DD4
/// Unit: main/melee/gr/grvenom

void grVenom_80204DD4(Ground_GObj* gobj)
{
    Ground* gp = gobj->user_data;
    HSD_JObj* jobj = gobj->hsd_obj;
    PAD_STACK(8);

    Ground_801C2ED0(jobj, gp->map_id);
    HSD_JObjSetScaleX(jobj, 1.0F);
    HSD_JObjSetScaleY(jobj, 1.0F);
}

bool grVenom_80204EF4(Ground_GObj* arg)
{
    return false;
}

void grVenom_80204EFC(Ground_GObj* arg)
{
    Ground_801C2FE0(arg);
}

void grVenom_80204F1C(Ground_GObj* arg) {}

void grVenom_80204F20(Ground_GObj* arg0)
{
    Ground* gp = arg0->user_data;
    HSD_JObj* jobj = arg0->hsd_obj;
    HSD_GObj* other;
    f32 scale;
    s32 state;
    int slot;
    PAD_STACK(0x10);

    slot = gp->u.arwing.xC8 = venom_state.current_slot;
    venom_state.arwings[slot] = arg0;

    other =
        grVenom_80203EAC(grVe_GroupLinkedMapGObjIds[venom_state.groups[slot]]);
    if (other != NULL) {
        Ground* other_gp = other->user_data;
        other_gp->x10_flags.b2 = 0;
        other_gp = other->user_data;
        if (other_gp != NULL) {
            other_gp->u.starfox.arwing_slot = slot;
        } else {
            OSReport("arwin:user data error\n");
        }
    }

    scale = Ground_801C0498();
    state = venom_state.types[slot];
    if (state >= 8) {
        goto check_scale_uniform;
    }
    if (state >= 1) {
        goto scale_nonuniform;
    }
    goto done_scale;

check_scale_uniform:
    if (state >= 12) {
        goto done_scale;
    }
    goto scale_uniform;

scale_nonuniform:
    HSD_JObjSetScaleX(jobj, scale);
    HSD_JObjSetScaleY(jobj, scale);
    HSD_JObjSetScaleZ(jobj, scale * venom_state.parameters->x34);
    goto done_scale;

scale_uniform:
    HSD_JObjSetScaleX(jobj, scale);
    HSD_JObjSetScaleY(jobj, scale);
    HSD_JObjSetScaleZ(jobj, scale);

done_scale:

    gp->u.arwing.xD4 = 1;
    gp->u.arwing.xD8 = 0;
}

bool grVenom_802052D8(Ground_GObj* arg)
{
    return false;
}

void grVenom_802052E0(Ground_GObj* gobj, Vec3* pos)
{
    Vec3 jobj_pos;
    Ground* gp;
    HSD_JObj* jobj;
    const Vec3* position_offset;
    int arwing_type;

    if (gobj != NULL) {
        gp = gobj->user_data;
        Ground_GetMapGObj(5);
        jobj = Ground_801C3FA4(gobj, 5);
        lb_8000B1CC(jobj, NULL, &jobj_pos);
        arwing_type = venom_state.types[gp->u.arwing.xC8];
        position_offset = &grVe_ArwingPositionOffsets[arwing_type];
        pos->x = jobj_pos.x + position_offset->x;
        pos->y = jobj_pos.y + position_offset->y;
        pos->z = jobj_pos.z + position_offset->z;
    } else {
        pos->x = pos->y = pos->z = 0.0F;
    }
}

/// grVenom_802053B0

void grVenom_802053B0(Ground_GObj* gobj)
{
    Vec3 sp28;
    Vec3 sp1C;
    Ground* gp2;
    s32 state;
    HSD_JObj* jobj;
    Ground* gp;
    int slot;
    PAD_STACK(0x10);

    gp = gobj->user_data;
    jobj = gobj->hsd_obj;
    slot = gp->u.arwing.xC8;

    if (venom_state.arwings[slot] == gobj) {
        if (gp->u.arwing.xD4 == 1) {
            gp->u.arwing.xD4 = 0;
            grAnime_801C8138(
                gobj, gp->map_id,
                grVe_ArwingMapAnimationIds[venom_state.types[slot]]);
            return;
        }

        state = venom_state.types[slot];
        if (state >= 8) {
            goto check_far;
        }
        if (state >= 1) {
            goto near_type;
        }
        goto type_done;

    check_far:
        if (state >= 12) {
            goto type_done;
        }
        goto far_type;

    near_type:
        grVenom_802052E0(gobj, &sp28);
        state = gp->u.arwing.xD8;
        switch (state) {
        case 0:
            if (!grVenom_80205DF8(&sp28)) {
                lbAudioAx_800237A8(0x6B6C0, 0x7F, 0x40);
                gp->u.arwing.xD8 = 1;
            }
            break;
        case 1:
            if (!grVenom_80205E84(&sp28)) {
                gp->u.arwing.xD8 = 2;
            }
            break;
        case 2:
            if (grVenom_80205E84(&sp28) == 1) {
                lbAudioAx_800237A8(0x6B6C2, 0x7F, 0x40);
                gp->u.arwing.xD8 = 3;
            }
            break;
        }
        goto type_done;

    far_type:
        HSD_JObjSetRotationY(jobj, 0.0F);

        gp2 = gobj->user_data;
        grVenom_802052E0(gobj, &sp1C);
        state = gp2->u.arwing.xD8;
        switch (state) {
        case 0:
            if (!grVenom_80205DF8(&sp1C)) {
                lbAudioAx_800237A8(0x6B6C0, 0x7F, 0x40);
                gp2->u.arwing.xD8 = 1;
            }
            break;
        case 1:
            if (!grVenom_80205E84(&sp1C)) {
                gp2->u.arwing.xD8 = 2;
            }
            break;
        case 2:
            if (grVenom_80205E84(&sp1C) == 1) {
                lbAudioAx_800237A8(0x6B6C2, 0x7F, 0x40);
                gp2->u.arwing.xD8 = 3;
            }
            break;
        }

    type_done:
        if (grAnime_801C83D0(gobj, 0, 7)) {
            venom_state.arwings[slot] = NULL;
            Ground_801C4A08(gobj);
        }
    } else {
        Ground_801C4A08(gobj);
    }
}

void grVenom_802056AC(Ground_GObj* arg) {}

static int grVe_ArwingAnimationArgs[5][2] = {
    { -1, -1 }, { 1, 0 }, { 2, 2 }, { 4, 4 }, { 5, 5 },
};

static int grVe_GroupMapGObjIds[5] = { 0x0A, 0x0A, 0x0B, 0x0C, 0x0D };

static int grVe_GroupCollisionJointIds[5] = { 4, 4, 4, 4, 3 };

void grVenom_802056B0(Ground_GObj* gobj)
{
    Ground* gp = gobj->user_data;
    HSD_JObj* jobj = gobj->hsd_obj;
    int group;
    int joint_id;

    Ground_801C2ED0(jobj, gp->map_id);
    gp->u.arwing.xC8 = venom_state.current_slot;
    gp->u.arwing.xD0 = 0;
    gp->u.arwing.xDC = 0.0F;
    gp->u.arwing.xE0 = (Vec3){ 0 };
    group = venom_state.groups[gp->u.arwing.xC8];
    joint_id = grVe_GroupCollisionJointIds[group];
    mpJointListAdd(joint_id);

    Ground_801C2ED0(jobj, gp->map_id);
}

bool grVenom_80205750(Ground_GObj* arg)
{
    return false;
}

void grVenom_80205758(Ground_GObj* gobj)
{
    Ground* gp = GET_GROUND(gobj);
    HSD_JObj* jobj = GET_JOBJ(gobj);

    if (venom_state.arwings[gp->u.arwing.xC8] != NULL) {
        Ground_UpdateStarFoxArwingVisibility(gp, jobj, 50.0F);
        {
            f32 scale = Ground_801C0498();
            {
                f32 s = scale * venom_state.parameters->x34;
                HSD_JObjSetScaleX(jobj, s);
            }
            {
                f32 s = scale * venom_state.parameters->x34;
                HSD_JObjSetScaleY(jobj, s);
            }
            {
                f32 s = scale * venom_state.parameters->x34;
                HSD_JObjSetScaleZ(jobj, s);
            }
        }
        Ground_801C2FE0(gobj);
    } else {
        mpLib_80057BC0(
            grVe_GroupCollisionJointIds[venom_state.groups[gp->u.arwing.xC8]]);
        Ground_801C4A08(gobj);
    }
}

static int grVe_GroupAnimationJointIds[5] = { 1, 1, 1, 1, 1 };

void grVenom_80205AD0(Ground_GObj* arg) {}

void grVenom_80205AD4(Ground_GObj* gobj)
{
    Ground* gp = gobj->user_data;
    HSD_JObj* jobj = GET_JOBJ(gobj);
    f32 scale;

    scale = Ground_801C0498();
    gp->u.starfox.arwing_slot = venom_state.current_slot;
    Ground_ClearStarFoxArwingGObjs(gp);
    Ground_AnimateStarFoxArwingWithBackground(gobj);

    switch (venom_state.types[gp->u.starfox.arwing_slot]) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7: {
        HSD_GObj* other = grVenom_80203EAC(
            grVe_GroupMapGObjIds
                [venom_state.groups[gp->u.starfox.arwing_slot]]);
        Ground_LinkStarFoxArwing(gp, other);
        break;
    }
    case 8:
    case 9:
    case 10:
    case 11:
        Ground_AttachStarFoxArwingModel(
            gobj, gp, venom_state.arwings[gp->u.starfox.arwing_slot], 5);
        break;
    default:
        Ground_DisableStarFoxArwingGObjs(gp);
        break;
    }
    HSD_JObjSetScaleX(jobj, scale * venom_state.parameters->x34);
    HSD_JObjSetScaleY(jobj, scale * venom_state.parameters->x34);
    HSD_JObjSetScaleZ(jobj, scale * venom_state.parameters->x34);
    Ground_ResetStarFoxArwingState(gp);
    gp->u.starfox.xF8 = (s32) venom_state.parameters->x2C;
    gp->u.starfox.xFC = 0;
    gp->u.starfox.x100 = HSD_Randi(2);
}

bool grVenom_80205DF0(Ground_GObj* arg)
{
    return false;
}

s32 grVenom_80205DF8(Vec3* pos)
{
    return Stage_IsOutsideBlastZone(pos);
}

/// grVenom_80205E84

s32 grVenom_80205E84(Vec3* pos)
{
    return Stage_IsOutsideBlastZoneWithMargin(pos, 20.0F);
}

static const Vec3 grVe_803B82D0 = { 0.0F, 0.0F, 0.0F };
static const Vec3 grVe_803B82DC = { 0.0F, 0.0F, 0.0F };

void grVenom_80205F30(Ground_GObj* gobj)
{
    u64 padA8;
    Vec3 sp94;
    Vec3 sp88;
    u8 pad70[0x18];
    Vec3 sp64;
    u8 pad5C[8];
    Vec3 sp50;
    Ground* gp;
    HSD_JObj* jobj;
    Ground* other_gp;
    HSD_GObj* other;
    HSD_JObj* tmp_jobj;
    s32 state;
    s32 fire_kind;
    s32 player_slot;
    s32 retries;
    s32 type_idx;
    HSD_JObj* helper;
    int arwing_slot;
    int arwing_group;

    gp = gobj->user_data;
    jobj = gobj->hsd_obj;
    arwing_slot = gp->u.starfox.arwing_slot;
    arwing_group = venom_state.groups[arwing_slot];
    sp94 = grVe_803B82D0;
    sp88 = grVe_803B82DC;
    PAD_STACK(0x18);

    if (venom_state.updates_disabled) {
        return;
    }

    if (venom_state.arwings[arwing_slot] != NULL) {
        if (arwing_group == 4) {
            tmp_jobj = Ground_801C3FA4(gobj, 1);
            HSD_JObjSetRotationZ(tmp_jobj, 0.0F);
        }

        state = venom_state.types[arwing_slot];
        switch (state) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            HSD_JObjSetRotationY(jobj, 0.0F);

            {
                s32 anim_state = gp->u.starfox.xF4;
                if (anim_state == 0) {
                    goto venom_80205F30_anim_zero;
                }
                if (anim_state < 0) {
                    goto venom_80205F30_anim_done;
                }
                if (anim_state >= 5) {
                    goto venom_80205F30_anim_done;
                }
                goto venom_80205F30_check_anim;
            venom_80205F30_anim_zero: {
                if (gp->u.starfox.xF8 <= 0) {
                    gp->u.starfox.xF4 = HSD_Randi(4) + 1;
                    fire_kind = -1;
                    switch (arwing_group) {
                    case 0:
                        break;
                    case 1:
                    case 2:
                    case 3:
                        fire_kind = 0;
                        break;
                    case 4:
                        fire_kind = 1;
                        break;
                    }
                    {
                        int animation_state = gp->u.starfox.xF4;
                        int animation_arg =
                            fire_kind < 0
                                ? grVe_ArwingAnimationArgs[animation_state - 1]
                                                          [1]
                                : grVe_ArwingAnimationArgs[animation_state]
                                                          [fire_kind];
                        grAnime_801C8098(
                            gobj, grVe_GroupAnimationJointIds[arwing_group], 7,
                            animation_arg, 0.0F, 1.0F);
                    }
                } else {
                    s32 anim_id = grVe_GroupAnimationJointIds[arwing_group];
                    tmp_jobj = Ground_801C3FA4(gobj, anim_id);
                    HSD_JObjSetRotationZ(tmp_jobj, 0.0F);
                }
                gp->u.starfox.xF8--;
            }
                goto venom_80205F30_anim_done;
            venom_80205F30_check_anim:
                if (grAnime_801C83D0(gobj, 0, 7)) {
                    gp->u.starfox.xF4 = 0;
                    gp->u.starfox.xF8 = (s32) venom_state.parameters->x2C;
                }
            venom_80205F30_anim_done:;
            }

            if ((other = venom_state.arwings[arwing_slot]) != NULL) {
                other_gp = other->user_data;
                Ground_GetMapGObj(5);
                lb_8000B1CC(Ground_801C3FA4(other, 5), NULL, &sp64);
                {
                    const Vec3* position_offset =
                        &grVe_ArwingPositionOffsets
                            [venom_state.types[other_gp->u.arwing.xC8]];
                    sp94.x = sp64.x + position_offset->x;
                    sp94.y = sp64.y + position_offset->y;
                    sp94.z = sp64.z + position_offset->z;
                }
            } else {
                sp94.x = sp94.y = sp94.z = 0.0F;
            }

            HSD_JObjSetTranslate(jobj, &sp94);

            {
                s32 anim_id = grVe_GroupAnimationJointIds[arwing_group];
                lb_8000B1CC(Ground_801C3FA4(gobj, anim_id), NULL, &sp94);
            }
            if (gp->u.starfox.linked_gobj != NULL) {
                Ground* sub = gp->u.starfox.linked_gobj->user_data;
                if (sub != NULL) {
                    sub->u.arwing.xE0 = sp94;
                }
            }

            {
                f32 rot_z;
                s32 anim_id = grVe_GroupAnimationJointIds[arwing_group];
                helper = Ground_801C3FA4(gobj, anim_id);
                rot_z = HSD_JObjGetRotationZ(helper);
                if (gp->u.starfox.linked_gobj != NULL) {
                    Ground* sub = gp->u.starfox.linked_gobj->user_data;
                    if (sub != NULL) {
                        sub->u.arwing.xDC = rot_z;
                    }
                }
            }
            break;
        case 8:
        case 9:
        case 10:
        case 11:
            if (!(gp->u.starfox.xF0 & 7) && HSD_Randi(8) == 0) {
                gp->u.starfox.xFC = 0;
                type_idx = venom_state.types[arwing_slot];
                switch (type_idx) {
                case 8:
                    if (gp->u.starfox.xF0 > 0x3C && gp->u.starfox.xF0 < 0xE6) {
                        gp->u.starfox.xFC = 1;
                    }
                    break;
                case 9:
                    if (gp->u.starfox.xF0 > 0x32 && gp->u.starfox.xF0 < 0xE6) {
                        gp->u.starfox.xFC = 1;
                    }
                    break;
                case 10:
                    if (gp->u.starfox.xF0 > 0x5A && gp->u.starfox.xF0 < 0x104)
                    {
                        gp->u.starfox.xFC = 1;
                    }
                    break;
                case 11:
                    if (gp->u.starfox.xF0 > 0x46 && gp->u.starfox.xF0 < 0xF0) {
                        gp->u.starfox.xFC = 1;
                    }
                    break;
                }
            } else {
                gp->u.starfox.xFC = 0;
            }

            {
                HSD_GObj* far_other;
                Ground* far_other_gp;

                if ((far_other = venom_state.arwings[arwing_slot]) != NULL) {
                    far_other_gp = far_other->user_data;
                    Ground_GetMapGObj(5);
                    lb_8000B1CC(Ground_801C3FA4(far_other, 5), NULL, &sp50);
                    {
                        const Vec3* position_offset =
                            &grVe_ArwingPositionOffsets
                                [venom_state.types[far_other_gp->u.arwing.xC8]];
                        sp94.x = sp50.x + position_offset->x;
                        sp94.y = sp50.y + position_offset->y;
                        sp94.z = sp50.z + position_offset->z;
                    }
                } else {
                    sp94.x = sp94.y = sp94.z = 0.0F;
                }
            }

            if (-100.0F > sp94.z && sp94.z > -2000.0F &&
                gp->u.starfox.xFC != 0)
            {
                gp->u.starfox.xFC = 0;
                player_slot = HSD_Randi(4);
                retries = 0;
                do {
                    player_slot = (player_slot + retries) & 3;
                    if (Player_GetPlayerSlotType(player_slot) != 3) {
                        break;
                    }
                    retries++;
                } while (retries < 4);

                Player_LoadPlayerCoords(player_slot, &sp88);
                if ((sp88.x > 0.0F && sp94.x > 0.0F) ||
                    (sp88.x < 0.0F && sp94.x < 0.0F))
                {
                    sp88.y += 5.0F;
                    lbAudioAx_800237A8(0x6B6C9, 0x7F, 0x40);
                    fire_kind = -1;
                    switch (arwing_group) {
                    case 0:
                        break;
                    case 1:
                    case 2:
                    case 3:
                        fire_kind = 0;
                        break;
                    case 4:
                        fire_kind = 1;
                        break;
                    }
                    if (fire_kind == 1) {
                        it_802E7654(gobj, Ground_801C3FA4(gobj, 7), &sp88, 3,
                                    0, venom_state.parameters->x34);
                    } else {
                        if (gp->u.starfox.x100 != 0) {
                            it_802E7654(gobj, Ground_801C3FA4(gobj, 5), &sp88,
                                        1, 0, venom_state.parameters->x34);
                        } else {
                            it_802E7654(gobj, Ground_801C3FA4(gobj, 6), &sp88,
                                        1, 0, venom_state.parameters->x34);
                        }
                        gp->u.starfox.x100 = (gp->u.starfox.x100 + 1) & 1;
                    }
                    grMaterial_801C9604(gobj, venom_state.parameters->x38, 0);
                }
            }
            break;
        }

        gp->u.starfox.xF0++;
    } else {
        int i;
        for (i = 0; i < ARRAY_SIZE(gp->u.starfox.article_gobjs); i++) {
            if (gp->u.starfox.article_gobjs[i] != NULL) {
                Ground_801C4A08(gp->u.starfox.article_gobjs[i]);
            }
        }
        Ground_801C4A08(gobj);
    }
}

void grVenom_80206870(Ground_GObj* arg) {}

void grVenom_80206874(Ground_GObj* gobj)
{
    Ground* gp = gobj->user_data;
    HSD_JObj* jobj = gobj->hsd_obj;
    f32 scale;

    scale = Ground_801C0498();
    gp->u.starfox.arwing_slot = venom_state.current_slot;
    Ground_ClearStarFoxArwingGObjs(gp);
    Ground_AnimateStarFoxArwing(gobj);

    switch (venom_state.types[gp->u.starfox.arwing_slot]) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7: {
        HSD_GObj* other = grVenom_80203EAC(
            grVe_GroupMapGObjIds
                [venom_state.groups[gp->u.starfox.arwing_slot]]);
        Ground_LinkStarFoxArwing(gp, other);
        break;
    }
    case 8:
    case 9:
    case 10:
    case 11:
        Ground_AttachStarFoxArwingModel(
            gobj, gp, venom_state.arwings[gp->u.starfox.arwing_slot], 5);
        break;
    default:
        Ground_DisableStarFoxArwingGObjs(gp);
        break;
    }

    HSD_JObjSetScaleX(jobj, scale * venom_state.parameters->x34);
    HSD_JObjSetScaleY(jobj, scale * venom_state.parameters->x34);
    HSD_JObjSetScaleZ(jobj, scale * venom_state.parameters->x34);

    Ground_ResetStarFoxArwingState(gp);

    gp->u.starfox.xF8 = (s32) venom_state.parameters->x2C;
    gp->u.starfox.xFC = 0;
}

bool grVenom_80206B68(Ground_GObj* arg)
{
    return false;
}

void grVenom_80206B70(Ground_GObj* arg)
{
    grVenom_80205F30(arg);
}

void grVenom_80206B90(Ground_GObj* arg) {}

void grVenom_80206B94(Ground_GObj* gobj)
{
    Ground* gp = GET_GROUND(gobj);
    smashTaunt_801E2550(gobj, &gp->u.smashtaunt);
}

bool grVenom_80206BBC(Ground_GObj* arg)
{
    return false;
}

void grVenom_80206BC4(Ground_GObj* gobj)
{
    Ground* gp = GET_GROUND(gobj);
    grCorneria_801E277C(gobj, &gp->u.smashtaunt);
}

void grVenom_80206BEC(Ground_GObj* arg) {}

bool grVenom_80206BF0(int arg0)
{
    s32 var_r30;
    Ground_GObj* wgobj;
    Ground* gp;
    PAD_STACK(32);

    if (Ground_GetMapGObj(8) != NULL) {
        return 0;
    }
    if (arg0 == 2) {
        var_r30 = HSD_Randi(5) + 8;
    } else if (arg0 == 20) {
        var_r30 = HSD_Randi(5) + 13;
    } else {
        return 0;
    }
    wgobj = grVenom_80203EAC(8);
    HSD_ASSERT(2244, wgobj);
    gp = wgobj->user_data;
    gp->u.starfox.xCC = 10;
    gp->u.starfox.arwing_slot = 0;
    gp->u.starfox.xC4.word = var_r30;
    return 1;
}

void grVenom_80206CB0(s32 arg0)
{
    HSD_GObj* gobj;

    gobj = Ground_GetMapGObj(8);
    if (gobj != NULL) {
        if (arg0 != 0) {
            Ground_801C4A08(gobj);
        }
        gobj = Ground_GetMapGObj(1);
        if (gobj != NULL) {
            Ground_801C4A08(gobj);
        }
        HSD_SisLib_803A5F50(1);
    }
}

s32 grVenom_80206D10(s32 arg0)
{
    s32 in_range;
    s32 result;
    u32 diff;

    if (stage_info.grkind == Gr_Kind_Venom && arg0 != -1) {
        result = mpJointFromLine(arg0);
        diff = result - 3;
        in_range = 1;
        if (diff > 1U) {
            in_range = 0;
        }
        if (in_range != 0) {
            return 1;
        }
    }
    return 0;
}

lbColl_80008D30_arg1* grVenom_80206D74(enum_t arg)
{
    return false;
}

bool grVenom_80206D7C(Vec3* pos, int arg1, HSD_JObj* arg2)
{
    Vec3 sp20;
    Vec3 sp14;
    HSD_GObj* gobj;
    Ground* gp;

    lb_8000B1CC(arg2, NULL, &sp20);
    gobj = Ground_GetMapGObj(5);
    if (gobj != NULL) {
        gp = gobj->user_data;
        if (gp != NULL && gp->u.venom_platform.lower_jobj == arg2) {
            lb_8000B1CC(gp->u.venom_platform.upper_jobj, NULL, &sp14);
            if (pos->y > sp14.y) {
                return 0;
            }
        }
    }
    if (pos->y > sp20.y) {
        return 1;
    }
    return 0;
}
