#include "native_holdable.h"

#ifdef MELEE_NATIVE

#include "native_source_item_visual.h"
#include "native_script_item_actor.h"
#include <melee/ft/native_fighter_runtime.h>
#include <melee/it/itdraw.h>
#include <melee/it/inlines.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/state.h>
#include <sysdolphin/baselib/tev.h>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>

void nativeHoldable_Draw(HSD_GObj* gobj, int pass)
{
    const NativeScriptItemProgram* spec=nativeScriptItem_Spec((Item_GObj*)gobj);
    if(!spec || spec->kind!=It_Kind_NativeHoldable || !spec->recipe)return;
    Item* item=GET_ITEM(gobj);
    if(!spec->recipe->billboard_png && !spec->recipe->billboard_png_size) {
        if(spec->article && item->xC4_article_data==spec->article)it_8026EECC(gobj,pass);
        return;
    }
    if(!spec->visual)return;
    const NativeSourceItemVisual* visual=spec->visual;
    const NativeItemDefinition* recipe=spec->recipe;
    if(!visual->pixels || visual->bank!=spec->bank || visual->identity!=spec->identity ||
       visual->owner!=spec->owner || visual->png!=recipe->billboard_png ||
       visual->png_bytes!=recipe->billboard_png_size || visual->size!=recipe->billboard_size ||
       !visual->width || !visual->height)return;
    // The GX-link passes are opaque, texture-edge, translucent. A PNG with
    // partial alpha belongs only in the final pass, not all three.
    if (pass != 2) {
        if (it_8026ECE0((Item_GObj*) gobj, pass)) {
            HSD_StateInvalidate(-1);
            HSD_StateInitTev();
            HSD_ClearVtxDesc();
        }
        return;
    }
    HSD_CObj* camera = HSD_CObjGetCurrent();
    if (camera == NULL) return;
    Mtx view;
    HSD_CObjGetViewingMtx(camera, view);
    GXLoadPosMtxImm(view, 0);
    GXSetCurrentMtx(0);
    GXTexObj texture;
    GXInitTexObj(&texture, visual->pixels, visual->width, visual->height,
                 GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXLoadTexObj(&texture, GX_TEXMAP0);
    GXClearVtxDesc();
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetNumChans(0);
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0,
                      GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GXSetCullMode(GX_CULL_NONE);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA,
                   GX_LO_NOOP);
    GXSetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0);
    float half = visual->size * 0.5f;
    const Vec3* p = &item->pos;
    float rx = view[0][0] * half, ry = view[0][1] * half,
          rz = view[0][2] * half;
    float ux = view[1][0] * half, uy = view[1][1] * half,
          uz = view[1][2] * half;
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(p->x - rx + ux, p->y - ry + uy, p->z - rz + uz);
    GXTexCoord2f32(0, 0);
    GXPosition3f32(p->x + rx + ux, p->y + ry + uy, p->z + rz + uz);
    GXTexCoord2f32(1, 0);
    GXPosition3f32(p->x + rx - ux, p->y + ry - uy, p->z + rz - uz);
    GXTexCoord2f32(1, 1);
    GXPosition3f32(p->x - rx - ux, p->y - ry - uy, p->z - rz - uz);
    GXTexCoord2f32(0, 1);
    GXEnd();
    it_8026ECE0((Item_GObj*) gobj, pass);
    HSD_StateInvalidate(-1);
    HSD_StateInitTev();
    HSD_ClearVtxDesc();
}

#endif
