#include "ftcoll.h"
#include "fighter.h"
#include "ftcommon.h"
#include "types.h"

static inline void comboCount_Push(Fighter* fp)
{
    Vec3* pos = &fp->coll_data.floor.normal;
    float temp_f2;
    float var_f2;
    if ((int) fp->x2090 < p_ftCommonData->x4C8) {
        var_f2 = p_ftCommonData->x4D0;
    } else {
        var_f2 = p_ftCommonData->x4D4;
    }
    temp_f2 = fp->facing_dir * var_f2;
    fp->cur_pos.x = -(pos->y * temp_f2 - fp->cur_pos.x);
    fp->cur_pos.y = -(-pos->x * temp_f2 - fp->cur_pos.y);
}

/// Combo count something + adjust FtPart_TopN
void ftColl_80076528(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    u16 fp_x2092 = fp->x2092;
    if (fp_x2092 != 0) {
        fp->x2092 = (u16) (fp_x2092 - 1);
        if (fp->victim_gobj == NULL && fp->ground_or_air == GA_Ground) {
            comboCount_Push(fp);
        }
    }
}

void ftColl_8007AF28(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    size_t i;
    for (i = 0; i < fp->hurt_capsules_len; i++) {
        fp->hurt_capsules[i].capsule.skip_update_pos = false;
    }
}
