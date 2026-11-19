#include "native_css_remote.h"
#include "native_css_cards.h"
#include "native_css_original.h"

#include <melee/gm/native_online_session.h>
#include <melee/lb/lbspdisplay.h>
#include <melee/sc/types.h>
#include <melee/ui/native_hub.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mobj.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>

enum { CSS_UNNUMBERED = 4, HAND_STYLES = 5, POINTER_IDLE_MS = 1200 };
static struct {
    HSD_GObj* owner;
    HSD_JObj* hand[HAND_STYLES][2];
    HSD_JObj* token[HAND_STYLES];
    NativeCssPointers sent;
    uint64_t last_sent;
} remote;

static HSD_JObj* pose(const StaticModelDesc* source, unsigned display,
                      bool hand, bool holding)
{
    if (!source->joint) return NULL;
    HSD_JObj* root = HSD_JObjLoadJoint(source->joint);
    if (!root) return NULL;
    HSD_JObjAddAnimAll(root, source->animjoint, source->matanim_joint,
                       source->shapeanim_joint);
    HSD_JObjReqAnimAll(root, 0);
    HSD_JObjAnimAll(root);
    const unsigned number = display == CSS_UNNUMBERED ? 0 : display;
    HSD_JObj* state = NULL;
    lb_80011E24(root, &state, hand ? 2 : 3, -1);
    if (hand) {
        HSD_ForeachAnim(state, JOBJ_TYPE, TOBJ_MASK, HSD_AObjReqAnim,
                        AOBJ_ARG_AF, (float)holding);
    } else {
        const unsigned color = display == CSS_UNNUMBERED ? 8 : display;
        HSD_ForeachAnim(state, JOBJ_TYPE, MOBJ_MASK, HSD_AObjReqAnim,
                        AOBJ_ARG_AF, (float)(color * 40));
    }
    HSD_JObj* label = NULL;
    lb_80011E24(root, &label, hand ? 3 : 4, -1);
    if (label) {
        unsigned frame = number * 4 + (hand ? number : 0);
        HSD_ForeachAnim(label, JOBJ_TYPE, TOBJ_MASK, HSD_AObjReqAnim,
                        AOBJ_ARG_AF, (float)frame);
        if (display == CSS_UNNUMBERED) HSD_JObjSetFlags(label, JOBJ_HIDDEN);
    }
    HSD_JObjAnimAll(root);
    HSD_ForeachAnim(root, JOBJ_TYPE, ALL_TYPE_MASK, HSD_AObjStopAnim,
                    AOBJ_ARG_AOV, 0, 0);
    if (!hand) {
        // The stock token has visible and hidden joints. Change the material
        // opacity without disturbing its authored silhouette or numbering.
        HSD_JObj* joint = root;
        do {
            for (HSD_DObj* mesh = HSD_JObjGetDObj(joint); mesh; mesh = mesh->next) {
                HSD_MObj* material = mesh->mobj;
                if (material && material->mat) {
                    HSD_MObjSetAlpha(material, material->mat->alpha * .25f);
                }
            }
            if (joint->child) joint = joint->child;
            else {
                while (joint != root && !joint->next) joint = joint->parent;
                joint = joint == root ? NULL : joint->next;
            }
        } while (joint);
    }
    return root;
}

static void place_art(HSD_JObj* art, float x, float y, float depth, int pass)
{
    if (!art) return;
    Vec3 point = { x, y, depth };
    nativeCssOriginal_TransformPoint(&point);
    HSD_JObjSetTranslate(art, &point);
    HSD_JObjDispAll(art, NULL, HSD_GObj_80390EB8(pass), 0);
}

static void draw(HSD_GObj* object, int pass)
{
    (void)object;
    const NativeRemoteCssPointers* points = nativeOnlineSession_CssPointers();
    if (!points || !nativeCssCards_Active() || nativeHub_IsVisible()) return;
    const uint64_t now = GetTickCount64();
    float left, right, top, bottom;
    nativeCssOriginal_Viewport(&left, &right, &top, &bottom);
    for (unsigned peer = 0; peer < NATIVE_SIGNALING_MAX_PARTICIPANTS; ++peer) {
        const NativeRemoteCssPointers* received = &points[peer];
        const NativeCssPointers* pointers = &received->pointers;
        if (!pointers->visible || now - received->received_ms > POINTER_IDLE_MS)
            continue;
        for (unsigned seat = 0; seat < 4; ++seat) {
            const unsigned bit = 1u << seat;
            if (!(pointers->visible & bit)) continue;
            unsigned display;
            if (!nativeCssCards_RemotePort(pointers->peer_id, seat, &display))
                continue;
            const bool holding = pointers->held & bit;
            const float x = left + (right - left) * pointers->point[seat][0] /
                NATIVE_CSS_POINTER_UNITS;
            const float y = top - (top - bottom) * pointers->point[seat][1] /
                NATIVE_CSS_POINTER_UNITS;
            if (holding) place_art(remote.token[display], x + 2.7f, y - 2, 1, pass);
            place_art(remote.hand[display][holding], x, y, 0, pass);
        }
    }
}

static void dispose(void* unused)
{
    (void)unused;
    for (unsigned i = 0; i < HAND_STYLES; ++i) {
        HSD_JObjRemoveAll(remote.hand[i][0]);
        HSD_JObjRemoveAll(remote.hand[i][1]);
        HSD_JObjRemoveAll(remote.token[i]);
    }
    memset(&remote, 0, sizeof(remote));
}

void nativeCssRemote_Begin(const StaticModelDesc* hand, const StaticModelDesc* token)
{
    if (remote.owner) HSD_GObjFree(remote.owner);
    remote.owner = GObj_Create(0xFE03, 5, 0x80);
    if (!remote.owner) return;
    GObj_InitUserData(remote.owner, 0, dispose, NULL);
    GObj_SetupGXLink(remote.owner, draw, 3, 0x7f);
    for (unsigned i = 0; i < HAND_STYLES; ++i) {
        remote.hand[i][0] = pose(hand, i, true, false);
        remote.hand[i][1] = pose(hand, i, true, true);
        remote.token[i] = pose(token, i, false, true);
    }
}

void nativeCssRemote_Publish(const NativeCssPointers* pointers)
{
    if (!remote.owner) return;
    const uint64_t now = GetTickCount64();
    const uint64_t elapsed = now - remote.last_sent;
    if (elapsed < 40 ||
        (elapsed < 420 && memcmp(pointers, &remote.sent, sizeof(*pointers)) == 0))
        return;
    if (nativeOnlineSession_PublishCssPointers(pointers)) {
        remote.sent = *pointers;
        remote.last_sent = now;
    }
}

void nativeCssRemote_End(void)
{
    if (remote.sent.visible) {
        const NativeCssPointers hidden = { 0 };
        nativeOnlineSession_PublishCssPointers(&hidden);
    }
    if (remote.owner) HSD_GObjFree(remote.owner);
}

void nativeCssRemote_Inspect(void)
{
    const NativeRemoteCssPointers* points = nativeOnlineSession_CssPointers();
    const uint64_t now = GetTickCount64();
    for (unsigned i = 0; points && i < NATIVE_SIGNALING_MAX_PARTICIPANTS; ++i) {
        const NativeCssPointers* p = &points[i].pointers;
        if (!p->visible || now - points[i].received_ms > POINTER_IDLE_MS) continue;
        for (unsigned seat = 0; seat < 4; ++seat) {
            unsigned display;
            if (!(p->visible & (1u << seat)) ||
                !nativeCssCards_RemotePort(p->peer_id, seat, &display)) continue;
            fprintf(stderr, "[css-hand] peer=%s seat=%u numbered=%d held=%d pos=%d,%d\n",
                    p->peer_id, seat, display < CSS_UNNUMBERED,
                    (p->held & (1u << seat)) != 0,
                    p->point[seat][0], p->point[seat][1]);
        }
    }
}
