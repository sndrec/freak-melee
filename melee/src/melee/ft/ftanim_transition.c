#include "ftanim.h"
#include "fighter.h"
#include "ftcommon.h"
#include "ftparts.h"
#include "types.h"
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/object.h>

void ftAnim_8006EBE8(HSD_GObj* gobj, float anim_start, float anim_rate,
                     float anim_blend_frames)
{
    static u32 const mask = AOBJ_MASK | COBJ_MASK | DOBJ_MASK | FOBJ_MASK |
                            FOG_MASK | JOBJ_MASK | LOBJ_MASK | POBJ_MASK |
                            ROBJ_MASK | WOBJ_MASK | RENDER_MASK | CHAN_MASK |
                            TEVREG_MASK | CBOBJ_MASK;
    HSD_JObj* root_jobj = GET_JOBJ(gobj);
    Fighter* fp = GET_FIGHTER(gobj);
    HSD_JObj* anim_jobj = fp->x8AC_animSkeleton;
    ftAnim_80070758(root_jobj);
    ftAnim_80070758(anim_jobj);
    if (anim_blend_frames == 0) {
        ftAnim_8006FA58(fp, FtPart_TransN, fp->x108_costume_joint->child);
        ftAnim_8006FE08(fp, false);
        ftAnim_80070710(root_jobj, anim_start);
        if (fp->x594_b1_loop) {
            HSD_ForeachAnim(root_jobj, JOBJ_TYPE, mask, HSD_AObjSetFlags,
                            AOBJ_ARG_AU, AOBJ_LOOP);
        }
        HSD_ForeachAnim(root_jobj, JOBJ_TYPE, mask, HSD_AObjSetRate,
                        AOBJ_ARG_AF, anim_rate);
    } else {
        ftAnim_8006FB88(fp, FtPart_TransN, fp->x108_costume_joint->child);
        ftAnim_8006FE08(fp, true);
        ftAnim_80070710(anim_jobj, anim_start);
        ftAnim_80070710(root_jobj, anim_start);
        if (fp->x594_b1_loop) {
            HSD_ForeachAnim(anim_jobj, JOBJ_TYPE, mask, HSD_AObjSetFlags,
                            AOBJ_ARG_AU, AOBJ_LOOP);
            HSD_ForeachAnim(root_jobj, JOBJ_TYPE, mask, HSD_AObjSetFlags,
                            AOBJ_ARG_AU, AOBJ_LOOP);
        }
        HSD_ForeachAnim(anim_jobj, JOBJ_TYPE, mask, HSD_AObjSetRate,
                        AOBJ_ARG_AF, anim_rate);
        HSD_ForeachAnim(root_jobj, JOBJ_TYPE, mask, HSD_AObjSetRate,
                        AOBJ_ARG_AF, anim_rate);
    }
    fp->x8A4_animBlendFrames = anim_blend_frames;
    fp->x8A8_anim_frame = 0;
}

void ftAnim_8006FA58(Fighter* fp, Fighter_Part part, HSD_Joint* joint)
{
    HSD_Joint* ancestors[30];
    int i = part;
    s32 depth = 0;

    while (joint != NULL) {
        while (ftParts_8007506C(fp->kind, i) != 0) {
            i++;
        }
        if (i == fp->ft_data->x8->x10) {
            if (fp->parts[i].flags_b0) {
                lb_8000B760(fp->parts[i].joint, joint);
            } else if (!fp->parts[i].flags_b5) {
                lb_8000B5DC(fp->parts[i].joint, joint);
            }
            ftCommon_8007F6A4(fp, fp->parts[i].joint);
        } else {
            if (fp->parts[i].flags_b0) {
                lb_8000B6A4(fp->parts[i].joint, joint);
            } else if (!fp->parts[i].flags_b5) {
                lb_8000B4FC(fp->parts[i].joint, joint);
            }
        }
        i++;
        ftAnim_GetNextJointInTree(&joint, &depth, ancestors);
    }
}

void ftAnim_8006FB88(Fighter* fp, Fighter_Part part, HSD_Joint* joint)
{
    HSD_Joint* ancestors[30];
    int i = part;
    s32 depth = 0;

    while (joint != NULL) {
        while (ftParts_8007506C(fp->kind, i) != 0) {
            i++;
        }
        if (i == fp->ft_data->x8->x10) {
            if (fp->parts[i].flags_b0) {
                lb_8000B760(fp->parts[i].x4_jobj2, joint);
                HSD_JObjClearFlags(fp->parts[i].x4_jobj2, JOBJ_USE_QUATERNION);
            } else if (!fp->parts[i].flags_b5) {
                lb_8000B5DC(fp->parts[i].x4_jobj2, joint);
            }
            ftCommon_8007F6A4(fp, fp->parts[i].x4_jobj2);
        } else {
            if (fp->parts[i].flags_b0) {
                lb_8000B6A4(fp->parts[i].x4_jobj2, joint);
                HSD_JObjClearFlags(fp->parts[i].x4_jobj2, JOBJ_USE_QUATERNION);
            } else if (!fp->parts[i].flags_b5) {
                lb_8000B4FC(fp->parts[i].x4_jobj2, joint);
            }
        }
        i++;
        ftAnim_GetNextJointInTree(&joint, &depth, ancestors);
    }
}

