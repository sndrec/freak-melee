#include "initialize.h"

static HSD_RenderPass current_render_pass;
static int current_pix_fmt;
static int current_z_fmt = GX_ZC_MID;

HSD_RenderPass HSD_GetCurrentRenderPass(void)
{
    return current_render_pass;
}

void HSD_StartRender(HSD_RenderPass pass)
{
    GXRenderModeObj* rmode = HSD_VIGetRenderMode();
    current_render_pass = pass;
    if (rmode->aa) {
        GXSetPixelFmt(GX_PF_RGB565_Z16, current_z_fmt);
    } else {
        GXSetPixelFmt(current_pix_fmt, GX_ZC_LINEAR);
    }
    GXSetFieldMode(rmode->field_rendering, rmode->xfbHeight < rmode->viHeight);
}

void HSD_Init_803755A8(void)
{
    // Does nothing, but need to force a comparison to make this match
    if (current_render_pass == HSD_RP_OFFSCREEN) {
        current_render_pass == 0;
    }
}

