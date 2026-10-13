#include "ftparts.h"
#include "fighter.h"
#include "ftdata.h"
#include "ftmaterial.h"
#include "types.h"
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/dobj.h>

void ftParts_80074194(Fighter* fighter, FighterBone* bone, HSD_JObj* jobj,
                      int* dobj_index, u32 tree_depth)
{
    HSD_DObj* dobj = HSD_JObjGetDObj(jobj);
    u32 jobj_flags = HSD_JObjGetFlags(jobj);
    int dobj_count = 0;
    HSD_MObj* mobj;

    PAD_STACK(8);

    bone->joint = jobj;

    bone->flags_b1 = true;
    bone->flags_b7 = jobj_flags & JOBJ_LIGHTING ? true : false;
    bone->flags2_b0 = jobj_flags & JOBJ_TEXGEN ? true : false;
    bone->flags2_b1 = jobj_flags & JOBJ_SPECULAR ? true : false;
    bone->flags2_b2 = jobj_flags & JOBJ_UNK_B18 ? true : false;
    bone->flags2_b3 = jobj_flags & JOBJ_UNK_B20 ? true : false;
    bone->flags2_b4 = jobj_flags & JOBJ_UNK_B19 ? true : false;
    bone->xC = tree_depth;

    while (true) {
        if (dobj == NULL) {
            break;
        }
        if (*dobj_index >= 124) {
            HSD_ASSERTREPORT(466, 0,
                             "fighter parts model dobj num over! player %d\n",
                             fighter->player_id);
        }
        fighter->dobj_list.data[*dobj_index] = dobj;
        mobj = dobj != NULL ? dobj->mobj : NULL;
        if (mobj != NULL) {
            hsdChangeClass(mobj, &ftMObj);
        }
        dobj = dobj != NULL ? dobj->next : NULL;
        dobj_count++;
        (*dobj_index)++;
    }
    if (dobj_count >= 128) {
        HSD_ASSERTREPORT(480, 0, "fighter dobj num over! player %d\n",
                         fighter->player_id);
    }
    if (*dobj_index != 0) {
        bone->xD = *dobj_index - 1;
    } else {
        bone->xD = 0;
    }
    bone->flags2_b6 = dobj_count != 0 ? true : false;
}

void ftParts_SetupParts(Fighter_GObj* fighter_obj)
{
    HSD_JObj* jobj = GET_JOBJ(fighter_obj);
    Fighter* fp = GET_FIGHTER(fighter_obj);
    u32 part = 0;
    u32 tree_depth = 0;
    int dobj_count = 0;

    if (ftPartsTable[fp->kind]->parts_num > MAX_FT_PARTS) {
        HSD_ASSERTREPORT(503, 0, "fighter parts num over! player %d\n",
                         fp->player_id);
    }

    while (jobj != NULL) {
        if (ftParts_8007506C(fp->kind, part) != 0) {
            fp->parts[part].joint = NULL;
            part++;
            continue;
        }

        ftParts_80074194(fp, &fp->parts[part], jobj, &dobj_count, tree_depth);
        part++;

        if (!(HSD_JObjGetFlags(jobj) & JOBJ_INSTANCE) &&
            HSD_JObjGetChild(jobj) != NULL)
        {
            // Descend the left side of the tree
            jobj = HSD_JObjGetChild(jobj);
            tree_depth++;
            continue;
        }

        if (HSD_JObjGetNext(jobj) != NULL) {
            // Visit bottom nodes from left to right
            jobj = HSD_JObjGetNext(jobj);
            continue;
        }

        while (true) {
            if (HSD_JObjGetParent(jobj) == NULL) {
                // Finished
                jobj = NULL;
                break;
            }

            // Go back up the tree until we can continue to the right
            if (HSD_JObjGetNext(HSD_JObjGetParent(jobj)) != NULL) {
                jobj = HSD_JObjGetNext(HSD_JObjGetParent(jobj));
                tree_depth--;
                break;
            }

            jobj = HSD_JObjGetParent(jobj);
            tree_depth--;
        }
    }

    fp->dobj_list.count = dobj_count;

    if (part != ftPartsTable[fp->kind]->parts_num) {
        HSD_ASSERTREPORT(546, 0, "fighter parts num not match! player %d\n",
                         fp->player_id);
    }
}

void ftParts_8007462C(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    int i = 0;
    HSD_JObj* jobj = fp->x8AC_animSkeleton;

    while (jobj != NULL) {
        if (ftParts_8007506C(fp->kind, i) != 0) {
            fp->parts[i].x4_jobj2 = NULL;
            i++;
            continue;
        }

        fp->parts[i].x4_jobj2 = jobj;
        i++;
        if (!(HSD_JObjGetFlags(jobj) & JOBJ_INSTANCE) &&
            HSD_JObjGetChild(jobj) != NULL)
        {
            jobj = HSD_JObjGetChild(jobj);
            continue;
        }
        if (HSD_JObjGetNext(jobj) != NULL) {
            jobj = HSD_JObjGetNext(jobj);
            continue;
        }
        while (true) {
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
    if (i != ftPartsTable[fp->kind]->parts_num) {
        HSD_ASSERTREPORT(593, 0, "fighter parts num not match! player %d\n",
                         fp->player_id);
    }
}

HSD_JObj* ftParts_8007482C(HSD_Joint* joint)
{
    HSD_JObj* jobj;
    HSD_JObjSetDefaultClass(HSD_CLASS_INFO(&ftIntpJObj));
    jobj = HSD_JObjLoadJoint(joint);
    HSD_JObjSetDefaultClass(NULL);
    return jobj;
}

