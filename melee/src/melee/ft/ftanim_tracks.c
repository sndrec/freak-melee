#include "ftanim.h"
#include "ftanim_track_parts.h"
#include "fighter.h"
#include "ftparts.h"
#include "types.h"
#include <melee/lb/lbanim.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/jobj.h>

void ftAnim_8006F4C8(Fighter* fp, bool do_blending, FigaTree* tree)
{
    s8* cur_node;
    FigaTrack* cur_track;
    unsigned count = ftPartsTable[fp->kind]->parts_num;
    unsigned i = 0;

    cur_node = tree->nodes;
    cur_track = tree->tracks;

    while (*cur_node != -1) {
        i = ftAnim_NextTrackPart(fp, i, count);
        if (i == count) break;
        if (fp->parts[i].flags_b1 && !fp->parts[i].flags_b0 && !fp->parts[i].flags_b5) {
            HSD_JObj* jobj = (do_blending ? fp->parts[i].x4_jobj2 : fp->parts[i].joint);
            lbAnim_8001E6D8(jobj, tree, cur_track, *cur_node);
        }

        {
            s8 frames = *cur_node++;
            i++;
            cur_track += frames;
        }
    }
}

void ftAnim_8006FCE4(Fighter* fp, bool do_blending)
{
    FigaTree* tree;       // r31
    int x594_bits;        // r30
    FighterKind kind;     // r29
    s8* cur_node;         // r28
    FigaTrack* cur_track; // r27
    int i;                // r26

    i = 0;
    tree = fp->x590;
    cur_node = tree->nodes;
    x594_bits = fp->x594_bits;
    cur_track = tree->tracks;
    kind = fp->x597_bits;

    while (*cur_node != -1) {
        int part;

        while (true) {
            u32 r3 = ftParts_8007506C(kind, i);
            if (r3 == 0 || r3 & x594_bits) {
                break;
            }
            i++;
        }

        part = ftPartsRemap(fp->kind, kind, i);
        if (part != 0xFFU) {
            if (fp->parts[part].flags_b1 && !fp->parts[part].flags_b0 &&
                !fp->parts[part].flags_b5)
            {
                HSD_JObj* jobj = (do_blending ? fp->parts[part].x4_jobj2 : fp->parts[part].joint);
                if (fp->parts[part].flags_b3) {
                    lbAnim_8001E6D8(jobj, tree, cur_track, *cur_node);
                } else {
                    lbAnim_8001E7E8(jobj, tree, cur_track, *cur_node);
                }
            }
        }

        cur_track = &cur_track[*cur_node];
        i++;
        cur_node++;
    }
}

void ftAnim_8006FE08(Fighter* fp, bool do_blending)
{
    if (fp->kind != fp->x597_bits) {
        ftAnim_8006FCE4(fp, do_blending);
    } else {
        ftAnim_8006F4C8(fp, do_blending, fp->x590);
    }
}

void ftAnim_8006E7B8(Fighter* fp, Fighter_Part part)
{
    HSD_JObj* jobj;
    int i;
    HSD_JObj* temp_r30;
    int temp_r27;
    int count = ftPartsTable[fp->kind]->parts_num;

    HSD_AObjInitEndCallBack();
    i = part;
    temp_r30 = fp->parts[part].joint;
    temp_r27 = fp->parts[part].xC;
    jobj = temp_r30;
    while (jobj != NULL) {
        while (i < count && !fp->parts[i].flags_b1) {
            i++;
        }
        if (i == count) break;
        if (fp->parts[i].xC <= temp_r27 && i != part) {
            break;
        }
        if (!fp->parts[i].flags_b0 && !fp->parts[i].flags_b5) {
            HSD_JObjAnim(jobj);
        }
        i++;

        if (!(HSD_JObjGetFlags(jobj) & 0x1000) &&
            HSD_JObjGetChild(jobj) != NULL)
        {
            jobj = HSD_JObjGetChild(jobj);
        } else if (HSD_JObjGetNext(jobj) != NULL) {
            jobj = HSD_JObjGetNext(jobj);
        } else {
            while (1) {
                if (HSD_JObjGetParent(jobj) == NULL ||
                    HSD_JObjGetParent(jobj) == temp_r30)
                {
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
    HSD_AObjInvokeCallBacks();
}

void ftAnim_80070710(HSD_JObj* jobj, float frame)
{
    HSD_JObjReqAnimAllByFlags(jobj, 1, frame);
}

void ftAnim_80070734(HSD_JObj* jobj, float frame)
{
    HSD_JObjReqAnimByFlags(jobj, 1, frame);
}

void ftAnim_80070758(HSD_JObj* jobj)
{
    HSD_JObjRemoveAnimAllByFlags(jobj, 1);
}

