#include "native_model_view.h"

#include <float.h>
#include <math.h>
#include <string.h>

#include "displayfunc.h"
#include "dobj.h"
#include "jobj.h"
#include "list.h"
#include "mobj.h"
#include "pobj.h"
#include "state.h"
#include <dolphin/gx.h>

enum {
    VIEW_MESHES = 1024
};
typedef struct ViewMesh {
    HSD_JObj* owner;
    HSD_DObj* mesh;
    HSD_JObj* weight_bone;
    bool hidden, highlight;
} ViewMesh;
static ViewMesh meshes[VIEW_MESHES];
static unsigned mesh_count;
static bool override_visibility;
static struct { const HSD_JObj* joint; u32 passes; } view_joints[512];
static unsigned view_joint_count;

void nativeModelView_Clear(void)
{
    mesh_count = 0;
    view_joint_count = 0;
    override_visibility = false;
}
void nativeModelView_Begin(unsigned count, bool visibility)
{
    view_joint_count = 0;
    override_visibility = visibility;
    mesh_count = count < VIEW_MESHES ? count : VIEW_MESHES;
}
void nativeModelView_Mesh(unsigned index, HSD_JObj* owner, HSD_DObj* mesh,
                          bool hidden, bool highlight, HSD_JObj* weight_bone)
{
    if (index < mesh_count) {
        meshes[index] = (ViewMesh){ owner, mesh, weight_bone, hidden, highlight };
        if (override_visibility && owner && mesh && !hidden) {
            const u32 mode = mesh->mobj ? mesh->mobj->rendermode : 0;
            const u32 pass = mode & RENDER_XLU ? (mode & RENDER_NO_ZUPDATE ? JOBJ_TEXEDGE : JOBJ_XLU) : JOBJ_OPA;
            for (HSD_JObj* joint = owner; joint; joint = joint->parent) {
                unsigned slot = 0;
                while (slot < view_joint_count && view_joints[slot].joint != joint) ++slot;
                if (slot == view_joint_count) {
                    if (view_joint_count == 512) break;
                    view_joints[slot].joint = joint; view_joints[slot].passes = 0; ++view_joint_count;
                }
                view_joints[slot].passes |= joint == owner ? pass : pass << 10;
            }
        }
    }
}

bool nativeModelView_OverridesVisibility(const HSD_JObj* joint)
{
    if (override_visibility)
        for (unsigned i = 0; i < view_joint_count; ++i)
            if (view_joints[i].joint == joint) return true;
    return false;
}
u32 nativeModelView_JointFlags(const HSD_JObj* joint)
{
    if (override_visibility)
        for (unsigned i = 0; i < view_joint_count; ++i)
            if (view_joints[i].joint == joint) return (joint->flags & ~JOBJ_HIDDEN) | view_joints[i].passes;
    return joint->flags;
}
u32 nativeModelView_MeshFlags(const HSD_DObj* mesh)
{
    for (unsigned i = 0; i < mesh_count; ++i) if (meshes[i].mesh == mesh) {
        if (meshes[i].hidden) return mesh->flags | DOBJ_HIDDEN;
        if (!override_visibility) return mesh->flags;
        const u32 mode = mesh->mobj ? mesh->mobj->rendermode : 0;
        const u32 pass = mode & RENDER_XLU ? (mode & RENDER_NO_ZUPDATE ? (HSD_TRSP_TEXEDGE << DOBJ_TRSP_SHIFT) : (HSD_TRSP_XLU << DOBJ_TRSP_SHIFT)) : (HSD_TRSP_OPA << DOBJ_TRSP_SHIFT);
        return (mesh->flags & ~(DOBJ_HIDDEN | (HSD_TRSP_OPA << DOBJ_TRSP_SHIFT) | (HSD_TRSP_TEXEDGE << DOBJ_TRSP_SHIFT) | (HSD_TRSP_XLU << DOBJ_TRSP_SHIFT))) | pass;
    }
    return mesh->flags;
}

static u16 be16(const u8* p)
{
    return (u16) ((p[0] << 8) | p[1]);
}
static float component(const HSD_VtxDescList* d, const u8* p, bool direct)
{
    const bool little = !direct && d->vertex_little_endian;
    u32 bits;
    float value;
    if (d->comp_type == GX_F32) {
        if (little) {
            memcpy(&bits, p, 4);
        } else {
            bits = ((u32) p[0] << 24) | ((u32) p[1] << 16) |
                   ((u32) p[2] << 8) | p[3];
        }
        memcpy(&value, &bits, 4);
        return value;
    }
    if (d->comp_type == GX_U8) {
        value = p[0];
    } else if (d->comp_type == GX_S8) {
        value = (s8) p[0];
    } else {
        u16 word;
        if (little) {
            memcpy(&word, p, 2);
        } else {
            word = be16(p);
        }
        value = d->comp_type == GX_S16 ? (float) (s16) word : (float) word;
    }
    return ldexpf(value, -(int) d->frac);
}

static unsigned value_bytes(const HSD_VtxDescList* d)
{
    if (d->attr <= GX_VA_TEX7MTXIDX) {
        return 1;
    }
    if (d->attr == GX_VA_CLR0 || d->attr == GX_VA_CLR1) {
        static const u8 bytes[] = { 2, 3, 4, 2, 3, 4 };
        return (unsigned) d->comp_type < 6 ? bytes[d->comp_type] : 0;
    }
    unsigned n;
    if (d->attr == GX_VA_POS) {
        n = d->comp_cnt == GX_POS_XY ? 2 : 3;
    } else if (d->attr == GX_VA_NRM || d->attr == GX_VA_NBT) {
        n = d->comp_cnt == GX_NRM_XYZ ? 3 : 9;
    } else {
        n = d->comp_cnt == GX_TEX_S ? 1 : 2;
    }
    const unsigned size = d->comp_type <= GX_S8    ? 1
                          : d->comp_type <= GX_S16 ? 2
                                                   : 4;
    return n * size;
}

static bool matrices(const ViewMesh* mesh, HSD_PObj* p, Mtx output[10],
                     float weights[10], unsigned* count)
{
    HSD_JObjSetupMatrix(mesh->owner);
    *count = 1;
    C_MTXCopy(mesh->owner->mtx, output[0]);
    weights[0] = mesh->weight_bone == mesh->owner ? 1 : 0;
    const unsigned type = p->flags & 0x3000;
    if (type == POBJ_SHAPEANIM) {
        return false;
    }
    if (type != POBJ_ENVELOPE) {
        if (p->u.jobj) {
            HSD_JObjSetupMatrix(p->u.jobj);
            C_MTXCopy(p->u.jobj->mtx, output[1]);
            weights[1] = mesh->weight_bone == p->u.jobj ? 1 : 0;
            *count = 2;
        }
        return true;
    }
    Mtx right_storage;
    MtxPtr right = _HSD_mkEnvelopeModelNodeMtx(mesh->owner, right_storage);
    *count = 0;
    for (HSD_SList* list = p->u.envelope_list; list && *count < 10;
         list = list->next)
    {
        const unsigned slot = (*count)++;
        HSD_Envelope* env = list->data;
        weights[slot] = 0;
        memset(output[slot], 0, sizeof(Mtx));
        if (!env) {
            return false;
        }
        const bool single = env->weight >= 1.0f - FLT_EPSILON;
        for (HSD_Envelope* e = env; e; e = e->next) {
            HSD_JObjSetupMatrix(e->jobj);
            if (e->jobj == mesh->weight_bone) {
                weights[slot] += e->weight;
            }
            Mtx m;
            if (!single || right) {
                if (!e->jobj->envelopemtx) {
                    return false;
                }
                C_MTXConcat(e->jobj->mtx, e->jobj->envelopemtx, m);
            } else {
                C_MTXCopy(e->jobj->mtx, m);
            }
            for (unsigned a = 0; a < 3; ++a) {
                for (unsigned b = 0; b < 4; ++b) {
                    output[slot][a][b] +=
                        m[a][b] * (single ? 1.0f : e->weight);
                }
            }
            if (single) {
                break;
            }
        }
        if (right) {
            Mtx temp;
            C_MTXConcat(output[slot], right, temp);
            C_MTXCopy(temp, output[slot]);
        }
    }
    return *count != 0;
}

typedef void (*BeginFn)(unsigned primitive, unsigned count, void* user);
typedef void (*VertexFn)(const Vec3* p, float weight, void* user);
typedef void (*EndFn)(void* user);
// Reads the original GX descriptor order, including stock indexed/quantized
// positions. No imported-mesh layout assumptions and no draw-time allocations.
static bool visit(const ViewMesh* mesh, HSD_PObj* p, BeginFn begin,
                  VertexFn vertex, EndFn end, void* user)
{
    Mtx mtx[10];
    float weights[10];
    unsigned matrix_count;
    if (!matrices(mesh, p, mtx, weights, &matrix_count)) {
        return false;
    }
    const u8* data = p->display;
    const u8* limit = data + ((size_t) p->n_display << 5);
    while (data < limit) {
        const unsigned command = *data++;
        if (!command) {
            continue;
        }
        const unsigned primitive = command & 0xF8;
        if ((primitive != GX_TRIANGLES && primitive != GX_TRIANGLESTRIP &&
             primitive != GX_TRIANGLEFAN && primitive != GX_QUADS) ||
            limit - data < 2)
        {
            return false;
        }
        const unsigned count = be16(data);
        data += 2;
        if (begin) {
            begin(primitive, count, user);
        }
        for (unsigned v = 0; v < count; ++v) {
            Vec3 pos = { 0 };
            unsigned matrix = 0;
            bool have_pos = false;
            for (const HSD_VtxDescList* d = p->verts; d->attr != GX_VA_NULL;
                 ++d)
            {
                if (d->attr_type == GX_NONE) {
                    continue;
                }
                unsigned size = d->attr_type == GX_INDEX8    ? 1
                                : d->attr_type == GX_INDEX16 ? 2
                                                             : value_bytes(d);
                if ((d->attr == GX_VA_NRM || d->attr == GX_VA_NBT) &&
                    d->comp_cnt == GX_NRM_NBT3 && d->attr_type != GX_DIRECT)
                {
                    size *= 3;
                }
                if (!size || (size_t) (limit - data) < size) {
                    return false;
                }
                const u8* value = data;
                data += size;
                if (d->attr == GX_VA_PNMTXIDX) {
                    matrix = value[0] / 3;
                }
                if (d->attr != GX_VA_POS) {
                    continue;
                }
                const bool direct = d->attr_type == GX_DIRECT;
                if (!direct) {
                    const unsigned index =
                        d->attr_type == GX_INDEX8 ? value[0] : be16(value);
                    const size_t offset = (size_t) index * d->stride;
                    if (!d->vertex || offset + value_bytes(d) > d->vertex_size)
                    {
                        return false;
                    }
                    value = (const u8*) d->vertex + offset;
                }
                unsigned stride = d->comp_type <= GX_S8    ? 1
                                  : d->comp_type <= GX_S16 ? 2
                                                           : 4;
                pos.x = component(d, value, direct);
                pos.y = component(d, value + stride, direct);
                if (d->comp_cnt == GX_POS_XYZ) {
                    pos.z = component(d, value + stride * 2, direct);
                }
                have_pos = true;
            }
            if (!have_pos || matrix >= matrix_count) {
                return false;
            }
            Vec3 world;
            C_MTXMultVec(mtx[matrix], &pos, &world);
            if (vertex) {
                vertex(&world, weights[matrix], user);
            }
        }
        if (end) {
            end(user);
        }
    }
    return true;
}

static void draw_begin(unsigned primitive, unsigned count, void* user)
{
    GXBegin(primitive, GX_VTXFMT0, (u16) count);
}
static void draw_vertex(const Vec3* p, float weight, void* user)
{
    const ViewMesh* mesh = user;
    GXColor color = { 137, 210, 255, 255 };
    if (!mesh->highlight) {
        // Continuous blue -> yellow -> orange -> red selected-bone heatmap.
        const float colors[4][3] = {
            { 0, 40, 255 }, { 255, 230, 0 }, { 255, 115, 0 }, { 255, 0, 0 }
        };
        float t = fminf(1, fmaxf(0, weight)) * 3;
        unsigned i = (unsigned) t;
        if (i > 2) {
            i = 2;
        }
        t -= i;
        color.r = (u8) (colors[i][0] + t * (colors[i + 1][0] - colors[i][0]));
        color.g = (u8) (colors[i][1] + t * (colors[i + 1][1] - colors[i][1]));
        color.b = (u8) (colors[i][2] + t * (colors[i + 1][2] - colors[i][2]));
    }
    GXPosition3f32(p->x, p->y, p->z);
    GXColor4u8(color.r, color.g, color.b, 255);
}
static void draw_end(void* user)
{
    GXEnd();
}

bool nativeModelView_Draw(HSD_DObj* mesh, Mtx view)
{
    ViewMesh* entry = NULL;
    for (unsigned i = 0; i < mesh_count; ++i) {
        if (meshes[i].mesh == mesh) {
            entry = &meshes[i];
            break;
        }
    }
    if (!entry) {
        return false;
    }
    if (entry->hidden) {
        return true;
    }
    if (!entry->highlight && !entry->weight_bone) {
        return false;
    }
    for (HSD_PObj* p = mesh->pobj; p; p = p->next) {
        if (!visit(entry, p, NULL, NULL, NULL, NULL)) {
            return false;
        }
    }
    HSD_StateInvalidate(HSD_STATE_ALL);
    HSD_ClearVtxDesc();
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXLoadPosMtxImm(view, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL,
                  GX_DF_NONE, GX_AF_NONE);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetNumIndStages(0);
    GXSetTevDirect(GX_TEVSTAGE0);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GXSetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_COPY);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GXSetColorUpdate(GX_TRUE);
    for (HSD_PObj* p = mesh->pobj; p; p = p->next) {
        unsigned cull = p->flags & (POBJ_CULLFRONT | POBJ_CULLBACK);
        GXSetCullMode(cull == POBJ_CULLFRONT  ? GX_CULL_FRONT
                      : cull == POBJ_CULLBACK ? GX_CULL_BACK
                                              : GX_CULL_NONE);
        visit(entry, p, draw_begin, draw_vertex, draw_end, entry);
    }
    HSD_ClearVtxDesc();
    HSD_PObjClearMtxMark(NULL, 0);
    HSD_StateInvalidate(HSD_STATE_ALL);
    return true;
}

typedef struct Pick {
    Vec3 origin, direction, first, previous, quad[4];
    unsigned primitive, index;
    float distance;
    int mesh, selected;
} Pick;
static Vec3 sub(Vec3 a, Vec3 b)
{
    return (Vec3){ a.x - b.x, a.y - b.y, a.z - b.z };
}
static Vec3 cross(Vec3 a, Vec3 b)
{
    return (Vec3){ a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                   a.x * b.y - a.y * b.x };
}
static float dot(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
static void triangle(Pick* pick, Vec3 a, Vec3 b, Vec3 c)
{
    Vec3 e1 = sub(b, a), e2 = sub(c, a), h = cross(pick->direction, e2);
    float det = dot(e1, h);
    if (fabsf(det) < 1e-8f) {
        return;
    }
    Vec3 s = sub(pick->origin, a);
    float u = dot(s, h) / det;
    if (u < 0 || u > 1) {
        return;
    }
    Vec3 q = cross(s, e1);
    float v = dot(pick->direction, q) / det;
    if (v < 0 || u + v > 1) {
        return;
    }
    float t = dot(e2, q) / det;
    if (t > 0 && t < pick->distance) {
        pick->distance = t;
        pick->selected = pick->mesh;
    }
}
static void pick_begin(unsigned primitive, unsigned count, void* user)
{
    Pick* p = user;
    p->primitive = primitive;
    p->index = 0;
}
static void pick_vertex(const Vec3* vertex, float weight, void* user)
{
    Pick* p = user;
    unsigned i = p->index++;
    Vec3 v = *vertex;
    if (p->primitive == GX_TRIANGLES) {
        p->quad[i % 3] = v;
        if (i % 3 == 2) {
            triangle(p, p->quad[0], p->quad[1], v);
        }
    } else if (p->primitive == GX_QUADS) {
        p->quad[i % 4] = v;
        if (i % 4 == 3) {
            triangle(p, p->quad[0], p->quad[1], p->quad[2]);
            triangle(p, p->quad[0], p->quad[2], v);
        }
    } else {
        if (i >= 2) {
            triangle(p, p->first, p->previous, v);
        }
        if (i == 0 || p->primitive == GX_TRIANGLESTRIP) {
            p->first = p->previous;
        }
        if (i == 0) {
            p->first = v;
        }
        p->previous = v;
    }
}
int nativeModelView_Pick(const float origin[3], const float direction[3])
{
    Pick pick = { 0 };
    memcpy(&pick.origin, origin, 12);
    memcpy(&pick.direction, direction, 12);
    pick.distance = FLT_MAX;
    pick.selected = -1;
    for (unsigned i = 0; i < mesh_count; ++i) {
        ViewMesh* mesh = &meshes[i];
        if (!mesh->mesh || mesh->hidden || mesh->mesh->flags & DOBJ_HIDDEN ||
            mesh->owner->flags & JOBJ_HIDDEN)
        {
            continue;
        }
        pick.mesh = (int) i;
        for (HSD_PObj* p = mesh->mesh->pobj; p; p = p->next) {
            visit(mesh, p, pick_begin, pick_vertex, NULL, &pick);
        }
    }
    return pick.selected;
}
