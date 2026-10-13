#include "lb_00B0.h"

#include <dolphin/mtx.h>
#include <melee/sc/types.h> // IWYU pragma: keep
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/dobj.h> // IWYU pragma: keep
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/quatlib.h>
#include <sysdolphin/baselib/robj.h>

/* 00B9D8 */ static void lb_8000B9D8(HSD_JObj*, float**, s32);
/* 00BECC */ static HSD_AnimJoint* lb_8000BECC(HSD_AnimJoint* animjoint);


bool lb_8000B074(HSD_JObj* jobj)
{
    HSD_AObj* aobj = jobj->aobj;

    if (aobj != NULL && !(aobj->flags & AOBJ_NO_ANIM)) {
        return true;
    }

    return false;
}

bool lb_8000B09C(HSD_JObj* jobj)
{
    while (jobj != NULL) {
        if (jobj->aobj != NULL && !(jobj->aobj->flags & AOBJ_NO_ANIM)) {
            return true;
        }
        if ((jobj->child == NULL && jobj->next == NULL) ||
            (jobj->flags & JOBJ_INSTANCE))
        {
            while (true) {
                if (jobj->parent == NULL) {
                    jobj = NULL;
                    break;
                }
                if (jobj->parent->next != NULL) {
                    jobj = jobj->parent->next;
                    break;
                }
                jobj = jobj->parent;
            }
        } else if (jobj->child != NULL) {
            jobj = jobj->child;
        } else {
            jobj = jobj->next;
        }
    }
    return false;
}

bool lb_8000B134(HSD_JObj* jobj)
{
    while (jobj != NULL) {
        if (jobj->aobj != NULL && (jobj->aobj->flags & AOBJ_REWINDED)) {
            return true;
        }
        if ((jobj->child == NULL && jobj->next == NULL) ||
            (jobj->flags & JOBJ_INSTANCE))
        {
            while (true) {
                if (jobj->parent == NULL) {
                    jobj = NULL;
                    break;
                }
                if (jobj->parent->next != NULL) {
                    jobj = jobj->parent->next;
                    break;
                }
                jobj = jobj->parent;
            }
        } else if (jobj->child != NULL) {
            jobj = jobj->child;
        } else {
            jobj = jobj->next;
        }
    }
    return false;
}

void lb_8000B804(HSD_JObj* jobj, HSD_Joint* joint)
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
    HSD_JObjSetFlags(jobj, JOBJ_MTX_DIRTY);

    lb_8000B804(jobj->next, joint->next);
    lb_8000B804(jobj->child, joint->child);
}

static void lb_8000B9D8(HSD_JObj* jobj, float** arg1, s32 arg2)
{
    float* tmp = *arg1;
    if (arg2 != 0) {
        HSD_AObjSetRate(jobj->aobj, *tmp);
    }
}

void lb_8000BA0C(HSD_JObj* jobj, float arg8)
{
    float* sp10 = &arg8;
    HSD_JObjWalkTree(jobj, lb_8000B9D8, &sp10);
}

void lbDObjSetRateAll(HSD_DObj* dobj, float val)
{
    if (dobj == NULL) {
        return;
    }
    if (dobj->pobj != NULL) {
        HSD_PObj* cur;
        for (cur = dobj->pobj; cur != NULL; cur = cur->next) {
            if (pobj_type(cur) == POBJ_SHAPEANIM &&
                cur->u.shape_set->aobj != NULL)
            {
                HSD_AObjSetRate(cur->u.shape_set->aobj, val);
            }
        }
    }
    if (dobj->mobj != NULL) {
        if (dobj->mobj->aobj != NULL) {
            HSD_AObjSetRate(dobj->mobj->aobj, val);
        }
        if (dobj->mobj->tobj != NULL) {
            HSD_TObj* cur;
            for (cur = dobj->mobj->tobj; cur != NULL; cur = cur->next) {
                if (cur->aobj != NULL) {
                    HSD_AObjSetRate(cur->aobj, val);
                }
            }
        }
    }
}

/// (exact copy of above, but with HSD_AObjReqAnim)
void lbDObjReqAnimAll(HSD_DObj* dobj, float val)
{
    if (dobj == NULL) {
        return;
    }
    if (dobj->pobj != NULL) {
        HSD_PObj* cur;
        for (cur = dobj->pobj; cur != NULL; cur = cur->next) {
            if (pobj_type(cur) == POBJ_SHAPEANIM &&
                cur->u.shape_set->aobj != NULL)
            {
                HSD_AObjReqAnim(cur->u.shape_set->aobj, val);
            }
        }
    }
    if (dobj->mobj != NULL) {
        if (dobj->mobj->aobj != NULL) {
            HSD_AObjReqAnim(dobj->mobj->aobj, val);
        }
        if (dobj->mobj->tobj != NULL) {
            HSD_TObj* cur;
            for (cur = dobj->mobj->tobj; cur != NULL; cur = cur->next) {
                if (cur->aobj != NULL) {
                    HSD_AObjReqAnim(cur->aobj, val);
                }
            }
        }
    }
}

static HSD_AnimJoint* lb_8000BECC(HSD_AnimJoint* animjoint)
{
    HSD_AnimJoint* tmp;
    if (animjoint == NULL) {
        return NULL;
    }
    if (animjoint->aobjdesc != NULL) {
        return animjoint;
    }
    tmp = lb_8000BECC(animjoint->child);
    if (tmp != NULL) {
        return tmp;
    }
    tmp = lb_8000BECC(animjoint->next);
    if (tmp != NULL) {
        return tmp;
    }
    return NULL;
}

float lb_8000BFF0(HSD_AnimJoint* animjoint)
{
    animjoint = lb_8000BECC(animjoint);
    if (animjoint != NULL) {
        return animjoint->aobjdesc->end_frame;
    }
    return 0;
}

void lb_8000C07C(HSD_JObj* jobj, s32 i, HSD_AnimJoint** arg3,
                 HSD_MatAnimJoint** arg4, HSD_ShapeAnimJoint** arg5)
{
    HSD_AnimJoint* phi_r4;
    HSD_MatAnimJoint* phi_r5;
    HSD_ShapeAnimJoint* phi_r6;

    if (arg3 != NULL) {
        phi_r4 = arg3[i];
    } else {
        phi_r4 = NULL;
    }
    if (arg4 != NULL) {
        phi_r5 = arg4[i];
    } else {
        phi_r5 = NULL;
    }
    if (arg5 != NULL) {
        phi_r6 = arg5[i];
    } else {
        phi_r6 = NULL;
    }
    HSD_JObjAddAnimAll(jobj, phi_r4, phi_r5, phi_r6);
}

void lb_8000C0E8(HSD_JObj* jobj, s32 i, DynamicModelDesc* arg2)
{
    lb_8000C07C(jobj, i, arg2->anims, arg2->matanims, arg2->shapeanims);
}

void lb_8000C1C0(HSD_JObj* jobj, HSD_JObj* constraint)
{
    HSD_RObj* robj = HSD_RObjAlloc();
    HSD_RObjSetFlags(robj, 0x90000001);
    HSD_RObjSetConstraintObj(robj, constraint);
    HSD_JObjPrependRObj(jobj, robj);
}

void lb_8000C228(HSD_JObj* jobj, HSD_JObj* constraint)
{
    HSD_RObj* robj = HSD_RObjAlloc();
    HSD_RObjSetFlags(robj, 0x90000002);
    HSD_RObjSetConstraintObj(robj, constraint);
    HSD_JObjPrependRObj(jobj, robj);
}

void lb_8000C290(HSD_JObj* jobj, HSD_JObj* constraint)
{
    HSD_RObj* robj = HSD_RObjAlloc();
    HSD_RObjSetFlags(robj, 0x90000004);
    HSD_RObjSetConstraintObj(robj, constraint);
    HSD_JObjPrependRObj(jobj, robj);
}

void lb_8000C2F8(HSD_JObj* jobj, HSD_JObj* constraint)
{
    lb_8000C1C0(jobj, constraint);
    lb_8000C290(jobj, constraint);
}

static inline HSD_RObj* robj_next(HSD_RObj* robj)
{
    if (robj != NULL) {
        return robj->next;
    }
    return NULL;
}

void lb_8000C390(HSD_JObj* jobj)
{
    HSD_RObj* next;
    HSD_RObj* cur = HSD_JObjGetRObj(jobj);

    while (true) {
        cur = HSD_RObjGetByType(cur, REFTYPE_JOBJ, 0);

        if (cur == NULL) {
            break;
        }

        next = robj_next(cur);
        HSD_JObjDeleteRObj(jobj, cur);
        HSD_RObjRemove(cur);
        cur = next;
    }
}

void lb_8000C420(HSD_JObj* jobj, u32 flags, float limit)
{
    HSD_RObj* robj = HSD_RObjAlloc();
    HSD_RObjSetFlags(robj, flags | 0xA0000000);
    if (robj != NULL) {
        robj->u.limit = limit;
    }
    HSD_JObjPrependRObj(jobj, robj);
}

void lb_8000C868(HSD_Joint* arg0, HSD_JObj* arg1, HSD_JObj* arg2, float arg8,
                 float arg9)
{
    Vec3 spC0;
    Vec3 spB4;
    Quaternion spA4;
    Quaternion sp94;
    Quaternion sum;
    Quaternion dif;
    float temp_f0;
    float temp_f10;
    float temp_f1;
    float temp_f2;
    float temp_f2_2;
    float temp_f3;
    float temp_f3_2;
    float temp_f4;
    float temp_f5;
    float temp_f5_2;
    float temp_f6_2;
    float temp_f8;
    float* temp_r3;
    float* temp_r3_2;
    s32 temp_r31;
    float phi_f1;
    float phi_f1_2;
    float phi_f1_3;

    arg2->translate.x = (arg0->position.x * arg8) + (arg1->translate.x * arg9);
    arg2->translate.y = (arg0->position.y * arg8) + (arg1->translate.y * arg9);
    arg2->translate.z = (arg0->position.z * arg8) + (arg1->translate.z * arg9);
    arg2->scale.x = (arg0->scale.x * arg8) + (arg1->scale.x * arg9);
    arg2->scale.y = (arg0->scale.y * arg8) + (arg1->scale.y * arg9);
    arg2->scale.z = (arg0->scale.z * arg8) + (arg1->scale.z * arg9);
    temp_r31 = HSD_JObjGetFlags(arg1) & 0x20000;
    if (temp_r31 == 0) {
        temp_f5 = arg0->rotation.x - arg1->rotate.x;
        temp_f3 = arg0->rotation.y - arg1->rotate.y;
        temp_f2 = arg0->rotation.z - arg1->rotate.z;
        phi_f1 = temp_f5 < 0 ? -temp_f5 : temp_f5;
        if (phi_f1 <= 1e-4F) {
            phi_f1_2 = temp_f3 < 0 ? -temp_f3 : temp_f3;
            if (phi_f1_2 <= 1e-4F) {
                phi_f1_3 = temp_f2 < 0 ? -temp_f2 : temp_f2;
                if (phi_f1_3 <= 1e-4F) {
                    arg2->rotate.x = arg0->rotation.x;
                    arg2->rotate.y = arg0->rotation.y;
                    arg2->rotate.z = arg0->rotation.z;
                    HSD_JObjClearFlags(arg2, 0x20000U);
                    HSD_JObjSetFlags(arg2, 0x40U);
                    return;
                }
            }
        }
    }
    spC0.x = arg0->rotation.x;
    spC0.y = arg0->rotation.y;
    spC0.z = arg0->rotation.z;
    EulerToQuat(&spC0, &spA4);
    if (temp_r31 != 0) {
        sp94 = arg1->rotate;
    } else {
        spB4.x = arg1->rotate.x;
        spB4.y = arg1->rotate.y;
        spB4.z = arg1->rotate.z;
        EulerToQuat(&spB4, (&sp94));
    }

    sum.x = SQ(spA4.x + sp94.x);
    sum.y = SQ(spA4.y + sp94.y);
    sum.z = SQ(spA4.z + sp94.z);
    sum.w = SQ(spA4.w + sp94.w);

    dif.x = SQ(spA4.x - sp94.x);
    dif.y = SQ(spA4.y - sp94.y);
    dif.z = SQ(spA4.z - sp94.z);
    dif.w = SQ(spA4.w - sp94.w);

    if (dif.x + dif.y + dif.z + dif.w > sum.x + sum.y + sum.z + sum.w) {
        sp94.x = -sp94.x;
        sp94.y = -sp94.y;
        sp94.z = -sp94.z;
        sp94.w = -sp94.w;
    }
    HSD_QuatLib_8037EF28(&spA4, &sp94, &arg2->rotate, arg9);
    HSD_JObjSetFlags(arg2, 0x20000U);
    HSD_JObjSetFlags(arg2, 0x40U);
}

static inline HSD_LObj* lobj_next(HSD_LObj* lobj)
{
    if (lobj == NULL) {
        return NULL;
    }
    return lobj->next;
}

HSD_LObj* lb_8000CDC0(HSD_LObj* cur)
{
    while (cur != NULL) {
        if (!(cur->flags & ((1 << 0) | (1 << 1))) &&
            !(HSD_LObjGetFlags(cur) & (1 << 5)))
        {
            return cur;
        }
        cur = lobj_next(cur);
    }
}

void lb_8000CE30(HSD_DObj* dobj, HSD_DObj* next)
{
    if (dobj != NULL) {
        dobj->next = next;
    }
}

void lb_8000CE40(HSD_JObj* jobj, HSD_DObj* dobj)
{
    if (jobj != NULL) {
        jobj->u.dobj = dobj;
    }
}
