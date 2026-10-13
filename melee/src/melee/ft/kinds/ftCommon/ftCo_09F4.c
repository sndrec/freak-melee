#include "ftCo_09F4.h"
#include "ftoverlay_lights.h"

#include <Runtime/platform.h>

#include <math.h>

#include <melee/ft/ftdevice.h>
#include <melee/gr/ground.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lbspdisplay.h>
#include <melee/sc/types.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/lobj.h>

/* 09F480 */ static void ftCo_8009F480(Fighter_GObj* gobj);
/* 09F54C */ static void ftCo_8009F54C(HSD_GObj* gobj, int code);



void ftCo_8009F480(Fighter_GObj* gobj)
{
    HSD_LObjAnimAll(gobj->hsd_obj);
}

void ftCo_8009F4A4(void)
{
    HSD_GObj* gobj = GObj_Create(12, 3, 0);
    HSD_LObj* lobj = lb_80011AC4(Ground_801C49B4());
    ftOverlayLights = lobj;
    ftOverlayOtherLights = lb_8000CDC0(lobj);
    Ground_801C2374(lobj);
    HSD_GObjObject_80390A70(gobj, HSD_GObj_LightKind, lobj);
    GObj_SetupGXLink(gobj, ftCo_8009F54C, 4U, 0U);
    HSD_LObjReqAnimAll(lobj, 0.0F);
    HSD_GObj_SetupProc(gobj, ftCo_8009F480, 1);
}

void ftCo_8009F54C(HSD_GObj* gobj, int code)
{
    HSD_LObj_803668EC(gobj->hsd_obj);
    HSD_LObjSetupInit(HSD_CObjGetCurrent());
}
