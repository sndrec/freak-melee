#include "ftparts.h"
#include "fighter.h"
#include "types.h"
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/debug.h>

void ftParts_JObjSetRotation(HSD_JObj* jobj, Vec4* quat)
{
    HSD_JObjSetRotation(jobj, quat);
    HSD_JObjClearFlags(jobj, JOBJ_USE_QUATERNION);
}

void ftPartSetRotX(Fighter* fp, int part_idx, f32 rotate_x)
{
    HSD_JObj* jobj = fp->parts[part_idx].joint;
    if (HSD_JObjGetFlags(jobj) & JOBJ_USE_QUATERNION) {
        HSD_JObj* jobj2 = fp->parts[part_idx].x4_jobj2;
        if (HSD_JObjGetFlags(jobj2) & JOBJ_USE_QUATERNION) {
            HSD_ASSERTREPORT(1120, 0, "cant set fighter rot x!\n");
        }
        HSD_JObjSetRotationX(jobj2, rotate_x);
    } else {
        HSD_JObjSetRotationX(jobj, rotate_x);
    }
}

void ftPartSetRotY(Fighter* fp, int part_idx, f32 rotate_y)
{
    HSD_JObj* jobj = fp->parts[part_idx].joint;
    if (HSD_JObjGetFlags(jobj) & JOBJ_USE_QUATERNION) {
        HSD_JObj* jobj2 = fp->parts[part_idx].x4_jobj2;
        if (HSD_JObjGetFlags(jobj2) & JOBJ_USE_QUATERNION) {
            HSD_ASSERTREPORT(1139, 0, "cant set fighter rot y!\n");
        }
        HSD_JObjSetRotationY(jobj2, rotate_y);
    } else {
        HSD_JObjSetRotationY(jobj, rotate_y);
    }
}

void ftPartSetRotZ(Fighter* arg0, int part_idx, f32 rotate_z)
{
    HSD_JObj* jobj;
    HSD_JObj* jobj2;

    jobj = arg0->parts[part_idx].joint;
    if (HSD_JObjGetFlags(jobj) & JOBJ_USE_QUATERNION) {
        jobj2 = arg0->parts[part_idx].x4_jobj2;
        if (HSD_JObjGetFlags(jobj2) & JOBJ_USE_QUATERNION) {
            HSD_ASSERTREPORT(1158, 0, "cant set fighter rot z!\n");
        }
        HSD_JObjSetRotationZ(jobj2, rotate_z);
    } else {
        HSD_JObjSetRotationZ(jobj, rotate_z);
    }
}

f32 ftPartGetRotX(Fighter* fp, int part_idx)
{
    HSD_JObj* jobj = fp->parts[part_idx].joint;
    if (HSD_JObjGetFlags(jobj) & JOBJ_USE_QUATERNION) {
        HSD_JObj* jobj = fp->parts[part_idx].x4_jobj2;
        if (HSD_JObjGetFlags(jobj) & JOBJ_USE_QUATERNION) {
            HSD_ASSERTREPORT(1177, 0, "cant get fighter rot x!\n");
        }
        return HSD_JObjGetRotationX(jobj);
    }
    return HSD_JObjGetRotationX(jobj);
}

f32 ftPartGetRotY(Fighter* fp, int part_idx)
{
    HSD_JObj* jobj = fp->parts[part_idx].joint;
    if (HSD_JObjGetFlags(jobj) & JOBJ_USE_QUATERNION) {
        HSD_JObj* jobj = fp->parts[part_idx].x4_jobj2;
        if (HSD_JObjGetFlags(jobj) & JOBJ_USE_QUATERNION) {
            HSD_ASSERTREPORT(1196, 0, "cant get fighter rot y!\n");
        }
        return HSD_JObjGetRotationY(jobj);
    }
    return HSD_JObjGetRotationY(jobj);
}

