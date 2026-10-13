#include "native_article_animation_live.h"
#include "native_article_animation_demand.h"
#include "native_article_animation_texture.h"
#include "native_script_item_actor.h"
#include "itmaterial.h"
#include "types.h"
#include <melee/ft/native_script_item_program.h>
#include <melee/ft/native_script_binding_item_model_resolve.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftparts.h>
#include <sysdolphin/baselib/native_animation_pool.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/tobj.h>
#include <sysdolphin/baselib/robj.h>
#include <limits.h>
#include <math.h>
#include <string.h>

enum { GRAPH_LIMIT = 512, VISIT_LIMIT = 8192 };

static bool pool_shape(const NativeMatchPool* pool, uint32_t capacity,
                       size_t object_size)
{
    return nativeAnimationGlobals.heap_epoch && pool->payload && pool->slots &&
        pool->free_order && pool->lifetime == nativeAnimationGlobals.heap_epoch &&
        pool->capacity == capacity && pool->object_size == object_size &&
        pool->stride == object_size && pool->used <= capacity &&
        pool->free_count == capacity - pool->used;
}

static bool available(const NativeMatchPool* pool, uint32_t count)
{
    if (count > pool->free_count) return false;
    // Only the slots that this allocation sequence will actually consume.
    uint8_t seen[NATIVE_MATCH_POOL_MAX_SLOTS / 8] = { 0 };
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t slot = pool->free_order[pool->free_count - 1 - i];
        if (slot >= pool->capacity || pool->slots[slot].occupied ||
            pool->slots[slot].generation == UINT32_MAX ||
            (seen[slot / 8] & (1u << (slot % 8)))) return false;
        seen[slot / 8] |= (uint8_t)(1u << (slot % 8));
    }
    return true;
}

bool nativeArticleAnimation_CanAllocate(uint32_t controllers, uint32_t tracks)
{
    return pool_shape(&nativeAObjPool, NATIVE_AOBJ_CAPACITY, sizeof(HSD_AObj)) &&
        pool_shape(&nativeFObjPool, NATIVE_FOBJ_CAPACITY, sizeof(HSD_FObj)) &&
        available(&nativeAObjPool, controllers) && available(&nativeFObjPool, tracks);
}

typedef struct LiveAdmission {
    const NativeArticleAnimationOrigin* origins;
    uint32_t origin_count;
    const HSD_Joint* descriptor;
    uint8_t controllers[NATIVE_AOBJ_CAPACITY / 8];
    uint8_t tracks[NATIVE_FOBJ_CAPACITY / 8];
    uint32_t visits;
    struct { uint32_t begin, end; } ranges[9];
    uint32_t range_count;
    uint32_t standard_classes;
    HSD_GObj* attachment_owner;
} LiveAdmission;

enum { StandardDrawable = 1, StandardMaterial = 2, StandardTexture = 4, StandardPolygon = 8 };
static bool standard(LiveAdmission* live, uint32_t kind)
{
    if (live->standard_classes & kind) return true;
    bool valid = kind == StandardDrawable ? HSD_DObjUsesStandardMethods() :
        kind == StandardMaterial ? HSD_MObjUsesStandardMethods() :
        kind == StandardTexture ? HSD_TObjUsesStandardMethods() : HSD_PObjUsesStandardMethods();
    if (valid) live->standard_classes |= kind;
    return valid;
}

static int origin_order(const NativeArticleAnimationOrigin* a,
                        const NativeArticleAnimationOrigin* b)
{
    uintptr_t x = (uintptr_t)a->target, y = (uintptr_t)b->target;
    if (x != y) return x < y ? -1 : 1;
    if (a->kind != b->kind) return a->kind < b->kind ? -1 : 1;
    x = (uintptr_t)a->controller; y = (uintptr_t)b->controller;
    if (x != y) return x < y ? -1 : 1;
    x = (uintptr_t)a->texture; y = (uintptr_t)b->texture;
    return x == y ? 0 : x < y ? -1 : 1;
}

static bool allowed(const LiveAdmission* live, const void* target,
                    const HSD_AObjDesc* origin, uint32_t kind)
{
    NativeArticleAnimationOrigin key = { .graph=live->descriptor,
        .target=target, .controller=origin, .kind=kind };
    const NativeArticleAnimationOrigin* entries = live->origins;
    // Transition preparation emits sorted, contiguous graph blocks. An owner
    // may append the same imported root from more than one immutable root row.
    for (uint32_t range = 0; range < live->range_count; ++range) {
        uint32_t begin = live->ranges[range].begin, end = live->ranges[range].end;
        uint32_t low = begin, high = end;
        while (low < high) {
            uint32_t mid = low + (high - low) / 2;
            if (origin_order(&entries[mid], &key) < 0) low = mid + 1;
            else high = mid;
        }
        if (low < end && entries[low].target == target &&
            entries[low].controller == origin && entries[low].kind == kind) return true;
    }
    return false;
}

static bool retain(const NativeMatchPool* pool, const void* object, uint8_t* seen)
{
    NativeMatchPoolReference reference;
    if (!nativeMatchPool_Reference(pool, object, &reference) ||
        reference.lifetime != nativeAnimationGlobals.heap_epoch ||
        reference.generation == UINT32_MAX ||
        nativeMatchPool_Resolve(reference) != object ||
        (seen[reference.slot / 8] & (1u << (reference.slot % 8)))) return false;
    seen[reference.slot / 8] |= (uint8_t)(1u << (reference.slot % 8));
    return true;
}

static bool controller(LiveAdmission* live, const HSD_AObj* a,
                       const void* target, uint32_t kind, const HSD_TexAnim* texture)
{
    if (!a) return true;
    if (!retain(&nativeAObjPool, a, live->controllers) || !a->native_origin ||
        a->hsd_obj || !allowed(live, target, a->native_origin, kind)) return false;
    const HSD_AObjDesc* origin = a->native_origin;
    if (origin->obj_id) return false;
    // JObjSortAnim moves the first TYPE_JOBJ track to the front and leaves the
    // relative order of every other track unchanged. Other targets do not sort.
    const HSD_FObjDesc* first = NULL;
    if (kind == NativeArticleAnimationTarget_Joint) {
        uint32_t count = 0;
        for (const HSD_FObjDesc* f = origin->fobjdesc; f; f = f->next) {
            if (++count > NATIVE_FOBJ_CAPACITY) return false;
            if (f->type == TYPE_JOBJ) { first = f; break; }
        }
    }
    const HSD_FObjDesc* expected = first ? first : origin->fobjdesc;
    const HSD_FObjDesc* remainder = origin->fobjdesc;
    bool special = first != NULL;
    const HSD_FObj* f = a->fobj;
    uint32_t count = 0;
    while (f || expected) {
        if (!f || !expected || ++count > NATIVE_FOBJ_CAPACITY ||
            !retain(&nativeFObjPool, f, live->tracks) || f->native_origin != expected ||
            f->ad_head != expected->ad || f->length != expected->length ||
            f->obj_type != expected->type || f->frac_value != expected->frac_value ||
            f->frac_slope != expected->frac_slope || !isfinite(expected->startframe) ||
            (double)expected->startframe <= (double)INT16_MIN - 1 ||
            (double)expected->startframe >= (double)INT16_MAX + 1 ||
            f->startframe != (s16)expected->startframe ||
            (f->flags & 15) > 6 || f->op > HSD_A_OP_KEY || f->op_intrp > HSD_A_OP_KEY)
            return false;
        if (kind == NativeArticleAnimationTarget_Texture &&
            (f->obj_type == HSD_A_T_TIMG || f->obj_type == HSD_A_T_TCLT)) {
            bool palette = f->obj_type == HSD_A_T_TCLT;
            uint32_t entries = texture ? (palette ? texture->n_tluttbl : texture->n_imagetbl) : 0;
            if (!texture || !entries || (palette ? !texture->tluttbl : !texture->imagetbl) ||
                !isfinite(f->p0) || !isfinite(f->p1) || f->p0 <= -1.0f || f->p1 <= -1.0f ||
                f->p0 >= entries || f->p1 >= entries ||
                (palette && entries > 255) || (f->op && f->op != HSD_A_OP_CON) ||
                (f->op_intrp && f->op_intrp != HSD_A_OP_CON)) return false;
        }
        uintptr_t head = (uintptr_t)f->ad_head, cursor = (uintptr_t)f->ad;
        if (f->ad && (!head || f->length > UINTPTR_MAX - head ||
                      cursor < head || cursor > head + f->length)) return false;
        f = f->next;
        if (special) { expected = remainder; special = false; }
        else expected = expected->next;
        if (first && expected == first) expected = expected->next;
    }
    return true;
}

static bool material_class(const HSD_MObj* material)
{
    HSD_ClassInfo* info = HSD_CLASS_METHOD(material);
    if (info == HSD_CLASS_INFO(&hsdMObj)) return true;
    // Item's native class specializes draw setup only. Animation attachment,
    // request, evaluation and removal are direct HSD_MObj functions, with no
    // class dispatch. Admit this exact initialized prefix, never a subclass.
    return info == HSD_CLASS_INFO(&it_mobj) && (info->head.flags & 1) &&
        info->head.obj_size == sizeof(HSD_MObj) &&
        info->head.info_size == sizeof(HSD_MObjInfo) &&
        info->head.parent == HSD_CLASS_INFO(&hsdMObj);
}

// Native RemoveAnim retains both the borrowed table and selected image.
// Validate the complete prepared resource union for this exact texture target.
static bool texture_tables(LiveAdmission* live, const HSD_TObj* t,
    const HSD_TObjDesc* d, const HSD_TexAnim** selected)
{
    bool table = !t->imagetbl, image = t->imagedesc == d->imagedesc;
    bool palette_table = !t->tluttbl, palette_selection = t->tlut_no == UINT8_MAX;
    bool controller_table = !t->aobj;
    if (t->aobj) {
        NativeMatchPoolReference reference;
        if (!nativeMatchPool_Reference(&nativeAObjPool, t->aobj, &reference) ||
            reference.lifetime != nativeAnimationGlobals.heap_epoch ||
            reference.generation == UINT32_MAX ||
            nativeMatchPool_Resolve(reference) != t->aobj) return false;
    }
    *selected = NULL;
    for (uint32_t range = 0; range < live->range_count; ++range) {
        for (uint32_t i = live->ranges[range].begin; i < live->ranges[range].end; ++i) {
            const NativeArticleAnimationOrigin* origin = &live->origins[i];
            if (origin->target != d || origin->kind != NativeArticleAnimationTarget_Texture ||
                !origin->texture) continue;
            const HSD_TexAnim* resource = origin->texture;
            if (resource->n_imagetbl > 512 ||
                (resource->n_imagetbl && !resource->imagetbl) || resource->n_tluttbl > 255 ||
                (resource->n_tluttbl && !resource->tluttbl)) return false;
            bool palettes = resource->n_tluttbl ? t->tluttbl != NULL : t->tluttbl == NULL;
            for (uint32_t entry = 0; palettes && entry < resource->n_tluttbl; ++entry) {
                if (++live->visits > VISIT_LIMIT) return false;
                const HSD_Tlut* actual = t->tluttbl[entry];
                const HSD_TlutDesc* expected = resource->tluttbl[entry];
                palettes = actual && expected && actual->lut == expected->lut &&
                    actual->fmt == expected->fmt && actual->n_entries == expected->n_entries;
            }
            if (palettes && resource->n_tluttbl) palettes = t->tluttbl[resource->n_tluttbl] == NULL;
            if (palettes) {
                palette_table = true;
                if (t->tlut_no < resource->n_tluttbl) palette_selection = true;
            }
            if (t->imagetbl == resource->imagetbl) {
                table = true;
                if (palettes && t->aobj && t->aobj->native_origin == origin->controller) {
                    controller_table = true; *selected = resource;
                }
            }
            for (uint32_t entry = 0; entry < resource->n_imagetbl; ++entry) {
                if (++live->visits > VISIT_LIMIT) return false;
                if (resource->imagetbl[entry] && t->imagedesc == resource->imagetbl[entry]) image = true;
            }
        }
    }
    return table && image && palette_table && palette_selection && controller_table;
}

static bool material(LiveAdmission* live, const HSD_MObj* m,
                     const HSD_MObjDesc* d)
{
    if (!m || !d) return m == NULL && d == NULL;
    if (++live->visits > VISIT_LIMIT || !material_class(m) ||
        !standard(live, StandardMaterial) || !m->mat || !d->mat ||
        (m->pe == NULL) != (d->pedesc == NULL) ||
        !controller(live, m->aobj, d, NativeArticleAnimationTarget_Material, NULL)) return false;
    NativeArticleAnimationTextureID textures[NATIVE_ARTICLE_ANIMATION_TEXTURE_LIMIT];
    uint32_t texture_count, index = 0;
    if (((m->rendermode & RENDER_SHADOW) && tobj_shadows) ||
        ((m->rendermode & RENDER_TOON) && tobj_toon && tobj_toon->imagedesc) ||
        !nativeArticleAnimation_TextureIDs(d, textures,
            NATIVE_ARTICLE_ANIMATION_TEXTURE_LIMIT, &texture_count)) return false;
    const HSD_TObj* t = m->tobj;
    const HSD_TObjDesc* td = d->texdesc;
    while (t || td) {
        const HSD_TexAnim* texture = NULL;
        if (!t || !td || ++live->visits > VISIT_LIMIT ||
            !standard(live, StandardTexture) || HSD_CLASS_METHOD(t) != HSD_CLASS_INFO(&hsdTObj) ||
            index >= texture_count || textures[index].descriptor != td ||
            ((t->flags ^ td->blend_flags) & (TEX_COORD_MASK | TEX_BUMP)) ||
            t->id != textures[index].id || t->src != td->src || t->lod != td->lod ||
            !texture_tables(live, t, td, &texture) ||
            (t->tev == NULL) != (td->tev == NULL) ||
            !controller(live, t->aobj, td, NativeArticleAnimationTarget_Texture, texture)) return false;
        // Image/palette selection uses prepared bounded CON streams. The live
        // palette copies must match a whole admitted descriptor table, including
        // its terminal NULL; render-assigned tlut_name is intentionally mutable.
        t = t->next; td = td->next; ++index;
    }
    return index == texture_count;
}

typedef struct JointZip {
    const HSD_Joint* descriptor;
    HSD_JObj* joint;
    HSD_JObj* parent;
} JointZip;

static bool attachment_references(const LiveAdmission* live, const HSD_RObj* reference)
{
    if (!reference) return true;
    NativeFighterLifetime owner;
    if (!live->attachment_owner ||
        !nativeFighterLifetime_Capture(live->attachment_owner->user_data, &owner) ||
        !ftPartsTable || (unsigned)owner.kind >= Ft_Kind_RuntimeCapacity ||
        !ftPartsTable[owner.kind] || !owner.data->parts) return false;
    uint32_t count = ftPartsTable[owner.kind]->parts_num;
    if (!count || count > MAX_FT_PARTS) return false;
    uint32_t channels = 0;
    for (; reference; reference = reference->next) {
        // lb_8000C1C0/C228/C290 install translation/rotation/scale constraints.
        // No descriptor RObj, expression, animation or other target is admitted.
        uint32_t channel = reference->flags & 7;
        if ((channel != 1 && channel != 2 && channel != 4) ||
            reference->flags != (0x90000000u | channel) ||
            (channels & channel) || reference->aobj) return false;
        channels |= channel;
        bool found = false;
        for (uint32_t i = 0; i < count; ++i)
            if (owner.data->parts[i].joint == reference->u.jobj) {
                found = true; break;
            }
        if (!found) return false;
    }
    return nativeFighterLifetime_Current(&owner);
}

static bool origins(LiveAdmission* live)
{
    if (live->origin_count > NATIVE_SCRIPT_ENTITY_ANIMATION_ORIGINS ||
        (live->origin_count && !live->origins)) return false;
    const NativeArticleAnimationOrigin* entries = live->origins;
    for (uint32_t begin = 0; begin < live->origin_count;) {
        uint32_t end = begin + 1;
        while (end < live->origin_count && entries[end].graph == entries[begin].graph &&
               origin_order(&entries[end - 1], &entries[end]) < 0) ++end;
        if (entries[begin].graph == live->descriptor) {
            if (live->range_count == 9) return false;
            live->ranges[live->range_count].begin = begin;
            live->ranges[live->range_count++].end = end;
        }
        begin = end;
    }
    return true;
}

bool nativeArticleAnimation_Live(const HSD_Joint* descriptor, HSD_JObj* root,
    const NativeScriptItemProgram* program, const NativeArticleAnimationOrigin* origin_data,
    uint32_t origin_count, HSD_GObj* attachment_owner)
{
    if (!descriptor || !root || root->parent || !program || !program->bank || !program->bank->valid ||
        !program->article || !program->article->x10_modelDesc ||
        program->model_root_count > NATIVE_FIGHTER_ARTICLE_MODEL_ROOT_LIMIT ||
        (program->model_root_count && !program->model_roots) ||
        nativeAnimationGlobals.end_callbacks || nativeAnimationGlobals.callback_depth ||
        !pool_shape(&nativeAObjPool, NATIVE_AOBJ_CAPACITY, sizeof(HSD_AObj)) ||
        !pool_shape(&nativeFObjPool, NATIVE_FOBJ_CAPACITY, sizeof(HSD_FObj)) ||
        !HSD_JObjUsesStandardMethods()) return false;
    bool owned = descriptor == program->article->x10_modelDesc->x0_joint;
    for (uint32_t i = 0; i < program->model_root_count; ++i)
        if (program->model_roots[i].root == descriptor) owned = true;
    if (!owned) return false;
    LiveAdmission live = { .descriptor = descriptor,
                          .origins = origin_data, .origin_count = origin_count,
                          .attachment_owner = attachment_owner };
    if (!origins(&live)) return false;
    JointZip stack[GRAPH_LIMIT] = { { descriptor, root, NULL } };
    const HSD_Joint* descriptors[GRAPH_LIMIT];
    HSD_JObj* joints[GRAPH_LIMIT];
    uint32_t count = 1, joint_count = 0;
    const uint32_t structural = JOBJ_INSTANCE | JOBJ_PTCL | JOBJ_SPLINE |
        JOBJ_EFFECTOR | JOBJ_USE_QUATERNION | JOBJ_USER_DEF_MTX |
        JOBJ_MTX_INDEP_PARENT | JOBJ_MTX_INDEP_SRT;
    const uint32_t world_matrix = JOBJ_USER_DEF_MTX |
        JOBJ_MTX_INDEP_PARENT | JOBJ_MTX_INDEP_SRT;
    while (count) {
        JointZip zip = stack[--count];
        const HSD_Joint* d = zip.descriptor;
        HSD_JObj* j = zip.joint;
        // SetWorldMatrix installs this complete flag set on the retained root.
        // It changes matrix ownership, not graph/controller identity. Children
        // and partial flag substitutions still require descriptor equality.
        uint32_t required = structural;
        if (j == root && (j->flags & world_matrix) == world_matrix)
            required &= ~world_matrix;
        if (!d || !j || joint_count == GRAPH_LIMIT ||
            HSD_CLASS_METHOD(j) != HSD_CLASS_INFO(&hsdJObj) ||
            j->id != (uintptr_t)d || j->parent != zip.parent ||
            ((j->flags ^ d->flags) & required) ||
            (j->flags & (JOBJ_INSTANCE | JOBJ_PTCL | JOBJ_SPLINE | JOBJ_EFFECTOR)) ||
            d->robjdesc || !attachment_references(&live, j->robj) ||
            !controller(&live, j->aobj, d, NativeArticleAnimationTarget_Joint, NULL)) return false;
        for (uint32_t i = 0; i < joint_count; ++i) if (joints[i] == j) return false;
        descriptors[joint_count] = d; joints[joint_count++] = j;
        if (d->next || j->next) {
            if (count == GRAPH_LIMIT || !d->next || !j->next) return false;
            stack[count++] = (JointZip){ d->next, j->next, zip.parent };
        }
        if (d->child || j->child) {
            if (count == GRAPH_LIMIT || !d->child || !j->child) return false;
            stack[count++] = (JointZip){ d->child, j->child, j };
        }
        const HSD_DObjDesc* dd = d->u.dobjdesc;
        const HSD_DObj* drawable = j->u.dobj;
        while (dd || drawable) {
            if (!dd || !drawable || ++live.visits > VISIT_LIMIT ||
                !standard(&live, StandardDrawable) ||
                HSD_CLASS_METHOD(drawable) != HSD_CLASS_INFO(&hsdDObj) || drawable->aobj ||
                !material(&live, drawable->mobj, dd->mobjdesc)) return false;
            const HSD_PObjDesc* pd = dd->pobjdesc;
            const HSD_PObj* polygon = drawable->pobj;
            while (pd || polygon) {
                if (!pd || !polygon || ++live.visits > VISIT_LIMIT ||
                    !standard(&live, StandardPolygon) ||
                    HSD_CLASS_METHOD(polygon) != HSD_CLASS_INFO(&hsdPObj) ||
                    pobj_type(polygon) != POBJ_SKIN || pobj_type(pd) != POBJ_SKIN ||
                    polygon->verts != pd->verts || polygon->display != pd->display ||
                    polygon->n_display != pd->n_display || polygon->flags != pd->flags)
                    return false;
                polygon = polygon->next; pd = pd->next;
            }
            drawable = drawable->next; dd = dd->next;
        }
    }
    // Skin references must resolve to this graph's exact positional descriptor,
    // not an unrelated graph that happens to share a global descriptor ID.
    for (uint32_t i = 0; i < joint_count; ++i) {
        const HSD_DObjDesc* dd = descriptors[i]->u.dobjdesc;
        const HSD_DObj* drawable = joints[i]->u.dobj;
        for (; dd; dd = dd->next, drawable = drawable->next) {
            const HSD_PObjDesc* pd = dd->pobjdesc;
            const HSD_PObj* polygon = drawable->pobj;
            for (; pd; pd = pd->next, polygon = polygon->next) {
                if (!pd->u.joint) { if (polygon->u.jobj) return false; continue; }
                bool found = false;
                for (uint32_t n = 0; n < joint_count; ++n)
                    if (descriptors[n] == pd->u.joint && joints[n] == polygon->u.jobj) {
                        found = true; break;
                    }
                if (!found) return false;
            }
        }
    }
    return true;
}
