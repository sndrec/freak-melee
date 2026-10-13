#include "video.h"
#include <dolphin/vi.h>
#include <aurora/aurora.h>

HSD_VIInfo HSD_VIData;

void HSD_VIInit(HSD_VIStatus* vi)
{
    // Native screen presentation belongs to Aurora; there is no CPU XFB queue.
    HSD_VIData = (HSD_VIInfo) { 0 };
    HSD_VIData.current.vi = *vi;
    HSD_VIData.efb.vi_all = HSD_VIData.current;
    HSD_VIData.perf.frame_period = 60;
    VIConfigure(&HSD_VIData.current.vi.rmode);
    GXSetCopyClear(vi->clear_clr, vi->clear_z);
    aurora_set_video_black(vi->black != 0);
}

void HSD_VISetConfigure(GXRenderModeObj* rmode)
{
    HSD_VIData.current.vi.rmode = *rmode;
    VIConfigure(&HSD_VIData.current.vi.rmode);
}

void HSD_VISetBlack(bool black)
{
    HSD_VIData.current.vi.black = black;
    aurora_set_video_black(black);
}
