#include "video.h"

#include <dolphin/gx.h>
#include <dolphin/os.h>
#include <dolphin/vi.h>

#define _p ((HSD_VIInfo*) &HSD_VIData)

static int HSD_VISearchXFBByStatus(HSD_VIXFBDrawDispStatus status)
{
    int i;

    for (i = 0; i < HSD_VI_XFB_MAX; i++) {
        if (_p->xfb[i].status == status) {
            return i;
        }
    }
    return -1;
}

HSD_VIRetraceCallback HSD_VISetUserPreRetraceCallback(HSD_VIRetraceCallback cb)
{
    HSD_VIRetraceCallback old = _p->pre_cb;

    _p->pre_cb = cb;

    return old;
}

HSD_VIRetraceCallback
HSD_VISetUserPostRetraceCallback(HSD_VIRetraceCallback cb)
{
    HSD_VIRetraceCallback old = _p->post_cb;

    _p->post_cb = cb;

    return old;
}

static int HSD_VIWaitXFBFlush_sub(void)
{
    int val;

    val = (HSD_VISearchXFBByStatus(HSD_VI_XFB_WAITDONE) != -1 ||
           HSD_VISearchXFBByStatus(HSD_VI_XFB_DRAWDONE) != -1 ||
           HSD_VISearchXFBByStatus(HSD_VI_XFB_NEXT) != -1)
              ? 1
              : 0;

    return val;
}

void HSD_VIWaitXFBFlushNoYield(void)
{
    if (HSD_VIGetNbXFB() < 2) {
        return;
    }

    while (HSD_VIWaitXFBFlush_sub()) {
    }
}

int HSD_VIGetXFBLastDrawDone(void)
{
    int idx = -1;

    if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_WAITDONE)) == -1) {
        if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_DRAWDONE)) == -1) {
            if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_NEXT)) == -1) {
                idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_DISPLAY);
            }
        }
    }

    return idx;
}
