#include "ftanim.h"
#include "types.h"
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/debug.h>

// Ancestors belong to one synchronous walk, never to retained match state.
// Callers start at depth zero; every occupied entry is written before reading.
void ftAnim_GetNextAnimJointInTree(HSD_AnimJoint** panimjoint, int* pdepth,
                                 HSD_AnimJoint* ancestors[30])
{
    HSD_AnimJoint* temp_r28 = *panimjoint;
    HSD_AnimJoint* var_r0;
    int i = *pdepth;

    if (temp_r28->child != NULL) {
        ancestors[i++] = temp_r28;
        if (i >= 30) {
            HSD_ASSERTREPORT(46, 0, "fighter joint depth num over!\n");
        }
        var_r0 = temp_r28->child;
    } else {
        if (temp_r28->next != NULL) {
            var_r0 = temp_r28->next;
        } else {
            while (1) {
                if (i == 0) {
                    var_r0 = NULL;
                    break;
                }
                if (ancestors[i - 1]->next != NULL) {
                    var_r0 = ancestors[--i]->next;
                    break;
                }
                i--;
            }
        }
    }
    *panimjoint = var_r0;
    *pdepth = i;
}

void ftAnim_GetNextMatAnimJointInTree(HSD_MatAnimJoint** pjoint, int* pdepth,
                                    HSD_MatAnimJoint* ancestors[30])
{
    HSD_MatAnimJoint* temp_r28;
    HSD_MatAnimJoint* var_r0;
    s32 i;

    temp_r28 = *pjoint;
    i = *pdepth;
    if (temp_r28->child != NULL) {
        ancestors[i++] = temp_r28;
        if (i >= 30) {
            HSD_ASSERTREPORT(87, 0, "fighter joint depth num over!\n");
        }
        var_r0 = temp_r28->child;
    } else {
        if (temp_r28->next != NULL) {
            var_r0 = temp_r28->next;
        } else {
            while (1) {
                if (i == 0) {
                    var_r0 = NULL;
                    break;
                }
                if (ancestors[i - 1]->next != 0U) {
                    var_r0 = ancestors[--i]->next;
                    break;
                }
                i--;
            }
        }
    }
    *pjoint = var_r0;
    *pdepth = i;
}

void ftAnim_GetNextJointInTree(HSD_Joint** pjoint, s32* pdepth,
                             HSD_Joint* ancestors[30])
{
    HSD_Joint* temp_r28;
    HSD_Joint* var_r0;
    int i;

    temp_r28 = *pjoint;
    i = *pdepth;
    if (temp_r28->child != NULL) {
        ancestors[i++] = temp_r28;
        if (i >= 30) {
            HSD_ASSERTREPORT(128, 0, "fighter joint depth num over!\n");
        }
        var_r0 = temp_r28->child;
    } else {
        if (temp_r28->next != NULL) {
            var_r0 = temp_r28->next;
        } else {
            while (1) {
                if (i == 0) {
                    var_r0 = NULL;
                    break;
                }
                if (ancestors[i - 1]->next != 0U) {
                    var_r0 = ancestors[--i]->next;
                    break;
                }
                i--;
            }
        }
    }
    *pjoint = var_r0;
    *pdepth = i;
}

