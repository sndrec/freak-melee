#include "native_model_joint.h"

#ifdef MELEE_NATIVE
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/quatlib.h>
#include <math.h>

HSD_JObj* nativeModelJoint_Resolve(HSD_JObj* root, const char* path)
{
    if (path == NULL) return NULL;
    HSD_JObj* joint = root;
    unsigned depth = 0;
    while (*path != '\0') {
        if (++depth > 12 || joint == NULL || *path < '0' || *path > '9')
            return NULL;
        unsigned ordinal = 0;
        do {
            ordinal = ordinal * 10 + (unsigned) (*path++ - '0');
            if (ordinal > 255) return NULL;
        } while (*path >= '0' && *path <= '9');
        joint = joint->child;
        while (ordinal-- > 0 && joint != NULL) joint = joint->next;
        if (*path == '\0') break;
        if (*path++ != '/' || *path == '\0') return NULL;
    }
    return joint;
}

float nativeModelJoint_Read(HSD_JObj* joint, unsigned channel)
{
    if (channel == 9) return joint != NULL;
    if (channel == 10 || channel == 15)
        return joint != NULL && !(joint->flags & JOBJ_HIDDEN);
    if (channel >= 11 && channel <= 14) {
        if (joint == NULL) return channel == 14 ? 1.0f : 0.0f;
        Quaternion rotation;
        HSD_JObjGetRotation(joint, &rotation);
        switch (channel) {
        case 11: return rotation.x;
        case 12: return rotation.y;
        case 13: return rotation.z;
        default: return rotation.w;
        }
    }
    if (joint == NULL || channel >= 9) return channel >= 6 ? 1.0f : 0.0f;
    if (channel >= 3 && channel < 6 && (joint->flags & JOBJ_USE_QUATERNION))
        return 0;
    switch (channel) {
    case 0: return joint->translate.x;
    case 1: return joint->translate.y;
    case 2: return joint->translate.z;
    case 3: return joint->rotate.x;
    case 4: return joint->rotate.y;
    case 5: return joint->rotate.z;
    case 6: return joint->scale.x;
    case 7: return joint->scale.y;
    default: return joint->scale.z;
    }
}

void nativeModelJoint_SetRoll(HSD_JObj* joint, float radians)
{
    if (!(joint->flags & JOBJ_USE_QUATERNION)) {
        HSD_JObjSetRotationZ(joint, radians);
        return;
    }
    Quaternion q = joint->rotate;
    float scale = fmaxf(fmaxf(fabsf(q.x), fabsf(q.y)), fmaxf(fabsf(q.z), fabsf(q.w)));
    Vec3 angles = {0};
    if (scale > 0 && isfinite(scale)) {
        q = (Quaternion){q.x / scale, q.y / scale, q.z / scale, q.w / scale};
        Mtx matrix;
        C_MTXQuat(matrix, &q);
        HSD_QuatLib_8037EB28(matrix, &angles);
    }
    angles.z = radians;
    EulerToQuat(&angles, &q);
    HSD_JObjSetRotation(joint, &q);
}

void nativeModelJoint_Write(HSD_JObj* joint, unsigned channel, float value)
{
    if (!isfinite(value)) return;
    if (joint == NULL) return;
    if (channel == 10) {
        if (value != 0) HSD_JObjClearFlagsAll(joint, JOBJ_HIDDEN);
        else HSD_JObjSetFlagsAll(joint, JOBJ_HIDDEN);
        return;
    }
    if (channel == 15) {
        if (value != 0) HSD_JObjClearFlags(joint, JOBJ_HIDDEN);
        else HSD_JObjSetFlags(joint, JOBJ_HIDDEN);
        return;
    }
    if (channel >= 11 && channel <= 14) {
        Quaternion rotation;
        HSD_JObjGetRotation(joint, &rotation);
        switch (channel) {
        case 11: rotation.x = value; break;
        case 12: rotation.y = value; break;
        case 13: rotation.z = value; break;
        case 14: rotation.w = value; break;
        }
        HSD_JObjSetRotation(joint, &rotation);
        return;
    }
    if (channel >= 3 && channel < 6 && (joint->flags & JOBJ_USE_QUATERNION))
        return;
    switch (channel) {
    case 0: HSD_JObjSetTranslateX(joint, value); break;
    case 1: HSD_JObjSetTranslateY(joint, value); break;
    case 2: HSD_JObjSetTranslateZ(joint, value); break;
    case 3: HSD_JObjSetRotationX(joint, value); break;
    case 4: HSD_JObjSetRotationY(joint, value); break;
    case 5: HSD_JObjSetRotationZ(joint, value); break;
    case 6: HSD_JObjSetScaleX(joint, value); break;
    case 7: HSD_JObjSetScaleY(joint, value); break;
    case 8: HSD_JObjSetScaleZ(joint, value); break;
    }
}
#endif
