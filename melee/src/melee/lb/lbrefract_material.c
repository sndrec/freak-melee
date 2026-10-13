#include "lbrefract.h"
#include "lbrefract_internal.h"
#include <dolphin/gx.h>
#include <sysdolphin/baselib/mobj.h>

lbRefract_State lbRefractState;

static Mtx texture_mtx = {
    { +0.5F, +0.0F, +0.0F, +0.5F },
    { +0.0F, -0.5F, +0.0F, +0.5F },
    { +0.0F, +0.0F, +0.0F, +1.0F },
};

static float texture_offset[2][3] = {
    { -0.5F, +0.0F, +0.0F },
    { +0.0F, -0.5F, +0.0F },
};

void lbRefract_80022998(HSD_MObj* mobj, u32 rendermode, s32 arg2)
{
    u64 write_z;
    GXCompare compare;

    HSD_TObjSetup(lbRefractState.tobj_list[arg2]);

    GXSetNumTexGens(2);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_NRM, GX_TEXMTX0,
                      GX_TRUE, GX_PTTEXMTX0);
    GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX3x4, GX_TG_POS, 0, GX_FALSE,
                      GX_PTTEXMTX1);

    GXLoadTexMtxImm(texture_mtx, GX_PTTEXMTX0, GX_MTX3x4);
    GXLoadTexMtxImm(lbRefractState.texture_mtx, GX_PTTEXMTX1, GX_MTX3x4);

    GXSetNumChans(0);
    GXSetNumTevStages(1);
    write_z = GX_ITF_8;
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR_NULL);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
                    GX_CC_TEXC);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
                    GX_CA_TEXA);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
                    GX_FALSE, GX_TEVPREV);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
                    GX_FALSE, GX_TEVPREV);
    GXSetNumIndStages(1);
    GXSetIndTexOrder(GX_INDTEXSTAGE0, GX_TEXCOORD0, GX_TEXMAP0);
    GXSetIndTexCoordScale(GX_INDTEXSTAGE0, GX_ITS_1, GX_ITS_1);
    GXSetIndTexMtx(GX_ITM_0, texture_offset, 1);

    GXSetTevIndirect(GX_TEVSTAGE0, GX_INDTEXSTAGE0, write_z, GX_ITB_ST,
                     GX_ITM_0, GX_ITW_OFF, GX_ITW_OFF, (GXBool) 0, (GXBool) 0,
                     GX_ITBA_OFF);

    GXSetColorUpdate(GX_TRUE);
    GXSetAlphaUpdate(GX_FALSE);

    write_z = (rendermode & RENDER_NO_ZUPDATE) ? 0 : 1;

    if (rendermode & RENDER_ZMODE_ALWAYS) {
        compare = GX_ALWAYS;
    } else {
        compare = GX_LEQUAL;
    }

    GXSetZMode(GX_TRUE, compare, write_z);
    GXSetZCompLoc(GX_TRUE);
    GXSetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_SET);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);
    GXSetDither(GX_FALSE);
}

