#include "lb_00B0.h"
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/quatlib.h>

void lb_8000C490(HSD_JObj* jobj1, HSD_JObj* jobj2, HSD_JObj* arg2, float arg8,
                 float arg9)
{
    float dx;
    float dy;
    float dz;
    float dw;
    s32 is_quat_1;
    s32 is_quat_2;

    Vec3 sp60;
    Vec3 sp54;
    Quaternion quat1;
    Quaternion quat2;

    float sx;
    float sy;
    float sz;
    float sw;

    float sum_square_diffs;
    float sum_square_sums;

    arg2->translate.x =
        (jobj1->translate.x * arg8) + (jobj2->translate.x * arg9);
    arg2->translate.y =
        (jobj1->translate.y * arg8) + (jobj2->translate.y * arg9);
    arg2->translate.z =
        (jobj1->translate.z * arg8) + (jobj2->translate.z * arg9);
    arg2->scale.x = (jobj1->scale.x * arg8) + (jobj2->scale.x * arg9);
    arg2->scale.y = (jobj1->scale.y * arg8) + (jobj2->scale.y * arg9);
    arg2->scale.z = (jobj1->scale.z * arg8) + (jobj2->scale.z * arg9);
    is_quat_1 = HSD_JObjGetFlags(jobj1) & 0x20000;
    is_quat_2 = HSD_JObjGetFlags(jobj2) & 0x20000;
    if (!is_quat_1 && !is_quat_2) {
        dx = jobj1->rotate.x - jobj2->rotate.x;
        dy = jobj1->rotate.y - jobj2->rotate.y;
        dz = jobj1->rotate.z - jobj2->rotate.z;
        if (ABS(dx) <= 0.0001f && ABS(dy) <= 0.0001f && ABS(dz) <= 0.0001f) {
            arg2->rotate = jobj1->rotate;
            HSD_JObjClearFlags(arg2, 0x20000);
            HSD_JObjSetFlags(arg2, 0x40);
            return;
        }
    }
    if (is_quat_1) {
        quat1 = jobj1->rotate;
    } else {
        sp60.x = jobj1->rotate.x;
        sp60.y = jobj1->rotate.y;
        sp60.z = jobj1->rotate.z;
        EulerToQuat(&sp60, &quat1);
    }
    if (is_quat_2) {
        quat2 = jobj2->rotate;
    } else {
        sp54.x = jobj2->rotate.x;
        sp54.y = jobj2->rotate.y;
        sp54.z = jobj2->rotate.z;
        EulerToQuat(&sp54, &quat2);
    }

    sx = SQ(quat1.x + quat2.x);
    sy = SQ(quat1.y + quat2.y);
    sz = SQ(quat1.z + quat2.z);
    sw = SQ(quat1.w + quat2.w);

    dx = SQ(quat1.x - quat2.x);
    dy = SQ(quat1.y - quat2.y);
    dz = SQ(quat1.z - quat2.z);
    dw = SQ(quat1.w - quat2.w);

    sum_square_sums = sx + sy + sz + sw;
    sum_square_diffs = dx + dy + dz + dw;

    if (sum_square_diffs > sum_square_sums) {
        quat2.x = -quat2.x;
        quat2.y = -quat2.y;
        quat2.z = -quat2.z;
        quat2.w = -quat2.w;
    }
    HSD_QuatLib_8037EF28(&quat1, &quat2, &arg2->rotate, arg9);
    HSD_JObjSetFlags(arg2, 0x20000U);
    HSD_JObjSetFlags(arg2, 0x40U);
}

void lbCopyJObjSRT(HSD_JObj* src, HSD_JObj* dst)
{
    dst->rotate = src->rotate;
    dst->scale = src->scale;
    dst->translate = src->translate;
    if (HSD_JObjGetFlags(src) & JOBJ_USE_QUATERNION) {
        HSD_JObjSetFlags(dst, JOBJ_USE_QUATERNION);
    } else {
        HSD_JObjClearFlags(dst, JOBJ_USE_QUATERNION);
    }
    HSD_JObjSetFlags(dst, JOBJ_MTX_DIRTY);
}

void lb_8000B4FC(HSD_JObj* jobj, HSD_Joint* joint)
{
    if (jobj == NULL || joint == NULL) {
        return;
    }
    jobj->rotate.x = joint->rotation.x;
    jobj->rotate.y = joint->rotation.y;
    jobj->rotate.z = joint->rotation.z;
    jobj->scale = joint->scale;
    jobj->translate = joint->position;
    HSD_JObjClearFlags(jobj, JOBJ_USE_QUATERNION);
    if (!(jobj->flags & JOBJ_MTX_INDEP_SRT)) {
        HSD_JObjSetMtxDirty(jobj);
    }
}

void lb_8000B5DC(HSD_JObj* jobj, HSD_Joint* joint)
{
    if (jobj == NULL || joint == NULL) {
        return;
    }
    jobj->rotate.x = joint->rotation.x;
    jobj->rotate.y = joint->rotation.y;
    jobj->rotate.z = joint->rotation.z;
    jobj->translate = joint->position;
    HSD_JObjClearFlags(jobj, JOBJ_USE_QUATERNION);
    if (!(jobj->flags & JOBJ_MTX_INDEP_SRT)) {
        HSD_JObjSetMtxDirty(jobj);
    }
}

void lb_8000B6A4(HSD_JObj* jobj, HSD_Joint* joint)
{
    if (jobj == NULL || joint == NULL) {
        return;
    }
    jobj->scale = joint->scale;
    jobj->translate = joint->position;
    if (!(jobj->flags & JOBJ_MTX_INDEP_SRT)) {
        HSD_JObjSetMtxDirty(jobj);
    }
}

void lb_8000B760(HSD_JObj* jobj, HSD_Joint* joint)
{
    if (jobj == NULL || joint == NULL) {
        return;
    }
    jobj->translate = joint->position;
    if (!(jobj->flags & JOBJ_MTX_INDEP_SRT)) {
        HSD_JObjSetMtxDirty(jobj);
    }
}

