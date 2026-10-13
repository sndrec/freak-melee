#include "psdisptev.h"

#include <dolphin/gx.h>
#include "psstructs.h"
#include "native_render_phase.h"

#ifdef MELEE_NATIVE
#pragma clang section data = "" bss = ""
#endif
static u32 prevTev[2];
#ifdef MELEE_NATIVE
#pragma clang section data = ".mstate$m"
#endif

void psSetupTevCommon(void)
{
#ifdef MELEE_NATIVE
    if (!nativeRenderGraphicsEnabled) return;
#endif
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1,
                    GX_TEVPREV);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1,
                    GX_TEVPREV);
    GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1,
                    GX_TEVPREV);
    GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1,
                    GX_TEVPREV);
    GXSetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevColorOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1,
                    GX_TEVPREV);
    GXSetTevAlphaOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1,
                    GX_TEVPREV);
    GXSetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);
    GXSetTevSwapMode(GX_TEVSTAGE1, GX_TEV_SWAP0, GX_TEV_SWAP0);
    GXSetTevSwapMode(GX_TEVSTAGE2, GX_TEV_SWAP0, GX_TEV_SWAP0);
}

void psSetupTevInvalidState(void)
{
    prevTev[0] = -1;
}

void psSetupTev(HSD_Particle* particle)
{
    u32 temp_r5 = particle->kind & 0x80100480;
    if (temp_r5 == prevTev[0]) {
        return;
    }

    prevTev[0] = temp_r5;
    // These four untextured modes consume PrimEnv during display. Keep that
    // state change even when the corresponding shader commands are omitted.
    switch (temp_r5) {
    case 0x80000080:
    case 0x80:
    case 0x100080:
    case 0x80100080:
        particle->kind &= ~0x80U;
        prevTev[0] &= ~0x80U;
        break;
    }
#ifdef MELEE_NATIVE
    if (!nativeRenderGraphicsEnabled) return;
#endif
    switch (prevTev[0]) {
    case 0x80000000:
        GXSetNumTevStages(2);
        GXSetNumTexGens(0);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL,
                      GX_COLOR0A0);
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_C2, GX_CC_RASC,
                        GX_CC_ZERO);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A2, GX_CA_ZERO, GX_CA_ZERO,
                        GX_CA_ZERO);
        GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_C0, GX_CC_CPREV,
                        GX_CC_ZERO);
        GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_A0, GX_CA_APREV,
                        GX_CA_ZERO);
        break;

    case 0x0:
        GXSetNumTevStages(1);
        GXSetNumTexGens(0);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL,
                      GX_COLOR0A0);
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C0, GX_CC_ZERO, GX_CC_ZERO,
                        GX_CC_ZERO);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A0, GX_CA_ZERO, GX_CA_ZERO,
                        GX_CA_ZERO);
        break;

    case 0x80000480:
        GXSetNumTevStages(3);
        GXSetNumTexGens(1);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C1, GX_CC_C0, GX_CC_TEXC,
                        GX_CC_ZERO);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A1, GX_CA_A0, GX_CA_TEXA,
                        GX_CA_ZERO);
        GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_C2, GX_CC_CPREV,
                        GX_CC_ZERO);
        GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_A2, GX_CA_APREV,
                        GX_CA_ZERO);
        GXSetTevColorIn(GX_TEVSTAGE2, GX_CC_ZERO, GX_CC_RASC, GX_CC_CPREV,
                        GX_CC_ZERO);
        GXSetTevAlphaIn(GX_TEVSTAGE2, GX_CA_APREV, GX_CA_ZERO, GX_CA_ZERO,
                        GX_CA_ZERO);
        break;

    case 0x80000400:
        GXSetNumTevStages(2);
        GXSetNumTexGens(1);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_C2, GX_CC_RASC,
                        GX_CC_ZERO);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A2, GX_CA_ZERO, GX_CA_ZERO,
                        GX_CA_ZERO);
        GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_TEXC, GX_CC_CPREV,
                        GX_CC_ZERO);
        GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_TEXA, GX_CA_APREV,
                        GX_CA_ZERO);
        break;

    case 0x400:
    case 0x480:
        GXSetNumTevStages(1);
        GXSetNumTexGens(1);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C1, GX_CC_C0, GX_CC_TEXC,
                        GX_CC_ZERO);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A1, GX_CA_A0, GX_CA_TEXA,
                        GX_CA_ZERO);
        break;

    case 0x100400:
        GXSetNumTevStages(1);
        GXSetNumTexGens(1);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        GXSetTevOp(GX_TEVSTAGE0, GX_MODULATE);
        break;

    case 0x100480:
        GXSetNumTevStages(2);
        GXSetNumTexGens(1);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C1, GX_CC_C0, GX_CC_TEXC,
                        GX_CC_ZERO);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A1, GX_CA_A0, GX_CA_TEXA,
                        GX_CA_ZERO);
        GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_CPREV, GX_CC_ZERO, GX_CC_ZERO,
                        GX_CC_ZERO);
        GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_RASA, GX_CA_APREV,
                        GX_CA_ZERO);
        break;

    case 0x80100400:
        GXSetNumTevStages(2);
        GXSetNumTexGens(1);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_C2, GX_CC_RASC,
                        GX_CC_ZERO);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_A2, GX_CA_RASA,
                        GX_CA_ZERO);
        GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_TEXC, GX_CC_CPREV,
                        GX_CC_ZERO);
        GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_TEXA, GX_CA_APREV,
                        GX_CA_ZERO);
        break;

    case 0x80100480:
        GXSetNumTevStages(3);
        GXSetNumTexGens(1);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C1, GX_CC_C0, GX_CC_TEXC,
                        GX_CC_ZERO);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A1, GX_CA_A0, GX_CA_TEXA,
                        GX_CA_ZERO);
        GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_C2, GX_CC_CPREV,
                        GX_CC_ZERO);
        GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_A2, GX_CA_APREV,
                        GX_CA_ZERO);
        GXSetTevColorIn(GX_TEVSTAGE2, GX_CC_ZERO, GX_CC_RASC, GX_CC_CPREV,
                        GX_CC_ZERO);
        GXSetTevAlphaIn(GX_TEVSTAGE2, GX_CA_ZERO, GX_CA_RASA, GX_CA_APREV,
                        GX_CA_ZERO);
        break;

    case 0x100000:
        GXSetNumTevStages(1);
        GXSetNumTexGens(0);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL,
                      GX_COLOR0A0);
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO,
                        GX_CC_ZERO);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_RASA, GX_CA_ZERO, GX_CA_ZERO,
                        GX_CA_ZERO);
        break;

    case 0x80100000:
        GXSetNumTevStages(2);
        GXSetNumTexGens(0);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL,
                      GX_COLOR0A0);
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_C2, GX_CC_RASC,
                        GX_CC_ZERO);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_A2, GX_CA_RASA,
                        GX_CA_ZERO);
        GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_C0, GX_CC_CPREV,
                        GX_CC_ZERO);
        GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_A0, GX_CA_APREV,
                        GX_CA_ZERO);
        break;
    }
}
