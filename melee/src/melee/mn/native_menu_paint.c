#include "native_menu_paint.h"

#include <math.h>

#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/state.h>
#include <sysdolphin/baselib/tev.h>
#include <sysdolphin/baselib/video.h>

static void vertex(float x, float y, float u, float v)
{
    GXPosition3f32(x, -y, -5);
    GXTexCoord2f32(u, v);
}
static void color(uint32_t rgba)
{
    GXSetTevColor(GX_TEVREG0,
                  (GXColor){ rgba >> 24, rgba >> 16, rgba >> 8, rgba });
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
                    GX_CC_C0);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
                    GX_CA_A0);
}
void nativeMenuPaint_Begin(const Mtx view)
{
    if (view) {
        GXLoadPosMtxImm(view, GX_PNMTX0);
    } else {
        Mtx44 projection;
        Mtx model;
        C_MTXOrtho(projection, 0, -480, 0, 640, 1, 100);
        C_MTXIdentity(model);
        GXSetProjection(projection, GX_ORTHOGRAPHIC);
        GXLoadPosMtxImm(model, GX_PNMTX0);
        const HSD_VIStatus* video = HSD_VIGetVIStatus();
        GXSetViewport(0, 0, video->rmode.fbWidth, video->rmode.efbHeight, 0,
                      1);
        GXSetScissor(0, 0, video->rmode.fbWidth, video->rmode.efbHeight);
    }
    GXSetCurrentMtx(GX_PNMTX0);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetNumChans(0);
    GXSetNumTexGens(1);
    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
    GXSetTevDirect(GX_TEVSTAGE0);
    GXSetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
                    GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
                    GX_TRUE, GX_TEVPREV);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA,
                   GX_LO_CLEAR);
    GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
    GXSetCullMode(GX_CULL_NONE);
    GXSetColorUpdate(GX_TRUE);
    GXSetAlphaUpdate(GX_FALSE);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GXSetFog(GX_FOG_NONE, 0, 0, 0, 1, (GXColor){ 0, 0, 0, 0 });
}
void nativeMenuPaint_End(void)
{
    HSD_StateInvalidate(-1);
    HSD_StateInitTev();
    HSD_ClearVtxDesc();
}
void nativeMenuPaint_Quad(const float xy[8], uint32_t rgba)
{
    color(rgba);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    for (unsigned i = 0; i < 4; ++i) {
        vertex(xy[i * 2], xy[i * 2 + 1], 0, 0);
    }
    GXEnd();
}
void nativeMenuPaint_Rect(float x, float y, float w, float h, uint32_t rgba)
{
    float xy[] = { x, y, x + w, y, x + w, y + h, x, y + h };
    nativeMenuPaint_Quad(xy, rgba);
}
void nativeMenuPaint_Round(float x, float y, float w, float h, float radius,
                           uint32_t rgba)
{
    float r = fminf(radius, fminf(w, h) * .5f);
    color(rgba);
    GXBegin(GX_TRIANGLEFAN, GX_VTXFMT0, 30);
    vertex(x + w * .5f, y + h * .5f, 0, 0);
    for (unsigned i = 0; i <= 28; ++i) {
        unsigned corner = (i / 7) % 4;
        float cx = (corner == 0 || corner == 3) ? x + w - r : x + r;
        float cy = corner < 2 ? y + h - r : y + r;
        // Start at the top-right corner and wind clockwise.
        float a = (float) i * (1.57079632679f / 7);
        vertex(cx + r * cosf(a), cy + r * sinf(a), 0, 0);
    }
    GXEnd();
}
void nativeMenuPaint_Text(const char* text, float x, float y, float w, float h,
                          uint32_t rgba, NativeMenuFontFace face,
                          bool centered)
{
    nativeMenuFont_Draw(face, text, x, -y, -5, w, h, rgba, centered);
}
void nativeMenuPaint_Image(void* pixels, unsigned width, unsigned height,
                           float x, float y, float w, float h)
{
    if (!pixels || !width || !height) {
        return;
    }
    GXTexObj texture;
    GXInitTexObj(&texture, pixels, width, height, GX_TF_RGBA8, GX_CLAMP,
                 GX_CLAMP, GX_FALSE);
    GXLoadTexObj(&texture, GX_TEXMAP0);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    vertex(x, y, 0, 0);
    vertex(x + w, y, 1, 0);
    vertex(x + w, y + h, 1, 1);
    vertex(x, y + h, 0, 1);
    GXEnd();
}
