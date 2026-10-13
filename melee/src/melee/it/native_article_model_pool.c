#include "native_article_model_pool.h"

#include <sysdolphin/baselib/debug.h>
#include <dolphin/os.h>
#include <limits.h>
#include <string.h>

typedef struct ModelPoolLayout {
    NativeHsdDemand capacity[NATIVE_HSD_RESERVATION_BINS];
    uint32_t count;
    size_t payload_offset, payload_bytes, bytes, alignment;
} ModelPoolLayout;

static bool merge(NativeHsdDemand* bins, uint32_t* count,
                  const NativeHsdDemand* demand, bool maximum)
{
    for (uint32_t i = 0; i < *count; ++i) {
        if (bins[i].allocator != demand->allocator || bins[i].size != demand->size)
            continue;
        if (maximum) {
            if (bins[i].count < demand->count) bins[i].count = demand->count;
        } else {
            if (demand->count > NATIVE_MATCH_POOL_MAX_SLOTS - bins[i].count)
                return false;
            bins[i].count += demand->count;
        }
        return true;
    }
    if (*count == NATIVE_HSD_RESERVATION_BINS) return false;
    bins[(*count)++] = *demand;
    return true;
}

static bool layout(const HSD_Joint* const* roots, uint32_t count,
                   uint32_t live_graph_limit, ModelPoolLayout* output)
{
    if (!roots || !count || count > NATIVE_ARTICLE_MODEL_POOL_ROOTS ||
        !live_graph_limit || live_graph_limit >= NATIVE_MATCH_POOL_MAX_SLOTS)
        return false;
    ModelPoolLayout result = { 0 };
    for (uint32_t r = 0; r < count; ++r) {
        for (uint32_t earlier = 0; earlier < r; ++earlier)
            if (roots[earlier] == roots[r]) return false;
        NativeArticleModelDemand demand = { 0 };
        bool censused = nativeArticleModel_Census(roots[r], &demand);
        if (!censused || !nativeArticleModel_EnvironmentReady(&demand)) {
            OSReport("[article-model] Root %u/%u rejected: census=%u uncovered=%08x "
                     "joints=%u materials=%u textures=%u.\n", r, count, censused,
                     demand.uncovered, demand.joints, demand.materials, demand.textures);
            return false;
        }
        // A lazy Vec bin can overlap a constructor bin. Add within one graph,
        // then take the maximum across alternatives, rather than adding all
        // package roots even though any slot contains just one chosen root.
        if (!merge(demand.construction, &demand.construction_count,
                   &demand.scale_vectors, false)) return false;
        for (uint32_t i = 0; i < demand.construction_count; ++i)
            if (!merge(result.capacity, &result.count,
                       &demand.construction[i], true)) return false;
    }
    for (uint32_t i = 0; i < result.count; ++i) {
        uint32_t multiplier = live_graph_limit + 1;
        if (result.capacity[i].count > NATIVE_MATCH_POOL_MAX_SLOTS / multiplier) {
            OSReport("[article-model] Capacity rejected: piece=%u per_graph=%u "
                     "graphs=%u required=%llu limit=%u.\n", result.capacity[i].size,
                     result.capacity[i].count, multiplier,
                     (unsigned long long)result.capacity[i].count * multiplier,
                     NATIVE_MATCH_POOL_MAX_SLOTS);
            return false;
        }
        result.capacity[i].count *= multiplier;
    }
    result.payload_bytes = nativeHsdReservation_Bytes(result.capacity,
                                                    result.count,
                                                    &result.alignment);
    if (!result.payload_bytes) return false;
    result.payload_offset = nativeMatchPool_Stride(
        count * sizeof(NativeArticleModelPoolEntry), result.alignment);
    if (!result.payload_offset || result.payload_bytes > SIZE_MAX - result.payload_offset)
        return false;
    result.bytes = result.payload_offset + result.payload_bytes;
    *output = result;
    return true;
}

bool nativeArticleModelPool_Size(const HSD_Joint* const* roots, uint32_t count,
    uint32_t live_graph_limit, size_t* bytes, size_t* alignment)
{
    ModelPoolLayout result;
    if (!bytes || !alignment || !layout(roots, count, live_graph_limit, &result))
        return false;
    *bytes = result.bytes;
    *alignment = result.alignment;
    return true;
}

bool nativeArticleModelPool_Bind(NativeArticleModelPool* owner, void* backing,
    size_t bytes, const HSD_Joint* const* roots, uint32_t count,
    uint32_t live_graph_limit, uint64_t epoch)
{
    ModelPoolLayout planned;
    if (!owner || !backing || !epoch ||
        !layout(roots, count, live_graph_limit, &planned) ||
        bytes < planned.bytes || ((uintptr_t)backing & (planned.alignment - 1)))
        return false;
    const uint8_t* vacant = (const uint8_t*)owner;
    for (size_t i = 0; i < sizeof(*owner); ++i) if (vacant[i]) return false;
    uintptr_t base = (uintptr_t)backing, destination = (uintptr_t)owner;
    if (bytes > UINTPTR_MAX - base || sizeof(*owner) > UINTPTR_MAX - destination ||
        (base < destination + sizeof(*owner) && destination < base + bytes))
        return false;
    // Root pointer input must not be overwritten by reservation initialization.
    uintptr_t input = (uintptr_t)roots;
    size_t input_bytes = count * sizeof(*roots);
    if (input_bytes > UINTPTR_MAX - input ||
        (base < input + input_bytes && input < base + bytes)) return false;
    if (!nativeHsdReservation_Bind(&owner->reservation,
            (uint8_t*)backing + planned.payload_offset, planned.payload_bytes,
            planned.capacity, planned.count, epoch)) return false;
    NativeArticleModelPoolEntry* entries = backing;
    for (uint32_t i = 0; i < count; ++i) {
        NativeArticleModelDemand checked;
        if (!nativeArticleModel_PrepareStatic(roots[i], &owner->reservation, &checked)) {
            // Preparation never retains allocations or registers a graph ID.
            bool released = nativeHsdReservation_Release(&owner->reservation);
            HSD_ASSERTREPORT(0, released, "Static material preparation retained allocations");
            memset(owner, 0, sizeof(*owner));
            return false;
        }
        entries[i] = (NativeArticleModelPoolEntry){ roots[i], checked };
    }
    owner->entries = entries;
    owner->backing = backing;
    owner->bytes = bytes;
    owner->epoch = epoch;
    owner->count = count;
    return true;
}

static const NativeArticleModelPoolEntry* entry(const NativeArticleModelPool* owner,
                                               const HSD_Joint* descriptor)
{
    if (!owner || !owner->epoch || !owner->entries ||
        !owner->count || owner->count > NATIVE_ARTICLE_MODEL_POOL_ROOTS) return NULL;
    for (uint32_t i = 0; i < owner->count; ++i)
        if (owner->entries[i].descriptor == descriptor) return &owner->entries[i];
    return NULL;
}

HSD_JObj* nativeArticleModelPool_Construct(NativeArticleModelPool* owner,
                                         const HSD_Joint* descriptor)
{
    const NativeArticleModelPoolEntry* model = entry(owner, descriptor);
    if (!model || !nativeArticleModel_EnvironmentReady(&model->demand) ||
        !nativeHsdReservation_Begin(&owner->reservation,
            model->demand.construction, model->demand.construction_count)) return NULL;
    HSD_JObj* graph = HSD_JObjLoadJoint((HSD_Joint*)descriptor);
    bool complete = nativeHsdReservation_End(&owner->reservation);
    // Healthy admitted constructors assert rather than return partial graphs;
    // reaching a quota fault means the allocation census contract is wrong.
    HSD_ASSERTREPORT(0, complete && graph, "Admitted static model construction exceeded its reservation");
    return graph;
}

bool nativeArticleModelPool_BeginMatrices(NativeArticleModelPool* owner, HSD_JObj* graph)
{
    if (!owner || !graph) return false;
    NativeMatchPoolReference reference;
    bool owned = false;
    size_t joint_piece = (sizeof(HSD_JObj) + 31) & ~(size_t)31;
    for (uint32_t i = 0; i < owner->reservation.count; ++i) {
        const NativeHsdReservationBin* bin = &owner->reservation.bins[i];
        // Membership in a Vec/ID/material bin is insufficient: do not inspect
        // graph fields until its entire native JObj allocation is established.
        if (!bin->allocator && bin->pool.object_size == joint_piece &&
            nativeMatchPool_Reference(&bin->pool, graph, &reference)) {
            owned = reference.lifetime == owner->epoch;
            break;
        }
    }
    if (!owned || HSD_CLASS_METHOD(graph) != HSD_CLASS_INFO(&hsdJObj) || graph->parent)
        return false;
    const NativeArticleModelPoolEntry* model = entry(owner, (const HSD_Joint*)graph->id);
    if (!model || !nativeArticleModel_EnvironmentReady(&model->demand)) return false;
    return nativeHsdReservation_Begin(&owner->reservation, &model->demand.scale_vectors, 1);
}

bool nativeArticleModelPool_Release(NativeArticleModelPool* owner)
{
    if (!owner || !owner->epoch || !nativeHsdReservation_Release(&owner->reservation))
        return false;
    memset(owner, 0, sizeof(*owner));
    return true;
}
