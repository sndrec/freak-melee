#include "native_article_animation_demand.h"
#include "native_article_animation_texture.h"

#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/robj.h>
#include <math.h>
#include <limits.h>
#include <string.h>

enum { TREE_LIMIT = 512, CHAIN_LIMIT = 16384, WORK_LIMIT = 131072,
       KEY_LIMIT = 65536 };
typedef enum TreeKind { JointTree, AnimTree, MaterialTree, ShapeTree } TreeKind;
typedef enum ChainKind { Drawables, Polygons, Textures, Materials, TextureAnims,
    ShapeDrawables, ShapeAnims, References, ReferenceAnims, Tracks } ChainKind;
typedef enum TargetKind { JointTarget, MaterialTarget, TextureTarget,
    ShapeTarget, ReferenceTarget } TargetKind;
typedef struct Census {
    NativeArticleAnimationDemand demand;
    uint32_t work;
} Census;

static bool work(Census* c)
{ return c->work++ < WORK_LIMIT; }
static const void* tree_child(TreeKind kind, const void* node)
{
    switch (kind) {
    case JointTree: return ((const HSD_Joint*)node)->child;
    case AnimTree: return ((const HSD_AnimJoint*)node)->child;
    case MaterialTree: return ((const HSD_MatAnimJoint*)node)->child;
    default: return ((const HSD_ShapeAnimJoint*)node)->child;
    }
}
static const void* tree_next(TreeKind kind, const void* node)
{
    switch (kind) {
    case JointTree: return ((const HSD_Joint*)node)->next;
    case AnimTree: return ((const HSD_AnimJoint*)node)->next;
    case MaterialTree: return ((const HSD_MatAnimJoint*)node)->next;
    default: return ((const HSD_ShapeAnimJoint*)node)->next;
    }
}

// Three-color walk distinguishes cycles from a shared immutable descriptor.
// Application counts below still visit shared descriptors at each native use.
static bool tree_valid(TreeKind kind, const void* root)
{
    if (!root) return true;
    const void* nodes[TREE_LIMIT];
    uint8_t colors[TREE_LIMIT] = { 1 };
    struct { uint16_t index; uint8_t edge; } stack[TREE_LIMIT] = { { 0, 0 } };
    uint32_t count = 1, depth = 1;
    nodes[0] = root;
    while (depth) {
        uint32_t index = stack[depth - 1].index;
        uint8_t edge = stack[depth - 1].edge++;
        if (edge == 2) { colors[index] = 2; --depth; continue; }
        const void* next = edge ? tree_next(kind, nodes[index]) :
                                  tree_child(kind, nodes[index]);
        if (!next) continue;
        uint32_t at = 0;
        while (at < count && nodes[at] != next) ++at;
        if (at < count) {
            if (colors[at] == 1) return false;
            continue;
        }
        if (count == TREE_LIMIT || depth == TREE_LIMIT) return false;
        nodes[count] = next; colors[count] = 1;
        stack[depth].index = (uint16_t)count++;
        stack[depth++].edge = 0;
    }
    return true;
}

static const void* chain_next(ChainKind kind, const void* node)
{
    if (!node) return NULL;
    switch (kind) {
    case Drawables: return ((const HSD_DObjDesc*)node)->next;
    case Polygons: return ((const HSD_PObjDesc*)node)->next;
    case Textures: return ((const HSD_TObjDesc*)node)->next;
    case Materials: return ((const HSD_MatAnim*)node)->next;
    case TextureAnims: return ((const HSD_TexAnim*)node)->next;
    case ShapeDrawables: return ((const HSD_ShapeAnimDObj*)node)->next;
    case ShapeAnims: return ((const HSD_ShapeAnim*)node)->next;
    case References: return ((const HSD_RObjDesc*)node)->next;
    case ReferenceAnims: return ((const HSD_RObjAnimJoint*)node)->next;
    default: return ((const HSD_FObjDesc*)node)->next;
    }
}
static bool chain_valid(Census* c, ChainKind kind, const void* node)
{
    const void* slow = node;
    const void* fast = node;
    for (uint32_t count = 0; node; ++count) {
        if (count == CHAIN_LIMIT || !work(c)) return false;
        node = chain_next(kind, node);
        slow = chain_next(kind, slow);
        fast = chain_next(kind, chain_next(kind, fast));
        if (slow && slow == fast) return false;
    }
    return true;
}

// The native operand parser consumes four bytes ONLY for frac == 0. Float
// class values with nonzero fraction bits are not equivalent to that format.
static unsigned scalar_width(uint8_t frac)
{
    if (frac == HSD_A_FRAC_FLOAT) return 4;
    switch (frac & 0xe0) {
    case HSD_A_FRAC_S16: case HSD_A_FRAC_U16: return 2;
    case HSD_A_FRAC_S8: case HSD_A_FRAC_U8: return 1;
    default: return 0;
    }
}
static bool packed_wait(const uint8_t* bytes, uint32_t length, uint32_t* at)
{
    unsigned shift = 0;
    for (unsigned count = 0; count < 5 && *at < length; ++count, shift += 7) {
        unsigned byte = bytes[(*at)++];
        // parseWait shifts signed int operands. Do not admit signed overflow
        // or shifts beyond its actual word; the stored u16 truncation is native.
        if (shift == 28 && (byte & 0x7f) > 7) return false;
        if (!(byte & 0x80)) return true;
    }
    return false;
}
static bool stream(Census* c, const HSD_FObjDesc* track)
{
    if (!track->length) {
        if (!track->ad) c->demand.uncovered |= NativeArticleAnimation_Stream;
        return true;
    }
    if (!track->ad || (uintptr_t)track->ad > UINTPTR_MAX - track->length)
        return false;
    uint32_t at = 0, keys = 0;
    while (at < track->length) {
        if (!work(c)) return false;
        unsigned header = track->ad[at++], op = header & 15;
        uint32_t count = ((header >> 4) & 7) + 1;
        if (header & 0x80) {
            unsigned shift = 3, byte;
            do {
                if (at == track->length || shift >= 32) return false;
                byte = track->ad[at++];
                if ((byte & 0x7f) > (UINT32_MAX - count) >> shift) return false;
                count += (byte & 0x7f) << shift;
                shift += 7;
            } while (byte & 0x80);
        }
        // Native nb_pack is u16. Unknown op semantics stay unadmitted rather
        // than pretending the remaining opaque bytes are validated operands.
        if (count > UINT16_MAX || count > KEY_LIMIT - keys) return false;
        if (op < HSD_A_OP_CON || op > HSD_A_OP_KEY) {
            c->demand.uncovered |= NativeArticleAnimation_Stream;
            return true;
        }
        for (uint32_t i = 0; i < count; ++i) {
            if (!work(c)) return false;
            uint8_t formats[2] = { track->frac_value, track->frac_slope };
            unsigned first = op == HSD_A_OP_SLP ? 1 : 0;
            unsigned last = op == HSD_A_OP_SPL || op == HSD_A_OP_SLP ? 2 : 1;
            for (unsigned operand = first; operand < last; ++operand) {
                unsigned width = scalar_width(formats[operand]);
                if (!width) {
                    c->demand.uncovered |= NativeArticleAnimation_Stream;
                    return true;
                }
                if (width > track->length - at) return false;
                // Integer fraction 31 shifts native signed 1 into its sign bit.
                if (formats[operand] && (formats[operand] & 31) == 31)
                    c->demand.uncovered |= NativeArticleAnimation_Stream;
                at += width;
            }
            ++keys;
            // Actual FObjAnimKey uses LOAD_WAIT too. The workshop key editor's
            // KEY shortcut is not the native grammar. A final value can end
            // without wait because FObjLoadWait first checks the stream extent.
            if (op != HSD_A_OP_SLP && at < track->length &&
                !packed_wait(track->ad, track->length, &at)) return false;
        }
    }
    return true;
}

// Native image and palette channels truncate scalar values to table indexes.
// Only whole CON streams are covered: no interpolation or slope overshoot.
static bool table_indices(const HSD_FObjDesc* track, uint32_t entries)
{
    if (!entries || entries > 512 || !track->ad || !track->length) return false;
    unsigned width = scalar_width(track->frac_value);
    if (!width || (track->frac_value && (track->frac_value & 31) == 31)) return false;
    uint32_t at = 0, keys = 0;
    while (at < track->length) {
        unsigned header = track->ad[at++];
        if ((header & 15) != HSD_A_OP_CON) return false;
        uint32_t count = ((header >> 4) & 7) + 1;
        if (header & 0x80) {
            unsigned shift = 3, byte;
            do {
                if (at == track->length || shift >= 32) return false;
                byte = track->ad[at++];
                if ((byte & 0x7f) > (UINT32_MAX - count) >> shift) return false;
                count += (byte & 0x7f) << shift; shift += 7;
            } while (byte & 0x80);
        }
        if (count > UINT16_MAX || count > KEY_LIMIT - keys) return false;
        for (uint32_t i = 0; i < count; ++i) {
            if (width > track->length - at) return false;
            const uint8_t* bytes = track->ad + at;
            float value;
            if (!track->frac_value) {
                uint32_t bits = bytes[0] | (uint32_t)bytes[1] << 8 |
                    (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
                memcpy(&value, &bits, sizeof(value));
            } else {
                int numerator = width == 2 ? bytes[0] | (unsigned)bytes[1] << 8 : bytes[0];
                if ((track->frac_value & 0xe0) == HSD_A_FRAC_S16) numerator = (int16_t)numerator;
                if ((track->frac_value & 0xe0) == HSD_A_FRAC_S8) numerator = (int8_t)numerator;
                value = (float)numerator / (float)(1u << (track->frac_value & 31));
            }
            // Native truncation admits (-1,0), which selects zero. Count <=512
            // also guarantees the converted positive index fits native int.
            if (!isfinite(value) || value <= -1.0f || value >= entries) return false;
            at += width; ++keys;
            if (at < track->length && !packed_wait(track->ad, track->length, &at)) return false;
        }
    }
    return true;
}

static void channel(Census* c, TargetKind kind, unsigned type,
                    const void* target, const HSD_FObjDesc* track,
                    const HSD_TexAnim* texture_anim)
{
    uint32_t* flags = &c->demand.uncovered;
    if (kind == JointTarget) {
        const HSD_Joint* joint = target;
        if (type >= HSD_A_J_ROTX && type <= HSD_A_J_ROTZ) {
            if (joint->flags & JOBJ_USE_QUATERNION)
                *flags |= NativeArticleAnimation_EulerQuaternion;
        } else if (type == HSD_A_J_PATH) *flags |= NativeArticleAnimation_Path;
        else if ((type >= HSD_A_J_SETBYTE0 && type <= HSD_A_J_SETFLOAT9) ||
                 (type >= 0x28 && type <= 0x2a))
            *flags |= NativeArticleAnimation_CallbackChannels;
        else if (type < HSD_A_J_TRAX || type > HSD_A_J_BRANCH)
            *flags |= NativeArticleAnimation_Channel;
    } else if (kind == MaterialTarget) {
        const HSD_MObjDesc* material = target;
        // Alpha is an ordinary F32 write. Other material channels perform
        // float-to-byte conversions whose interpolated range is not proven.
        if (type != HSD_A_M_ALPHA || !material->mat)
            *flags |= NativeArticleAnimation_Channel;
    } else if (kind == TextureTarget) {
        const HSD_TObjDesc* texture = target;
        // Translation/scale/rotation/blending are F32 writes. Table indexes
        // require bounded constant streams; color conversions remain excluded.
        bool scalar = (type >= HSD_A_T_TRAU && type <= HSD_A_T_BLEND) ||
                      type == HSD_A_T_TS_BLEND ||
                      (type == HSD_A_T_LOD_BIAS && texture->lod);
        bool image = type == HSD_A_T_TIMG && texture_anim && texture_anim->imagetbl &&
            table_indices(track, texture_anim->n_imagetbl);
        // 255 is the native default-palette sentinel, not an array index.
        // Cover ordinary palette entries 0..254; sentinel authoring is separate.
        bool palette = type == HSD_A_T_TCLT && texture_anim && texture_anim->tluttbl &&
            texture_anim->n_tluttbl <= 255 && table_indices(track, texture_anim->n_tluttbl);
        if (!scalar && !image && !palette)
            *flags |= NativeArticleAnimation_Channel;
    } else if (kind == ShapeTarget) *flags |= NativeArticleAnimation_Shape;
    else if (type != TYPE_ROBJ) *flags |= NativeArticleAnimation_Channel;
}
static bool controller(Census* c, const HSD_AObjDesc* aobj,
                       TargetKind kind, const void* target, const HSD_TexAnim* texture)
{
    if (!aobj) return true;
    if (!work(c) || c->demand.controllers == CHAIN_LIMIT ||
        !chain_valid(c, Tracks, aobj->fobjdesc)) return false;
    ++c->demand.controllers;
    if (aobj->obj_id) c->demand.uncovered |= NativeArticleAnimation_ObjReferences;
    for (const HSD_FObjDesc* track = aobj->fobjdesc; track; track = track->next) {
        // This is the actual F32->s16 assignment in HSD_FObjLoadDesc. Fractional
        // values are legal if their truncation is representable; no rounding.
        double first = (double)track->startframe;
        if (!work(c) || c->demand.tracks == CHAIN_LIMIT || !isfinite(first) ||
            first <= (double)INT16_MIN - 1 || first >= (double)INT16_MAX + 1 ||
            !stream(c, track)) return false;
        ++c->demand.tracks;
        channel(c, kind, track->type, target, track, texture);
    }
    return true;
}

static bool material(Census* c, const HSD_MObjDesc* model,
                     const HSD_MatAnim* anim)
{
    if (!model || !anim) return true;
    if (!controller(c, anim->aobjdesc, MaterialTarget, model, NULL) ||
        !chain_valid(c, Textures, model->texdesc) ||
        !chain_valid(c, TextureAnims, anim->texanim)) return false;
    NativeArticleAnimationTextureID textures[NATIVE_ARTICLE_ANIMATION_TEXTURE_LIMIT];
    uint32_t count;
    if (!nativeArticleAnimation_TextureIDs(model, textures,
        NATIVE_ARTICLE_ANIMATION_TEXTURE_LIMIT, &count)) return false;
    for (uint32_t i = 0; i < count; ++i) {
        const HSD_TObjDesc* texture = textures[i].descriptor;
        if (!work(c)) return false;
        const HSD_TexAnim* selected = anim->texanim;
        while (selected && selected->id != textures[i].id) {
            if (!work(c)) return false;
            selected = selected->next;
        }
        if (!selected) continue;
        if (selected->n_imagetbl > 512 ||
            (selected->n_imagetbl && !selected->imagetbl)) return false;
        if (!controller(c, selected->aobjdesc, TextureTarget, texture, selected)) return false;
        if (selected->n_tluttbl > 512 || (selected->n_tluttbl && !selected->tluttbl))
            return false;
        if (selected->n_tluttbl > 255)
            c->demand.uncovered |= NativeArticleAnimation_PaletteTables;
        for (uint32_t entry = 0; entry < selected->n_tluttbl; ++entry) {
            if (!work(c)) return false;
            const HSD_TlutDesc* palette = selected->tluttbl[entry];
            if (!palette || !palette->lut || !palette->n_entries ||
                (unsigned)palette->fmt > GX_TL_RGB5A3) return false;
        }
    }
    return true;
}
static bool shape(Census* c, const HSD_PObjDesc* polygons,
                  const HSD_ShapeAnim* anim)
{
    if (!anim || !polygons) return true;
    if (!chain_valid(c, Polygons, polygons) || !chain_valid(c, ShapeAnims, anim))
        return false;
    c->demand.uncovered |= NativeArticleAnimation_Shape;
    for (; polygons; polygons = polygons->next) {
        if (!work(c)) return false;
        // Native AddAnim asserts shape type even when the zip has run out of
        // animation nodes. Count only nodes that actually load a controller.
        if (pobj_type(polygons) != POBJ_SHAPEANIM || !polygons->u.shape_set)
            c->demand.uncovered |= NativeArticleAnimation_Channel;
        if (anim && !controller(c, anim->aobjdesc, ShapeTarget, NULL, NULL)) return false;
        if (anim) anim = anim->next;
    }
    return true;
}

typedef struct Zip {
    const HSD_Joint* joint;
    const HSD_AnimJoint* anim;
    const HSD_MatAnimJoint* material;
    const HSD_ShapeAnimJoint* shape;
} Zip;
bool nativeArticleAnimation_Census(const HSD_Joint* root,
    const HSD_AnimJoint* animation, const HSD_MatAnimJoint* materials,
    const HSD_ShapeAnimJoint* shapes, NativeArticleAnimationDemand* output)
{
    if (!root || !output || !tree_valid(JointTree, root) ||
        !tree_valid(AnimTree, animation) || !tree_valid(MaterialTree, materials) ||
        !tree_valid(ShapeTree, shapes)) return false;
    Census c = { 0 };
    Zip stack[TREE_LIMIT] = { { root, animation, materials, shapes } };
    uint32_t depth = 1, visits = 0;
    while (depth) {
        Zip zip = stack[--depth];
        if (!work(&c) || visits++ == TREE_LIMIT) return false;
        const HSD_Joint* joint = zip.joint;
        if (joint->flags & JOBJ_INSTANCE) c.demand.uncovered |= NativeArticleAnimation_Instance;
        if (joint->flags & (JOBJ_PTCL | JOBJ_SPLINE))
            c.demand.uncovered |= NativeArticleAnimation_ParticleOrSpline;
        if (zip.anim) {
            if (!controller(&c, zip.anim->aobjdesc, JointTarget, joint, NULL) ||
                !chain_valid(&c, References, joint->robjdesc) ||
                !chain_valid(&c, ReferenceAnims, zip.anim->robj_anim)) return false;
            const HSD_RObjDesc* ref = joint->robjdesc;
            const HSD_RObjAnimJoint* anim = zip.anim->robj_anim;
            for (; ref && anim; ref = ref->next, anim = anim->next) {
                if (!controller(&c, anim->aobjdesc, ReferenceTarget, NULL, NULL)) return false;
                c.demand.uncovered |= NativeArticleAnimation_ObjReferences;
            }
        }
        if (!(joint->flags & (JOBJ_PTCL | JOBJ_SPLINE))) {
            const HSD_DObjDesc* drawable = joint->u.dobjdesc;
            const HSD_MatAnim* mat = zip.material ? zip.material->matanim : NULL;
            const HSD_ShapeAnimDObj* sh = zip.shape ? zip.shape->shapeanimdobj : NULL;
            if (!chain_valid(&c, Drawables, drawable) || !chain_valid(&c, Materials, mat) ||
                !chain_valid(&c, ShapeDrawables, sh)) return false;
            for (; drawable; drawable = drawable->next) {
                if (!chain_valid(&c, Polygons, drawable->pobjdesc)) return false;
                for (const HSD_PObjDesc* polygon = drawable->pobjdesc; polygon;
                     polygon = polygon->next) {
                    if (!work(&c)) return false;
                    if (pobj_type(polygon) == POBJ_SHAPEANIM)
                        c.demand.uncovered |= NativeArticleAnimation_Shape;
                }
                if (!work(&c) || !material(&c, drawable->mobjdesc, mat) ||
                    !shape(&c, drawable->pobjdesc, sh ? sh->shapeanim : NULL)) return false;
                if (mat) mat = mat->next;
                if (sh) sh = sh->next;
            }
        }
        if (!(joint->flags & JOBJ_INSTANCE)) {
            const HSD_AnimJoint* anim = zip.anim ? zip.anim->child : NULL;
            const HSD_MatAnimJoint* mat = zip.material ? zip.material->child : NULL;
            const HSD_ShapeAnimJoint* sh = zip.shape ? zip.shape->child : NULL;
            for (const HSD_Joint* child = joint->child; child; child = child->next) {
                if (depth == TREE_LIMIT || !work(&c)) return false;
                stack[depth++] = (Zip){ child, anim, mat, sh };
                if (anim) anim = anim->next;
                if (mat) mat = mat->next;
                if (sh) sh = sh->next;
            }
        }
    }
    *output = c.demand;
    return true;
}
