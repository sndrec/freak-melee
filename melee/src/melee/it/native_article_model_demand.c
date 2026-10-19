#include "native_article_model_demand.h"

#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/id.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/robj.h>
#include <dolphin/os.h>
#include <string.h>

enum { CENSUS_NODE_LIMIT = 512 };

static bool add(NativeArticleModelDemand* demand, HSD_ObjAllocData* allocator,
                size_t size)
{
    if (!size || size > UINT32_MAX ||
        (allocator && (!allocator->size || allocator->heap_limit_flag))) return false;
    uint32_t rounded = allocator ? allocator->size :
                       (uint32_t)((size + 31) & ~(size_t)31);
    for (uint32_t i = 0; i < demand->construction_count; ++i) {
        NativeHsdDemand* bin = &demand->construction[i];
        if (bin->allocator == allocator && bin->size == rounded) {
            if (bin->count == NATIVE_MATCH_POOL_MAX_SLOTS) return false;
            ++bin->count;
            return true;
        }
    }
    if (demand->construction_count == NATIVE_HSD_RESERVATION_BINS) return false;
    demand->construction[demand->construction_count++] =
        (NativeHsdDemand){ allocator, rounded, 1 };
    return true;
}

static void named(NativeArticleModelDemand* d, const char* name,
                  const char* expected)
{
    if (name && strcmp(name, expected)) d->uncovered |= NativeArticleModel_CustomClass;
}

static bool repeated(NativeArticleModelDemand* d, size_t size, uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i)
        if (!add(d, NULL, size)) return false;
    return true;
}

// MakeColorGenTExp scans BOTH operand arrays for constants, including an
// inactive channel. It creates one expression, then one staging expression
// per active stored-register operand occurrence (not per unique register).
// Restrict arithmetic to the ordinary ADD/SUB combiner; other native shader
// operation families remain unbudgeted rather than relying on optimization.
static bool custom_tev_demand(const HSD_TObjTevDesc* t,
                              uint32_t* expressions, uint32_t* constants)
{
    const uint32_t channels = TOBJ_TEVREG_ACTIVE_COLOR_TEV |
                              TOBJ_TEVREG_ACTIVE_ALPHA_TEV;
    const uint32_t registers = TOBJ_TEVREG_ACTIVE_KONST |
                               TOBJ_TEVREG_ACTIVE_TEV0 |
                               TOBJ_TEVREG_ACTIVE_TEV1;
    // TObjMakeTExp does not call MakeColorGenTExp without an active channel.
    // The record itself is still allocated by TObjLoad, but its operands and
    // operation settings produce no extra expressions or constants.
    if (!(t->active & channels)) {
        *expressions = 0;
        *constants = 0;
        return true;
    }
    if (t->active & ~(channels | registers)) return false;
    if ((t->active & TOBJ_TEVREG_ACTIVE_COLOR_TEV) &&
        ((t->color_op != GX_TEV_ADD && t->color_op != GX_TEV_SUB) ||
         t->color_bias != GX_TB_ZERO || t->color_scale != GX_CS_SCALE_1 ||
         t->color_clamp != GX_ENABLE)) return false;
    if ((t->active & TOBJ_TEVREG_ACTIVE_ALPHA_TEV) &&
        ((t->alpha_op != GX_TEV_ADD && t->alpha_op != GX_TEV_SUB) ||
         t->alpha_bias != GX_TB_ZERO || t->alpha_scale != GX_CS_SCALE_1 ||
         t->alpha_clamp != GX_ENABLE)) return false;
    uint32_t sources = 0, stages = 1;
    const u8* color = &t->color_a;
    const u8* alpha = &t->alpha_a;
    for (unsigned i = 0; i < 4; ++i) {
        switch (color[i]) {
        case GX_CC_ZERO: case GX_CC_ONE: case GX_CC_HALF:
        case GX_CC_TEXC: case GX_CC_TEXA: break;
        case TOBJ_TEV_CC_KONST_RGB: case TOBJ_TEV_CC_KONST_RRR:
        case TOBJ_TEV_CC_KONST_GGG: case TOBJ_TEV_CC_KONST_BBB:
        case TOBJ_TEV_CC_KONST_AAA: case TOBJ_TEV_CC_TEX0_RGB:
        case TOBJ_TEV_CC_TEX0_AAA: case TOBJ_TEV_CC_TEX1_RGB:
        case TOBJ_TEV_CC_TEX1_AAA:
            sources |= 1u << (color[i] - TOBJ_TEV_CC_KONST_RGB);
            if (color[i] >= TOBJ_TEV_CC_TEX0_RGB &&
                (t->active & TOBJ_TEVREG_ACTIVE_COLOR_TEV)) ++stages;
            break;
        default: return false;
        }
        switch (alpha[i]) {
        case GX_CA_ZERO: case GX_CA_TEXA: break;
        case TOBJ_TEV_CA_KONST_R: case TOBJ_TEV_CA_KONST_G:
        case TOBJ_TEV_CA_KONST_B: case TOBJ_TEV_CA_KONST_A:
            sources |= 1u << (alpha[i] - TOBJ_TEV_CA_KONST_R + 1);
            break;
        case TOBJ_TEV_CA_TEX0_A:
            sources |= 1u << 6;
            if (t->active & TOBJ_TEVREG_ACTIVE_ALPHA_TEV) ++stages;
            break;
        case TOBJ_TEV_CA_TEX1_A:
            sources |= 1u << 8;
            if (t->active & TOBJ_TEVREG_ACTIVE_ALPHA_TEV) ++stages;
            break;
        default: return false;
        }
    }
    uint32_t count = 0;
    for (; sources; sources >>= 1) count += sources & 1;
    *expressions = stages;
    *constants = count;
    return true;
}

// Default MObjMakeTExp allocates base1, diffuse1, specular3 and at most one
// final merge. Each participating TObjMakeTExp creates one expression and at
// most two blending constants. Simplification/scheduling allocate no objects;
// the final compiler creates <= expression-count stage descriptors.
static bool tev_demand(NativeArticleModelDemand* d, const HSD_MObjDesc* m)
{
    uint32_t expressions = 2 + !!(m->rendermode & RENDER_DIFFUSE) +
                           3 * !!(m->rendermode & RENDER_SPECULAR);
    uint32_t constants = (m->rendermode & RENDER_VERTEX ? 0 : 2) +
                         !!(m->rendermode & RENDER_SPECULAR);
    uint32_t textures = 0;
    bool covered = true;
    // The environment must be captured/validated by real construction admission.
    d->uncovered |= NativeArticleModel_MaterialEnvironment;
    for (const HSD_TObjDesc* t = m->texdesc; t; t = t->next) {
        if (++textures > 8 || t->class_name ||
            (t->blend_flags & TEX_BUMP) ||
            (t->blend_flags & TEX_COORD_MASK) == TEX_COORD_TOON)
            covered = false;
        uint32_t color = t->blend_flags & TEX_COLORMAP_MASK;
        uint32_t alpha = t->blend_flags & TEX_ALPHAMAP_MASK;
        if (color > TEX_COLORMAP_SUB || alpha > TEX_ALPHAMAP_SUB) covered = false;
        uint32_t groups = !!(t->blend_flags & (TEX_LIGHTMAP_DIFFUSE | TEX_LIGHTMAP_AMBIENT)) +
                          !!((m->rendermode & RENDER_SPECULAR) &&
                             (t->blend_flags & TEX_LIGHTMAP_SPECULAR)) +
                          !!(t->blend_flags & TEX_LIGHTMAP_EXT);
        uint32_t custom_expressions = 0, custom_constants = 0;
        if (t->tev && !custom_tev_demand(t->tev, &custom_expressions,
                                        &custom_constants)) covered = false;
        expressions += groups * (1 + custom_expressions);
        constants += groups * ((color == TEX_COLORMAP_BLEND) +
                               (alpha == TEX_ALPHAMAP_BLEND) + custom_constants);
    }
    // Compiler has32 DAG slots; actual GX stage mapping accepts only16. This
    // conservative bound avoids relying on optimizations to fit either limit.
    if (m->class_name || (m->rendermode & RENDER_SHADOW) ||
        (m->rendermode & RENDER_BLENDING) == RENDER_NO_ZUPDATE || expressions > 16)
        covered = false;
    if (!covered) {
        OSReport("[article-model] Unbudgeted material: mode=%08x textures=%u expressions=%u.\n",
                 m->rendermode, textures, expressions);
        for (const HSD_TObjDesc* t = m->texdesc; t; t = t->next) {
            OSReport("[article-model] Texture flags=%08x custom=%u.\n", t->blend_flags, t->tev != NULL);
            if (t->tev) OSReport("[article-model] TEV active=%08x color=(%u,%u,%u,%u) alpha=(%u,%u,%u,%u).\n",
                t->tev->active, t->tev->color_op, t->tev->color_bias, t->tev->color_scale, t->tev->color_clamp,
                t->tev->alpha_op, t->tev->alpha_bias, t->tev->alpha_scale, t->tev->alpha_clamp);
        }
        d->uncovered |= NativeArticleModel_TevCompiler;
        return true;
    }
    if (!repeated(d, sizeof(HSD_TETev), expressions) ||
        !repeated(d, sizeof(HSD_TECnst), constants) ||
        !repeated(d, sizeof(HSD_TExpTevDesc), expressions)) return false;
    d->tev_expressions += expressions;
    d->tev_constants += constants;
    d->tev_descriptors += expressions;
    return true;
}

// TObjAssignResources assigns even an otherwise inactive normal texture to a
// hardware slot. TObjSetup therefore needs these inputs independently of TEV
// participation. Zero scale is intentionally allowed by native TObjMakeMtx.
static bool texture_draw_input(const HSD_TObjDesc* texture)
{
    if (!texture->repeat_s || !texture->repeat_t || !texture->imagedesc ||
        !texture->imagedesc->image_ptr) return false;
    switch (texture->imagedesc->format) {
    case GX_TF_C4: case GX_TF_C8: case GX_TF_C14X2:
        return texture->tlutdesc && texture->tlutdesc->lut;
    case GX_TF_I4: case GX_TF_I8: case GX_TF_IA4: case GX_TF_IA8:
    case GX_TF_RGB565: case GX_TF_RGB5A3: case GX_TF_RGBA8: case GX_TF_CMPR:
        return true;
    default: return false;
    }
}

// The importer validates display commands and byte extents against the DAT.
// This construction census retains its trusted-pointer contract, but must not
// admit a visible PObj that will dereference a missing/unterminated vertex list.
static bool polygon_draw_input(const HSD_PObjDesc* polygon)
{
    if ((polygon->flags & (POBJ_CULLFRONT | POBJ_CULLBACK)) ==
        (POBJ_CULLFRONT | POBJ_CULLBACK)) return true;
    if (!polygon->verts || (polygon->n_display && !polygon->display)) return false;
    u32 seen = 0;
    for (unsigned i = 0; i <= GX_VA_MAX_ATTR; ++i) {
        const HSD_VtxDescList* vertex = &polygon->verts[i];
        if (vertex->attr == GX_VA_NULL) return true;
        u32 attribute = vertex->attr == GX_VA_NBT ? GX_VA_NRM : (u32)vertex->attr;
        if (attribute > GX_VA_TEX7 || (seen & ((u32)1 << attribute)) ||
            (u32)vertex->attr_type > GX_INDEX16) return false;
        seen |= (u32)1 << attribute;
        if (vertex->attr_type == GX_INDEX8 || vertex->attr_type == GX_INDEX16) {
            if (!vertex->vertex || !vertex->stride || vertex->stride > 255) return false;
        }
    }
    return false;
}

static bool material(NativeArticleModelDemand* d, const HSD_MObjDesc* m)
{
    if (!m) return true;
    if (++d->materials > CENSUS_NODE_LIMIT || !m->mat ||
        !add(d, NULL, sizeof(HSD_MObj)) ||
        !add(d, NULL, sizeof(HSD_Material))) return false;
    named(d, m->class_name, "hsd_mobj");
    if (m->pedesc && !add(d, NULL, sizeof(HSD_PEDesc))) return false;
    for (const HSD_TObjDesc* t = m->texdesc; t; t = t->next) {
        if (++d->textures > CENSUS_NODE_LIMIT || !add(d, NULL, sizeof(HSD_TObj)))
            return false;
        if (!texture_draw_input(t)) return false;
        named(d, t->class_name, "hsd_tobj");
        if (t->tlutdesc && !add(d, NULL, sizeof(HSD_Tlut))) return false;
        if (t->tev && !add(d, NULL, sizeof(HSD_TObjTev))) return false;
        // ImageDesc and LOD are borrowed by TObjLoad, not cloned.
    }
    return tev_demand(d, m);
}

bool nativeArticleModel_Census(const HSD_Joint* root, NativeArticleModelDemand* out)
{
    if (!root || !out || !HSD_IDGetAllocData()->size ||
        !HSD_VecGetAllocData()->size) return false;
    NativeArticleModelDemand d = { 0 };
    const HSD_Joint* pending[CENSUS_NODE_LIMIT];
    const HSD_Joint* visited[CENSUS_NODE_LIMIT];
    uint32_t count = 1;
    pending[0] = root;
    while (count) {
        const HSD_Joint* j = pending[--count];
        if (++d.joints > CENSUS_NODE_LIMIT || !add(&d, NULL, sizeof(HSD_JObj)) ||
            !add(&d, HSD_IDGetAllocData(), sizeof(IDEntry))) return false;
        visited[d.joints - 1] = j;
        named(&d, j->class_name, "hsd_jobj");
        if (j->flags & JOBJ_EFFECTOR) d.uncovered |= NativeArticleModel_References;
        if (j->flags & JOBJ_INSTANCE) d.uncovered |= NativeArticleModel_Instance;
        if (j->next) {
            if (count == CENSUS_NODE_LIMIT) return false;
            pending[count++] = j->next;
        }
        if (!(j->flags & JOBJ_INSTANCE) && j->child) {
            if (count == CENSUS_NODE_LIMIT) return false;
            pending[count++] = j->child;
        }
        if (j->mtx && !add(&d, HSD_MtxGetAllocData(), sizeof(Mtx))) return false;
        if (j->flags & (JOBJ_PTCL | JOBJ_SPLINE)) {
            d.uncovered |= NativeArticleModel_ParticleOrSpline;
        } else for (const HSD_DObjDesc* o = j->u.dobjdesc; o; o = o->next) {
            if (++d.drawables > CENSUS_NODE_LIMIT || !add(&d, NULL, sizeof(HSD_DObj)))
                return false;
            named(&d, o->class_name, "hsd_dobj");
            // DObjDisp performs MObj setup even when its polygon list is empty.
            if (!o->mobjdesc || !material(&d, o->mobjdesc)) return false;
            for (const HSD_PObjDesc* p = o->pobjdesc; p; p = p->next) {
                if (++d.polygons > CENSUS_NODE_LIMIT || !add(&d, NULL, sizeof(HSD_PObj)))
                    return false;
                named(&d, p->class_name, "hsd_pobj");
                switch (pobj_type(p)) {
                case POBJ_SKIN:
                    if (!polygon_draw_input(p)) return false;
                    break;
                case POBJ_SHAPEANIM: d.uncovered |= NativeArticleModel_Shape; break;
                case POBJ_ENVELOPE: d.uncovered |= NativeArticleModel_Envelope; break;
                case POBJ_SEPARATE_ENVELOPES: d.uncovered |= NativeArticleModel_Envelope; break;
                default: return false;
                }
            }
        }
        for (const HSD_RObjDesc* r = j->robjdesc; r; r = r->next) {
            if (++d.references > CENSUS_NODE_LIMIT ||
                !add(&d, HSD_RObjGetAllocData(), sizeof(HSD_RObj))) return false;
            // Expression buffers and Rvalue topology are not yet bounded here.
            d.uncovered |= NativeArticleModel_References;
        }
    }
    // PObj skin references are resolved through the descriptor-keyed global
    // ID table. A reference outside this graph would borrow unrelated live
    // state (or assert when no such ID exists), so it is not a static owner.
    for (uint32_t i = 0; i < d.joints; ++i) {
        const HSD_Joint* joint = visited[i];
        if (joint->flags & (JOBJ_PTCL | JOBJ_SPLINE)) continue;
        for (const HSD_DObjDesc* drawable = joint->u.dobjdesc; drawable;
             drawable = drawable->next) {
            for (const HSD_PObjDesc* polygon = drawable->pobjdesc; polygon;
                 polygon = polygon->next) {
                if (pobj_type(polygon) != POBJ_SKIN || !polygon->u.joint) continue;
                bool owned = false;
                for (uint32_t n = 0; n < d.joints; ++n)
                    if (visited[n] == polygon->u.joint) { owned = true; break; }
                if (!owned) d.uncovered |= NativeArticleModel_References;
            }
        }
    }
    d.scale_vectors = (NativeHsdDemand){ HSD_VecGetAllocData(),
                                       HSD_VecGetAllocData()->size, d.joints };
    *out = d;
    return true;
}
