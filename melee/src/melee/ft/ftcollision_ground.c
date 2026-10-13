#include "ft_081B.h"
#include "fighter.h"
#include "types.h"
#include <melee/ft/kinds/ftCommon/types.h>
#include <melee/mp/mpcoll.h>
#include <melee/mp/mplib.h>

/// Return whether the fighter remains supported after grounded collision.
bool ft_80082708(Fighter_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    CollData* cd = &fp->coll_data;
    cd->last_pos = cd->cur_pos;
    cd->cur_pos = fp->cur_pos;
    {
        bool supported = mpColl_8004B108(cd);
        fp->cur_pos = cd->cur_pos;
        return supported;
    }
}

bool ft_800827A0(Fighter_GObj* gobj)
{
    Fighter* fp;
    CollData* coll;
    bool tmp;

    fp = GET_FIGHTER(gobj);
    coll = &fp->coll_data;
    coll->last_pos = coll->cur_pos;
    coll->cur_pos = fp->cur_pos;
    tmp = mpColl_8004B2DC(coll);
    fp->cur_pos = coll->cur_pos;

    if (tmp) {
        return true;
    } else {
        return false;
    }
}

ftCollisionBox* ft_80082838(ftCollisionBox* arg0, ftCollisionBox* arg1,
                            f32 arg2)
{
    if (arg2 < 0.0f) {
        arg0->top = arg1->top;
        arg0->bottom = arg1->bottom;
        arg0->left.y = arg1->left.y;
        arg0->right.y = arg1->right.y;
        arg0->right.x = -arg1->left.x;
        arg0->left.x = -arg1->right.x;
        return arg0;
    }
    return arg1;
}

bool ft_80082888(Fighter_GObj* arg0, ftCollisionBox* arg1)
{
    Fighter* fp;
    CollData* coll;
    bool tmp;
    ftCollisionBox sp18;
    ftCollisionBox* box;

    fp = GET_FIGHTER(arg0);
    coll = &fp->coll_data;
    fp->coll_data.last_pos = fp->coll_data.cur_pos;
    fp->coll_data.cur_pos = fp->cur_pos;

    box = ft_80082838(&sp18, arg1, fp->facing_dir);
    tmp = mpColl_8004B21C(coll, box);
    fp->cur_pos = coll->cur_pos;

    if (tmp) {
        return true;
    } else {
        return false;
    }
}

bool ft_80082978(HSD_GObj* gobj, ftCollisionBox* arg1)
{
    Fighter* fp;
    CollData* coll;
    ftCollisionBox sp18;
    ftCollisionBox* box;
    bool tmp;

    fp = GET_FIGHTER(gobj);
    coll = &fp->coll_data;
    fp->coll_data.last_pos = fp->coll_data.cur_pos;
    fp->coll_data.cur_pos = fp->cur_pos;
    box = ft_80082838(&sp18, arg1, fp->facing_dir);
    tmp = mpColl_8004B3F0(coll, box);
    fp->cur_pos = coll->cur_pos;

    if (tmp) {
        return true;
    } else {
        return false;
    }
}

bool ft_80082A68(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    CollData* coll = &fp->coll_data;
    fp->coll_data.cur_pos = fp->cur_pos;
    fp->coll_data.last_pos = fp->coll_data.cur_pos;
    fp->coll_data.last_pos.y += 10.0f;
    fp->coll_data.cur_pos.y -= 10.0f;
    if (mpColl_800471F8(coll)) {
        fp->cur_pos = coll->cur_pos;
        return true;
    } else {
        return false;
    }
}
