#include "native_article_resource_animation.h"
#include "native_article_resource_model.h"
#include "native_article_animation_live.h"
#include "native_article_animation_demand.h"
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/gm/native_rollback_pages.h>
#include <melee/gm/native_match_arena.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/native_animation_pool.h>
#include <sysdolphin/baselib/native_hsd_reservation.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct AnimationOwner {
    const NativeScriptItemProgram* program;
    NativeSourcePackage* package;
    const NativeScriptCompileBank* bank;
    const NativeScriptItemModelRootAsset* roots;
    const NativeScriptItemModelAnimationAsset* bundles;
    uint64_t identity;
    uint32_t root_count, bundle_count, origin_begin, origin_count;
    uint32_t recipe_begin, recipe_count;
    uint16_t capacity;
    int16_t reserved_part;
} AnimationOwner;

typedef struct AnimationRecipe {
    struct {
        uint32_t root_ordinal, bundle_ordinal;
        uint8_t path[NATIVE_ARTICLE_RESOURCE_ANIMATION_PATH], depth;
    } input;
    const HSD_Joint* target;
    NativeArticleAnimationDemand demand;
} AnimationRecipe;

typedef struct AnimationScene {
    AnimationOwner owners[NATIVE_ARTICLE_RESOURCE_ANIMATION_OWNERS];
    AnimationRecipe recipes[NATIVE_ARTICLE_RESOURCE_ANIMATION_APPLICATIONS];
    NativeArticleAnimationOrigin origins[NATIVE_SCRIPT_ENTITY_ANIMATION_ORIGINS];
    NativeSourcePackage* packages[Ft_Kind_RuntimeCapacity];
    uint64_t epoch, heap_epoch;
    uint32_t owner_count, recipe_count, origin_count, package_count;
} AnimationScene;

// Section placement captures the actual prepared ledger, with leased immutable
// assets. Controllers remain in the existing saved AObj/FObj pools.
static AnimationScene scene;

static bool boundary(void)
{
    return !(nativeMatchSceneArena.lifetime && nativeMatchSceneArena.sealed) && !nativeRollbackPages_IsInitialized() && nativeScriptRuntime_Idle() &&
        !nativeHsdReservation_Active() && !HSD_GObj_CurrentInvokedProc &&
        !HSD_GObj_CurrentInvokedProcGObj && !HSD_GObj_804D7818 &&
        !HSD_GObj_DelayedProcInfo.flags;
}

static const HSD_Joint* descriptor_path(const HSD_Joint* joint,
    const uint8_t* path, unsigned depth)
{
    for (unsigned i = 0; joint && i < depth; ++i) {
        joint = joint->child;
        for (unsigned n = 0; joint && n < path[i]; ++n) joint = joint->next;
    }
    return joint;
}

static bool empty_graphs(void)
{
    for (uint32_t c = 0; c < NATIVE_ARTICLE_RESOURCE_COLLECTIONS; ++c) {
        const ResourceCollection* set = &nativeArticleResourceCollections[c];
        if (set->active_operations || set->created > NATIVE_ARTICLE_RESOURCE_NODES) return false;
        for (uint32_t n = 0; n < set->created; ++n)
            if (set->nodes[n].model) return false;
    }
    return true;
}

static bool unpin(AnimationScene* state)
{
    while (state->package_count) {
        if (!nativeSourcePackage_Unpin(state->packages[state->package_count - 1])) return false;
        --state->package_count;
    }
    return true;
}

static bool prepare_owner(AnimationScene* candidate, const NativeScriptItemProgram* prepared,
    NativeSourcePackage* package)
{
    // Publication copies program rows and rebases internal selector/state
    // pointers. Asset identity is shared, row address is deliberately different.
    const NativeScriptItemProgram* program = nativeScriptItem_Program(prepared->author_kind, prepared->owner);
    if (candidate->owner_count == NATIVE_ARTICLE_RESOURCE_ANIMATION_OWNERS ||
        !program || nativeFighterRuntime_SourcePackage(prepared->author_kind) != package ||
        program->author_kind != prepared->author_kind || program->owner != prepared->owner ||
        program->identity != prepared->identity || program->bank != prepared->bank ||
        program->article != prepared->article || program->kind != prepared->kind ||
        program->model_roots != prepared->model_roots || program->model_root_count != prepared->model_root_count ||
        program->model_animations != prepared->model_animations ||
        program->model_animation_count != prepared->model_animation_count ||
        program->resource_capacity != prepared->resource_capacity ||
        program->resource_reserved_part != prepared->resource_reserved_part ||
        program->bank != &package->compiler.bank || !program->bank->valid ||
        !nativeArticleResource_ValidateRecipe(program->resource_capacity,
            program->resource_reserved_part, package->assets.parts.parts_num) ||
        program->resource_capacity < 2 || program->resource_capacity > NATIVE_ARTICLE_RESOURCE_NODES ||
        program->model_root_count > NATIVE_FIGHTER_ARTICLE_MODEL_ROOT_LIMIT ||
        (program->model_root_count && !program->model_roots) ||
        program->model_animation_count > NATIVE_FIGHTER_ARTICLE_MODEL_ANIMATION_LIMIT ||
        (program->model_animation_count && !program->model_animations) ||
        program->resource_selector_count > NATIVE_SCRIPT_ITEM_RESOURCE_SELECTORS ||
        (program->resource_selector_count && !program->resource_selectors)) return false;
    AnimationOwner* owner = &candidate->owners[candidate->owner_count++];
    *owner = (AnimationOwner){ .program=program, .package=package, .bank=program->bank,
        .roots=program->model_roots, .bundles=program->model_animations, .identity=program->identity,
        .root_count=program->model_root_count, .bundle_count=program->model_animation_count,
        .capacity=program->resource_capacity, .reserved_part=program->resource_reserved_part,
        .recipe_begin=candidate->recipe_count, .origin_begin=candidate->origin_count };
    for (uint32_t i = 0; i < program->resource_selector_count; ++i) {
        const NativeScriptItemResourceSelector* selector = &program->resource_selectors[i];
        if ((selector->semantic & NATIVE_SCRIPT_ITEM_RESOURCES_OPERATION_MASK) != NATIVE_SCRIPT_ITEM_RESOURCES_ANIMATION ||
            selector->operation != NativeScriptItemResource_AddAnimation - NativeScriptItemResource_Resources) continue;
        if (selector->depth > NATIVE_ARTICLE_RESOURCE_ANIMATION_PATH || selector->animation >= owner->bundle_count) return false;
        bool admitted = false;
        for (uint32_t root = 0; root < owner->root_count; ++root) {
            const HSD_Joint* target = descriptor_path(owner->roots[root].root, selector->path, selector->depth);
            if (!target) continue;
            const NativeScriptItemModelAnimationAsset* bundle = &owner->bundles[selector->animation];
            AnimationRecipe prepared = { .input={ .root_ordinal=root, .bundle_ordinal=selector->animation,
                .depth=selector->depth }, .target=target };
            memcpy(prepared.input.path, selector->path, selector->depth);
            if (!nativeArticleAnimation_Census(target, bundle->anim, bundle->matanim, bundle->shapeanim,
                &prepared.demand)) return false;
            if (prepared.demand.uncovered) continue;
            admitted = true;
            bool duplicate = false;
            for (uint32_t n = owner->recipe_begin; n < candidate->recipe_count; ++n) {
                const AnimationRecipe* prior = &candidate->recipes[n];
                if (prior->input.root_ordinal == root && prior->input.bundle_ordinal == selector->animation &&
                    prior->input.depth == selector->depth && !memcmp(prior->input.path, selector->path, selector->depth)) duplicate = true;
            }
            if (duplicate) continue;
            if (candidate->recipe_count == NATIVE_ARTICLE_RESOURCE_ANIMATION_APPLICATIONS) return false;
            candidate->recipes[candidate->recipe_count++] = prepared;
            ++owner->recipe_count;
        }
        // A dynamic node may load any declared root. Only compatible tuples
        // are admitted; a call matching none is an authoring error at transition.
        if (!admitted) return false;
    }
    for (uint32_t root = 0; root < owner->root_count; ++root) {
        const HSD_Joint* graph = owner->roots[root].root;
        bool duplicate = false;
        for (uint32_t previous = 0; previous < root; ++previous)
            if (owner->roots[previous].root == graph) duplicate = true;
        if (duplicate) continue;
        NativeArticleAnimationApplication applications[NATIVE_ARTICLE_RESOURCE_ANIMATION_APPLICATIONS];
        uint32_t application_count = 0;
        for (uint32_t n = owner->recipe_begin; n < candidate->recipe_count; ++n) {
            const AnimationRecipe* recipe = &candidate->recipes[n];
            if (owner->roots[recipe->input.root_ordinal].root != graph) continue;
            const NativeScriptItemModelAnimationAsset* bundle = &owner->bundles[recipe->input.bundle_ordinal];
            applications[application_count++] = (NativeArticleAnimationApplication){
                recipe->target, bundle->anim, bundle->matanim, bundle->shapeanim };
        }
        if (!application_count) continue;
        uint32_t written;
        if (!nativeArticleAnimation_PrepareApplications(graph, applications, application_count,
            candidate->origins + candidate->origin_count,
            NATIVE_SCRIPT_ENTITY_ANIMATION_ORIGINS - candidate->origin_count, &written)) return false;
        candidate->origin_count += written;
        owner->origin_count += written;
    }
    return true;
}

bool nativeArticleResourceAnimation_Prepare(void)
{
    if (!boundary() || scene.epoch || !nativeGObjPool.lifetime ||
        !nativeAnimationGlobals.heap_epoch || !nativeArticleAnimation_CanAllocate(0, 0) ||
        !empty_graphs())
        return false;
    AnimationScene* candidate = calloc(1, sizeof(*candidate));
    if (!candidate) return false;
    for (unsigned kind = 0; kind < Ft_Kind_RuntimeCapacity; ++kind) {
        NativeSourcePackage* package = (NativeSourcePackage*)nativeFighterRuntime_SourcePackage((FighterKind)kind);
        if (!package) continue;
        bool seen = false, needed = false;
        for (uint32_t p = 0; p < candidate->package_count; ++p)
            if (candidate->packages[p] == package) seen = true;
        if (seen) continue;
        if (package->address != package || !package->ready || !package->assets.ready ||
            package->program.items.count > NATIVE_SCRIPT_ENTITY_LIMIT) goto failure;
        for (uint32_t i = 0; i < package->program.items.count; ++i) {
            const NativeScriptItemProgram* program = &package->program.items.programs[i];
            if (!program->resource_capacity) continue;
            if (!prepare_owner(candidate, program, package)) goto failure;
            needed = true;
        }
        if (needed) {
            if (candidate->package_count == Ft_Kind_RuntimeCapacity || !nativeSourcePackage_Pin(package)) goto failure;
            candidate->packages[candidate->package_count++] = package;
        }
    }
    candidate->epoch = nativeGObjPool.lifetime;
    candidate->heap_epoch = nativeAnimationGlobals.heap_epoch;
    scene = *candidate;
    free(candidate);
    return true;
failure:
    { bool released = unpin(candidate);
      HSD_ASSERTREPORT(0, released, "Resource animation package lease failed to unwind"); }
    free(candidate);
    return false;
}

bool nativeArticleResourceAnimation_Retire(void)
{
    if (!boundary() || !empty_graphs()) return false;
    if (!scene.epoch) return true;
    if (!unpin(&scene)) return false;
    memset(&scene, 0, sizeof(scene));
    return true;
}

static const AnimationOwner* retained_owner(const ResourceCapture* resource)
{
    if (!scene.epoch || scene.epoch != nativeGObjPool.lifetime ||
        scene.heap_epoch != nativeAnimationGlobals.heap_epoch ||
        scene.owner_count > NATIVE_ARTICLE_RESOURCE_ANIMATION_OWNERS ||
        scene.recipe_count > NATIVE_ARTICLE_RESOURCE_ANIMATION_APPLICATIONS ||
        scene.origin_count > NATIVE_SCRIPT_ENTITY_ANIMATION_ORIGINS ||
        !nativeArticleResource_Current(resource) || !nativeArticleResourceModel_OwnerReady(&resource->owner)) return NULL;
    const NativeScriptItemProgram* p = resource->owner.program;
    for (uint32_t i = 0; i < scene.owner_count; ++i) {
        const AnimationOwner* owner = &scene.owners[i];
        if (owner->program != p) continue;
        if (owner->package != nativeFighterRuntime_SourcePackage(p->author_kind) ||
            owner->package->address != owner->package || !owner->package->ready ||
            p->identity != owner->identity || p->bank != owner->bank ||
            p->model_roots != owner->roots || p->model_root_count != owner->root_count ||
            p->model_animations != owner->bundles || p->model_animation_count != owner->bundle_count ||
            p->resource_capacity != owner->capacity || p->resource_reserved_part != owner->reserved_part ||
            resource->root_index >= owner->root_count ||
            owner->roots[resource->root_index].root != resource->descriptor ||
            owner->recipe_begin > scene.recipe_count || owner->recipe_count > scene.recipe_count-owner->recipe_begin ||
            owner->origin_begin > scene.origin_count || owner->origin_count > scene.origin_count-owner->origin_begin)
            return NULL;
        return owner;
    }
    return NULL;
}

typedef struct AnimationCapture {
    ResourceCapture resource;
    const AnimationOwner* owner;
    HSD_JObj* joint;
    const HSD_Joint* target;
} AnimationCapture;

static bool live(const AnimationCapture* capture)
{
    return retained_owner(&capture->resource) == capture->owner &&
        nativeArticleAnimation_Live(capture->resource.descriptor, capture->resource.model,
            capture->owner->program, scene.origins + capture->owner->origin_begin,
            capture->owner->origin_count,
            nativeScriptItem_AttachmentOwner(capture->resource.owner.object));
}

static bool capture(Item_GObj* object, unsigned index, const uint8_t* path,
                    unsigned depth, AnimationCapture* saved)
{
    if (depth > NATIVE_ARTICLE_RESOURCE_ANIMATION_PATH || (depth && !path) ||
        nativeHsdReservation_Active() || !nativeArticleResource_BeginOperation(object, index, &saved->resource))
        return false;
    saved->owner = retained_owner(&saved->resource);
    if (saved->owner && saved->resource.model && saved->resource.descriptor && live(saved)) {
        saved->joint = saved->resource.model;
        saved->target = saved->resource.descriptor;
        for (unsigned i = 0; saved->joint && i < depth; ++i) {
            saved->joint = saved->joint->child;
            saved->target = saved->target->child;
            for (unsigned n = 0; saved->joint && n < path[i]; ++n) {
                saved->joint = saved->joint->next;
                saved->target = saved->target->next;
            }
        }
        if (saved->joint && saved->target && live(saved)) return true;
    }
    nativeArticleResource_EndOperation(&saved->resource);
    return false;
}

static bool finish(AnimationCapture* saved)
{
    bool valid = live(saved);
    bool current = nativeArticleResource_EndOperation(&saved->resource);
    return valid && current;
}

bool nativeArticleResource_AddAnimation(Item_GObj* object, unsigned index,
    const uint8_t* path, unsigned depth, uint32_t bundle_ordinal)
{
    AnimationCapture saved;
    if (!capture(object, index, path, depth, &saved)) return false;
    const AnimationRecipe* recipe = NULL;
    for (uint32_t i = 0; i < saved.owner->recipe_count; ++i) {
        const AnimationRecipe* row = &scene.recipes[saved.owner->recipe_begin + i];
        if (row->input.root_ordinal == saved.resource.root_index && row->input.bundle_ordinal == bundle_ordinal &&
            row->input.depth == depth && (!depth || !memcmp(row->input.path, path, depth)) &&
            row->target == saved.target) { recipe = row; break; }
    }
    bool applied = recipe && nativeArticleAnimation_CanAllocate(recipe->demand.controllers, recipe->demand.tracks);
    if (applied) {
        const NativeScriptItemModelAnimationAsset* bundle = &saved.owner->bundles[bundle_ordinal];
        HSD_JObjAddAnimAll(saved.joint, (HSD_AnimJoint*)bundle->anim,
            (HSD_MatAnimJoint*)bundle->matanim, (HSD_ShapeAnimJoint*)bundle->shapeanim);
    }
    bool current = finish(&saved);
    return applied && current;
}

bool nativeArticleResource_RequestAnimation(Item_GObj* object, unsigned index,
    const uint8_t* path, unsigned depth, float frame)
{
    AnimationCapture saved;
    if (!isfinite(frame) || !capture(object, index, path, depth, &saved)) return false;
    HSD_JObjReqAnimAll(saved.joint, frame);
    return finish(&saved);
}

bool nativeArticleResource_AdvanceAnimation(Item_GObj* object, unsigned index,
    const uint8_t* path, unsigned depth)
{
    AnimationCapture saved;
    if (!capture(object, index, path, depth, &saved)) return false;
    HSD_JObjAnimAll(saved.joint);
    return finish(&saved);
}

bool nativeArticleResource_RemoveAnimation(Item_GObj* object, unsigned index,
    const uint8_t* path, unsigned depth)
{
    AnimationCapture saved;
    if (!capture(object, index, path, depth, &saved)) return false;
    HSD_JObjRemoveAnimAll(saved.joint);
    return finish(&saved);
}

bool nativeArticleResource_SetAnimationRate(Item_GObj* object, unsigned index,
    const uint8_t* path, unsigned depth, float rate)
{
    AnimationCapture saved;
    if (!isfinite(rate) || !capture(object, index, path, depth, &saved)) return false;
    lb_8000BA0C(saved.joint, rate);
    return finish(&saved);
}
