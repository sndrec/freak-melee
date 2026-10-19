#include "pobj.h"
#include "native_render_phase.h"

#include <math.h> // IWYU pragma: keep
#include <string.h>

#include "aobj.h"
#include "class.h"
#include "debug.h"
#include "displayfunc.h"
#include "forward.h"
#include "id.h"
#include "jobj.h"
#include "memory.h"
#include "mtx.h"
#include "perf.h"
#include "state.h"
#include "shape.h"
#include "tobj.h"
#include "util.h"
#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#include <dolphin/os.h>

static void PObjInfoInit(void);

HSD_PObjInfo hsdPObj = { PObjInfoInit };

static HSD_PObjInfo* default_class = NULL;

// Shape interpolation overwrites every consumed vertex/normal before emitting
// the display list. Stable scratch has no lifetime/allocator state to restore.
#ifdef MELEE_NATIVE
#pragma clang section data = "" bss = ""
#endif
static f32 vertex_buffer[HSD_MAX_SHAPE_VERTICES][3];
static f32 normal_buffer[HSD_MAX_SHAPE_NORMAL_VECTORS][3];
#ifdef MELEE_NATIVE
#pragma clang section data = ".mstate$m"
#endif

static HSD_VtxDescList* prev_vtxdesclist_array = NULL;
static HSD_VtxDescList* prev_vtxdesc = NULL;

static struct {
    void* obj;
    u32 mark;
} mtx_mark[2];

u32 HSD_PObjGetFlags(HSD_PObj* pobj)
{
    if (pobj != NULL) {
        return pobj->flags;
    }
    return 0;
}

void HSD_PObjRemoveAnimByFlags(HSD_PObj* pobj, u32 flags)
{
    if (pobj == NULL) {
        return;
    }

    if (flags & POBJ_ANIM && pobj_type(pobj) == POBJ_SHAPEANIM) {
        HSD_ShapeSet* shape_set = pobj->u.shape_set;
        HSD_AObjRemove(shape_set->aobj);
        shape_set->aobj = NULL;
    }
}

void HSD_PObjRemoveAnimAllByFlags(HSD_PObj* pobj, u32 flags)
{
    HSD_PObj* pp;

    if (pobj == NULL) {
        return;
    }

    for (pp = pobj; pp != NULL; pp = pp->next) {
        HSD_PObjRemoveAnimByFlags(pp, flags);
    }
}

void HSD_PObjAddAnim(HSD_PObj* pobj, HSD_ShapeAnim* shapeanim)
{
    HSD_ShapeSet* st;
    if (pobj == NULL) {
        return;
    }
    HSD_ASSERT(247, pobj_type(pobj) == POBJ_SHAPEANIM && pobj->u.shape_set);
    if (shapeanim == NULL) {
        return;
    }

    st = pobj->u.shape_set;
    if (st->aobj) {
        HSD_AObjRemove(st->aobj);
    }
    st->aobj = HSD_AObjLoadDesc(shapeanim->aobjdesc);
}

void HSD_PObjAddAnimAll(HSD_PObj* pobj, HSD_ShapeAnim* shapeanim)
{
    HSD_PObj* po;
    HSD_ShapeAnim* sa;

    if (pobj == NULL || shapeanim == NULL) {
        return;
    }

    for (po = pobj, sa = shapeanim; po != NULL; po = po->next, sa = next_p(sa))
    {
        HSD_PObjAddAnim(po, sa);
    }
}

void HSD_PObjReqAnimByFlags(HSD_PObj* pobj, f32 startframe, u32 flags)
{
    if (pobj == NULL) {
        return;
    }

    if (flags & POBJ_ANIM && pobj_type(pobj) == POBJ_SHAPEANIM) {
        HSD_AObjReqAnim(pobj->u.shape_set->aobj, startframe);
    }
}

void HSD_PObjReqAnimAllByFlags(HSD_PObj* pobj, f32 startframe, u32 flags)
{
    HSD_PObj* pp;

    if (pobj == NULL) {
        return;
    }

    for (pp = pobj; pp; pp = pp->next) {
        HSD_PObjReqAnimByFlags(pp, startframe, flags);
    }
}

static void ShapeSetSetAnimResult(HSD_ShapeSet* shape_set, u32 type,
                                  HSD_ObjData* val)
{
    if (shape_set->flags & SHAPESET_ADDITIVE) {
        shape_set->blend.bp[type - HSD_A_S_W0] = val->fv;
    } else {
        shape_set->blend.bl = val->fv;
    }
}

static void PObjUpdateFunc(void* obj, int type, HSD_ObjData* val)
{
    HSD_PObj* pobj = HSD_POBJ(obj);
    if (pobj == NULL) {
        return;
    }

    if (pobj_type(pobj) == POBJ_SHAPEANIM) {
        ShapeSetSetAnimResult(pobj->u.shape_set, type, val);
    }
}

void HSD_PObjAnim(HSD_PObj* pobj)
{
    if (pobj != NULL && pobj_type(pobj) == POBJ_SHAPEANIM) {
        HSD_AObjInterpretAnim(pobj->u.shape_set->aobj, pobj, PObjUpdateFunc);
    }
}

void HSD_PObjAnimAll(HSD_PObj* pobj)
{
    HSD_PObj* pp;

    if (pobj != NULL) {
        for (pp = pobj; pp != NULL; pp = pp->next) {
            HSD_PObjAnim(pp);
        }
    }
}

static HSD_Envelope* HSD_EnvelopeAlloc(void)
{
    HSD_Envelope* envelope = hsdAllocMemPiece(sizeof(HSD_Envelope));
    HSD_ASSERT(441, envelope);
    memset(envelope, 0, sizeof(HSD_Envelope));
    return envelope;
}

static void HSD_EnvelopeFree(HSD_Envelope* env)
{
    if (env != NULL) {
        hsdFreeMemPiece(env, sizeof(HSD_Envelope));
    }
}

static void HSD_EnvelopeListFree(HSD_SList* list)
{
    while (list) {
        HSD_Envelope* env = list->data;
        while (env) {
            HSD_Envelope* next = env->next;
            HSD_JObjUnrefThis(env->jobj);
            HSD_EnvelopeFree(env);
            env = next;
        }
        list = HSD_SListRemove(list);
    }
}

static HSD_SList* loadEnvelopeDesc(HSD_EnvelopeDesc** edesc_p)
{
    HSD_SList* list = NULL;
    HSD_SList** list_p = &list;

    if (edesc_p == NULL) {
        return NULL;
    }

    while (*edesc_p) {
        HSD_Envelope* envelope = NULL;
        HSD_Envelope** env_p = &envelope;
        HSD_EnvelopeDesc* edesc = *edesc_p;

        while (edesc->joint) {
            *env_p = HSD_EnvelopeAlloc();
            (*env_p)->weight = edesc->weight;
            env_p = &((*env_p)->next);
            edesc++;
        }

        (*list_p) = HSD_SListAlloc();
        (*list_p)->data = envelope;
        list_p = &((*list_p)->next);
        edesc_p++;
    }
    return list;
}

static void HSD_ShapeSetFree(HSD_ShapeSet* shape_set)
{
    if (!shape_set) {
        return;
    }
    hsdFreeMemPiece(shape_set, sizeof(HSD_ShapeSet));
}

static void HSD_ShapeSetRemove(HSD_ShapeSet* shape_set)
{
    if (shape_set == NULL) {
        return;
    }

    if (shape_set->flags & SHAPESET_ADDITIVE) {
        hsdFreeMemPiece(shape_set->blend.bp, shape_set->nb_shape * sizeof(f32));
    }

    HSD_AObjRemove(shape_set->aobj);
    HSD_PObjRemoveAll(shape_set->skin);
    HSD_ShapeSetFree(shape_set);
}

static HSD_ShapeSet* loadShapeSetDesc(HSD_ShapeSetDesc* sdesc)
{
    int i;

    HSD_ShapeSet* shape_set = hsdAllocMemPiece(sizeof(HSD_ShapeSet));
    HSD_ASSERT(498, shape_set);
    memset(shape_set, 0, sizeof(HSD_ShapeSet));
    shape_set->flags = sdesc->flags;
    shape_set->nb_shape = sdesc->nb_shape;
    shape_set->nb_vertex_index = sdesc->nb_vertex_index;
    shape_set->vertex_desc = sdesc->vertex_desc;
    shape_set->vertex_idx_list = sdesc->vertex_idx_list;
    shape_set->nb_normal_index = sdesc->nb_normal_index;
    shape_set->normal_desc = sdesc->normal_desc;
    shape_set->normal_idx_list = sdesc->normal_idx_list;
    if (shape_set->flags & SHAPESET_ADDITIVE) {
        shape_set->blend.bp =
            shape_set->nb_shape ? (f32*) hsdAllocMemPiece(
                shape_set->nb_shape * sizeof(f32)) : NULL;
        for (i = 0; i < shape_set->nb_shape; i++) {
            shape_set->blend.bp[i] = sdesc->default_blend ? sdesc->default_blend[i] : 0.0f;
        }
    } else {
        shape_set->blend.bl = sdesc->default_blend ? sdesc->default_blend[0] : 0.0f;
    }
    shape_set->aobj = NULL;
    shape_set->target_names = sdesc->target_names;
    shape_set->skin = sdesc->flags&SHAPESET_SKIN ? HSD_PObjLoadDesc(sdesc->skin) : NULL;
    HSD_ASSERT(0, !(sdesc->flags&SHAPESET_SKIN) || shape_set->skin);
    return shape_set;
}

static s32 PObjLoad(HSD_PObj* pobj, HSD_PObjDesc* desc)
{
    // The factory has already linked the initialized tail.
    pobj->verts = desc->verts;
    pobj->flags = desc->flags;
    pobj->n_display = desc->n_display;
    pobj->display = desc->display;
    switch (pobj_type(pobj)) {
    case POBJ_SHAPEANIM:
        pobj->u.shape_set = loadShapeSetDesc(desc->u.shape_set);
        break;

    case POBJ_ENVELOPE:
        pobj->u.envelope_list = loadEnvelopeDesc(desc->u.envelope_p);
        break;
    case POBJ_SEPARATE_ENVELOPES:
        pobj->u.matrices = hsdAllocMemPiece(sizeof(*pobj->u.matrices));
        HSD_ASSERT(0, pobj->u.matrices);
        pobj->u.matrices->positions = loadEnvelopeDesc(desc->u.matrices->positions);
        pobj->u.matrices->textures = loadEnvelopeDesc(desc->u.matrices->textures);
        break;

    case POBJ_SKIN:
        break;

    default:
        HSD_Panic(__FILE__, 580, "pobj: unexected type.\n");
    }

    // _HSD_NeedCacheInvalidate(HSD_CACHE_VTX); // NOTE: This does not exist in
    // Melee's sysdolphin, but is a potential optimization

    return 0;
}

HSD_PObj* HSD_PObjLoadDesc(HSD_PObjDesc* pobjdesc)
{
    HSD_PObj* pending = NULL;
    for (; pobjdesc != NULL; pobjdesc = pobjdesc->next) {
        HSD_PObj* pobj;
        HSD_ClassInfo* info;

        if (!pobjdesc->class_name ||
            !(info = hsdSearchClassInfo(pobjdesc->class_name)))
        {
            pobj = HSD_PObjAlloc();
        } else {
            pobj = hsdNew(info);
            HSD_ASSERT(605, pobj);
        }
        // Preserve the original forward allocation / reverse load order.
        // Before initialization, verts holds this object's source descriptor;
        // the temporary reverse list needs no recursion or scratch allocation.
        pobj->verts = (HSD_VtxDescList*) pobjdesc;
        pobj->next = pending;
        pending = pobj;
    }
    HSD_PObj* head = NULL;
    while (pending) {
        HSD_PObj* pobj = pending;
        HSD_PObjDesc* desc = (HSD_PObjDesc*) pobj->verts;
        pending = pobj->next;
        pobj->next = head;
        HSD_POBJ_METHOD(pobj)->load(pobj, desc);
        head = pobj;
    }
    return head;
}

void HSD_PObjRemove(HSD_PObj* pobj)
{
    hsdDelete(pobj);
}

void HSD_PObjRemoveAll(HSD_PObj* pobj)
{
    HSD_PObj* next;

    while (pobj != NULL) {
        next = pobj->next;
        HSD_PObjRemove(pobj);
        pobj = next;
    }
}

HSD_PObjInfo* HSD_PObjGetDefaultClass(void)
{
    return default_class ? default_class : &hsdPObj;
}

void HSD_PObjSetDefaultClass(HSD_PObjInfo* info)
{
    if (info != NULL) {
        HSD_ASSERT(673, hsdIsDescendantOf(info, &hsdPObj));
    }
    default_class = info;
}

HSD_PObj* HSD_PObjAlloc(void)
{
    HSD_PObj* pobj = hsdNew((HSD_ClassInfo*) (HSD_PObjGetDefaultClass()));
    HSD_ASSERT(703, pobj);
    return pobj;
}

void HSD_PObjFree(HSD_PObj* pobj)
{
    if (pobj) {
        HSD_CLASS_METHOD(pobj)->destroy((HSD_Class*) pobj);
    }
}

static void resolveEnvelope(HSD_SList* list, HSD_EnvelopeDesc** edesc_p)
{
    if (list == NULL || edesc_p == NULL) {
        return;
    }

    for (; list && *edesc_p; list = list->next, edesc_p++) {
        HSD_Envelope* env = list->data;
        HSD_EnvelopeDesc* edesc = *edesc_p;

        while (env && edesc->joint) {
            HSD_JObjUnrefThis(env->jobj);
            env->jobj = HSD_IDGetData((uintptr_t) edesc->joint, NULL);
            HSD_ASSERT(736, env->jobj);
            HSD_JObjRefThis(env->jobj);
            env = env->next;
            edesc++;
        }
    }
}

void HSD_PObjResolveRefs(HSD_PObj* pobj, HSD_PObjDesc* pdesc)
{
    if (!pobj || !pdesc) {
        return;
    }

    switch (pobj_type(pobj)) {
    case POBJ_SHAPEANIM:
        HSD_PObjResolveRefsAll(pobj->u.shape_set->skin,pdesc->u.shape_set->skin);
        break;
    case POBJ_ENVELOPE:
        resolveEnvelope(pobj->u.envelope_list, pdesc->u.envelope_p);
        break;
    case POBJ_SEPARATE_ENVELOPES:
        resolveEnvelope(pobj->u.matrices->positions, pdesc->u.matrices->positions);
        resolveEnvelope(pobj->u.matrices->textures, pdesc->u.matrices->textures);
        break;

    case POBJ_SKIN:
        HSD_JObjUnrefThis(pobj->u.jobj);
        pobj->u.jobj = NULL;
        if (pdesc->u.joint != NULL) {
            pobj->u.jobj = HSD_IDGetData((uintptr_t) pdesc->u.joint, NULL);
            HSD_ASSERT(0x2FB, pobj->u.jobj);
            HSD_JObjRefThis(pobj->u.jobj);
        }
        break;

    default:
        break;
    }
}

void HSD_PObjResolveRefsAll(HSD_PObj* pobj, HSD_PObjDesc* pdesc)
{
    for (; pobj != NULL && pdesc != NULL;
         pobj = pobj->next, pdesc = pdesc->next)
    {
        HSD_PObjResolveRefs(pobj, pdesc);
    }
}

void HSD_ClearVtxDesc(void)
{
    GXClearVtxDesc();
    prev_vtxdesclist_array = 0;
    prev_vtxdesc = 0;
}

static void setupArrayDesc(HSD_VtxDescList* desc_list)
{
    HSD_VtxDescList* desc;

    if (prev_vtxdesclist_array != desc_list) {
        for (desc = desc_list; desc->attr != GX_VA_NULL; desc++) {
            if (desc->attr_type == GX_INDEX8 || desc->attr_type == GX_INDEX16) {
                GXSETARRAY(desc->attr, desc->vertex, desc->vertex_size,
                           desc->stride, desc->vertex_little_endian);
            }
        }
        prev_vtxdesclist_array = desc_list;
    }
}

static void setupVtxDesc(HSD_PObj* pobj)
{
    HSD_VtxDescList* desc;

    if (prev_vtxdesc != pobj->verts) {
        GXClearVtxDesc();
        for (desc = pobj->verts; desc->attr != GX_VA_NULL; desc++) {
            GXSetVtxDesc(desc->attr, desc->attr_type);
            switch (desc->attr) {
            case GX_VA_PNMTXIDX:
            case GX_VA_TEX0MTXIDX:
            case GX_VA_TEX1MTXIDX:
            case GX_VA_TEX2MTXIDX:
            case GX_VA_TEX3MTXIDX:
            case GX_VA_TEX4MTXIDX:
            case GX_VA_TEX5MTXIDX:
            case GX_VA_TEX6MTXIDX:
            case GX_VA_TEX7MTXIDX:
                break;
            default:
                GXSetVtxAttrFmt(GX_VTXFMT0, desc->attr, desc->comp_cnt,
                                desc->comp_type, desc->frac);
            }
        }
        prev_vtxdesc = pobj->verts;
    }
}

static void setupShapeAnimArrayDesc(HSD_VtxDescList* desc_list)
{
    HSD_VtxDescList* desc;

    for (desc = desc_list; desc->attr != GX_VA_NULL; desc++) {
        if (desc->attr_type == GX_INDEX8 || desc->attr_type == GX_INDEX16) {
            switch (desc->attr) {
            case GX_VA_POS:
            case GX_VA_NRM:
            case GX_VA_NBT:
                break;
            default:
                GXSETARRAY(desc->attr, desc->vertex, desc->vertex_size,
                           desc->stride, desc->vertex_little_endian);
            }
        }
    }
    prev_vtxdesclist_array = NULL;
}

static void setupShapeAnimVtxDesc(HSD_PObj* pobj)
{
    HSD_VtxDescList* desc;

    GXClearVtxDesc();
    for (desc = pobj->verts; desc->attr != GX_VA_NULL; desc++) {
        switch (desc->attr) {
        case GX_VA_NRM:
        case GX_VA_POS:
        case GX_VA_NBT:
            GXSetVtxDesc(desc->attr, GX_DIRECT);
            GXSetVtxAttrFmt(GX_VTXFMT0, desc->attr, desc->comp_cnt, GX_F32, 0);
            break;

        case GX_VA_PNMTXIDX:
        case GX_VA_TEX0MTXIDX:
        case GX_VA_TEX1MTXIDX:
        case GX_VA_TEX2MTXIDX:
        case GX_VA_TEX3MTXIDX:
        case GX_VA_TEX4MTXIDX:
        case GX_VA_TEX5MTXIDX:
        case GX_VA_TEX6MTXIDX:
        case GX_VA_TEX7MTXIDX:
            GXSetVtxDesc(desc->attr, desc->attr_type);
            break;

        default:
            GXSetVtxDesc(desc->attr, desc->attr_type);
            GXSetVtxAttrFmt(GX_VTXFMT0, desc->attr, desc->comp_cnt,
                            desc->comp_type, desc->frac);
        }
    }
    prev_vtxdesc = NULL;
}

static void interpretShapeAnimDisplayList(HSD_PObj* pobj, const HSD_ShapeSet* shape, float (*vertex)[3],
                                          float (*normal)[3])
{
    u8* dl = pobj->display;
    int length = pobj->n_display << 5;
    int l;

    for (l = 0; l + 3 < length;) {
        int n = dl[1] << 8 | dl[2];
        int m = 3;
        int i, j;

        if ((dl[0] & GX_OPCODE_MASK) == GX_NOP) {
            break;
        }
        GXBegin((GXPrimitive) (dl[0] & GX_OPCODE_MASK),
                (GXVtxFmt) (dl[0] & GX_VAT_MASK), n);
        for (i = 0; i < n; i++) {
            for (j = 0;; j++) {
                HSD_VtxDescList* desc = &pobj->verts[j];
                if (desc->attr == GX_VA_NULL) {
                    break;
                } else {
                    u16 idx = dl[m++];
                    switch (desc->attr) {
                    case GX_VA_PNMTXIDX:
                    case GX_VA_TEX0MTXIDX:
                    case GX_VA_TEX1MTXIDX:
                    case GX_VA_TEX2MTXIDX:
                    case GX_VA_TEX3MTXIDX:
                    case GX_VA_TEX4MTXIDX:
                    case GX_VA_TEX5MTXIDX:
                    case GX_VA_TEX6MTXIDX:
                    case GX_VA_TEX7MTXIDX:
                        GXTexCoord1u8(idx);
                        break;

                    case GX_VA_POS:
                        if (desc->attr_type == GX_INDEX16) {
                            idx = (idx << 8) | dl[m++];
                        }
                        // Only this draw's regenerated prefix is valid scratch.
                        HSD_ASSERT(0, idx < shape->nb_vertex_index);
                        GXPosition3f32(vertex[idx][0], vertex[idx][1],
                                       vertex[idx][2]);
                        break;

                    case GX_VA_NRM:
                        if (desc->attr_type == GX_INDEX16) {
                            idx = (idx << 8) | dl[m++];
                        }
                        HSD_ASSERT(0, idx < shape->nb_normal_index);
                        GXNormal3f32(normal[idx][0], normal[idx][1],
                                     normal[idx][2]);
                        break;

                    case GX_VA_NBT:
                        if (desc->attr_type == GX_INDEX16) {
                            idx = (idx << 8) | dl[m++];
                        }
                        HSD_ASSERT(0, idx < shape->nb_normal_index);
                        idx *= 3;
                        GXNormal3f32(normal[idx + 0][0], normal[idx + 0][1],
                                     normal[idx + 0][2]);
                        GXNormal3f32(normal[idx + 1][0], normal[idx + 1][1],
                                     normal[idx + 1][2]);
                        GXNormal3f32(normal[idx + 2][0], normal[idx + 2][1],
                                     normal[idx + 2][2]);
                        break;

                    case GX_VA_TEX0:
                    case GX_VA_TEX1:
                    case GX_VA_TEX2:
                    case GX_VA_TEX3:
                    case GX_VA_TEX4:
                    case GX_VA_TEX5:
                    case GX_VA_TEX6:
                    case GX_VA_TEX7:
                        if (desc->attr_type == GX_INDEX16) {
                            idx = (idx << 8) | dl[m++];
                            GXTexCoord1x16(idx);
                        } else {
                            GXTexCoord1x8(idx);
                        }
                        break;

                    case GX_VA_CLR0:
                    case GX_VA_CLR1:
                        if (desc->attr_type == GX_INDEX16) {
                            idx = (idx << 8) | dl[m++];
                            GXColor1x16(idx);
                        } else if (desc->attr_type == GX_INDEX8) {
                            GXColor1x8(idx);
                        } else {
                            switch (desc->comp_type) {
                            case GX_RGB565:
                            case GX_RGBA4:
                                GXColor1u16((idx << 8) | dl[m++]);
                                break;
                            case GX_RGB8:
                            case GX_RGBA6:
                                GXColor3u8(idx, dl[m], dl[m + 1]);
                                m += 2;
                                break;
                            case GX_RGBA8:
                            case GX_RGBX8:
                                GXColor4u8(idx, dl[m], dl[m + 1], dl[m + 2]);
                                m += 3;
                                break;
                            }
                        }
                        break;
                    default:
                        if (desc->attr_type == GX_INDEX16) {
                            idx = (idx << 8) | dl[m++];
                        }
                        OSReport("attr(%d) is not supported by sysdolphin\n",
                                 desc->attr);
                        break;
                    }
                }
            }
        }
        GXEnd();
        l += m;
        dl += m;
    }
}

void HSD_PObjClearMtxMark(void* obj, u32 mark)
{
    int i;

    for (i = 0; i < 2; i++) {
        mtx_mark[i].obj = obj;
        mtx_mark[i].mark = mark;
    }
}

void HSD_PObjSetMtxMark(int idx, void* obj, u32 mark)
{
    if (idx < 0 || idx >= 2) return;
    mtx_mark[idx].obj = obj;
    mtx_mark[idx].mark = mark;
}

void HSD_PObjGetMtxMark(int idx, void** obj, u32* mark)
{
    HSD_ASSERT(1655, obj);
    HSD_ASSERT(1656, mark);

    if (idx < 0 || 2 <= idx) {
        *obj = NULL;
        *mark = 0;
    } else {
        *obj = mtx_mark[idx].obj;
        *mark = mtx_mark[idx].mark;
    }
}

static PObjSetupFlag GetSetupFlags(HSD_JObj* jobj, u32 rendermode)
{
    PObjSetupFlag flags = SETUP_NONE;
    if (!(rendermode & 0x4000000)) {
        if (jobj->flags & JOBJ_LIGHTING) {
            flags |= SETUP_NORMAL;
        }

        if (_HSD_TObjGetCurrentByType(NULL, TEX_COORD_REFLECTION)) {
            flags |= SETUP_NORMAL | SETUP_REFLECTION;
        }

        if (_HSD_TObjGetCurrentByType(NULL, TEX_COORD_HILIGHT)) {
            flags |= SETUP_NORMAL | SETUP_HIGHLIGHT;
        }
    }

    return flags;
}

static void SetupRigidModelMtx(HSD_PObj* pobj, Mtx vmtx, Mtx pmtx,
                               u32 rendermode)
{
    HSD_JObj* jobj;
    Mtx n;
    PObjSetupFlag flags;

    jobj = HSD_JObjGetCurrent();

    {
        void* obj;
        u32 mark;

        HSD_PObjGetMtxMark(0, &obj, &mark);
        if (obj == jobj && mark == HSD_MTX_RIGID) {
            return;
        }
        HSD_PObjSetMtxMark(0, jobj, HSD_MTX_RIGID);
    }

    GXSetCurrentMtx(GX_PNMTX0);
    GXLoadPosMtxImm(pmtx, GX_PNMTX0);
    HSD_PerfCountMtxLoad();

    flags = GetSetupFlags(jobj, rendermode);

    if (flags & SETUP_NORMAL) {
        HSD_MtxInverseTranspose(pmtx, n);
        if (jobj->flags & JOBJ_LIGHTING) {
            GXLoadNrmMtxImm(n, GX_PNMTX0);
            HSD_PerfCountMtxLoad();
        }
        if (flags & SETUP_NORMAL_PROJECTION) {
            GXLoadTexMtxImm(n, GX_TEXMTX0, GX_MTX3x4);
            HSD_PerfCountMtxLoad();
        }
    }
}

static void SetupSharedVtxModelMtx(HSD_PObj* pobj, Mtx vmtx, Mtx pmtx,
                                   u32 rendermode)
{
    HSD_JObj* jobj;
    Mtx n0, n1, m;
    PObjSetupFlag flags = SETUP_NONE;

    jobj = HSD_JObjGetCurrent();
    {
        void* obj;
        u32 mark;

        HSD_PObjGetMtxMark(0, &obj, &mark);
        if (obj != jobj || mark != HSD_MTX_RIGID) {
            flags |= SETUP_JOINT0;
        }

        HSD_PObjGetMtxMark(1, &obj, &mark);
        if (obj != pobj->u.jobj || mark != HSD_MTX_RIGID) {
            flags |= SETUP_JOINT1;
        }
    }

    if (flags == SETUP_NONE) {
        return;
    }

    HSD_PObjSetMtxMark(0, jobj, HSD_MTX_RIGID);
    HSD_PObjSetMtxMark(1, pobj->u.jobj, HSD_MTX_RIGID);
    flags |= GetSetupFlags(jobj, rendermode);

#ifdef MUST_MATCH
    if (flags | SETUP_JOINT0)
#endif
    {
        GXSetCurrentMtx(GX_PNMTX0);
        GXLoadPosMtxImm(pmtx, GX_PNMTX0);
        HSD_PerfCountMtxLoad();

        if (flags & SETUP_NORMAL) {
            HSD_MtxInverseTranspose(pmtx, n0);
            if (jobj->flags & 0x80) {
                GXLoadNrmMtxImm(n0, GX_PNMTX0);
                HSD_PerfCountMtxLoad();
            }
            if (flags & SETUP_NORMAL_PROJECTION) {
                GXLoadTexMtxImm(n0, GX_TEXMTX0, GX_MTX3x4);
                HSD_PerfCountMtxLoad();
            }
        }
    }
#ifdef MUST_MATCH
    if (flags | SETUP_JOINT1)
#endif
    {
        ///@todo Unused stack
        u8 _[4];
        HSD_JObjSetupMatrix(pobj->u.jobj);
        C_MTXConcat(vmtx, pobj->u.jobj->mtx, m);
        GXLoadPosMtxImm(m, GX_PNMTX1);
        HSD_PerfCountMtxLoad();

        if (flags & SETUP_NORMAL) {
            HSD_MtxInverseTranspose(m, n1);
            if (jobj->flags & 0x80) {
                GXLoadNrmMtxImm(n1, GX_PNMTX1);
                HSD_PerfCountMtxLoad();
            }
            if (flags & SETUP_NORMAL_PROJECTION) {
                GXLoadTexMtxImm(n1, GX_TEXMTX1, GX_MTX3x4);
                HSD_PerfCountMtxLoad();
            }
        }
    }
}

static void SetupEnvelopeModelMtx(HSD_PObj* pobj, Mtx vmtx, Mtx pmtx,
                                  u32 rendermode)
{
    HSD_JObj* jobj;
    HSD_SList* list;
    int MtxIdx = 0;
    MtxPtr right;
    Mtx mtx;
    PObjSetupFlag flags = SETUP_NONE;

    jobj = HSD_JObjGetCurrent();
    HSD_PObjClearMtxMark(NULL, HSD_MTX_ENVELOPE);
    flags = GetSetupFlags(jobj, rendermode);
    right = _HSD_mkEnvelopeModelNodeMtx(jobj, mtx);

    for (MtxIdx = 0, list = pobj->u.envelope_list; MtxIdx < 10 && list;
         MtxIdx++, list = list->next)
    {
        Mtx mtx, tmp;
        MtxPtr mtxp;
        HSD_Envelope* envelope = list->data;
        s32 mtx_no = HSD_Index2PosNrmMtx(MtxIdx);
        int perf = 0;

        HSD_ASSERT(1872, envelope);
        if (envelope->weight >= (1.0f - FLT_EPSILON)) {
            HSD_JObjSetupMatrix(envelope->jobj);
            if (right) {
                C_MTXConcat(envelope->jobj->mtx, envelope->jobj->envelopemtx,
                          mtx);
                mtxp = mtx;
            } else {
                mtxp = envelope->jobj->mtx;
            }
        } else {
            mtx[0][0] = mtx[0][1] = mtx[0][2] = mtx[0][3] = mtx[1][0] =
                mtx[1][1] = mtx[1][2] = mtx[1][3] = mtx[2][0] = mtx[2][1] =
                    mtx[2][2] = mtx[2][3] = 0.0f;
            while (envelope) {
                HSD_JObj* jp;

                HSD_ASSERT(1892, envelope->jobj);
                jp = envelope->jobj;
                HSD_JObjSetupMatrix(jp);
                HSD_ASSERT(1895, jp->mtx);
                HSD_ASSERT(1896, jp->envelopemtx);

                C_MTXConcat(jp->mtx, jp->envelopemtx, tmp);
                HSD_MtxScaledAdd(tmp, mtx, mtx, envelope->weight);
                perf++;
                envelope = envelope->next;
            }
            mtxp = mtx;
        }
        HSD_PerfCountEnvelopeBlending(perf);
        if (right) {
            C_MTXConcat(mtxp, right, mtx);
        }
        C_MTXConcat(vmtx, mtxp, tmp);
        GXLoadPosMtxImm(tmp, mtx_no);
        HSD_PerfCountMtxLoad();

        if (flags & SETUP_NORMAL) {
            HSD_MtxInverseTranspose(tmp, mtx);
            if (jobj->flags & JOBJ_LIGHTING) {
                GXLoadNrmMtxImm(mtx, mtx_no);
                HSD_PerfCountMtxLoad();
            }
            if (flags & SETUP_NORMAL_PROJECTION) {
                GXLoadTexMtxImm(mtx, HSD_Index2TexMtx(MtxIdx), GX_MTX3x4);
                HSD_PerfCountMtxLoad();
            }
        }
    }
}

// Native authored meshes use the same envelope math with independent position
// and texture slots. Stock polygons retain their original setup path.
void HSD_PObjSetupEnvelopeMatrices(HSD_PObj* pobj, Mtx view, u32 rendermode,
                                   MtxPtr normal_adjustment)
{
    HSD_JObj* owner = HSD_JObjGetCurrent();
    const PObjSetupFlag flags = GetSetupFlags(owner, rendermode);
    Mtx right_storage;
    MtxPtr right = _HSD_mkEnvelopeModelNodeMtx(owner, right_storage);
    HSD_PObjClearMtxMark(NULL, HSD_MTX_ENVELOPE);
    for (unsigned texture = 0; texture < 2; ++texture) {
        HSD_SList* list = texture ? pobj->u.matrices->textures : pobj->u.matrices->positions;
        for (unsigned slot = 0; list && slot < 10; list = list->next, ++slot) {
            Mtx matrix = { 0 }, temporary, transformed;
            HSD_Envelope* binding = list->data;
            unsigned influences = 0;
            // Admission requires normalized inverse-bind envelopes. Evaluate
            // both sets even without a reflected material, matching hidden pose
            // reconstruction's joint/constraint processing.
            for (; binding; binding = binding->next) {
                HSD_JObjSetupMatrix(binding->jobj);
                C_MTXConcat(binding->jobj->mtx, binding->jobj->envelopemtx, temporary);
                HSD_MtxScaledAdd(temporary, matrix, matrix, binding->weight);
                ++influences;
            }
            HSD_PerfCountEnvelopeBlending(influences);
            if (right) {
                C_MTXConcat(matrix, right, temporary);
                C_MTXCopy(temporary, matrix);
            }
            C_MTXConcat(view, matrix, transformed);
            const u32 position_slot = HSD_Index2PosNrmMtx(slot);
            if (!texture) {
                GXLoadPosMtxImm(transformed, position_slot);
                HSD_PerfCountMtxLoad();
            }
            if (!(flags & SETUP_NORMAL)) continue;
            if (normal_adjustment) {
                C_MTXConcat(normal_adjustment, transformed, temporary);
                HSD_MtxInverseTranspose(temporary, matrix);
            } else HSD_MtxInverseTranspose(transformed, matrix);
            if (!texture && (owner->flags & JOBJ_LIGHTING)) {
                GXLoadNrmMtxImm(matrix, position_slot);
                HSD_PerfCountMtxLoad();
            }
            if (texture && (flags & SETUP_NORMAL_PROJECTION)) {
                GXLoadTexMtxImm(matrix, HSD_Index2TexMtx(slot), GX_MTX3x4);
                HSD_PerfCountMtxLoad();
            }
        }
    }
}

#ifdef MELEE_NATIVE
static void PObjPreparePose(HSD_PObj* pobj)
{
    HSD_JObj* jobj = HSD_JObjGetCurrent();
    switch (pobj_type(pobj)) {
    case POBJ_SKIN:
        if (pobj->u.jobj) {
            void* obj0;
            void* obj1;
            u32 mark0;
            u32 mark1;
            HSD_PObjGetMtxMark(0, &obj0, &mark0);
            HSD_PObjGetMtxMark(1, &obj1, &mark1);
            if (obj0 == jobj && mark0 == HSD_MTX_RIGID &&
                obj1 == pobj->u.jobj && mark1 == HSD_MTX_RIGID) {
                return;
            }
            HSD_PObjSetMtxMark(0, jobj, HSD_MTX_RIGID);
            HSD_PObjSetMtxMark(1, pobj->u.jobj, HSD_MTX_RIGID);
            HSD_JObjSetupMatrix(pobj->u.jobj);
            break;
        }
        /* Rigid meshes have no additional joint dependencies. */
        HSD_PObjSetMtxMark(0, jobj, HSD_MTX_RIGID);
        break;
    case POBJ_SHAPEANIM:
        if(pobj->u.shape_set->skin) {
            for(HSD_PObj* batch=pobj->u.shape_set->skin;batch;batch=batch->next)PObjPreparePose(batch);
        } else HSD_PObjSetMtxMark(0, jobj, HSD_MTX_RIGID);
        break;
    case POBJ_ENVELOPE:
    case POBJ_SEPARATE_ENVELOPES: {
        HSD_SList* list;
        int index;
        HSD_PObjClearMtxMark(NULL, HSD_MTX_ENVELOPE);
        /* Match the draw path's palette limit and single-weight shortcut.
         * SetupMatrix can evaluate joint constraints and dirty parents; keep
         * that order without constructing camera/normal/skin matrices. */
        const unsigned sets = pobj_type(pobj) == POBJ_SEPARATE_ENVELOPES ? 2 : 1;
        for (unsigned set = 0; set < sets; ++set) {
            list = sets == 1 ? pobj->u.envelope_list : set ? pobj->u.matrices->textures : pobj->u.matrices->positions;
            for (index = 0; index < 10 && list;
                 ++index, list = list->next) {
                HSD_Envelope* envelope = list->data;
                HSD_JObjSetupMatrix(envelope->jobj);
                if (envelope->weight >= (1.0f - FLT_EPSILON)) {
                    continue;
                }
                for (envelope = envelope->next; envelope;
                     envelope = envelope->next) {
                    HSD_JObjSetupMatrix(envelope->jobj);
                }
            }
        }
        break;
    }
    }
}
#endif

static void PObjSetupMtx(HSD_PObj* pobj, Mtx vmtx, Mtx pmtx, u32 rendermode)
{
#ifdef MELEE_NATIVE
    if (!nativeRenderGeometryEnabled) {
        PObjPreparePose(pobj);
        return;
    }
#endif
    switch (pobj_type(pobj)) {
    case POBJ_SKIN:
        if (!pobj->u.jobj) {
            SetupRigidModelMtx(pobj, vmtx, pmtx, rendermode);
        } else {
            SetupSharedVtxModelMtx(pobj, vmtx, pmtx, rendermode);
        }
        break;
    case POBJ_SHAPEANIM:
        if(!pobj->u.shape_set->skin)SetupRigidModelMtx(pobj, vmtx, pmtx, rendermode);
        break;
    case POBJ_ENVELOPE:
        SetupEnvelopeModelMtx(pobj, vmtx, pmtx, rendermode);
        break;
    case POBJ_SEPARATE_ENVELOPES:
        HSD_PObjSetupEnvelopeMatrices(pobj, vmtx, rendermode, NULL);
        break;
    }
}

static void PObjDispSimplePrimitive(HSD_PObj* pobj, u32 rendermode)
{
    setupArrayDesc(pobj->verts);
    setupVtxDesc(pobj);

    GXCallDisplayList(pobj->display, pobj->n_display << 5);
}

static void PObjDispShapeAnim(HSD_PObj* pobj, Mtx view, Mtx model, u32 rendermode)
{
    HSD_ASSERT(0x7B1, pobj->u.shape_set);
    HSD_ShapeSet* shape=pobj->u.shape_set;
    HSD_ASSERT(0, HSD_ShapeSetEvaluate(shape, vertex_buffer, normal_buffer));
    if(shape->skin) {
        for(HSD_PObj* batch=shape->skin;batch;batch=batch->next) {
            HSD_POBJ_METHOD(batch)->setup_mtx(batch,view,model,rendermode);
            setupShapeAnimArrayDesc(batch->verts);setupShapeAnimVtxDesc(batch);
            interpretShapeAnimDisplayList(batch,shape,vertex_buffer,normal_buffer);
        }
    } else {
        setupShapeAnimArrayDesc(pobj->verts);setupShapeAnimVtxDesc(pobj);
        interpretShapeAnimDisplayList(pobj,shape,vertex_buffer,normal_buffer);
    }
}

void HSD_PObjDisp(HSD_PObj* pobj, Mtx vmtx, Mtx pmtx, u32 rendermode)
{
    switch (pobj->flags & (POBJ_CULLFRONT | POBJ_CULLBACK)) {
    case 0x0:
        HSD_StateSetCullMode(GX_CULL_NONE);
        break;
    case POBJ_CULLFRONT:
        HSD_StateSetCullMode(GX_CULL_FRONT);
        break;
    case POBJ_CULLBACK:
        HSD_StateSetCullMode(GX_CULL_BACK);
        break;
    case POBJ_CULLFRONT | POBJ_CULLBACK:
        return;
    }

    HSD_POBJ_METHOD(pobj)->setup_mtx(pobj, vmtx, pmtx, rendermode);
#ifdef MELEE_NATIVE
    if (!nativeRenderGeometryEnabled) {
        ++nativeRenderModelPrimitivesSkipped;
        return;
    }
#endif
    if (pobj_type(pobj) == POBJ_SHAPEANIM) {
        PObjDispShapeAnim(pobj, vmtx, pmtx, rendermode);
    } else {
        PObjDispSimplePrimitive(pobj, rendermode);
    }
}

static void PObjRelease(HSD_Class* o)
{
    HSD_PObj* pobj = HSD_POBJ(o);

    switch (pobj_type(pobj)) {
    case POBJ_SHAPEANIM:
        HSD_ShapeSetRemove(pobj->u.shape_set);
        break;
    case POBJ_ENVELOPE:
        HSD_EnvelopeListFree(pobj->u.envelope_list);
        break;
    case POBJ_SEPARATE_ENVELOPES:
        HSD_EnvelopeListFree(pobj->u.matrices->positions);
        HSD_EnvelopeListFree(pobj->u.matrices->textures);
        hsdFreeMemPiece(pobj->u.matrices, sizeof(*pobj->u.matrices));
        break;
    case POBJ_SKIN:
        HSD_JObjUnrefThis(pobj->u.jobj);
        break;
    default:
        break;
    }
    HSD_PARENT_INFO(&hsdPObj)->release(o);
}

static void PObjAmnesia(HSD_ClassInfo* info)
{
    if (info == HSD_CLASS_INFO(default_class)) {
        default_class = NULL;
    }
    if (info == HSD_CLASS_INFO(&hsdPObj)) {
        prev_vtxdesclist_array = NULL;
        prev_vtxdesc = NULL;
    }
    HSD_PARENT_INFO(&hsdPObj)->amnesia(info);
}

static void PObjInfoInit(void)
{
    hsdInitClassInfo(HSD_CLASS_INFO(&hsdPObj), HSD_CLASS_INFO(&hsdClass),
                     "sysdolphin_base_library", "hsd_pobj",
                     sizeof(HSD_PObjInfo), sizeof(HSD_PObj));
    HSD_CLASS_INFO(&hsdPObj)->release = PObjRelease;
    HSD_CLASS_INFO(&hsdPObj)->amnesia = PObjAmnesia;
    HSD_POBJ_INFO(&hsdPObj)->disp = HSD_PObjDisp;
    HSD_POBJ_INFO(&hsdPObj)->setup_mtx = PObjSetupMtx;
    HSD_POBJ_INFO(&hsdPObj)->load = PObjLoad;
}

bool HSD_PObjUsesStandardMethods(void)
{
    HSD_ClassInfo* info = HSD_CLASS_INFO(&hsdPObj);
    HSD_ClassInfo* named = hsdSearchClassInfo("hsd_pobj");
    if (HSD_PObjGetDefaultClass() != &hsdPObj) return false;
    return (info->head.flags & 1) &&
           info->head.obj_size == sizeof(HSD_PObj) &&
           info->head.info_size == sizeof(HSD_PObjInfo) &&
           (!named || named == info) &&
           info->alloc == _hsdClassAlloc && info->init == _hsdClassInit &&
           info->destroy == _hsdClassDestroy && info->release == PObjRelease &&
           hsdPObj.load == PObjLoad &&
           hsdPObj.disp == HSD_PObjDisp &&
           hsdPObj.setup_mtx == PObjSetupMtx;
}
