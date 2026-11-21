#include "ftcostume_assets.h"
#include "ftcostume_limits.h"
#include "native_fighter_attachment.h"
#include <melee/it/native_article_animation_texture.h>

#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/robj.h>
#include <sysdolphin/baselib/tobj.h>

#include <stdint.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

typedef struct CostumeDrawWork {
    u32 joints, meshes, polygons, display_bytes, influences, particle_entries;
} CostumeDrawWork;

typedef struct CostumeJointTree {
    const HSD_Joint* owned[2 * NATIVE_MODEL_MAX_JOINTS];
    bool has_envelope_skeleton[2 * NATIVE_MODEL_MAX_JOINTS];
    CostumeDrawWork draw[2 * NATIVE_MODEL_MAX_JOINTS];
    u32 count;
    u32 construction_units;
    bool has_instances;
} CostumeJointTree;

static const HSD_Joint** joint_slot(CostumeJointTree* tree, const HSD_Joint* joint)
{
    uintptr_t hash = (uintptr_t) joint / sizeof(void*);
    size_t slots = sizeof(tree->owned) / sizeof(tree->owned[0]);
    size_t slot = hash % slots;
    while (tree->owned[slot] && tree->owned[slot] != joint)
        slot = (slot + 1) % slots;
    return &tree->owned[slot];
}

static bool collect_joints(CostumeJointTree* tree, const HSD_Joint* joint, u32 depth,
                           bool parent_has_skeleton)
{
    for (; joint != NULL; joint = joint->next) {
        if (depth >= FT_COSTUME_DEPTH_LEVELS || tree->count == NATIVE_MODEL_MAX_JOINTS)
            return false;
        const HSD_Joint** slot = joint_slot(tree, joint);
        if (*slot) return false;
        *slot = joint;
        // _HSD_mkEnvelopeModelNodeMtx uses the nearest skeleton ancestor.
        bool has_skeleton = parent_has_skeleton;
        if (joint->flags & JOBJ_SKELETON_ROOT) has_skeleton = true;
        else if (joint->flags & JOBJ_SKELETON) has_skeleton = joint->mtx != NULL;
        tree->has_envelope_skeleton[slot - tree->owned] = has_skeleton;
        ++tree->count;
        if (joint->flags & JOBJ_INSTANCE) {
            if (!joint->child) return false;
            tree->has_instances = true;
        } else if (!collect_joints(tree, joint->child, depth + 1, has_skeleton)) {
            return false;
        }
    }
    return true;
}

static bool validate_instance_draw(CostumeJointTree* tree, const HSD_Joint* joint,
                                   u32 depth, CostumeDrawWork* work)
{
    if (depth >= FT_COSTUME_DEPTH_LEVELS || ++work->joints > NATIVE_MODEL_MAX_JOINTS * 8)
        return false;
    if (joint->flags & JOBJ_INSTANCE) {
        // DispAll draws the referenced joint and its descendants. Its siblings
        // belong to its ordinary parent and are not part of an instance draw.
        return *joint_slot(tree, joint->child) &&
               validate_instance_draw(tree, joint->child, depth + 1, work);
    }
    const CostumeDrawWork* own = &tree->draw[joint_slot(tree, joint) - tree->owned];
    work->meshes += own->meshes;
    work->polygons += own->polygons;
    work->display_bytes += own->display_bytes;
    work->influences += own->influences;
    work->particle_entries += own->particle_entries;
    if (work->meshes > NATIVE_MODEL_MAX_MESHES || work->polygons > NATIVE_MODEL_MAX_POLYGONS ||
        work->display_bytes > NATIVE_MODEL_MAX_DISPLAY_BYTES ||
        work->influences > NATIVE_MODEL_MAX_SKIN_INFLUENCES ||
        work->particle_entries > NATIVE_MODEL_MAX_PARTICLE_ENTRIES) return false;
    for (const HSD_Joint* child = joint->child; child; child = child->next)
        if (!validate_instance_draw(tree, child, depth + 1, work)) return false;
    return true;
}

static bool validate_envelopes(CostumeJointTree* tree, HSD_EnvelopeDesc* const* table,
                               u32 constant_slots, bool separate, bool owner_is_root,
                               u32* remaining)
{
    if (!table) return false;
    for (unsigned slot = 0; table[slot]; ++slot) {
        if (slot >= 10) return false; // GX position and texture matrix slots.
        const HSD_EnvelopeDesc* first = table[slot];
        if (constant_slots & (1U << slot)) {
            if (first->joint != NULL) return false;
            continue;
        }
        if (!first->joint) return false;
        const bool blended = separate || first->weight < 1.0f - FLT_EPSILON;
        for (const HSD_EnvelopeDesc* entry = first; entry->joint; ++entry) {
            if (!*remaining || !isfinite(entry->weight) ||
                !*joint_slot(tree, entry->joint)) return false;
            --*remaining;
            // A rigid envelope on the skeleton root uses its joint matrix directly.
            if ((blended || (entry == first && !owner_is_root)) && !entry->joint->mtx)
                return false;
        }
    }
    return true;
}

static bool validate_polygon_skin(CostumeJointTree* tree, const HSD_PObjDesc* polygon,
                                  bool has_skeleton, bool owner_is_root, u32* remaining)
{
    if (pobj_type(polygon) == POBJ_SKIN)
        return !polygon->u.joint || *joint_slot(tree, polygon->u.joint);
    if (!has_skeleton) return false;
    if (pobj_type(polygon) == POBJ_ENVELOPE)
        return validate_envelopes(tree, polygon->u.envelope_p, 0, false, owner_is_root, remaining);
    const HSD_EnvelopeMatricesDesc* matrices = polygon->u.matrices;
    return pobj_type(polygon) == POBJ_SEPARATE_ENVELOPES && matrices &&
        validate_envelopes(tree, matrices->positions, 0, true, owner_is_root, remaining) &&
        validate_envelopes(tree, matrices->textures, matrices->constant_mask, true, owner_is_root, remaining);
}

static bool validate_joint_contents(CostumeJointTree* tree, u32 mesh_limit)
{
    unsigned meshes = 0, polygons = 0, particle_entries = 0;
    u32 expression_inputs = 0;
    u32 remaining = NATIVE_MODEL_MAX_SKIN_INFLUENCES;
    size_t display_bytes = 0;
    for (size_t i = 0; i < sizeof(tree->owned) / sizeof(tree->owned[0]); ++i) {
        const HSD_Joint* joint = tree->owned[i];
        if (!joint) continue;
        unsigned constraints = 0;
        for (const HSD_RObjDesc* r = joint->robjdesc; r; r = r->next) {
            ++tree->construction_units;
            if (++constraints > 256) return false;
            switch (r->flags & ROBJ_TYPE_MASK) {
            case REFTYPE_JOBJ:
                if (!r->u.joint || !*joint_slot(tree, r->u.joint)) return false;
                break;
            case REFTYPE_IKHINT:
                if (!r->u.ik_hint) return false;
                break;
            case REFTYPE_EXP:
            case REFTYPE_BYTECODE: {
                const HSD_RvalueList* input;
                if ((r->flags & ROBJ_TYPE_MASK) == REFTYPE_EXP) {
                    if (!r->u.exp) return false;
                    input = r->u.exp->rvalue;
                } else {
                    if (!r->u.bcexp) return false;
                    input = r->u.bcexp->rvalue;
                }
                for (; input && input->joint; ++input)
                    if (++expression_inputs > 4096 || !*joint_slot(tree, input->joint)) return false;
                break;
            }
            }
        }
        if (union_type_ptcl(joint)) {
            const unsigned start = particle_entries;
            for (const HSD_SList* p = joint->u.ptcl; p; p = p->next) {
                if (++particle_entries > NATIVE_MODEL_MAX_PARTICLE_ENTRIES) return false;
            }
            tree->draw[i].particle_entries = particle_entries - start;
        }
        if (!union_type_dobj(joint)) continue;
        const u32 start_meshes = meshes, start_polygons = polygons, start_remaining = remaining;
        const size_t start_bytes = display_bytes;
        unsigned siblings = 0;
        for (const HSD_DObjDesc* d = joint->u.dobjdesc; d; d = d->next) {
            // DObj and TObj constructors recursively initialize their siblings.
            if (++siblings > 256 || ++meshes > mesh_limit) return false;
            const HSD_MObjDesc* m = d->mobjdesc;
            if (m) {
                ++tree->construction_units;
                if (!m->mat || (m->rendermode & RENDER_BLENDING) == RENDER_NO_ZUPDATE)
                    return false;
                unsigned textures = 0;
                for (const HSD_TObjDesc* t = m->texdesc; t; t = t->next) {
                    ++tree->construction_units;
                    if (++textures > GX_MAX_TEXMAP || !t->imagedesc ||
                        (u32)t->wrap_s > GX_MIRROR || (u32)t->wrap_t > GX_MIRROR ||
                        (u32)t->magFilt > GX_LINEAR) return false;
                    GXTexFmt format = t->imagedesc->format;
                    if ((format == GX_TF_C4 || format == GX_TF_C8 || format == GX_TF_C14X2) &&
                        !t->tlutdesc) return false;
                }
            }
            for (const HSD_PObjDesc* p = d->pobjdesc; p; p = p->next) {
                if (++polygons > NATIVE_MODEL_MAX_POLYGONS) return false;
                display_bytes += (size_t)p->n_display * 32;
                if (display_bytes > NATIVE_MODEL_MAX_DISPLAY_BYTES) return false;
                const bool is_root = (joint->flags & JOBJ_SKELETON_ROOT) != 0;
                if (pobj_type(p) == POBJ_SHAPEANIM) {
                    const HSD_ShapeSetDesc* shape = p->u.shape_set;
                    if (!shape || ((shape->flags & SHAPESET_SKIN) && !shape->skin)) return false;
                    if (shape->flags & SHAPESET_SKIN) {
                        for (const HSD_PObjDesc* batch = shape->skin; batch; batch = batch->next) {
                            if (++polygons > NATIVE_MODEL_MAX_POLYGONS) return false;
                            display_bytes += (size_t)batch->n_display * 32;
                            if (display_bytes > NATIVE_MODEL_MAX_DISPLAY_BYTES ||
                                !validate_polygon_skin(tree, batch, tree->has_envelope_skeleton[i],
                                                       is_root, &remaining)) return false;
                        }
                    }
                } else if (!validate_polygon_skin(tree, p, tree->has_envelope_skeleton[i],
                                                  is_root, &remaining)) return false;
            }
        }
        if (tree->has_instances) {
            tree->draw[i].meshes = meshes - start_meshes;
            tree->draw[i].polygons = polygons - start_polygons;
            tree->draw[i].display_bytes = (u32)(display_bytes - start_bytes);
            tree->draw[i].influences = start_remaining - remaining;
        }
    }
    tree->construction_units += tree->count + meshes + polygons + particle_entries +
                                (NATIVE_MODEL_MAX_SKIN_INFLUENCES - remaining);
    return true;
}

static bool validate_controller(CostumeJointTree* tree, const HSD_AObjDesc* anim,
                                const HSD_Joint* animated_joint, u32* budget)
{
    if (!anim) return true;
    if (!*budget || !isfinite(anim->end_frame)) return false;
    --*budget;
    const HSD_Joint* target = (const HSD_Joint*)anim->obj_id;
    if (target && (!anim->obj_id_is_joint || !*joint_slot(tree, target))) return false;
    unsigned count = 0;
    for (const HSD_FObjDesc* f = anim->fobjdesc; f; f = f->next) {
        if (!*budget || ++count > 256 || (f->length && !f->ad) ||
            !isfinite(f->startframe) || f->startframe <= -32769.0f ||
            f->startframe >= 32768.0f) return false;
        --*budget;
        if (animated_joint) {
            if (f->type == HSD_A_J_PATH && (!target || !union_type_spline(target) ||
                union_type_ptcl(target) || !target->u.spline)) return false;
            if (f->type >= HSD_A_J_ROTX && f->type <= HSD_A_J_ROTZ &&
                (animated_joint->flags & JOBJ_USE_QUATERNION)) return false;
        }
    }
    return true;
}

static bool validate_joint_animation(CostumeJointTree* tree, const HSD_Joint* model,
                                     const HSD_AnimJoint* anim, u32* budget)
{
    if (!anim) return true;
    if (!validate_controller(tree, anim->aobjdesc, model, budget)) return false;
    const HSD_RObjAnimJoint* binding = anim->robj_anim;
    for (const HSD_RObjDesc* r = model->robjdesc; r && binding; r = r->next) {
        if (!validate_controller(tree, binding->aobjdesc, NULL, budget)) return false;
        binding = binding->next;
    }
    // HSD_JObjAddAnimAll pairs children in order and skips instance children.
    if (!(model->flags & JOBJ_INSTANCE)) {
        const HSD_AnimJoint* child = anim->child;
        for (const HSD_Joint* j = model->child; j && child; j = j->next) {
            if (!validate_joint_animation(tree, j, child, budget)) return false;
            child = child->next;
        }
    }
    return true;
}

static bool validate_material_tree(CostumeJointTree* tree, const HSD_Joint* model,
                                   const HSD_MatAnimJoint* joint,
                                   u32 depth, u32* remaining, u32* budget)
{
    for (; joint != NULL; joint = joint->next, model = model ? model->next : NULL) {
        if (depth >= FT_COSTUME_DEPTH_LEVELS || *remaining == 0) return false;
        --*remaining;
        const HSD_DObjDesc* draw = model && union_type_dobj(model) ? model->u.dobjdesc : NULL;
        for (const HSD_MatAnim* m = joint->matanim; m; m = m->next, draw = draw ? draw->next : NULL) {
            if (!*budget) return false;
            --*budget;
            if (!validate_controller(tree, m->aobjdesc, NULL, budget)) return false;
            NativeArticleAnimationTextureID slots[GX_MAX_TEXMAP];
            uint32_t count = 0, matched = 0;
            if (m->texanim && draw && draw->mobjdesc &&
                !nativeArticleAnimation_TextureIDs(draw->mobjdesc, slots, GX_MAX_TEXMAP, &count))
                return false;
            for (const HSD_TexAnim* t = m->texanim; t; t = t->next) {
                // Each palette becomes an owned runtime object. Count table
                // entries at each use, even if several tracks share a table.
                u32 entries = 1u + t->n_tluttbl + t->n_imagetbl;
                if (entries > *budget) return false;
                *budget -= entries;
                if (!validate_controller(tree, t->aobjdesc, NULL, budget)) return false;
                for (u32 i = 0; i < count; ++i) {
                    if (slots[i].id != t->id || (matched & (1u << i))) continue;
                    matched |= 1u << i;
                    const HSD_TObjDesc* target = slots[i].descriptor;
                    if (!HSD_TextureAnimationFits(t, target->tev != NULL, target->tlutdesc != NULL))
                        return false;
                }
            }
        }
        const HSD_Joint* child = model && !(model->flags & JOBJ_INSTANCE) ? model->child : NULL;
        if (!validate_material_tree(tree, child, joint->child, depth + 1, remaining, budget))
            return false;
    }
    return true;
}

FtModelValidity ftCostumeAssets_ValidateModel(const HSD_Joint* joint,
    const HSD_AnimJoint* animation, const HSD_MatAnimJoint* materials,
    u32 mesh_limit, FtModelCounts* counts)
{
    CostumeJointTree tree = {0};
    if (!joint || !collect_joints(&tree, joint, 0, false) || !validate_joint_contents(&tree, mesh_limit))
        return FT_MODEL_INVALID_JOINT;
    if (tree.has_instances) {
        CostumeDrawWork work = {0};
        for (const HSD_Joint* root = joint; root; root = root->next)
            if (!validate_instance_draw(&tree, root, 0, &work)) return FT_MODEL_INVALID_JOINT;
    }
    u32 remaining = NATIVE_MODEL_MAX_JOINTS;
    const u32 animation_limit = 16384;
    u32 budget = animation_limit;
    if (!validate_joint_animation(&tree, joint, animation, &budget))
        return FT_MODEL_INVALID_JOINT_ANIMATION;
    if (!validate_material_tree(&tree, joint, materials, 0, &remaining, &budget))
        return FT_MODEL_INVALID_MATERIAL_ANIMATION;
    if (counts) *counts = (FtModelCounts){
        tree.count, tree.construction_units + (animation_limit - budget)
    };
    return FT_MODEL_VALID;
}

HSD_Joint* ftCostumeAssets_LoadAttachment(FtCostumeAssets* assets, const char* symbol)
{
    u32 offset;
    if (!assets || !assets->joint || assets->import.status != HSD_DAT_OK || !symbol ||
        !HSD_DatFindRoot(&assets->dat, symbol, &offset)) return NULL;
    HSD_DatImport* import = &assets->import;
    const HSD_DatImport saved = *import;
    // A new mesh can extend the drawable prefix of an already imported vertex
    // array. All other model descriptors and payloads append to the arena.
    size_t size_count = 0;
    for (u32 i = 0; i < saved.record_count; ++i) {
        const HSD_DatImportRecord* r = &saved.records[i];
        if (r->type == DAT_JOINT && r->offset == offset) {
            // A cached descendant can now be constructed as its own model.
            // Its constraints must resolve inside that smaller joint tree.
            HSD_Joint* root = HSD_DatImportGraph(import, offset, DAT_JOINT);
            if (!root) {
                *import = saved;
                return NULL;
            }
            return ftCostumeAssets_ValidateModel(root, NULL, NULL, NATIVE_ATTACHMENT_DOBJS, NULL)
                == FT_MODEL_VALID ? root : NULL;
        }
        if (r->type == DAT_VTXDESC) size_count += r->count;
    }
    u32* sizes = size_count ? malloc(size_count * sizeof(*sizes)) : NULL;
    if (size_count && !sizes) return NULL;
    size_t index = 0;
    for (u32 i = 0; i < saved.record_count; ++i) {
        const HSD_DatImportRecord* r = &saved.records[i];
        if (r->type != DAT_VTXDESC) continue;
        const HSD_VtxDescList* list = r->object;
        for (u32 j = 0; j < r->count; ++j) sizes[index++] = list[j].vertex_size;
    }
    HSD_Joint* joint = HSD_DatImportGraph(import, offset, DAT_JOINT);
    if (ftCostumeAssets_ValidateModel(joint, NULL, NULL, NATIVE_ATTACHMENT_DOBJS, NULL) != FT_MODEL_VALID) {
        index = 0;
        for (u32 i = 0; i < saved.record_count; ++i) {
            const HSD_DatImportRecord* r = &saved.records[i];
            if (r->type != DAT_VTXDESC) continue;
            HSD_VtxDescList* list = r->object;
            for (u32 j = 0; j < r->count; ++j) list[j].vertex_size = sizes[index++];
        }
        // Existing hash entries never move when the importer appends records.
        for (u32 i = 0; i < saved.slot_count; ++i)
            if (saved.slots[i] > saved.record_count) saved.slots[i] = 0;
        *import = saved;
        joint = NULL;
    }
    free(sizes);
    return joint;
}

bool ftCostumeAssets_Import(const void* bytes, size_t size,
                            const char* joint_name,
                            const char* matanim_joint_name, void* arena,
                            size_t arena_size, u32 record_capacity,
                            FtCostumeAssets* output)
{
    u32 joint_root;
    u32 matanim_root = 0;
    if (output == NULL) return false;
    memset(output, 0, sizeof(*output));
    if (joint_name == NULL) {
        output->import.status = HSD_DAT_SYMBOL;
        return false;
    }
    output->import.status = HSD_DatOpen(&output->dat, bytes, size);
    if (output->import.status != HSD_DAT_OK) return false;
    if (HSD_DatImportInit(&output->import, &output->dat, arena, arena_size,
                          record_capacity) != HSD_DAT_OK) return false;
    output->import.texture_byte_limit = NATIVE_MODEL_MAX_TEXTURE_BYTES;
    output->import.display_list_limit = NATIVE_MODEL_MAX_DISPLAY_BYTES;
    if (!HSD_DatFindRoot(&output->dat, joint_name, &joint_root)) {
        output->import.status = HSD_DAT_SYMBOL;
        return false;
    }
    HSD_Joint* joint = HSD_DatImportGraph(&output->import, joint_root, DAT_JOINT);
    HSD_MatAnimJoint* materials = NULL;
    if (joint == NULL) {
        return false;
    }
    if (matanim_joint_name != NULL && matanim_joint_name[0] != '\0') {
        if (!HSD_DatFindRoot(&output->dat, matanim_joint_name,
                             &matanim_root))
        {
            output->import.status = HSD_DAT_SYMBOL;
            return false;
        }
        materials = HSD_DatImportGraph(
            &output->import, matanim_root, DAT_MATANIM_JOINT);
        if (materials == NULL) {
            return false;
        }
    }
    FtModelCounts counts;
    FtModelValidity validity = ftCostumeAssets_ValidateModel(joint, NULL, materials,
        NATIVE_MODEL_MAX_MESHES, &counts);
    if (validity != FT_MODEL_VALID) {
        output->import.status = HSD_DAT_DESCRIPTOR;
        output->import.error_offset = validity == FT_MODEL_INVALID_JOINT ? joint_root : matanim_root;
        return false;
    }
    output->joint_count = counts.joints;
    output->joint = joint;
    output->matanim_joint = materials;
    return true;
}

bool ftCostumeAssets_Load(const void* bytes, size_t size,
                          const char* joint_name,
                          const char* matanim_joint_name,
                          FtCostumeAssets* output)
{
    if (!output) return false;
    memset(output, 0, sizeof(*output));
    size_t arena_size = FT_COSTUME_IMPORT_ARENA_SIZE;
    u32 records = FT_COSTUME_IMPORT_RECORD_CAPACITY;
    for (;;) {
        void* arena = malloc(arena_size);
        if (!arena) {
            memset(output, 0, sizeof(*output));
            output->import.status = HSD_DAT_CAPACITY;
            return false;
        }
        if (ftCostumeAssets_Import(bytes, size, joint_name, matanim_joint_name,
                                   arena, arena_size, records, output)) {
            output->arena = arena;
            return true;
        }
        const bool capacity = output->import.status == HSD_DAT_CAPACITY;
        const bool record_limit = output->import.record_count == records;
        free(arena);
        // Preserve diagnostics, but never expose pointers into a failed arena.
        output->joint = NULL;
        output->matanim_joint = NULL;
        output->joint_count = 0;
        output->import.arena = NULL;
        output->import.records = NULL;
        output->import.slots = NULL;
        if (!capacity) return false;
        if (record_limit) {
            records *= 2;
        } else {
            if (arena_size == FT_COSTUME_IMPORT_MAX_ARENA_SIZE) return false;
            arena_size *= 2;
        }
    }
}
