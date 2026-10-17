#include "native_hub_hand.h"
#include <melee/sc/types.h>
#include <melee/lb/lbspdisplay.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/state.h>
#include <sysdolphin/baselib/tev.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/wobj.h>

static HSD_GObj* hand_owner;
static HSD_GObj* camera_owner;
static unsigned displayed_state;

static void forget_owner(void* owner)
{
    *(HSD_GObj**) owner = NULL;
}
void nativeHubHand_Unload(void)
{
    if (hand_owner) {
        HSD_GObjFree(hand_owner);
    }
    if (camera_owner) {
        HSD_GObjFree(camera_owner);
    }
}
void nativeHubHand_Load(const StaticModelDesc* model)
{
    nativeHubHand_Unload();
    if (!model || !model->joint) {
        return;
    }
    HSD_WObjDesc eye = { .pos = { 0, 0, 100 } },
                 interest = { .pos = { 0, 0, 0 } };
    HSD_CObjDesc descriptor = { .ortho = { .projection_type = PROJ_ORTHO,
                                           .viewport = { 0, 640, 0, 480 },
                                           .scissor = { 0, 640, 0, 480 },
                                           .eyepos = &eye,
                                           .interest = &interest,
                                           .nnear = 1,
                                           .ffar = 1000,
                                           .top = 0,
                                           .bottom = -480,
                                           .left = 0,
                                           .right = 640 } };
    camera_owner = GObj_Create(0xFE00, 3, 0x80);
    GObj_InitUserData(camera_owner, 0, forget_owner, &camera_owner);
    HSD_GObjObject_80390A70(camera_owner, HSD_GObj_CameraKind,
                            HSD_CObjLoadDesc(&descriptor));
    hand_owner = GObj_Create(0xFE01, 5, 0x80);
    GObj_InitUserData(hand_owner, 0, forget_owner, &hand_owner);
    HSD_JObj* hand = HSD_JObjLoadJoint(model->joint);
    HSD_GObjObject_80390A70(hand_owner, HSD_GObj_JObjKind, hand);
    HSD_JObjAddAnimAll(hand, model->animjoint, model->matanim_joint,
                       model->shapeanim_joint);
    HSD_JObjReqAnimAll(hand, 0);
    HSD_JObjAnimAll(hand);
    HSD_ForeachAnim(hand, JOBJ_TYPE, ALL_TYPE_MASK, HSD_AObjStopAnim,
                    AOBJ_ARG_AOV, 0, 0);
    Vec3 origin = { 0, 0, 0 };
    HSD_JObjSetTranslate(hand, &origin);
    displayed_state = ~0U;
}
bool nativeHubHand_Ready(void)
{
    return hand_owner && camera_owner;
}
void nativeHubHand_Draw(float x, float y, bool pressed)
{
    if (!nativeHubHand_Ready()) {
        return;
    }
    HSD_JObj* hand = hand_owner->hsd_obj;
    unsigned state = pressed ? 1 : 0;
    if (state != displayed_state) {
        HSD_JObj* pose = NULL;
        lb_80011E24(hand, &pose, 2, -1);
        HSD_ForeachAnim(pose, JOBJ_TYPE, TOBJ_MASK, HSD_AObjReqAnim,
                        AOBJ_ARG_AF, (float) state);
        HSD_JObjAnimAll(pose);
        HSD_ForeachAnim(pose, JOBJ_TYPE, TOBJ_MASK, HSD_AObjStopAnim,
                        AOBJ_ARG_AOV, 0, 0);
        displayed_state = state;
    }
    HSD_StateInvalidate(-1);
    HSD_StateInitTev();
    HSD_ClearVtxDesc();
    HSD_CObj* camera = camera_owner->hsd_obj;
    HSD_CObj* previous = HSD_CObjGetCurrent();
    if (!previous) {
        return;
    }
    if (!HSD_CObjSetCurrent(camera)) {
        return;
    }
    // The pointing fingertip is (4.5, -0.5) in the authored hand's
    // coordinates. Anchor that point to the input position rather than the
    // model origin.
    Mtx view = { { 4, 0, 0, x - 18 }, { 0, 4, 0, -y + 2 }, { 0, 0, 4, -100 } };
    HSD_JObjDispAll(hand, view, HSD_TRSP_ALL, 0);
    HSD_CObjEndCurrent();
    HSD_CObjSetCurrent(previous);
}
