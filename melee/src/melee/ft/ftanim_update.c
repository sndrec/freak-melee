#include "ftanim.h"
#include "fighter.h"
#include "ftcommon.h"
#include "ftdata.h"
#include "ftparts.h"
#include "types.h"
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lbvector.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>

/// load bearing, fixes float order
static void scale_inline(float scale, Vec3* sp38)
{
    float f1 = 1.0F / scale;
    sp38->x *= f1;
    sp38->y *= f1;
    sp38->z *= f1;
}

void ftAnim_8006E054(Fighter* fp, HSD_JObj* jobj, HSD_JObj* arg2,
                     HSD_JObj* arg3)
{
    Vec3 sp5C;
    Vec3 sp50;
    Vec3 sp44;
    Vec3 sp38;
    Vec3 sp2C;
    u32 temp_r4;
    u32 temp_r4_2;
    u32 temp_r4_3;
    u32 temp_r4_4;

    HSD_AObjInitEndCallBack();

    while (jobj != NULL) {
        if (jobj == arg2) {
            if (jobj != NULL) {
                sp50.x = sp50.y = sp50.z = 0.0F;
                HSD_JObjAnim(jobj);
                fp->x698 = fp->x68C_transNPos;
                HSD_JObjGetTranslation(jobj, &fp->x68C_transNPos);
                if (!fp->x594_b6) {
                    float temp_f1 = ftCommon_GetModelScale(fp);
                    fp->x68C_transNPos.x *= temp_f1;
                    fp->x68C_transNPos.y *= temp_f1;
                    fp->x68C_transNPos.z *= temp_f1;
                } else {
                    float temp_f1_2 = fp->co_attrs.model_scaling;
                    fp->x68C_transNPos.x *= temp_f1_2;
                    fp->x68C_transNPos.y *= temp_f1_2;
                    fp->x68C_transNPos.z *= temp_f1_2;
                }
                fp->x6B0 = fp->x6A4_transNOffset;
                lbVector_Diff(&fp->x68C_transNPos, &fp->x698,
                              &fp->x6A4_transNOffset);
                HSD_JObjSetTranslate(jobj, &sp50);
            }
        } else if (fp->x594_b5 && jobj == arg3) {
            if (jobj != NULL) {
                sp44.x = sp44.y = sp44.z = 0.0F;
                HSD_JObjAnim(jobj);
                fp->x6CC = fp->x6C0;
                HSD_JObjGetTranslation(jobj, &fp->x6C0);
                if (!fp->x594_b6) {
                    float temp_f1_3 = ftCommon_GetModelScale(fp);
                    fp->x6C0.x *= temp_f1_3;
                    fp->x6C0.y *= temp_f1_3;
                    fp->x6C0.z *= temp_f1_3;
                } else {
                    float temp_f1_4 = fp->co_attrs.model_scaling;
                    fp->x6C0.x *= temp_f1_4;
                    fp->x6C0.y *= temp_f1_4;
                    fp->x6C0.z *= temp_f1_4;
                }
                fp->x6E4 = fp->x6D8;
                lbVector_Diff(&fp->x6C0, &fp->x6CC, &fp->x6D8);
                HSD_JObjSetTranslate(jobj, &sp44);
            }
        } else {
            HSD_JObjAnim(jobj);
        }

        if (!(HSD_JObjGetFlags(jobj) & 0x1000) &&
            HSD_JObjGetChild(jobj) != NULL)
        {
            jobj = HSD_JObjGetChild(jobj);
        } else if (HSD_JObjGetNext(jobj) != NULL) {
            jobj = HSD_JObjGetNext(jobj);
        } else {
            while (1) {
                if (HSD_JObjGetParent(jobj) == NULL) {
                    jobj = NULL;
                    break;
                }
                if (HSD_JObjGetNext(HSD_JObjGetParent(jobj)) != NULL) {
                    jobj = HSD_JObjGetNext(HSD_JObjGetParent(jobj));
                    break;
                }
                jobj = HSD_JObjGetParent(jobj);
            }
        }
    }
    if (fp->x594_b5) {
        sp5C.x = fp->x68C_transNPos.x - fp->x6C0.x;
        sp5C.y = fp->x68C_transNPos.y - fp->x6C0.y;
        sp5C.z = fp->x68C_transNPos.z - fp->x6C0.z;
        HSD_JObjSetTranslate(arg2, &sp5C);
        fp->x68C_transNPos = fp->x6C0;
        fp->x698 = fp->x6CC;
        fp->x6A4_transNOffset = fp->x6D8;
        fp->x6B0 = fp->x6E4;
    }
    if (!fp->x2226_b2 && fp->x2221_b2) {
        jobj = fp->parts[fp->ft_data->x8->x10].joint;
        ftCommon_GetModelScale(fp);
        HSD_JObjGetTranslation(jobj, &sp2C);
        sp38 = fp->x68C_transNPos;
        if (!fp->x594_b6) {
            scale_inline(ftCommon_GetModelScale(fp), &sp38);
        } else {
            scale_inline(fp->co_attrs.model_scaling, &sp38);
        }
        lbVector_Sub(&sp2C, &sp38);
        HSD_JObjSetTranslate(jobj, &sp2C);
    }
    HSD_AObjInvokeCallBacks();
}

void ftAnim_8006E9B4(Fighter_GObj* gobj)
{
    Fighter* fp;
    HSD_JObj* jobj;      // r27
    HSD_JObj* anim_jobj; // r27
    float framerate;     // f1
    float blend_t_inv;   // f31
    float blend_t;       // f30

    fp = GET_FIGHTER(gobj);

    if (fp->anim_id == -1) {
        return;
    }

    jobj = GET_JOBJ(gobj);
    if (fp->x8A4_animBlendFrames == 0.0F) {
        HSD_JObjClearFlagsAll(jobj, JOBJ_USE_QUATERNION);
        if (fp->x594_b0) {
            ftAnim_8006E054(
                fp, jobj,
                fp->parts[ftParts_GetBoneIndex(fp, FtPart_TransN)].joint,
                fp->parts[ftParts_GetBoneIndex(fp, 0x35)].joint);
        } else {
            ftAnim_8006E7B8(fp, FtPart_TopN);
        }
    } else {
        anim_jobj = fp->x8AC_animSkeleton;
        framerate = lbGetJObjFramerate(anim_jobj);
        fp->x8A8_anim_frame += framerate;
        if (fp->x8A4_animBlendFrames <= fp->x8A8_anim_frame) {
            fp->x8A8_anim_frame = fp->x8A4_animBlendFrames;
            blend_t = 1.0F;
            blend_t_inv = 0.0F;
        } else {
            float remaining; // f2
            remaining = fp->x8A4_animBlendFrames - fp->x8A8_anim_frame;
            blend_t = framerate / (framerate + remaining);
            blend_t_inv = 1.0F - blend_t;
        }
        ftAnim_8006E7B8(fp, FtPart_TopN);
        if (fp->x594_b0) {
            ftAnim_8006E054(
                fp, anim_jobj,
                fp->parts[ftParts_GetBoneIndex(fp, FtPart_TransN)].x4_jobj2,
                fp->parts[ftParts_GetBoneIndex(fp, 0x35)].joint);
        } else {
            HSD_JObjAnimAll(anim_jobj);
        }
        if (blend_t_inv) {
            ftAnim_8006FE9C(fp, FtPart_TransN, blend_t, blend_t_inv);
        } else {
            ftAnim_8006FF74(fp, FtPart_TransN);
        }
    }

    if (fp->x594_b2) {
        if (ftAnim_8006F3DC(gobj) < fp->cur_anim_frame) {
            fp->x898_unk += fp->cur_anim_frame + fp->frame_speed_mul;
        }
    }

    fp->cur_anim_frame = ftAnim_8006F3DC(gobj);
}

float ftAnim_8006F3DC(Fighter_GObj* fighter_gobj)
{
    Fighter* fp = GET_FIGHTER(fighter_gobj);

    if (fp->x8A4_animBlendFrames == 0.0F) {
        int i;
        for (i = 0; i < ftPartsTable[fp->kind]->parts_num; i++) {
            if (fp->parts[i].flags_b1 && !fp->parts[i].flags_b0 &&
                !fp->parts[i].flags_b5)
            {
                HSD_AObj* aobj = fp->parts[i].joint->aobj;
                if (aobj != 0) {
                    return aobj->curr_frame;
                }
            }
        }
    } else {
        return lbGetJObjCurrFrame(fp->x8AC_animSkeleton);
    }
    return 0.0F;
}


bool ftAnim_80070FD0(Fighter* fp)
{
    float rotation = ftPartGetRotX(fp, ftParts_GetBoneIndex(fp, 0x35));
    if (fp->x100 != rotation) {
        fp->x100 = rotation;
        return true;
    }
    return false;
}
