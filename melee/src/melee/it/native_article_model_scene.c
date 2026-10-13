#include "native_article_model_scene.h"
#include "native_article_model.h"
#include "native_article_model_pool.h"
#include "native_item_lifetime.h"
#include <melee/it/types.h>

#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_script_binding_item_model_resolve.h>
#include <melee/gm/native_rollback_pages.h>
#include <melee/gm/native_match_arena.h>
#include <melee/lb/lbheap.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/id.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/robj.h>
#include <string.h>

enum { MODEL_SLOTS = NATIVE_ARTICLE_MODEL_SCENE_CAPACITY };

typedef struct SceneModel {
    NativeItemLifetime owner;
    NativeMatchPoolReference graph;
    const NativeSourcePackage* package;
    const HSD_Joint* descriptor;
    HSD_JObj* root;
    uint32_t slot, root_index;
} SceneModel;

// Non-HOST .mstate owner/registry: the existing page oracle captures this and
// the actual main-heap backing, including pool generations/free lists. Package
// descriptors remain immutable; no model storage is allocated in package malloc.
static struct {
    NativeArticleModelPool pool;
    SceneModel models[MODEL_SLOTS];
    NativeSourcePackage* packages[Ft_Kind_RuntimeCapacity];
    NativeItemLifetime matrix_owner;
    uint64_t epoch;
    uint32_t package_count, matrix_slot;
} scene;

static bool boundary(void)
{
    return !(nativeMatchSceneArena.lifetime && nativeMatchSceneArena.sealed) && !nativeRollbackPages_IsInitialized() && nativeScriptRuntime_Idle() &&
        !nativeHsdReservation_Active() && !HSD_GObj_CurrentInvokedProc &&
        !HSD_GObj_CurrentInvokedProcGObj && !HSD_GObj_804D7818 &&
        !HSD_GObj_DelayedProcInfo.flags;
}

static bool pinned(const NativeSourcePackage* package)
{
    for (uint32_t i = 0; i < scene.package_count; ++i)
        if (scene.packages[i] == package) return true;
    return false;
}

bool nativeArticleModelScene_ResolveOwner(const NativeItemLifetime* owner,
                     const NativeSourcePackage** output)
{
    if (!owner->source_owned || !nativeItemLifetime_Current(owner) ||
        !scene.epoch || scene.epoch != nativeGObjPool.lifetime ||
        owner->reference.lifetime != scene.epoch) return false;
    const NativeSourcePackage* package =
        nativeFighterRuntime_SourcePackage(owner->program->author_kind);
    if (!package || !pinned(package) || package->address != package ||
        !package->ready || !package->assets.ready ||
        owner->bank != &package->compiler.bank ||
        nativeScriptItem_Program(owner->program->author_kind,
                                 owner->program->owner) != owner->program)
        return false;
    if (package->program.items.count > NATIVE_SCRIPT_ENTITY_LIMIT) return false;
    bool declared = false;
    // Publication rebases ItemProgram structs. Compare the exact immutable
    // declaration identity/external asset tables, not the parent Fighter's ID.
    // The origins and selectors live inside ItemPrograms and are rebased into
    // the published copy. The canonical published-program check above proves
    // that owner; their addresses cannot equal the package's prepared arrays.
    for (uint32_t i = 0; i < package->program.items.count; ++i) {
        const NativeScriptItemProgram* item = &package->program.items.programs[i];
        if (item->owner == owner->program->owner &&
            item->identity == owner->program_identity && item->bank == owner->bank &&
            item->kind == owner->kind && item->article == owner->article &&
            item->model_roots == owner->program->model_roots &&
            item->model_root_count == owner->program->model_root_count &&
            item->clones_primary_model == owner->program->clones_primary_model &&
            item->model_animations == owner->program->model_animations &&
            item->model_animation_count == owner->program->model_animation_count &&
            item->animation_origin_count == owner->program->animation_origin_count)
            declared = true;
    }
    if (!declared) return false;
    if (output) *output = package;
    return true;
}

static bool graph_reference(HSD_JObj* graph, NativeMatchPoolReference* output)
{
    for (uint32_t i = 0; i < scene.pool.reservation.count; ++i)
        if (nativeMatchPool_Reference(&scene.pool.reservation.bins[i].pool,
                                     graph, output)) return true;
    return false;
}

static bool model_current(const SceneModel* model)
{
    NativeItemLifetime current;
    if (!model->root || !nativeItemLifetime_Capture(model->owner.object, &current))
        return false;
    // Removal is an engine lifecycle phase, not a permanent registry identity.
    // Capture proves safe physical access before refreshing that one field;
    // nativeArticleModelScene_ResolveOwner() still compares every originally retained owner/schema field.
    NativeItemLifetime retained = model->owner;
    retained.removing = current.removing;
    const NativeSourcePackage* package;
    return nativeMatchPool_Resolve(model->graph) == model->root &&
        model->graph.lifetime == scene.epoch && nativeArticleModelScene_ResolveOwner(&retained, &package) &&
        package == model->package && model->slot >= 1 && model->slot <= 3 &&
        model->owner.data->native_models[model->slot - 1] == model->root &&
        (model->root_index==NATIVE_ARTICLE_MODEL_PRIMARY_ROOT ?
            model->owner.program->clones_primary_model && model->owner.article->x10_modelDesc &&
            model->owner.article->x10_modelDesc->x0_joint==model->descriptor :
            model->owner.program->model_roots &&
            model->root_index < model->owner.program->model_root_count &&
            model->owner.program->model_roots[model->root_index].root == model->descriptor);
}

bool nativeArticleModelScene_SlotCurrent(const NativeArticleModelSceneSlot* view)
{
    if (!view || view->slot < 1 || view->slot > 3 || !view->epoch ||
        view->epoch != scene.epoch || view->epoch != nativeGObjPool.lifetime ||
        !view->owner.source_owned || !nativeItemLifetime_Current(&view->owner) ||
        view->owner.data->native_models[view->slot - 1] != view->root)
        return false;
    // Empty-slot projections are useful even without a root declaration. A
    // declared table additionally requires the selected, leased source package.
    if ((view->owner.program->model_root_count || view->owner.program->clones_primary_model) &&
        !nativeArticleModelScene_ResolveOwner(&view->owner, NULL))
        return false;
    for (uint32_t i = 0; i < MODEL_SLOTS; ++i) {
        const SceneModel* entry = &scene.models[i];
        if (!entry->root || entry->owner.object != view->owner.object ||
            entry->slot != view->slot) continue;
        return view->root && model_current(entry) && entry->root == view->root &&
            entry->descriptor == view->descriptor && entry->root_index == view->root_index &&
            entry->graph.pool == view->graph.pool &&
            entry->graph.slot == view->graph.slot &&
            entry->graph.generation == view->graph.generation &&
            entry->graph.lifetime == view->graph.lifetime &&
            nativeMatchPool_Resolve(view->graph) == view->root;
    }
    return view->root == NULL && view->descriptor == NULL && view->root_index == UINT32_MAX;
}

bool nativeArticleModelScene_CaptureSlot(Item_GObj* object, unsigned slot,
                                        NativeArticleModelSceneSlot* output)
{
    NativeArticleModelSceneSlot view = { .epoch = scene.epoch, .slot = slot,
                                       .root_index = UINT32_MAX };
    if (!output || slot < 1 || slot > 3 ||
        !nativeItemLifetime_Capture(object, &view.owner) || !view.owner.source_owned)
        return false;
    view.root = view.owner.data->native_models[slot - 1];
    for (uint32_t i = 0; i < MODEL_SLOTS; ++i) {
        const SceneModel* entry = &scene.models[i];
        if (entry->root && entry->owner.object == object && entry->slot == slot) {
            view.graph = entry->graph;
            view.descriptor = entry->descriptor;
            view.root_index = entry->root_index;
            break;
        }
    }
    if (!nativeArticleModelScene_SlotCurrent(&view)) return false;
    *output = view;
    return true;
}

bool nativeArticleModelScene_Prepare(void)
{
    if (!boundary() || scene.epoch || !nativeGObjPool.lifetime) return false;
    const HSD_Joint* roots[NATIVE_ARTICLE_MODEL_POOL_ROOTS];
    uint32_t count = 0;
    // Include any selected source kind, including future stock source publication.
    for (unsigned kind = 0; kind < Ft_Kind_RuntimeCapacity; ++kind) {
        const NativeSourcePackage* package =
            nativeFighterRuntime_SourcePackage((FighterKind)kind);
        if (!package) continue;
        bool duplicate = false, needs_lease = false;
        for (uint32_t i = 0; i < scene.package_count; ++i)
            if (scene.packages[i] == package) duplicate = true;
        if (duplicate) continue;
        if (package->entities.count > NATIVE_SCRIPT_ENTITY_LIMIT ||
            package->program.items.count > NATIVE_SCRIPT_ENTITY_LIMIT) goto failure;
        for (uint32_t e = 0; e < package->entities.count; ++e) {
            const NativeScriptItemAsset* asset = &package->entities.rows[e];
            if (asset->model_root_count > NATIVE_FIGHTER_ARTICLE_MODEL_ROOT_LIMIT ||
                (asset->model_root_count && !asset->model_roots)) goto failure;
            for (uint32_t r = 0; r < asset->model_root_count; ++r) {
                const HSD_Joint* root = asset->model_roots[r].root;
                if (!root) goto failure;
                needs_lease = true;
                bool seen = false;
                for (uint32_t n = 0; n < count; ++n) if (roots[n] == root) seen = true;
                if (!seen) {
                    if (count == NATIVE_ARTICLE_MODEL_POOL_ROOTS) goto failure;
                    roots[count++] = root;
                }
            }
        }
        // Lease by authored operations, not origin count: a primary-only empty
        // bundle or request/remove-only program can have no controllers at all.
        // Its immutable selectors and source owner still need this scene lease.
        for (uint32_t i = 0; i < package->program.items.count; ++i) {
            const NativeScriptItemProgram* program = &package->program.items.programs[i];
            if(program->clones_primary_model) {
                const HSD_Joint* root=program->article && program->article->x10_modelDesc ?
                    program->article->x10_modelDesc->x0_joint:NULL;
                if(!root)goto failure;
                needs_lease=true;
                bool seen=false;
                for(uint32_t n=0;n<count;++n)if(roots[n]==root)seen=true;
                if(!seen) {
                    if(count==NATIVE_ARTICLE_MODEL_POOL_ROOTS)goto failure;
                    roots[count++]=root;
                }
            }
            if (program->model_selector_count > NATIVE_SCRIPT_ITEM_MODEL_SELECTORS ||
                (program->model_selector_count && !program->model_selectors)) goto failure;
            for (uint32_t s = 0; s < program->model_selector_count; ++s) {
                uint32_t domain = program->model_selectors[s].semantic &
                    NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK;
                if (domain >= NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADD &&
                    domain <= NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_RATE) needs_lease = true;
            }
        }
        if (needs_lease) {
            if (scene.package_count == Ft_Kind_RuntimeCapacity ||
                !nativeSourcePackage_Pin((NativeSourcePackage*)package)) goto failure;
            scene.packages[scene.package_count++] = (NativeSourcePackage*)package;
        }
    }
    if (!count) { scene.epoch = nativeGObjPool.lifetime; return true; }
    // Actual allocator init, only when not yet initialized; never reset a live
    // descriptor. Class initialization preserves the actual effective defaults;
    // EnvironmentReady subsequently rejects unsupported method selections.
    if (!HSD_IDGetAllocData()->size) HSD_IDInitAllocData();
    if (!HSD_VecGetAllocData()->size) HSD_VecInitAllocData();
    if (!HSD_MtxGetAllocData()->size) HSD_MtxInitAllocData();
    if (!HSD_RObjGetAllocData()->size) HSD_RObjInitAllocData();
    ClassInfoInit((HSD_ClassInfo*)&hsdJObj);
    ClassInfoInit((HSD_ClassInfo*)&hsdDObj);
    ClassInfoInit((HSD_ClassInfo*)&hsdPObj);
    ClassInfoInit((HSD_ClassInfo*)&hsdMObj);
    ClassInfoInit((HSD_ClassInfo*)&hsdTObj);
    size_t bytes, alignment;
    if (!nativeArticleModelPool_Size(roots, count, MODEL_SLOTS, &bytes, &alignment))
        goto failure;
    void* backing = lbHeap_80015BD0(0, bytes);
    if (!backing) goto failure;
    if (((uintptr_t)backing & (alignment - 1)) ||
        !nativeArticleModelPool_Bind(&scene.pool, backing, bytes, roots, count,
                                     MODEL_SLOTS, nativeGObjPool.lifetime)) {
        lbHeap_80015CA8(0, backing);
        goto failure;
    }
    scene.epoch = nativeGObjPool.lifetime;
    return true;
failure:
    while (scene.package_count) {
        bool released = nativeSourcePackage_Unpin(scene.packages[--scene.package_count]);
        HSD_ASSERTREPORT(0, released, "Secondary model package lease failed to unwind");
    }
    memset(&scene, 0, sizeof(scene));
    return false;
}

bool nativeArticleModelScene_Load(Item_GObj* object, unsigned slot, unsigned root_index)
{
    NativeItemLifetime owner;
    const NativeSourcePackage* package;
    if (slot < 1 || slot > 3 || scene.matrix_slot || nativeHsdReservation_Active() ||
        !nativeItemLifetime_Capture(object, &owner) || !nativeArticleModelScene_ResolveOwner(&owner, &package))
        return false;
    const HSD_Joint* descriptor;
    if(root_index==NATIVE_ARTICLE_MODEL_PRIMARY_ROOT) {
        if(!owner.program->clones_primary_model || !owner.article->x10_modelDesc)return false;
        descriptor=owner.article->x10_modelDesc->x0_joint;
    } else {
        if(!owner.program->model_roots || root_index>=owner.program->model_root_count)return false;
        descriptor=owner.program->model_roots[root_index].root;
    }
    if(!descriptor)return false;
    uint32_t vacancy = MODEL_SLOTS;
    bool prior_registered = owner.data->native_models[slot - 1] == NULL;
    for (uint32_t i = 0; i < MODEL_SLOTS; ++i) {
        SceneModel* entry = &scene.models[i];
        if (!entry->root && vacancy == MODEL_SLOTS) vacancy = i;
        if (entry->root && entry->owner.object == object && entry->slot == slot) {
            if (!model_current(entry)) return false;
            prior_registered = true;
            vacancy = i;
            break;
        }
    }
    if (vacancy == MODEL_SLOTS || !prior_registered) return false;
    HSD_JObj* root = nativeArticleModelPool_Construct(&scene.pool, descriptor);
    if (!root) return false;
    NativeMatchPoolReference reference;
    // Standard admitted construction invokes no authored callbacks. IDs now
    // name the new graph, so an impossible continuation breach is a scene
    // contract fault, not a recoverable failure that pretends old IDs survived.
    bool retained = nativeArticleModelScene_ResolveOwner(&owner, NULL) && graph_reference(root, &reference);
    HSD_ASSERTREPORT(0, retained, "Admitted secondary construction lost its retained owner");
    nativeArticleModel_Destroy(object, slot);
    HSD_ASSERTREPORT(0, nativeArticleModelScene_ResolveOwner(&owner, NULL), "Secondary destruction lost its retained owner");
    scene.models[vacancy] = (SceneModel){ owner, reference, package,
                                         descriptor, root, slot, root_index };
    owner.data->native_models[slot - 1] = root;
    return true;
}

bool nativeArticleModelScene_Revoke(Item_GObj* object, unsigned slot, HSD_JObj* root)
{
    if (scene.matrix_slot) return false;
    for (uint32_t i = 0; i < MODEL_SLOTS; ++i) {
        SceneModel* entry = &scene.models[i];
        if (!entry->root || entry->root != root) continue;
        if (entry->owner.object != object || entry->slot != slot ||
            nativeMatchPool_Resolve(entry->owner.reference) != object ||
            object->user_data != entry->owner.data ||
            nativeMatchPool_Resolve(entry->graph) != root) return false;
        memset(entry, 0, sizeof(*entry));
        return true;
    }
    // An unregistered address from this reservation is not an ordinary graph:
    // it may be a borrowed candidate or an already freed/reused scene root.
    uintptr_t address = (uintptr_t)root, base = (uintptr_t)scene.pool.reservation.backing;
    return !base || address < base || address - base >= scene.pool.reservation.backing_bytes;
}

bool nativeArticleModelScene_BeginMatrices(Item_GObj* object, unsigned slot)
{
    if (scene.matrix_slot || slot < 1 || slot > 3) return false;
    for (uint32_t i = 0; i < MODEL_SLOTS; ++i) {
        SceneModel* entry = &scene.models[i];
        if (!entry->root || entry->owner.object != object || entry->slot != slot) continue;
        NativeItemLifetime owner;
        if (!model_current(entry) || !nativeItemLifetime_Capture(object, &owner) ||
            !nativeArticleModelPool_BeginMatrices(&scene.pool, entry->root)) return false;
        scene.matrix_owner = owner;
        scene.matrix_slot = i + 1;
        return true;
    }
    return false;
}

bool nativeArticleModelScene_EndMatrices(void)
{
    if (!scene.matrix_slot || scene.matrix_slot > MODEL_SLOTS) return false;
    bool retained = model_current(&scene.models[scene.matrix_slot - 1]) &&
                    nativeItemLifetime_Current(&scene.matrix_owner);
    bool complete = nativeHsdReservation_End(&scene.pool.reservation);
    scene.matrix_slot = 0;
    memset(&scene.matrix_owner, 0, sizeof(scene.matrix_owner));
    return retained && complete;
}

bool nativeArticleModelScene_Retire(void)
{
    if (!boundary() || scene.matrix_slot) return false;
    if (!scene.epoch) return true;
    for (uint32_t i = 0; i < MODEL_SLOTS; ++i) {
        SceneModel* entry = &scene.models[i];
        if (!entry->root) continue;
        if (nativeMatchPool_Resolve(entry->graph) != entry->root) return false;
        Item_GObj* object = nativeMatchPool_Resolve(entry->owner.reference);
        if (object && object->classifier == HSD_GOBJ_CLASS_ITEM &&
            object->user_data == entry->owner.data &&
            entry->owner.data->native_models[entry->slot - 1] == entry->root)
            entry->owner.data->native_models[entry->slot - 1] = NULL;
        HSD_JObj* root = entry->root;
        memset(entry, 0, sizeof(*entry));
        HSD_JObjRemoveAll(root);
    }
    void* backing = scene.pool.backing;
    if (backing && !nativeArticleModelPool_Release(&scene.pool)) return false;
    if (backing) lbHeap_80015CA8(0, backing);
    while (scene.package_count) {
        if (!nativeSourcePackage_Unpin(scene.packages[scene.package_count - 1])) return false;
        --scene.package_count;
    }
    memset(&scene, 0, sizeof(scene));
    return true;
}

// Packages/recipes and pool geometry are prepared once and leased for the scene.
// The reservation owner captures bins and backing; only live model registrations
// and the balanced matrix-claim context are additional mutable state here.
bool nativeArticleModelScene_AdmitHistory(NativeMatchArena* arena)
{
    if (!arena || !arena->lifetime || scene.epoch!=arena->lifetime ||
        scene.matrix_slot || nativeHsdReservation_Active()) return false;
    for (unsigned i=0;i<MODEL_SLOTS;++i)
        if (scene.models[i].root && !model_current(&scene.models[i])) return false;
    return nativeMatchArena_AddRoot(arena,scene.models,sizeof(scene.models)) &&
        nativeMatchArena_AddRoot(arena,&scene.matrix_owner,sizeof(scene.matrix_owner)) &&
        nativeMatchArena_AddRoot(arena,&scene.matrix_slot,sizeof(scene.matrix_slot));
}
