#include "ftCo_09F4.h"

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
#include <sysdolphin/baselib/wobj.h>

#include "ftoverlay_lights.h"

HSD_LObj* ftOverlayLights;
HSD_LObj* ftOverlayOtherLights;

static HSD_WObjDesc overlay_light_position = {
    .pos = { 0.57f, 0.57f, 0.57f },
};

static HSD_LightDesc node0 = {
    NULL,           NULL, 0x0005, 0x0000, { 0xFF, 0xFF, 0xFF, 0xFF },
    &overlay_light_position, 0, 0,
};

static LightList node1 = { &node0, NULL };

static LightList* node2[] = { &node1, NULL };

void ftCo_8009F578(Fighter* fp)
{
    fp->x588 = lb_80011AC4(node2);
}

void ftCo_8009F5AC(Fighter* fp)
{
    ColorOverlay* data = ftCo_800C0658(fp);
    if (!data->x7C_flag2) {
        return;
    }
    if (fp->x2221_b3) {
        {
            Vec3 position;
            float angle_yz = MTXDegToRad(data->x78_light_rot_yz);
            float angle_x = MTXDegToRad(data->x74_light_rot_x) *
                            HSD_JObjGetRotationY(GET_JOBJ(fp->gobj)) / M_PI_2;
            position.y = -sinf(-angle_yz);
            position.z = cosf(-angle_yz);
            position.x = position.z * sinf(angle_x);
            position.z *= cosf(angle_x);
            HSD_LObjSetPosition(fp->x588, &position);
        }
        if (data->x7C_light_enable) {
            GXColor color = node0.color;
            color.a = data->x50_light_color.a;
            HSD_LObjSetColor(fp->x588, color);
            HSD_LObjSetFlags(fp->x588, LOBJ_ALPHA);
        } else {
            HSD_LObjSetColor(fp->x588, data->x50_light_color);
            HSD_LObjClearFlags(fp->x588, LOBJ_ALPHA);
        }
        fp->x2221_b3 = false;
    }
    if (data->x7C_light_enable) {
        HSD_LObj_803668EC(ftOverlayLights);
    } else {
        HSD_LObjSetCurrentAll(ftOverlayOtherLights);
    }
    HSD_LObjAddCurrent(fp->x588);
    HSD_LObjSetupInit(HSD_CObjGetCurrent());
}

void ftCo_8009F75C(Fighter* fp, bool set_alpha)
{
    ColorOverlay* data = ftCo_800C0658(fp);
    if (data->x7C_flag2 && data->x7C_light_enable) {
        if (set_alpha) {
            HSD_LObjSetFlags(fp->x588, LOBJ_ALPHA);
        } else {
            HSD_LObjClearFlags(fp->x588, LOBJ_ALPHA);
        }
        if (set_alpha) {
            HSD_LObj_803668EC(ftOverlayLights);
        } else {
            HSD_LObjSetCurrentAll(ftOverlayOtherLights);
        }
        HSD_LObjAddCurrent(fp->x588);
        HSD_LObjSetupInit(HSD_CObjGetCurrent());
    }
}

void ftCo_8009F7F8(Fighter* fp)
{
    if (ftCo_800C0658(fp)->x7C_flag2) {
        HSD_LObj_803668EC(ftOverlayLights);
        HSD_LObjSetupInit(HSD_CObjGetCurrent());
    }
}
