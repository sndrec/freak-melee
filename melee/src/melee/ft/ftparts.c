#include "ftparts.h"

#include <placeholder.h>

#include "fighter.h"
#include "forward.h"
#include "ftdata.h"
#include "ftmaterial.h"
#include "inlines.h"
#include "types.h"
#ifdef MELEE_NATIVE
#include "native_costume_runtime.h"
#include "native_fighter_attachment.h"
#include "native_fighter_pool.h"
#endif
#include <dolphin/mtx.h>
#include <melee/lb/lbrefract.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/displayfunc.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/perf.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/util.h>

void ftParts_8007487C(FtPartsDesc* desc, FtPartsVis* vis, u32 costume_id,
                      DObjList* arg3, DObjList* arg4)
{
    void*(*vis_table)[4];
    PAD_STACK(0x8);

    vis_table = desc->vis_table;
    vis->model_num = desc->model_num;
    if (vis->model_num > 11) {
        HSD_ASSERTREPORT(627, 0, "fighter parts model num over!\n");
    }

    vis->xC[0] =
        vis_table[costume_id][0] ? vis_table[costume_id][0] : vis_table[0][0];
    vis->xC[1] =
        vis_table[costume_id][1] ? vis_table[costume_id][1] : vis_table[0][1];
    vis->xC[2] =
        vis_table[costume_id][2] ? vis_table[costume_id][2] : vis_table[0][2];
    vis->xC[3] =
        vis_table[costume_id][3] ? vis_table[costume_id][3] : vis_table[0][3];
    vis->xC[4] = 0;
    vis->cleared[0] = true;
    vis->cleared[1] = true;
    vis->cleared[2] = true;
    vis->cleared[3] = true;
    vis->cleared[4] = true;
    ftParts_80074D7C(vis, 0, arg3);
    ftParts_80074D7C(vis, 1, arg3);
    ftParts_80074D7C(vis, 2, arg4);
    ftParts_80074D7C(vis, 3, arg3);
    ftParts_80074D7C(vis, 4, arg3);
}

void ftParts_800749CC(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    int i;

    u32 variant = fp->x619_costume_id;
#ifdef MELEE_NATIVE
    variant = nativeCostumeRuntime_ModelVariant(fp);
    if (variant >= fp->ft_data->x8->x0.costume_count) variant = 0;
#endif
    ftParts_8007487C(&fp->ft_data->x8->x0, &fp->x5AC, variant,
                     &fp->dobj_list, &fp->x203C);
    for (i = 0; i < fp->x5AC.model_num; i++) {
        fp->x5F4_arr[i].prev = -1;
    }
    ftParts_80074ACC(gobj);
}

void ftParts_80074A4C(Fighter_GObj* gobj, int model_idx, int val)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->x5F4_arr[model_idx].prev = val;
    fp->x221D_b2 = true;
}

int ftParts_80074A74(Fighter_GObj* gobj, int model_idx)
{
    Fighter* fp = GET_FIGHTER(gobj);
    return fp->x5F4_arr[model_idx].prev;
}

void ftParts_80074A8C(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    int i;
#ifdef MELEE_NATIVE
    bool visibility_changed = false;
#endif

    for (i = 0; i < fp->x5AC.model_num; i++) {
#ifdef MELEE_NATIVE
        visibility_changed |= fp->x5F4_arr[i].idx != fp->x5F4_arr[i].prev;
#endif
        fp->x5F4_arr[i].idx = fp->x5F4_arr[i].prev;
    }
    fp->x221D_b2 = false;
#ifdef MELEE_NATIVE
    if (visibility_changed) {
        // Reapply saved selections on the next draw. The draw flags can
        // still reflect the previous action's temporary selections.
        ftParts_80074D7C(&fp->x5AC, 0, &fp->dobj_list);
    }
#endif
}

void ftParts_80074ACC(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    int i;

    for (i = 0; i < fp->x5AC.model_num; i++) {
        fp->x5F4_arr[i].idx = -1;
    }
    fp->x221D_b2 = false;
}

void ftParts_80074B0C(Fighter_GObj* gobj, int model_idx, int val)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (val != fp->x5F4_arr[model_idx].idx) {
        fp->x5F4_arr[model_idx].idx = val;
        fp->x221D_b2 = true;
        ftParts_80074D7C(&fp->x5AC, 0, &fp->dobj_list);
    }
}

void ftParts_80074B6C(Fighter* fp, FtPartsVis* vis, int idx,
                      DObjList* dobj_list)
{
    FtPartsVisLookup* lookup = vis->xC[idx]; // r4
    if (lookup != NULL && !vis->cleared[idx]) {
        int i; // r26
        for (i = 0; i < vis->model_num; i++) {
            int r25 = (int) fp->x5F4_arr[i].idx; // r25
            int j;                               // r24
            for (j = 0; j < lookup[i].x0; j++) {
                TempS* r27 = &lookup[i].x4[j]; // r27
                u8* r0 = r27->x4;              // r0
                if (j == r25) {
                    int k; // r20
                    for (k = 0; k < r27->x0; k++) {
                        HSD_DObjClearFlags(dobj_list->data[r0[k]], 1);
                    }
                } else {
                    int k; // r20
                    for (k = 0; k < r27->x0; k++) {
                        HSD_DObjSetFlags(dobj_list->data[r0[k]], 1);
                    }
                }
            }
            vis->cleared[idx] = 1;
        }
    }
}

void ftParts_80074CA0(FtPartsVis* vis, int idx, DObjList* dobj_list)
{
    FtPartsVisLookup* lookup = vis->xC[idx]; // r0
    if (lookup != NULL && !vis->cleared[idx]) {
        int i; // r24
        for (i = 0; i < vis->model_num; i++) {
            int j; // r23
            for (j = 0; j < lookup[i].x0; j++) {
                TempS* r26; // r26
                int k;      // r22
                u8* r29;    // r29
                r26 = &lookup[i].x4[j];
                r29 = r26->x4;
                for (k = 0; k < r26->x0; k++) {
                    HSD_DObjClearFlags(dobj_list->data[r29[k]], 1);
                }
            }
        }
        vis->cleared[idx] = true;
    }
}

void ftParts_80074D7C(FtPartsVis* vis, int idx, DObjList* dobj_list)
{
    FtPartsVisLookup* lookup = vis->xC[idx]; // r0
    if (lookup != NULL && vis->cleared[idx]) {
        int i; // r24
        for (i = 0; i < vis->model_num; i++) {
            int j; // r23
            for (j = 0; j < lookup[i].x0; j++) {
                TempS* r26; // r26
                int k;      // r22
                u8* r29;    // r29
                r26 = &lookup[i].x4[j];
                r29 = r26->x4;
                for (k = 0; k < r26->x0; k++) {
                    HSD_DObjSetFlags(dobj_list->data[r29[k]], 1);
                }
            }
        }
        vis->cleared[idx] = false;
    }
}

void ftParts_80074E58(Fighter* fp)
{
    int i;

#ifdef MELEE_NATIVE
    NativeFighterPoolSlot* own=nativeFighterPool_Slot(fp);
    HSD_ASSERTREPORT(0,own!=NULL,"Fighter parts need a live native owner.\n");
    fp->parts=own->parts;fp->dobj_list.data=own->primary_dobjs;
#else
    fp->parts = HSD_ObjAlloc(&fighter_parts_alloc_data);
    fp->dobj_list.data = HSD_ObjAlloc(&fighter_dobj_list_alloc_data);
#endif

    for (i = 0; i < ftPartsTable[fp->kind]->parts_num; i++) {
        fp->parts[i].flags8 = 0;
        fp->parts[i].flagsC = 0;
    }

    fp->parts[0].flags_b3 = true;
    fp->parts[ftParts_GetBoneIndex(fp, FtPart_TransN)].flags_b3 = true;
    fp->parts[ftParts_GetBoneIndex(fp, FtPart_XRotN)].flags_b3 = true;
    fp->parts[ftParts_GetBoneIndex(fp, FtPart_YRotN)].flags_b3 = true;
    fp->parts[ftParts_GetBoneIndex(fp, FtPart_HipN)].flags_b3 = true;
    fp->parts[ftParts_GetBoneIndex(fp, FtPart_TransN2)].flags_b3 = true;
    fp->parts[ftParts_GetBoneIndex(fp, FtPart_TransN)].flags_b4 = true;
    fp->parts[ftParts_GetBoneIndex(fp, 0x35)].flags_b4 = true;
}

void ftParts_800750C8(Fighter* fp, enum_t arg1, bool arg2)
{
    if (arg1 == 2) {
        if (arg2) {
            ftParts_80074B6C(fp, &fp->x5AC, 2, &fp->x203C);
        } else {
            ftParts_80074D7C(&fp->x5AC, 2, &fp->x203C);
        }
#ifdef MELEE_NATIVE
        nativeFighterAttachment_Visibility(fp, 2, arg2);
#endif
        if (ftData_UnkIntBoolFunc0.model_events[fp->kind] != NULL) {
            ftData_UnkIntBoolFunc0.model_events[fp->kind](fp, 2, arg2);
        }
        if (arg2) {
            ftParts_80074B6C(fp, &fp->x5AC, 3, &fp->dobj_list);
        } else {
            ftParts_80074D7C(&fp->x5AC, 3, &fp->dobj_list);
        }
#ifdef MELEE_NATIVE
        nativeFighterAttachment_Visibility(fp, 3, arg2);
#endif
        if (ftData_UnkIntBoolFunc0.model_events[fp->kind] != NULL) {
            ftData_UnkIntBoolFunc0.model_events[fp->kind](fp, 3, arg2);
        }
    } else {
        if (arg2) {
            ftParts_80074B6C(fp, &fp->x5AC, arg1, &fp->dobj_list);
        } else {
            ftParts_80074D7C(&fp->x5AC, arg1, &fp->dobj_list);
        }
#ifdef MELEE_NATIVE
        nativeFighterAttachment_Visibility(fp, arg1, arg2);
#endif
        if (ftData_UnkIntBoolFunc0.model_events[fp->kind] != NULL) {
            ftData_UnkIntBoolFunc0.model_events[fp->kind](fp, arg1, arg2);
        }
    }
}

HSD_TObj* ftParts_80075240(DObjList* arg0, int n)
{
    int tobj_i;
    int i;

    HSD_MObj* mobj;
    HSD_TObj* tobj;
    HSD_DObj* dobj;

    PAD_STACK(4);

    tobj_i = 0;

    for (i = 0; i < (int) arg0->count; i++) {
        dobj = arg0->data[i];
        if (dobj != NULL) {
            mobj = dobj != NULL ? dobj->mobj : NULL;
            if (mobj != NULL) {
                tobj = HSD_MObjGetTObj(mobj);
                while (true) {
                    if (tobj == NULL) {
                        break;
                    }
                    if (tobj_i == n) {
                        return tobj;
                    }
                    tobj_i++;
                    tobj = HSD_TObjGetNext(tobj);
                }
            }
        }
    }
    HSD_ASSERTREPORT(948, 0, "can't find tobj!\n");
}

/**
 * Inserts a new jobj relative to a root jobj, with the location specified by
 * the first argument.
 */
void ftParts_80075304(u8 type, HSD_JObj* root, HSD_JObj* new_jobj)
{
    switch (type) {
    case 0:
        /**
         * If type is 0, the new jobj is inserted as a child of the root,
         * and children of the root become children of the new jobj.
         */
        {
            HSD_JObj* child;
            if (root == NULL) {
                child = NULL;
            } else {
                child = root->child;
            }
            new_jobj->child = child;
            new_jobj->parent = root;
            root->child = new_jobj;
            if (child != NULL) {
                child->parent = new_jobj;
            }
            break;
        }
    case 1:
        /**
         * If type is 1, the new jobj is inserted as a child of the root,
         * alongside any existing children of the root.
         */
        {
            HSD_JObj* child;
            if (root == NULL) {
                child = NULL;
            } else {
                child = root->child;
            }
            new_jobj->next = child;
            new_jobj->parent = root;
            root->child = new_jobj;
            break;
        }
    case 2:
        /**
         * If type is 2, the new jobj is inserted as a sibling of the root.
         * Existing siblings of the root become children of the new jobj.
         */
        {
            HSD_JObj* next;
            if (root == NULL) {
                next = NULL;
            } else {
                next = root->next;
            }
            new_jobj->child = next;
            new_jobj->parent = root->parent;
            root->next = new_jobj;
            break;
        }
    case 3:
        /**
         * If type is 3, the new jobj is inserted as a sibling of the root,
         * alongside any existing siblings.
         */
        {
            HSD_JObj* next;
            if (root == NULL) {
                next = NULL;
            } else {
                next = root->next;
            }
            new_jobj->next = next;
            new_jobj->parent = root->parent;
            root->next = new_jobj;
            break;
        }
    }
}

void ftParts_800753D4(Fighter* arg0, struct Fighter_804D6540_x0_t* arg1,
                      HSD_Joint* arg2)
{
    HSD_Joint* ancestors[30];
    HSD_Joint* sp6C;
    HSD_Joint sp2C;

    HSD_JObj* temp_r31;
    HSD_JObj* temp_r30;
    int i;
    u32 tree_depth;
    int dobj_index;

    sp6C = arg2;
    if (arg1->x3 != 0xFF) {
        s32 depth = 0;
        for (i = 0; i < arg1->x3; i++) {
            ftAnim_GetNextJointInTree(&sp6C, &depth, ancestors);
        }
    }
    sp2C = *sp6C;
    sp2C.next = 0;
    sp2C.child = 0;

    temp_r30 = ftParts_8007482C(&sp2C);
    temp_r31 = ftParts_8007482C(&sp2C);

    ftParts_80075304(arg1->x2, arg0->parts[arg1->x1].joint, temp_r30);
    ftParts_80075304(arg1->x2, arg0->parts[arg1->x1].x4_jobj2, temp_r31);

    tree_depth = arg0->parts[arg1->x1].xC;
    if (arg1->x2 == 0 || arg1->x2 == 1) {
        tree_depth++;
    }
    dobj_index = 0;
    ftParts_80074194(arg0, &arg0->parts[arg1->x0], temp_r30, &dobj_index,
                     tree_depth);

    arg0->parts[arg1->x0].x4_jobj2 = temp_r31;
    arg0->parts[arg1->x0].flags_b2 = true;
}

void ftParts_800755E8(Fighter* fp, struct Fighter_804D6540_x0_t* arg1)
{
    FighterBone* bone = &fp->parts[arg1->x0];
    HSD_JObjRemove(bone->joint);
    HSD_JObjRemove(bone->x4_jobj2);
    bone->joint = NULL;
    bone->x4_jobj2 = NULL;
    bone->flags_b1 = false;
    bone->flags_b2 = false;
}

void ftParts_80075650(Fighter_GObj* arg0, HSD_JObj* jobj, DObjList* arg2)
{
    s32 var_r30;
    HSD_DObj* dobj;

    HSD_DObj* var_r0;
    HSD_MObj* temp_r0;

    PAD_STACK(8);

    var_r30 = 0;
    while (jobj != NULL) {
        dobj = HSD_JObjGetDObj(jobj);
        while (true) {
            if (dobj == NULL) {
                break;
            }
            if (var_r30 >= 0x20) {
                HSD_ASSERTREPORT(1063, 0,
                                 "fighter parts model dobj num over!\n");
            }
            arg2->data[var_r30] = dobj;
            temp_r0 = dobj->mobj;
            if (temp_r0 != NULL) {
                hsdChangeClass(temp_r0, &ftMObj);
            }
            if (dobj != NULL) {
                var_r0 = dobj->next;
            } else {
                var_r0 = NULL;
            }
            dobj = var_r0;
            var_r30 += 1;
        }
        if (!(HSD_JObjGetFlags(jobj) & JOBJ_INSTANCE) &&
            HSD_JObjGetChild(jobj) != NULL)
        {
            jobj = HSD_JObjGetChild(jobj);
        } else {
            if (HSD_JObjGetNext(jobj) != NULL) {
                jobj = HSD_JObjGetNext(jobj);
            } else {
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
        }
    }
}

static char* ftParts_803C0BEC = "cant get fighter rot z!\n";
