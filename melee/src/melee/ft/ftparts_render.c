#include "ftparts.h"

#include <placeholder.h>

#include "fighter.h"
#include "forward.h"
#include "ftdata.h"
#include "ftmaterial.h"
#include "inlines.h"
#include "types.h"
#include <dolphin/mtx.h>
#include <melee/lb/lbrefract.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/displayfunc.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/native_render_phase.h>
#include <sysdolphin/baselib/perf.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/util.h>

static struct {
    Mtx mtx;
    bool has_z_scale;
} ft_jobj_scale;

HSD_JObjInfo ftJObj = { ftParts_JObjInfoInit };
HSD_JObjInfo ftIntpJObj = { ftParts_IntpJObjInfoInit };
HSD_PObjInfo ftPObj = { ftParts_PObjInfoInit };

void ftParts_JObjMakePositionMtx(HSD_JObj* jobj, Mtx mtx, Mtx rmtx)
{
    Fighter* fighter = (Fighter*) HSD_GObj_804D7814->user_data;

    hsdJObj.make_pmtx(jobj, mtx, rmtx);

    if (fighter->x34_scale.z != 1.0F) {
        Mtx temp_mtx;
        float scale_z = HSD_JObjGetScaleZ(jobj);
        float scale_y = HSD_JObjGetScaleY(jobj);
        float scale_x = HSD_JObjGetScaleX(jobj);
        C_MTXScale(temp_mtx, 1.0F / scale_x, 1.0F / scale_y, 1.0F / scale_z);
        C_MTXConcat(rmtx, temp_mtx, ft_jobj_scale.mtx);
        HSD_MtxInverse(rmtx, temp_mtx);
        C_MTXConcat(ft_jobj_scale.mtx, temp_mtx, ft_jobj_scale.mtx);
        ft_jobj_scale.has_z_scale = true;
    } else {
        ft_jobj_scale.has_z_scale = false;
    }
}

void ftParts_JObjInfoInit(void)
{
    hsdInitClassInfo(HSD_CLASS_INFO(&ftJObj), HSD_CLASS_INFO(&hsdJObj),
                     "fighter_class_library", "ft_jobj", sizeof(HSD_JObjInfo),
                     sizeof(HSD_JObj));
    HSD_JOBJ_INFO(&ftJObj)->make_pmtx = ftParts_JObjMakePositionMtx;
}

void ftParts_80073758(HSD_JObj* jobj)
{
    hsdChangeClass(jobj, &ftJObj);
}

s32 ftParts_IntpJObjLoad(HSD_JObj* jobj, HSD_Joint* joint, HSD_JObj* parent)
{
    HSD_DObjDesc* dobjdesc = joint->u.dobjdesc;
    s32 ret;
    joint->u.dobjdesc = NULL;
    ret = hsdJObj.load(jobj, joint, parent);
    joint->u.dobjdesc = dobjdesc;
    return ret;
}

void ftParts_IntpJObjInfoInit(void)
{
    hsdInitClassInfo(HSD_CLASS_INFO(&ftIntpJObj), HSD_CLASS_INFO(&hsdJObj),
                     "fighter_class_library", "ft_intp_jobj",
                     sizeof(HSD_JObjInfo), sizeof(HSD_JObj));
    HSD_JOBJ_INFO(&ftIntpJObj)->load = ftParts_IntpJObjLoad;
}

static inline PObjSetupFlag ftPartsGetSetupFlags(HSD_JObj* jobj,
                                                 u32 rendermode)
{
    PObjSetupFlag flags = SETUP_NONE;
    if (!(rendermode & RENDER_SHADOW)) {
        if (jobj->flags & JOBJ_LIGHTING) {
            flags |= SETUP_NORMAL;
        }
        if (_HSD_TObjGetCurrentByType(NULL, TEX_COORD_REFLECTION) != NULL) {
            flags |= SETUP_NORMAL | SETUP_REFLECTION;
        }
        if (_HSD_TObjGetCurrentByType(NULL, TEX_COORD_HILIGHT) != NULL) {
            flags |= SETUP_NORMAL | SETUP_HIGHLIGHT;
        }
    }
    return flags;
}

static inline void ftPartsSetupZScaleMtx(Mtx src, Mtx dst)
{
    Mtx scale_mtx;
    if (ft_jobj_scale.has_z_scale) {
        C_MTXConcat(ft_jobj_scale.mtx, src, scale_mtx);
        HSD_MtxInverseTranspose(scale_mtx, dst);
    } else {
        HSD_MtxInverseTranspose(src, dst);
    }
}

static inline void ftPartsSetupNrmMtx(HSD_JObj* jobj, Mtx mtx, GXPosNrmMtx id)
{
    if (jobj->flags & JOBJ_LIGHTING) {
        GXLoadNrmMtxImm(mtx, id);
        HSD_PerfCountMtxLoad();
    }
}

static inline void ftPartsSetupTexMtx(Mtx mtx, GXTexMtx id)
{
    GXLoadTexMtxImm(mtx, id, GX_MTX3x4);
    HSD_PerfCountMtxLoad();
}

void ftPartsSetupRigidMtx(HSD_PObj* pobj, Mtx vmtx, Mtx pmtx, u32 rendermode)
{
    HSD_JObj* jobj;
    MtxPtr tmp;          // r29
    PObjSetupFlag flags; // r28

    Mtx mtx;             // sp54
    HSD_JObj* mark_jobj; // sp50
    u32 mark;            // sp4C

    tmp = pmtx;
    jobj = HSD_JObjGetCurrent();
    HSD_PObjGetMtxMark(0, (void**) &mark_jobj, &mark);
    if (mark_jobj != jobj || mark != HSD_MTX_RIGID) {
        HSD_PObjSetMtxMark(0, jobj, HSD_MTX_RIGID);
        GXSetCurrentMtx(GX_PNMTX0);

        GXLoadPosMtxImm(tmp, GX_PNMTX0);
        HSD_PerfCountMtxLoad();
        flags = ftPartsGetSetupFlags(jobj, rendermode);
        if (flags & SETUP_NORMAL) {
            ftPartsSetupZScaleMtx(tmp, mtx);
            ftPartsSetupNrmMtx(jobj, mtx, GX_PNMTX0);

            if (flags & SETUP_NORMAL_PROJECTION) {
                ftPartsSetupTexMtx(mtx, GX_TEXMTX0);
            }
        }
    }
}

void ftPartsSetupSharedVtxMtx(HSD_PObj* pobj, MtxPtr vmtx, MtxPtr pmtx,
                              u32 rendermode)
{
    HSD_JObj* jobj;
    PObjSetupFlag flags = SETUP_NONE; // r28

    Mtx mtx0;            // spE4
    Mtx mtx1;            // spB4
    Mtx tmp;             // sp84
    HSD_JObj* mark_jobj; // sp80
    u32 mark;            // sp7C

    jobj = HSD_JObjGetCurrent();

    HSD_PObjGetMtxMark(0, (void**) &mark_jobj, &mark);
    if (mark_jobj != jobj || mark != HSD_MTX_RIGID) {
        flags |= SETUP_JOINT0;
    }
    HSD_PObjSetMtxMark(0, jobj, HSD_MTX_RIGID);

    HSD_PObjGetMtxMark(1, (void**) &mark_jobj, &mark);
    if (mark_jobj != pobj->u.jobj || mark != HSD_MTX_RIGID) {
        flags |= SETUP_JOINT1;
    }
    HSD_PObjSetMtxMark(1, pobj->u.jobj, HSD_MTX_RIGID);

    if (flags == SETUP_NONE) {
        return;
    }

    flags |= ftPartsGetSetupFlags(jobj, rendermode);

#ifdef MUST_MATCH
    if (flags | SETUP_NORMAL)
#endif
    {
        GXSetCurrentMtx(GX_PNMTX0);

        GXLoadPosMtxImm(pmtx, GX_PNMTX0);
        HSD_PerfCountMtxLoad();
        if (flags & SETUP_NORMAL) {
            ftPartsSetupZScaleMtx(pmtx, mtx0);
            ftPartsSetupNrmMtx(jobj, mtx0, GX_PNMTX0);

            if (flags & SETUP_NORMAL_PROJECTION) {
                ftPartsSetupTexMtx(mtx0, GX_TEXMTX0);
            }
        }
    }

#ifdef MUST_MATCH
    if (flags | SETUP_REFLECTION)
#endif
    {
        HSD_JObjSetupMatrix(pobj->u.jobj);
        C_MTXConcat(vmtx, pobj->u.jobj->mtx, tmp);

        GXLoadPosMtxImm(tmp, GX_PNMTX1);
        HSD_PerfCountMtxLoad();
        if (flags & SETUP_NORMAL) {
            ftPartsSetupZScaleMtx(tmp, mtx1);
            ftPartsSetupNrmMtx(jobj, mtx1, GX_PNMTX1);

            if (flags & SETUP_NORMAL_PROJECTION) {
                ftPartsSetupTexMtx(mtx1, GX_TEXMTX1);
            }
        }
    }
}

void ftPartsSetupEnvelopeMtx(HSD_PObj* pobj, MtxPtr vmtx, MtxPtr pmtx,
                             u32 rendermode)
{
    HSD_JObj* jobj;           // r23
    HSD_SList* envelope_list; // r22
    int i;                    // r21
    MtxPtr node_mtxp;         // r20
    Mtx spAC;                 // spAC
    MtxPtr mtxp;              // r19
    PObjSetupFlag flags;      // r17

    jobj = HSD_JObjGetCurrent();
    HSD_PObjClearMtxMark(NULL, HSD_MTX_ENVELOPE);
    flags = ftPartsGetSetupFlags(jobj, rendermode);
    node_mtxp = _HSD_mkEnvelopeModelNodeMtx(jobj, spAC);
    envelope_list = pobj->u.envelope_list;
    for (i = 0; i < 10 && envelope_list != NULL; i++) {
        Mtx mtx;                // sp7C
        Mtx tmp;                // sp4C
        HSD_Envelope* envelope; // r19
        u32 mtx_id;             // r18
        int envelope_count;     // r17

        envelope = envelope_list->data;
        mtx_id = HSD_Index2PosNrmMtx(i);
        envelope_count = 0;
        HSD_ASSERT(328, envelope);
        if (envelope->weight >= 1.0F) {
            HSD_JObjSetupMatrix(envelope->jobj);
            if (node_mtxp != NULL) {
                C_MTXConcat(envelope->jobj->mtx, envelope->jobj->envelopemtx,
                            mtx);
                mtxp = mtx;
            } else {
                mtxp = envelope->jobj->mtx;
            }
        } else {
            mtx[0][0] = mtx[0][1] = mtx[0][2] = mtx[0][3] = mtx[1][0] =
                mtx[1][1] = mtx[1][2] = mtx[1][3] = mtx[2][0] = mtx[2][1] =
                    mtx[2][2] = mtx[2][3] = 0.0F;
            while (envelope != NULL) {
                HSD_JObj* jp;
                HSD_ASSERT(348, envelope->jobj);
                jp = envelope->jobj;
                HSD_JObjSetupMatrix(jp);
                HSD_ASSERT(351, jp->mtx);
                HSD_ASSERT(352, jp->envelopemtx);
                C_MTXConcat(jp->mtx, jp->envelopemtx, tmp);
                HSD_MtxScaledAdd(tmp, mtx, mtx, envelope->weight);
                envelope = envelope->next;
                envelope_count++;
            }
            mtxp = mtx;
        }
        HSD_PerfCountEnvelopeBlending(envelope_count);
        if (node_mtxp != NULL) {
            C_MTXConcat(mtxp, node_mtxp, mtx);
        }
        C_MTXConcat(vmtx, mtxp, tmp);

        GXLoadPosMtxImm(tmp, mtx_id);
        HSD_PerfCountMtxLoad();
        if (flags & SETUP_NORMAL) {
            ftPartsSetupZScaleMtx(tmp, mtx);
            ftPartsSetupNrmMtx(jobj, mtx, mtx_id);

            if (flags & SETUP_NORMAL_PROJECTION) {
                ftPartsSetupTexMtx(mtx, HSD_Index2TexMtx(i));
            }
        }

        envelope_list = envelope_list->next;
    }
}

void ftParts_PObjSetupMtx(HSD_PObj* pobj, Mtx vmtx, Mtx pmtx, u32 rendermode)
{
    if (!nativeRenderGeometryEnabled || !ft_jobj_scale.has_z_scale) {
        hsdPObj.setup_mtx(pobj, vmtx, pmtx, rendermode);
        return;
    }
    switch (pobj_type(pobj)) {
    case POBJ_SKIN:
        if (pobj->u.jobj == NULL) {
            ftPartsSetupRigidMtx(pobj, vmtx, pmtx, rendermode);
        } else {
            ftPartsSetupSharedVtxMtx(pobj, vmtx, pmtx, rendermode);
        }
        break;
    case POBJ_SHAPEANIM:
        ftPartsSetupRigidMtx(pobj, vmtx, pmtx, rendermode);
        break;
    case POBJ_ENVELOPE:
        ftPartsSetupEnvelopeMtx(pobj, vmtx, pmtx, rendermode);
        break;
    case POBJ_SEPARATE_ENVELOPES:
        HSD_PObjSetupEnvelopeMatrices(pobj, vmtx, rendermode, ft_jobj_scale.mtx);
        break;
    }
}

void ftParts_PObjInfoInit(void)
{
    hsdInitClassInfo(HSD_CLASS_INFO(&ftPObj), HSD_CLASS_INFO(&hsdPObj),
                     "fighter_class_library", "ft_pobj", sizeof(HSD_PObjInfo),
                     sizeof(HSD_PObj));
    HSD_POBJ_INFO(&ftPObj)->load = lbRefract_PObjLoad;
    HSD_POBJ_INFO(&ftPObj)->setup_mtx = ftParts_PObjSetupMtx;
}

void ftPartsPObjSetDefaultClass(void)
{
    HSD_PObjSetDefaultClass(&ftPObj);
}

void ftPartsPObjClearDefaultClass(void)
{
    HSD_PObjSetDefaultClass(NULL);
}

