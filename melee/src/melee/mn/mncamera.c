#include "mncamera.h"
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/gobj.h>
#include <math.h>

// Main menu camera orbit, shared with the native hub. The first C-stick outside
// the original .4 deadzone controls pitch/yaw, up to thirty degrees per axis.
void mnCamera_Tick(HSD_GObj* object)
{
    HSD_CObj* camera = object->hsd_obj;
    const MnCamera* data = object->user_data;
    float x = 0, y = 0, pitch = 0, yaw = 0;
    for (unsigned port = 0; port < 4; ++port) {
        x = HSD_PadCopyStatus[port].nml_subStickX;
        y = HSD_PadCopyStatus[port].nml_subStickY;
        if (!(x > -.4f && x < .4f && y > -.4f && y < .4f)) break;
    }
    if (x <= -.4f || x >= .4f) yaw = 30 * ((x - copysignf(.4f, x)) / .6f);
    if (y <= -.4f || y >= .4f) pitch = 30 * ((y - copysignf(.4f, y)) / .6f);
    HSD_CObjInit(camera, data->rest);
    Vec3 up = { 0, 1, 0 };
    Vec3 forward, right, interest, eye;
    Mtx rotation;
    HSD_CObjGetEyeVector(camera, &forward);
    HSD_CObjGetInterest(camera, &interest);
    C_VECCrossProduct(&up, &forward, &right);
    PSVECNormalize(&right, &right);
    C_MTXRotAxisRad(rotation, &right, .017453292f * pitch);
    C_MTXMultVec(rotation, &forward, &forward);
    C_MTXRotAxisRad(rotation, &up, .017453292f * yaw);
    C_MTXMultVec(rotation, &forward, &forward);
    C_VECScale(&forward, &forward, HSD_CObjGetEyeDistance(camera));
    C_VECSubtract(&interest, &forward, &eye);
    HSD_CObjSetEyePosition(camera, &eye);
}
