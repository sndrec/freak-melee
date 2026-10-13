#include <melee/ft/native_source_assets.h>
#include <melee/ft/native_fighter_project.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/it/itarticle_data.h>
#include <melee/it/native_article_model_demand.h>
#include <melee/it/native_article_model_pool.h>
#include <melee/it/native_article_resource.h>
#include <sysdolphin/baselib/native_hsd_reservation.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/id.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/object.h>
#include <sysdolphin/baselib/memory.h>
#include <dolphin/os/OSAlloc.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, graphs, matrices, replacements, refused, id_cases;
static unsigned environment_rejections;
static unsigned char heap_bytes[16 * 1024 * 1024] ATTRIBUTE_ALIGN(64);
static NativeSourceImportedAssets assets[3];
static NativeFighterProject projects[3];
static NativeHsdReservation reservation;
static NativeScriptDiagnostic diagnostic;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"secondary line %u: %s (%s)\n",__LINE__,#x,diagnostic.message); exit(1); } } while (0)

typedef struct Node {
    uint32_t flags, children, drawables, polygons, materials, textures, tev;
    Quaternion rotation;
    Vec3 scale, position, accumulated;
    Mtx matrix;
    bool has_scale;
} Node;
typedef struct Graph { Node nodes[512]; unsigned count; } Graph;
static Graph normal, scaled, observed;

static void inspect(HSD_JObj* root, Graph* graph, bool make_matrix)
{
    for (HSD_JObj* j = root; j; j = j->next) {
        CHECK(graph->count < 512);
        if (make_matrix) { HSD_JObjSetupMatrix(j); ++matrices; }
        Node* n = &graph->nodes[graph->count++];
        n->flags = j->flags; n->rotation = j->rotate;
        n->scale = j->scale; n->position = j->translate;
        memcpy(n->matrix, j->mtx, sizeof(n->matrix));
        n->has_scale = j->scl != NULL;
        if (j->scl) n->accumulated = *j->scl;
        for (HSD_JObj* c = j->child; c; c = c->next) ++n->children;
        for (HSD_DObj* d = j->u.dobj; d; d = d->next) {
            ++n->drawables;
            for (HSD_PObj* p = d->pobj; p; p = p->next) ++n->polygons;
            if (d->mobj) {
                ++n->materials;
                for (HSD_TObj* t = d->mobj->tobj; t; t = t->next) ++n->textures;
                for (HSD_TExpTevDesc* t = d->mobj->tevdesc; t; t = (HSD_TExpTevDesc*)t->desc.next) ++n->tev;
            }
        }
        if (j->child) inspect(j->child, graph, make_matrix);
    }
}
static void view(HSD_JObj* root, Graph* graph, bool matrix)
{ memset(graph, 0, sizeof(*graph)); inspect(root, graph, matrix); }
static void scale(HSD_JObj* root)
{ Vec3 value = { 2.0f, 3.0f, 4.0f }; HSD_JObjSetScale(root, &value); }
static bool empty(void)
{
    for (uint32_t i = 0; i < reservation.count; ++i)
        if (reservation.bins[i].pool.used) return false;
    return true;
}
static bool reserved(const void* pointer)
{
    return (uintptr_t)pointer >= (uintptr_t)reservation.backing &&
        (uintptr_t)pointer - (uintptr_t)reservation.backing < reservation.backing_bytes;
}
static NativeMatchPoolReference reference(const void* pointer)
{
    NativeMatchPoolReference result = { 0 };
    for (uint32_t i = 0; i < reservation.count; ++i)
        if (nativeMatchPool_Reference(&reservation.bins[i].pool, pointer, &result)) return result;
    CHECK(false); return result;
}
static void ids(HSD_JObj* root)
{
    for (HSD_JObj* j = root; j; j = j->next) {
        CHECK(HSD_IDGetData(j->id, NULL) == j);
        for (HSD_JObj* child = j->child; child; child = child->next) CHECK(child->parent == j);
        if (j->child) ids(j->child);
    }
}
static void inspect_storage(HSD_JObj* root)
{
    for (HSD_JObj* j = root; j; j = j->next) {
        CHECK(reserved(j)); if (j->scl) CHECK(reserved(j->scl));
        for (HSD_DObj* d = j->u.dobj; d; d = d->next) {
            CHECK(reserved(d));
            for (HSD_PObj* p = d->pobj; p; p = p->next) CHECK(reserved(p));
            if (d->mobj) {
                CHECK(reserved(d->mobj) && reserved(d->mobj->mat));
                for (HSD_TObj* t = d->mobj->tobj; t; t = t->next) CHECK(reserved(t));
                for (HSD_TExpTevDesc* t = d->mobj->tevdesc; t; t = (HSD_TExpTevDesc*)t->desc.next) CHECK(reserved(t));
            }
        }
        if (j->child) inspect_storage(j->child);
    }
}
static HSD_JObj* load(HSD_Joint* descriptor, const NativeArticleModelDemand* demand, OSHeapHandle heap)
{
    s32 available = OSCheckHeap(heap);
    CHECK(nativeHsdReservation_Begin(&reservation, demand->construction, demand->construction_count));
    HSD_JObj* graph = HSD_JObjLoadJoint(descriptor);
    CHECK(graph && nativeHsdReservation_End(&reservation));
    CHECK(OSCheckHeap(heap) == available); inspect_storage(graph); ++graphs;
    ids(graph);
    return graph;
}
static void lazy(HSD_JObj* graph, const NativeArticleModelDemand* demand, OSHeapHandle heap)
{
    s32 available = OSCheckHeap(heap);
    CHECK(nativeHsdReservation_Begin(&reservation, &demand->scale_vectors, 1));
    scale(graph); view(graph, &observed, true);
    CHECK(nativeHsdReservation_End(&reservation));
    CHECK(OSCheckHeap(heap) == available && !memcmp(&observed, &scaled, sizeof(scaled)));
    inspect_storage(graph);
}
static void reject_environment(HSD_Joint* descriptor, const NativeArticleModelDemand* demand)
{
    NativeArticleModelDemand output, sentinel;
    memset(&sentinel, 0xA5, sizeof(sentinel)); output = sentinel;
    NativeHsdReservation metadata = reservation;
    void* snapshot = malloc(reservation.backing_bytes); CHECK(snapshot);
    memcpy(snapshot, reservation.backing, reservation.backing_bytes);
    CHECK(!nativeArticleModel_EnvironmentReady(demand));
    CHECK(!nativeArticleModel_PrepareStatic(descriptor, &reservation, &output));
    CHECK(!memcmp(&output, &sentinel, sizeof(output)) && !nativeHsdReservation_Active());
    CHECK(!memcmp(&metadata, &reservation, sizeof(metadata)) &&
          !memcmp(snapshot, reservation.backing, reservation.backing_bytes));
    free(snapshot); ++environment_rejections;
}
static void environment_cases(HSD_Joint* descriptor, const NativeArticleModelDemand* demand)
{
    CHECK(demand->materials);
    HSD_MObjInfo saved_material = hsdMObj;
    hsdMObj.load = NULL; reject_environment(descriptor, demand); hsdMObj = saved_material;
    hsdMObj.make_texp = NULL; reject_environment(descriptor, demand); hsdMObj = saved_material;
    HSD_TObj* saved_toon = tobj_toon;
    HSD_ImageDesc* saved_image = saved_toon ? saved_toon->imagedesc : NULL;
    HSD_ImageDesc image = { 0 };
    HSD_MObjSetToonTextureImage(&image); reject_environment(descriptor, demand);
    HSD_MObjSetToonTextureImage(saved_image);
    if (!saved_toon) { HSD_TObjRemove(tobj_toon); tobj_toon = NULL; }
    CHECK(nativeArticleModel_EnvironmentReady(demand));
}
static void cohort(HSD_Joint* descriptor, unsigned fighter, unsigned offset, OSHeapHandle heap)
{
    printf("{\"stage\":\"graph-census\",\"fighter\":%u,\"attribute_offset\":%u}\n",fighter,offset); fflush(stdout);
    NativeArticleModelDemand demand = { 0 };
    CHECK(nativeArticleModel_Census(descriptor, &demand));
    CHECK(!(demand.uncovered & ~NativeArticleModel_MaterialEnvironment));
    CHECK(nativeArticleModel_EnvironmentReady(&demand));
    if (fighter == 2 && offset == 84) {
        // The production scene reserves all bounded resource graph slots.
        // This real material graph exceeded the former 16K class-piece ceiling.
        size_t full_bytes = 0, full_alignment = 0;
        const HSD_Joint* roots[] = { descriptor };
        CHECK(nativeArticleModelPool_Size(roots, 1,
            NATIVE_ARTICLE_RESOURCE_COLLECTIONS * NATIVE_ARTICLE_RESOURCE_NODES,
            &full_bytes, &full_alignment));
        printf("{\"stage\":\"full-resource-layout\",\"bytes\":%llu,\"alignment\":%llu}\n",
            (unsigned long long)full_bytes, (unsigned long long)full_alignment);
    }
    NativeHsdDemand capacity[97];
    memcpy(capacity, demand.construction, demand.construction_count * sizeof(*capacity));
    unsigned count = demand.construction_count;
    for (unsigned i = 0; i < count; ++i) capacity[i].count *= 2;
    bool merged = false;
    for (unsigned i = 0; i < count; ++i)
        if (capacity[i].allocator == demand.scale_vectors.allocator && capacity[i].size == demand.scale_vectors.size) {
            capacity[i].count += 2 * demand.scale_vectors.count; merged = true;
        }
    if (!merged) { capacity[count] = demand.scale_vectors; capacity[count++].count *= 2; }
    size_t alignment = 0, bytes = nativeHsdReservation_Bytes(capacity, count, &alignment);
    void* backing = _aligned_malloc(bytes, alignment); CHECK(bytes && backing);
    CHECK(nativeHsdReservation_Bind(&reservation, backing, bytes, capacity, count, 100 + fighter * 2 + offset));
    if (!fighter && offset == 36) environment_cases(descriptor, &demand);
    NativeArticleModelDemand prepared = { 0 };
    s32 available = OSCheckHeap(heap);
    CHECK(nativeArticleModel_PrepareStatic(descriptor, &reservation, &prepared));
    CHECK(!prepared.uncovered && empty() && OSCheckHeap(heap) == available);
    CHECK(nativeArticleModel_EnvironmentReady(&prepared)); demand = prepared;
    HSD_JObj* ordinary = HSD_JObjLoadJoint(descriptor); CHECK(ordinary); ++graphs;
    view(ordinary, &normal, false); CHECK(normal.count == demand.joints);
    scale(ordinary); view(ordinary, &scaled, true);
    CHECK(HSD_IDGetData((uintptr_t)descriptor, NULL) == ordinary);
    HSD_JObjRemoveAll(ordinary); CHECK(HSD_IDGetData((uintptr_t)descriptor, NULL) == NULL);
    HSD_JObj* first = load(descriptor, &demand, heap);
    NativeMatchPoolReference old_reference = reference(first);
    view(first, &observed, false); CHECK(!memcmp(&normal, &observed, sizeof(normal)));
    lazy(first, &demand, heap);
    HSD_JObj* replacement = load(descriptor, &demand, heap); lazy(replacement, &demand, heap);
    CHECK(HSD_IDGetData((uintptr_t)descriptor, NULL) == replacement);
    ++id_cases;
    void* unchanged = malloc(bytes); CHECK(unchanged); memcpy(unchanged, backing, bytes);
    NativeHsdReservation metadata = reservation;
    CHECK(!nativeHsdReservation_Begin(&reservation, demand.construction, demand.construction_count));
    CHECK(!nativeHsdReservation_Active() && !memcmp(unchanged, backing, bytes) && !memcmp(&metadata, &reservation, sizeof(metadata)));
    CHECK(HSD_IDGetData((uintptr_t)descriptor, NULL) == replacement); ++refused;
    view(first, &observed, false); CHECK(!memcmp(&observed, &scaled, sizeof(scaled)));
    HSD_JObjRemoveAll(first); CHECK(HSD_IDGetData((uintptr_t)descriptor, NULL) == replacement); ++replacements;
    ids(replacement);
    CHECK(nativeMatchPool_Resolve(old_reference) == NULL);
    HSD_JObjRemoveAll(replacement); CHECK(empty() && HSD_IDGetData((uintptr_t)descriptor, NULL) == NULL);
    HSD_JObj* reused = load(descriptor, &demand, heap); lazy(reused, &demand, heap);
    // Reuse stays inside the same reserved payload; exact slot order is owned by NativeMatchPool.
    CHECK(reserved(reused) && reference(reused).generation > 1);
    CHECK(nativeMatchPool_Resolve(old_reference) == NULL);
    HSD_JObjRemoveAll(reused); CHECK(empty());
    // Removing the newest duplicate does not restore the older ID mapping.
    first = load(descriptor, &demand, heap); replacement = load(descriptor, &demand, heap);
    HSD_JObjRemoveAll(replacement); CHECK(HSD_IDGetData((uintptr_t)descriptor, NULL) == NULL);
    CHECK(first->id == (uintptr_t)descriptor); ++id_cases;
    HSD_JObjRemoveAll(first); CHECK(empty());
    CHECK(nativeHsdReservation_Release(&reservation)); _aligned_free(backing); free(unchanged);
    printf("{\"fighter\":%u,\"attribute_offset\":%u,\"joints\":%u,\"materials\":%u,\"textures\":%u,\"tev_bound\":%u,\"reservation_bytes\":%llu}\n",fighter,offset,demand.joints,demand.materials,demand.textures,demand.tev_expressions,(unsigned long long)bytes); fflush(stdout);
}
int main(int argc, char** argv)
{
    CHECK(argc == 4);
    void* start = OSInitAlloc(heap_bytes, heap_bytes + sizeof(heap_bytes), 1);
    OSHeapHandle heap = OSCreateHeap(start, heap_bytes + sizeof(heap_bytes)); CHECK(heap >= 0);
    HSD_SetHeap(heap); OSSetCurrentHeap(heap); HSD_ObjSetHeap(sizeof(heap_bytes), NULL); HSD_ObjInit(); HSD_IDSetup();
    ClassInfoInit((HSD_ClassInfo*)&hsdJObj); ClassInfoInit((HSD_ClassInfo*)&hsdDObj);
    ClassInfoInit((HSD_ClassInfo*)&hsdPObj); ClassInfoInit((HSD_ClassInfo*)&hsdMObj);
    ClassInfoInit((HSD_ClassInfo*)&hsdTObj);
    for (unsigned f = 0; f < 3; ++f) {
        CHECK(nativeFighterProject_Open(argv[f + 1], &projects[f]));
        NativeSourceAssetInput input = { projects[f].fighter_dat, projects[f].fighter_size,
            projects[f].animation_dat, projects[f].animation_size, projects[f].costume_dat, projects[f].costume_size };
        CHECK(nativeSourceAssets_Import(&projects[f].facts, &input, Ft_Kind_CustomFirst, &assets[f], &diagnostic));
        Article* arrow = NULL;
        const char* name = f == 2 ? "It_Kind_Ness_Yoyo" :
                           f ? "It_Kind_CLink_Arrow" : "It_Kind_Link_Arrow";
        for (unsigned i = 0; i < assets[f].facts.article_count; ++i)
            if (!strcmp(assets[f].facts.articles[i].declaration, name)) arrow = assets[f].character.fighter_items[assets[f].facts.articles[i].slot];
        CHECK(arrow);
        const NativeFighterArticleModelRoot roots[] = {
            { 1, f == 2 ? 80 : 36 }, { 2, f == 2 ? 84 : 40 }
        };
        CHECK(itArticle_ImportModelRoots(&assets[f].character.import, arrow, roots, 2));
        for (unsigned r = 0; r < 2; ++r) { CHECK(arrow->native_model_roots[r].root); cohort(arrow->native_model_roots[r].root, f, roots[r].attribute_offset, heap); }
        nativeSourceAssets_Release(&assets[f]); nativeFighterProject_Close(&projects[f]);
    }
    CHECK(HSD_IDGetAllocData()->used == 0 && HSD_VecGetAllocData()->used == 0);
    CHECK(hsdJObj.parent.parent.head.nb_exist == 0 && hsdDObj.parent.head.nb_exist == 0 &&
          hsdPObj.parent.head.nb_exist == 0 && hsdMObj.parent.head.nb_exist == 0 &&
          hsdTObj.parent.head.nb_exist == 0);
    printf("{\"status\":\"PASS\",\"checks\":%u,\"graphs\":%u,\"matrix_visits\":%u,\"replacement_commits\":%u,\"exhaustion_preflights\":%u,\"id_collision_cases\":%u,\"environment_rejections\":%u}\n",checks,graphs,matrices,replacements,refused,id_cases,environment_rejections);
    return 0;
}
