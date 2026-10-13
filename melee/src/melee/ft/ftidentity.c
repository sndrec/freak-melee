#include "ftlib.h"
#include "ftdevice.h"
#include "fighter.h"
#include "types.h"
#include <sysdolphin/baselib/gobj.h>

bool ftLib_80086960(HSD_GObj* arg)
{
    if (arg != NULL && arg->classifier == HSD_GOBJ_CLASS_FIGHTER) {
        return true;
    } else {
        return false;
    }
}

u8 ftLib_80086BE0(HSD_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    return fp->player_id;
}

s32 ftLib_800874BC(HSD_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    return fp->x221F_b4;
}


ColorOverlay* ftCo_800C0658(Fighter* fp)
{
    return fp->x408.x28_colanim.ptr != NULL ? &fp->x408 : &fp->x488;
}

