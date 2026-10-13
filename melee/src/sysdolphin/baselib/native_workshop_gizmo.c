#include "native_workshop_gizmo.h"

#include <math.h>
#include <string.h>

#include "pobj.h"
#include "state.h"
#include <dolphin/gx.h>

static struct {
    HSD_JObj* owner;
    Vec3 center, axes[3];
    float size;
    unsigned operation, selected;
} gizmo;
static Vec3 add(Vec3 a, Vec3 b)
{
    return (Vec3){ a.x + b.x, a.y + b.y, a.z + b.z };
}
static Vec3 scale(Vec3 a, float s)
{
    return (Vec3){ a.x * s, a.y * s, a.z * s };
}
static void vertex(Vec3 p, GXColor c)
{
    GXPosition3f32(p.x, p.y, p.z);
    GXColor4u8(c.r, c.g, c.b, c.a);
}
static void triangle(Vec3 a, Vec3 b, Vec3 c, GXColor color)
{
    vertex(a, color);
    vertex(b, color);
    vertex(c, color);
}
static Vec3 radial(unsigned axis, float angle, float radius)
{
    return scale(add(scale(gizmo.axes[(axis + 1) % 3], cosf(angle)),
                     scale(gizmo.axes[(axis + 2) % 3], sinf(angle))),
                 radius);
}
void nativeWorkshopGizmo_Clear(void)
{
    gizmo.owner = NULL;
}
void nativeWorkshopGizmo_Set(HSD_JObj* owner, const float center[3],
                             const float axes[3][3], float size,
                             unsigned operation, unsigned selected)
{
    gizmo.owner = owner;
    memcpy(&gizmo.center, center, 12);
    memcpy(gizmo.axes, axes, 36);
    gizmo.size = size;
    gizmo.operation = operation;
    gizmo.selected = selected;
}
void nativeWorkshopGizmo_Draw(HSD_JObj* owner, Mtx view)
{
    if (owner != gizmo.owner || !owner || gizmo.size <= 0) {
        return;
    }
    HSD_StateInvalidate(HSD_STATE_ALL);
    HSD_ClearVtxDesc();
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXLoadPosMtxImm(view, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL,
                  GX_DF_NONE, GX_AF_NONE);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetNumIndStages(0);
    GXSetTevDirect(GX_TEVSTAGE0);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GXSetCullMode(GX_CULL_NONE);
    GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
    GXSetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_COPY);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GXSetColorUpdate(GX_TRUE);
    const GXColor colors[] = { { 239, 92, 88, 255 },
                               { 105, 205, 132, 255 },
                               { 90, 145, 239, 255 } };
    const float tau = 6.28318530718f, radius = gizmo.size * .018f;
    for (unsigned axis = 0; axis < 3; ++axis) {
        GXColor color = (gizmo.selected & (1u << axis))
                            ? (GXColor){ 255, 219, 105, 255 }
                            : colors[axis];
        if (gizmo.operation == 1) {
            // A torus has a stable silhouette as the camera orbits the bone.
            GXBegin(GX_TRIANGLES, GX_VTXFMT0, 32 * 8 * 6);
            for (unsigned segment = 0; segment < 32; ++segment) {
                for (unsigned side = 0; side < 8; ++side) {
                    Vec3 points[4];
                    for (unsigned i = 0; i < 4; ++i) {
                        float t = tau * (segment + (i == 1 || i == 2)) / 32;
                        float u = tau * (side + (i >= 2)) / 8;
                        points[i] = add(
                            gizmo.center,
                            add(radial(axis, t, gizmo.size + radius * cosf(u)),
                                scale(gizmo.axes[axis], radius * sinf(u))));
                    }
                    triangle(points[0], points[1], points[2], color);
                    triangle(points[0], points[2], points[3], color);
                }
            }
            GXEnd();
        } else {
            Vec3 end = add(gizmo.center, scale(gizmo.axes[axis], gizmo.size));
            Vec3 neck =
                add(gizmo.center, scale(gizmo.axes[axis], gizmo.size * .82f));
            GXBegin(GX_TRIANGLES, GX_VTXFMT0, 12 * 6);
            for (unsigned side = 0; side < 12; ++side) {
                Vec3 a = radial(axis, tau * side / 12, radius),
                     b = radial(axis, tau * (side + 1) / 12, radius);
                triangle(add(gizmo.center, a), add(gizmo.center, b),
                         add(neck, b), color);
                triangle(add(gizmo.center, a), add(neck, b), add(neck, a),
                         color);
            }
            GXEnd();
            if (gizmo.operation == 0) {
                GXBegin(GX_TRIANGLES, GX_VTXFMT0, 12 * 6);
                for (unsigned side = 0; side < 12; ++side) {
                    Vec3 a = add(neck,
                                 radial(axis, tau * side / 12, radius * 3)),
                         b = add(neck, radial(axis, tau * (side + 1) / 12,
                                              radius * 3));
                    triangle(a, b, end, color);
                    triangle(neck, b, a, color);
                }
                GXEnd();
            } else {
                Vec3 corners[8];
                for (unsigned i = 0; i < 8; ++i) {
                    Vec3 p = end;
                    for (unsigned a = 0; a < 3; ++a) {
                        p = add(p,
                                scale(gizmo.axes[a], ((i >> a) & 1 ? 1 : -1) *
                                                         radius * 2.5f));
                    }
                    corners[i] = p;
                }
                const unsigned faces[][4] = { { 0, 1, 3, 2 }, { 4, 6, 7, 5 },
                                              { 0, 4, 5, 1 }, { 2, 3, 7, 6 },
                                              { 0, 2, 6, 4 }, { 1, 5, 7, 3 } };
                GXBegin(GX_TRIANGLES, GX_VTXFMT0, 36);
                for (unsigned f = 0; f < 6; ++f) {
                    triangle(corners[faces[f][0]], corners[faces[f][1]],
                             corners[faces[f][2]], color);
                    triangle(corners[faces[f][0]], corners[faces[f][2]],
                             corners[faces[f][3]], color);
                }
                GXEnd();
            }
        }
    }
    HSD_ClearVtxDesc();
    HSD_StateInvalidate(HSD_STATE_ALL);
}
