#include "ftanim.h"
#include "fighter.h"
#include "ftparts.h"
#include "types.h"
#include "ftdata.h"
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/gobj.h>

void ftAnim_8006FE48(Fighter_GObj* fighter_gobj)
{
    Fighter* fp = GET_FIGHTER(fighter_gobj);
    fp->x8AC_animSkeleton = ftParts_8007482C(fp->x108_costume_joint);
    fp->x8A4_animBlendFrames = 0.0F;
    fp->x8A8_anim_frame = 0.0F;
    ftParts_8007462C(fighter_gobj);
}

void ftAnim_8006FE9C(Fighter* fp, Fighter_Part start, float t, float t_inv)
{
    int i;
    for (i = start; i < ftPartsTable[fp->kind]->parts_num; i++) {
        if (fp->parts[i].flags_b1 && !fp->parts[i].flags_b0 &&
            !fp->parts[i].flags_b5)
        {
            if (fp->parts[i].flags_b4) {
                lbCopyJObjSRT(fp->parts[i].x4_jobj2, fp->parts[i].joint);
            } else {
                lb_8000C490(fp->parts[i].x4_jobj2, fp->parts[i].joint,
                            fp->parts[i].joint, t, t_inv);
            }
        }
    }
}

void ftAnim_8006FF74(Fighter* fp, Fighter_Part start)
{
    int i;
    for (i = start; i < ftPartsTable[fp->kind]->parts_num; i++) {
        if (fp->parts[i].flags_b1 && !fp->parts[i].flags_b0 &&
            !fp->parts[i].flags_b5)
        {
            lbCopyJObjSRT(fp->parts[i].x4_jobj2, fp->parts[i].joint);
        }
    }
}

