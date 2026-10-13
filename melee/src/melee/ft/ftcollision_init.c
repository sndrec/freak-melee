#include "ft_081B.h"
#include "fighter.h"
#include "types.h"
#include <melee/mp/mpcoll.h>
#include <sysdolphin/baselib/gobj.h>

void ft_80081B38(Fighter_GObj* gobj)
{
    u8* sp8;
    Fighter* fp;
    CollData* coll;
    FighterBone* bones;
    ftData_x44_t* temp_r29;
    f32 temp_f0;
    f32 temp_f3;
    u8 _[8];

    fp = GET_FIGHTER(gobj);
    fp->ecb_lock = 0;
    coll = &fp->coll_data;
    fp->coll_data.cur_pos = fp->cur_pos;
    fp->coll_data.last_pos = fp->coll_data.cur_pos;
    mpColl_80041EE4(coll);
    coll->x34_flags.b1234 = 1;
    bones = fp->parts;
    temp_r29 = fp->ft_data->x44;
    mpColl_SetECBSource_JObj(
        coll, gobj, bones->joint, bones[temp_r29->unk0].joint,
        bones[temp_r29->unk2].joint, bones[temp_r29->unk4].joint,
        bones[temp_r29->unk6].joint, bones[temp_r29->unk8].joint,
        bones[temp_r29->unkA].joint, temp_r29->unkC * fp->x34_scale.y);
    temp_f3 = fp->x34_scale.y;
    mpColl_SetLedgeSnap(coll, temp_r29->ledge_snap_x * temp_f3,
                        temp_r29->ledge_snap_y * temp_f3,
                        temp_r29->ledge_snap_height * temp_f3);
    coll->x50 = fp->co_attrs.weight;
    temp_f0 = 10.0F * fp->x34_scale.y;
    if (coll->ecb_source.kind == ECBSource_JObj) {
        coll->ecb_source.x128 = temp_f0;
        coll->ecb_source.x12C = temp_f0;
    }
}

void ft_80081C88(Fighter_GObj* dst_gobj, f32 scl_y)
{
    float temp_f0;

    Fighter* fp = GET_FIGHTER(dst_gobj);
    ftData_x44_t* temp_r3 = fp->ft_data->x44;
    {
        CollData* temp_r5 = &fp->coll_data;

        float tmp = temp_r3->unkC * scl_y;
        ;
        if (fp->coll_data.ecb_source.kind == ECBSource_JObj) {
            temp_r5->ecb_source.x124 = tmp;
        }

        {
            float ledge_snap_height = temp_r3->ledge_snap_height * scl_y;
            float ledge_snap_y = temp_r3->ledge_snap_y * scl_y;
            float ledge_snap_x = temp_r3->ledge_snap_x * scl_y;

            if (temp_r5->ecb_source.kind == ECBSource_JObj) {
                temp_r5->ledge_snap_x = ledge_snap_x;
                temp_r5->ledge_snap_y = ledge_snap_y;
                temp_r5->ledge_snap_height = ledge_snap_height;
            }
        }
        temp_r5->x50 = fp->co_attrs.weight;
        temp_f0 = 10.0F * fp->x34_scale.y;
        if (temp_r5->ecb_source.kind == ECBSource_JObj) {
            temp_r5->ecb_source.x128 = temp_f0;
            temp_r5->ecb_source.x12C = temp_f0;
        }
    }
}
