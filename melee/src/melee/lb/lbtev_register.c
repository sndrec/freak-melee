#include "lb_00B0.h"
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/tev.h>

static s32 lb_803B9FC0[] = { 14, 14, 14, 14, 2, 4, 6, 0 };
static s32 lb_803B9FE0[] = { 12, 13, 14, 15 };
static s32 lb_803B9FF0[] = { 7, 4, 5, 6 };
static s32 lb_803BA000[] = { 6, 6, 6, 6, 1, 2, 3, 0 };
static s32 lb_803BA020[] = { 28, 29, 30, 31 };
static s32 lb_803BA030[] = { 7, 4, 5, 6 };

static s32 lbGetFreeColorRegImpl(s32 i0, HSD_TevDesc* tevdesc, HSD_TExp* texp1,
                                 HSD_TExp* texp2)
{
    bool register_used[8];
    int i;

    for (i = 0; i < 8; i++) {
        register_used[i] = false;
    }
    while (texp1 != NULL) {
        if (texp1->cnst.reg < 8) {
            register_used[texp1->cnst.reg] = true;
        }
        texp1 = texp1->cnst.next;
    }
    while (tevdesc != NULL) {
        s32 idx = lb_803B9FF0[tevdesc->u.tevconf.clr_out_reg];
        register_used[idx] = true;
        tevdesc = tevdesc->next;
    }
    while (texp2 != NULL) {
        if (texp2->cnst.reg < 8) {
            register_used[texp2->cnst.reg] = true;
        }
        texp2 = texp2->cnst.next;
    }
    for (i = i0; i < 7; i++) {
        if (!register_used[i]) {
            return i;
        }
    }
    return -1;
}

s32 lbGetFreeColorRegister(s32 i0, HSD_MObj* mobj, HSD_TExp* texp)
{
    return lbGetFreeColorRegImpl(i0, &mobj->tevdesc->desc, mobj->texp, texp);
}

s32 lb_8000CC8C(s32 i)
{
    return lb_803B9FC0[i];
}

s32 lb_8000CCA4(s32 i)
{
    return lb_803B9FE0[i];
}

static s32 lbGetFreeAlphaRegImpl(s32 i0, HSD_TevDesc* cur, HSD_TExp* arg2,
                                 HSD_TExp* arg3)
{
    bool register_used[8];
    int i;

    for (i = 0; i < 8; i++) {
        register_used[i] = false;
    }
    while (cur != NULL) {
        s32 idx = lb_803BA030[cur->u.tevconf.alpha_out_reg];
        register_used[idx] = true;
        cur = cur->next;
    }
    for (i = i0; i < 7; i++) {
        if (!register_used[i]) {
            return i;
        }
    }
    return -1;
}

s32 lbGetFreeAlphaRegister(s32 i0, HSD_MObj* mobj, HSD_TExp* texp)
{
    return lbGetFreeAlphaRegImpl(i0, &mobj->tevdesc->desc, mobj->texp, texp);
}

s32 lb_8000CD90(s32 i)
{
    return lb_803BA000[i];
}

s32 lb_8000CDA8(s32 i)
{
    return lb_803BA020[i];
}

