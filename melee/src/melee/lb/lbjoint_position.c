#include "lb_00B0.h"
#include <sysdolphin/baselib/jobj.h>

void lb_8000B1CC(HSD_JObj* arg0, Vec3* pos0, Vec3* pos1)
{
    Quaternion r;
    Vec3 s;

    if (arg0 == NULL) {
        *pos1 = *pos0;
        return;
    }
    if (arg0->parent != NULL) {
        HSD_JObjSetupMatrix(arg0);
        if (pos0 == NULL || (!pos0->x && !pos0->y && !pos0->z)) {
            pos1->x = arg0->mtx[0][3];
            pos1->y = arg0->mtx[1][3];
            pos1->z = arg0->mtx[2][3];
        } else {
            C_MTXMultVec(arg0->mtx, pos0, pos1);
        }
        return;
    }
    if (pos0 == NULL || (!pos0->x && !pos0->y && !pos0->z)) {
        HSD_JObjGetTranslation(arg0, pos1);
        return;
    }
    HSD_JObjGetRotation(arg0, &r);
    HSD_JObjGetScale(arg0, &s);
    if (!r.x && !r.y && !r.z && s.x == 1 && s.y == 1 && s.z == 1) {
        HSD_JObjGetTranslation(arg0, pos1);
        pos1->x += pos0->x;
        pos1->y += pos0->y;
        pos1->z += pos0->z;
        return;
    }
    HSD_JObjSetupMatrix(arg0);
    C_MTXMultVec(arg0->mtx, pos0, pos1);
}
